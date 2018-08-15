#include "self_optimize.h"

typedef struct context_optimizer_self {
  bool disjunction;
  bool equality;
  bool add;
  List * already_constrain_rel;
  int varno_to_add;
  int varlevelsup_to_add;
  int varno_to_add_or;
  int varlevelsup_to_add_or;
  int varno_to_add_eq;
  int varlevelsup_to_add_eq; 
  bool stop;
}context_optimizer_self;

typedef struct temp_context_rewrite {
  int varno_to_add;
  int varlevelsup_to_add;
  bool to_add;
}temp_context_rewrite;

typedef struct temp_context {
  int varno_to_add;
  int varlevelsup_to_add;
  Node * constraint_to_add;
  bool to_add;
  bool already_add;
}temp_context;


context_optimizer_self* make_context_optimizer_self(void);
temp_context_rewrite * make_temp_context_rewrite(void);
temp_context * make_temp_context(Node * constraint_to_add,int varno_to_add,int varlevelsup_to_add);
Node * my_modifier_self(Node *node, context_optimizer_self *context);
bool init_rel(Node *node, temp_context_rewrite *context);
Node * modif_rel (Node *node , void *context);
Node * modif_targetEntry(Node *node , temp_context_rewrite *context);
Node * create_from_query(RangeTblEntry * rte, Node * constraint, List * listTargetEntry);
Node * rewrite_self(Query *q, List *constraint_to_add);
List * add_constrain(Node *already_constrain_rel, temp_context *context);
Node * temp_modifier_add (Node *already_constrain_rel, temp_context *context);
Node * mutator_deep(Node * node, void * context);

context_optimizer_self* make_context_optimizer_self(){
  context_optimizer_self * ctx = malloc(sizeof(context_optimizer_self));
  ctx->disjunction = false;
  ctx->equality = false;
  ctx->add = false;
  ctx->already_constrain_rel = NULL;
  ctx->varno_to_add = -1;
  ctx->varlevelsup_to_add = -1;
  ctx->varno_to_add_or = -1;
  ctx->varlevelsup_to_add_or = -1;
  ctx->varno_to_add_eq = -1;
  ctx->varlevelsup_to_add_eq = -1;
  ctx->stop = false;
  return ctx;
}

temp_context_rewrite * make_temp_context_rewrite(){
  temp_context_rewrite * ctx = palloc(sizeof(temp_context_rewrite));
  ctx->varno_to_add = -1;
  ctx->varlevelsup_to_add = -1;
  ctx->to_add = false;
    return ctx;
}

temp_context * make_temp_context(Node * constraint_to_add,int varno_to_add,int varlevelsup_to_add){
  temp_context * ctx = palloc(sizeof(temp_context));
  ctx->varno_to_add = varno_to_add;
  ctx->varlevelsup_to_add = varlevelsup_to_add;
  ctx->constraint_to_add = constraint_to_add;
  ctx->to_add = false;
  ctx->already_add = false;
  return ctx;
}


List * add_constrain(Node *already_constrain_rel, temp_context *context){
  /* elog(LOG,"HERE Add :  %s\n",nodeToString(already_constrain_rel)); */
   if (already_constrain_rel == NULL)
     return lappend((List *)already_constrain_rel,context->constraint_to_add);

    List * result = NULL;
    List * list = (List *)already_constrain_rel;
    const ListCell *cell;
  	foreach(cell, list)
	  {
      Node * t = lfirst(cell);
      if(context->already_add){
        result = lappend(result,t);
        continue;
      }
      Node * new = temp_modifier_add(t,context);
      if(context->to_add && !context->already_add){
        context->to_add = false;
        context->already_add = true;
        List * tmp_list = NULL;
        tmp_list = lappend(tmp_list,new);
        tmp_list = lappend(tmp_list,context->constraint_to_add);
        result = lappend(result,makeBoolExpr(AND_EXPR,tmp_list,-1));
      }
      else{
        result = lappend(result,new);
      }
    }
    if(!context->already_add){
        result =  lappend(result,context->constraint_to_add);
    }
    /* list_free(list); */
    return result;
}

