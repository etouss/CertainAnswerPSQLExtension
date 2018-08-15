#include "split_optimize.h"

typedef struct context_split {
  int which_i_choose;
  bool have_to_duplicate;
  bool stop;
}context_split;


context_split * make_context_split(void);
Node * split_rel (Node *node , context_split *context);
bool have_to_split(Node *node , void * context);
List * trueSplit(Node * negNode);
Node * optimize_split(Query *q);
Node * mutator_split_deep(Node * node, void * context);

context_split * make_context_split(){
  context_split * ctx = palloc(sizeof(context_split));
  ctx->which_i_choose = 0;
  ctx->have_to_duplicate = true;
  ctx->stop = false;
  return ctx;
}

Node * split_rel (Node *node , context_split *context){
  /* elog(LOG,"arg %d : %s \n ",nodeTag(node),nodeToString(node)); */
  if(context->stop) return node;
  if (node == NULL)
    return NULL;
  if(IsA(node, BoolExpr)){
     BoolExpr * bExpr =(BoolExpr *) node; 
       if(bExpr->boolop == OR_EXPR){
       /* elog(LOG,"arg : %s \n",nodeToString(node)); */
       context->stop = true;
       List * or_args = bExpr->args;
       Node * my_arg = list_nth(or_args,context->which_i_choose);
       if(list_length(or_args) == context->which_i_choose +1){
         /*Stop duplicate*/
         /* elog(LOG,"HERE %d:%s \n",context->which_i_choose +1,nodeToString(or_args)); */
         context->have_to_duplicate = false;
       }
       else{
         /*Duplicate*/
         /* elog(LOG,"arg : %s \n",nodeToString(my_arg)); */
         context->which_i_choose ++;
         context->have_to_duplicate = true;
       }
       return copyObject(my_arg);
     }
    return expression_tree_mutator(node, split_rel, (void *)context);
  }
  return expression_tree_mutator(node, split_rel, (void *)context);

}

bool have_to_split(Node *node , void * context){
  if (node == NULL)
    return false;
   if(IsA(node, BoolExpr)){
     BoolExpr * bExpr =(BoolExpr *) node; 
    if(bExpr->boolop == OR_EXPR ){
      return true;
    }
   }
  return expression_tree_walker(node,have_to_split,context);
}


List * trueSplit(Node * negNode){
  
  List * result = NULL;
  List * pile = NULL;
  
  pile = lappend(pile,negNode);
  while(list_length(pile) != 0){
    Node * backup = list_nth(pile,0);
    pile = list_delete_first(pile);
    if(!have_to_split(backup,NULL)){
      result = lappend(result,backup);
      continue;
    }
    context_split * ctx = make_context_split();
    do{
      ctx->stop = false;
      Node * new_node = split_rel(backup,ctx);
      /* elog(LOG,"BACKUP: %s \n",nodeToString(backup)); */
      /* elog(LOG,"NEW NODE : %s \n",nodeToString(new_node)); */
      pile = lappend(pile,new_node);
    }while(ctx->have_to_duplicate);
  }
  return result;
}

Node * mutator_split_deep(Node * node, void * context){
  if(node == NULL)
    return NULL;
  if(IsA(node,BoolExpr)){
    BoolExpr * be = (BoolExpr *)node;
    if(be->boolop == NOT_EXPR){
      if(IsA(list_nth(be->args,0),SubLink)){
        SubLink * sb = (SubLink *)list_nth(be->args,0);
        ((Query *)sb->subselect)->jointree = (FromExpr *)mutator_split_deep((Node *)((Query *)sb->subselect)->jointree,context);
        return optimize_split((Query *)sb->subselect);
      }

    }
  }
  return expression_tree_mutator(node,mutator_split_deep,context);
}

void optimize_split_deep(Query *q){
  q->jointree = (FromExpr *)mutator_split_deep((Node *)q->jointree,NULL);
}


 Node * optimize_split(Query *q){
  List * result = trueSplit(q->jointree->quals);
  List * nexits = NULL;
  const ListCell *cell;
  foreach(cell, result)
	{
    Node * t = lfirst(cell);
    SubLink * exist = makeNode(SubLink);
    Query * tmp = (Query *)copyObject(q);
    tmp->jointree->quals = t;
    exist->subselect = (Node *)tmp;
    nexits = lappend(nexits,makeBoolExpr(NOT_EXPR,lappend(NULL,exist),-1));
    /* nexits->args = lappend(nexits->args,exist); */
  }
  if(nexits->length > 1)
    return (Node *)makeBoolExpr(AND_EXPR,nexits,-1);
  return list_nth(nexits,0);
}