Node * temp_modifier_add (Node *already_constrain_rel, temp_context *context){
  /* elog(LOG,"HERE Modifier :  %s\n",nodeToString(already_constrain_rel)); */
  if (already_constrain_rel == NULL){
    /* context->to_add = ; */
    return NULL;
  }
  if(IsA(already_constrain_rel,Var)){
    Var * v = (Var *) already_constrain_rel;
    if(v->varlevelsup == context->varlevelsup_to_add && v->varno == context->varno_to_add){
      context->to_add = true;
    }
    return (Node *)v;
  }

  if(IsA(already_constrain_rel,BoolExpr)){
    Node * tmp = expression_tree_mutator(already_constrain_rel, temp_modifier_add, (void *) context);
    BoolExpr * expr = (BoolExpr *)tmp;
    if(expr->boolop == AND_EXPR && context->to_add){
        context->to_add = false;
        context->already_add = true;
        return (Node *)lappend(expr->args,context->constraint_to_add);
    }
    return tmp;
  }
  return expression_tree_mutator(already_constrain_rel, temp_modifier_add, (void *) context);
}

Node * my_modifier_self(Node *node, context_optimizer_self *context){
  if (node == NULL)
 			return node;


  if(IsA(node,Query)){
    return node;
  }

  if(IsA(node,Var)){
    Var * v = (Var *)node;
    if(context->stop) return node;
    if(context->disjunction && context->varno_to_add_or != -1 && (context->varno_to_add_or != v->varno || context->varlevelsup_to_add_or != v->varlevelsup )){
      context->add = false;
      context->stop = true;
      return node;
    }
    if(context->equality && context->varno_to_add_eq != -1 && (context->varno_to_add_eq != v->varno || context->varlevelsup_to_add_eq != v->varlevelsup )){
      context->add = false;
      context->stop = true;
      return node;
    }
    context->varno_to_add = v->varno;
    context->varlevelsup_to_add = v->varlevelsup;
    if(context->disjunction){
      context->varno_to_add_or = v->varno;
      context->varlevelsup_to_add_or = v->varlevelsup;
    }
    if(context->equality){
      context->varno_to_add_eq = v->varno;
      context->varlevelsup_to_add_eq = v->varlevelsup;
    }
    context->add = true;
    return node;
  }
  if(IsA(node,BoolExpr)){
    /*Deal with OR*/
    BoolExpr * bExpr =(BoolExpr *) node;
      if(bExpr->boolop == OR_EXPR){
        /* elog(LOG,"\nTHERENOT\n"); */
        context->disjunction = true;
        context->varno_to_add_or = -1;
        context->varlevelsup_to_add_or = -1;
        Node * retour =  expression_tree_mutator(node, my_modifier_self, (void *) context);
        context->disjunction =  false;
        context->varno_to_add = -1;
        context->varlevelsup_to_add = -1;
        /* elog(LOG,"OR check : %s\n",nodeToString(node)); */
        if(context->add){
          /*Add*/
         context->add = false;
         context->stop = false;
         /* elog(LOG,"ADD :: %s\n to :: %s\n",nodeToString(node),nodeToString(context->already_constrain_rel)); */
         context->already_constrain_rel = add_constrain((Node *)context->already_constrain_rel,make_temp_context(node,context->varno_to_add,context->varlevelsup_to_add));
         return makeBoolConst(true, false);
        }
        return retour;
      }
  }

  if(IsA(node,NullTest)){
    Node * retour = expression_tree_mutator(node, my_modifier_self, (void *) context);
    if(context->add && !context->disjunction &&!context->stop){
      context->add = false;
      elog(LOG,"ADD :: %s\n to :: %s\n",nodeToString(node),nodeToString(context->already_constrain_rel));
      context->already_constrain_rel = add_constrain((Node *)context->already_constrain_rel,make_temp_context(node,context->varno_to_add,context->varlevelsup_to_add));
      return makeBoolConst(true, false);;
    }
    return retour;

  }

  if(IsA(node,OpExpr)){
    context->equality = true;
    Node * retour = expression_tree_mutator(node, my_modifier_self, (void *) context);
    context->equality = false;
    context->varno_to_add_eq = -1;
    context->varlevelsup_to_add_eq = -1;
    if(context->add && !context->disjunction &&!context->stop){
      elog(LOG,"ADD :: %s\n to :: %s\n",nodeToString(node),nodeToString(context->already_constrain_rel));
      context->add = false;
      context->already_constrain_rel = add_constrain((Node *)context->already_constrain_rel,make_temp_context(node,context->varno_to_add,context->varlevelsup_to_add));
      return makeBoolConst(true, false);;
    }
    return retour;
  }
  return expression_tree_mutator(node, my_modifier_self, (void *) context);
}

bool init_rel(Node *node, temp_context_rewrite *context){
  if (node == NULL)
 			return false;
  if(IsA(node, Var)){
    Var *v = (Var *)node;
    context->varno_to_add = v->varno;
    context->varlevelsup_to_add = v->varlevelsup;
    return true;
  }
  return expression_tree_walker(node, init_rel, (void *) context);
}

Node * modif_rel (Node *node , void *context){
  if (node == NULL)
    return NULL;
  if(IsA(node, Var)){
    Var *v = (Var *)copyObject(node);
    v->varno = 1;
    /* context->varlevelsup_to_add = v->varlevelsup; */
    return (Node *)v;
  }
  return expression_tree_mutator(node, modif_rel, context);

}

Node * modif_targetEntry(Node *node , temp_context_rewrite *context){
  /* static int i = 1; */
  if (node == NULL)
    return NULL;
  if(IsA(node, Var)){
    Var *v = (Var *)copyObject(node);
    if(v->varno != context->varno_to_add)
      return NULL;
    v->varno = 1;
    context->to_add = true;
    return (Node *)v;
  }
  if(IsA(node,List)){
    List * result = NULL;
    const ListCell *cell;
    int i = 1;
    foreach(cell, (List *)node)
	  { 
      Node * t = copyObject(lfirst(cell));
      TargetEntry * n = (TargetEntry *)modif_targetEntry(t,context);
      if(context->to_add == true){
        n->resno = i;
        i++;
        result = lappend(result,n);
        context->to_add = false;
      }
    }
    return (Node *)result;
  }
  return expression_tree_mutator(node, modif_targetEntry, (void *)context);

}

Node * create_from_query(RangeTblEntry * rte, Node * constraint, List * listTargetEntry){
  Query * q = makeNode(Query);
  q->commandType = 1;
  q->canSetTag = true;
  q->hasAggs = false;
  q->hasWindowFuncs = false;
  q->hasSubLinks = false;
  q->hasDistinctOn = false;
  q->hasRecursive = false;
  q->hasModifyingCTE = false;
  q->hasForUpdate = false;
  q->hasRowSecurity = false;
  q->targetList = listTargetEntry;
  q->rtable = lappend(q->rtable,rte);
  RangeTblRef * rtr = makeNode(RangeTblRef);
  rtr->rtindex = 1;
  q->jointree = makeFromExpr(lappend(NULL,rtr),constraint);
  return (Node *)q;
}




Node * rewrite_self(Query *q, List *constraint_to_add){
  /*For each constraint*/
  static int i = 0;
  const ListCell *cell;
  foreach(cell, constraint_to_add)
	{
    i++;
    Node * t = lfirst(cell);
    temp_context_rewrite * ctx = make_temp_context_rewrite();
    /*We recup the rel to constrain*/
    init_rel(t,ctx);
    /*Modify the constraint to match the new varno ie. 1*/
    Node * n_constraint = modif_rel(t,NULL);
    /*Retrieve target entry from the query and adjust it to match new value ie reno and varno*/
    List * l = (List *)modif_targetEntry((Node *)copyObject(q->targetList),ctx);
    /*Actualy create a new query matching the constraint*/
    Query * nq = (Query *)create_from_query(copyObject(list_nth(q->rtable,ctx->varno_to_add - 1)),n_constraint,l);

    /*Modify the query for the "FORM" part to include the new Query*/
    RangeTblEntry * ef = list_nth(q->rtable,ctx->varno_to_add - 1);
    ef->rtekind = RTE_SUBQUERY;
    ef->subquery = nq;
    char * test = malloc(sizeof(char)*16);
    sprintf(test,"a%d",i);
    ef->alias = makeAlias(test,NULL);
    ef->eref = makeAlias(test,ef->eref->colnames);
  }
  /*Return NULL because pointer modification*/
  return NULL;
}


Node * mutator_deep(Node * node, void * context){
  if(node == NULL)
    return NULL;
  if(IsA(node,Query)){
      Query * q = (Query *) node;
      q->jointree = (FromExpr *)expression_tree_mutator((Node *)q->jointree, mutator_deep, (void *)context);
      optimize_self_constraint(q);
      return (Node *)q;
  }
  return expression_tree_mutator(node, mutator_deep, (void *)context);
}

/*Doesn't go in nested loop*/
void optimize_self_constraint(Query *q){
  /*Init the context with variable that are alone*/
  if(q->rtable->length == 1)return;
  context_optimizer_self * ctx_self = make_context_optimizer_self();
  /*Return all the self_relation in "already_constrain_rel" and remove constraint from Q*/
  q->jointree = (FromExpr *)my_modifier_self((Node *)q->jointree,ctx_self);
  /*Modify the rel in the "FROM" to match constraint*/
  rewrite_self(q,ctx_self->already_constrain_rel);
}

void optimize_self_constraint_deep(Query *q){
    mutator_deep((Node *)q->jointree,NULL);
}
