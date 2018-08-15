#include "exist_optimize.h"

typedef struct context_exists {
  List * composante_connexe_sub_level;
  List * list_composantes_connexes;
  int varno_en_cours;
  bool var_sub;
}context_exists;

typedef struct temp_context_connex_rew {
  List * list_constraint;
  List * list_varno;
  bool to_remove;
}temp_context_connex_rew;

typedef struct temp_context_connex_rew2 {
  List * list_varno;
  List * rtbl;
  List * targetList;
  bool to_add;
}temp_context_connex_rew2;

temp_context_connex_rew * make_temp_context_connex_rew(List * l);
temp_context_connex_rew2 * make_temp_context_connex_rew2(List * l);
context_exists * make_context_exists(int nb_varno);
int is_in_list(int varno, List *list_varno);
bool create_connexe(Node * node, context_exists * ctx);
Node * mutator_conn(Node * node, temp_context_connex_rew * ctx);
Node * mutator_target(Node * node, temp_context_connex_rew2 * ctx);
Node * create_from_query_list(List * list_rte, Node * constraint, List * listTargetEntry);
Node * mutator_varquals(Node * node, temp_context_connex_rew2 * ctx);
Node * mutator_varTarget(Node * node, temp_context_connex_rew2 * ctx);
void rewrite_connexe(Query *q, List * list_varno);
bool is_sub(int varno,context_exists * ctx);
List * return_list(int varno,context_exists * ctx);
void remove_from_list(int varno,context_exists * ctx);
int deacrease(int varno, List * list_varno);
Node * mutator_exist_deep(Node * node, void * context);

temp_context_connex_rew * make_temp_context_connex_rew(List * l){
  temp_context_connex_rew * ctx = palloc(sizeof(temp_context_connex_rew));
  ctx->list_constraint = NULL;
  ctx->list_varno = l;
  ctx->to_remove = false;
  return ctx;
}

temp_context_connex_rew2 * make_temp_context_connex_rew2(List * l){
  temp_context_connex_rew2 * ctx = palloc(sizeof(temp_context_connex_rew));
  ctx->list_varno = l;
  ctx->rtbl = NULL;
  ctx->targetList = NULL;
  ctx->to_add = false;
  return ctx;
}

context_exists * make_context_exists(int nb_varno){
  context_exists * ctx = palloc(sizeof(context_exists));
  ctx->composante_connexe_sub_level = NULL;
  ctx->list_composantes_connexes = NULL;
  for(int i = 0;i<nb_varno;i++){
    List * tmp = NULL;
    tmp = lappend_int(tmp,i+1);
    ctx->list_composantes_connexes = lappend(ctx->list_composantes_connexes,tmp);
  }
  ctx->varno_en_cours = -1;
  ctx->var_sub = false;
  return ctx;
}

bool is_sub(int varno,context_exists * ctx){
  const ListCell *cell;
  foreach(cell, ctx->composante_connexe_sub_level)
  {
    int v = lfirst_int(cell);
    if(v == varno){
      return true;
    }
  }
  return false;

}

List * return_list(int varno,context_exists * ctx){
  const ListCell *cell;
  foreach(cell, ctx->list_composantes_connexes) 
	{
    List * compo = lfirst(cell);
    const ListCell *cell_b;
    foreach(cell_b, compo){
      int v = lfirst_int(cell_b);
      if(v == varno){
        return compo;
      }
    }
  }
  foreach(cell, ctx->composante_connexe_sub_level)
  {
    int v = lfirst_int(cell);
    if(v == varno){
      return ctx->composante_connexe_sub_level;
    }
  }
  /* List * result = NULL; */
  /* result =  lappend_int(result,varno); */
  /* ctx->list_composantes_connexes = lappend(ctx->list_composantes_connexes,result); */
  return NULL;
}

void remove_from_list(int varno,context_exists * ctx){
  List * result = NULL;
  const ListCell *cell;
  foreach(cell, ctx->list_composantes_connexes) 
	{
    bool to_add = true;
    List * compo = lfirst(cell);
    const ListCell *cell_b;
    foreach(cell_b, compo){
      int v = lfirst_int(cell_b);
      if(v == varno){
        to_add = false;
        break;
      }
    }
    if(to_add){
      result = lappend(result,compo);
    }
  }
  ctx->list_composantes_connexes = result;
}




int is_in_list(int varno, List *list_varno){
  const ListCell *cell;
  int i = 1;
  foreach(cell, list_varno) 
	{
    if(varno == lfirst_int(cell))
      return i;
    i++;
  }
  return 0;
}

int deacrease(int varno, List * list_varno){
  int i = 0;
  const ListCell *cell;
  foreach(cell, list_varno) 
	{
    if(varno > lfirst_int(cell)){
      i++;
    }
  }
  return varno - i;
  
}



bool create_connexe(Node * node, context_exists * ctx){
  if (node == NULL)
    return false;
  if(IsA(node, OpExpr)){
    bool retour = expression_tree_walker(node,create_connexe,(void *)ctx);
    ctx->varno_en_cours = -1;
    ctx->var_sub = false;
    return retour;
  }
  if(IsA(node,Var)){
    Var * v = (Var *)node;
    if(ctx->varno_en_cours == -1){
      ctx->varno_en_cours = v->varno;
      if(v->varlevelsup > 0 || is_sub(v->varno,ctx))
        ctx->var_sub = true;
      return false;
    }
    bool sub = is_sub(v->varno,ctx);
    if(ctx->var_sub && (sub || v->varlevelsup > 0))
      return false;
    if(ctx->var_sub && !sub && v->varlevelsup == 0 ){ 
      List * b = return_list(v->varno,ctx);
      ctx->composante_connexe_sub_level = list_concat(ctx->composante_connexe_sub_level,b);
      remove_from_list(v->varno,ctx);
      /*Supprime b*/
      return false;
    }

    if(sub || v->varlevelsup > 0){
      List * a = return_list(ctx->varno_en_cours,ctx);
      ctx->composante_connexe_sub_level = list_concat(ctx->composante_connexe_sub_level,a);
      remove_from_list(ctx->varno_en_cours,ctx);
      return false;
      /*Remove a*/
    }
    List * a = return_list(ctx->varno_en_cours,ctx);
    List * b = return_list(v->varno,ctx);
    if(a == b) return false;
    remove_from_list(ctx->varno_en_cours,ctx);
    remove_from_list(v->varno,ctx);
    ctx->list_composantes_connexes = lappend(ctx->list_composantes_connexes,list_concat(a,b));
    return false;
  }
  return expression_tree_walker(node,create_connexe,(void *)ctx);
}

Node * mutator_conn(Node * node, temp_context_connex_rew * ctx){
  /*Capture constraint, and delete them*/
  if (node == NULL)
    return node;
  if (IsA(node,OpExpr)){
    Node * n = expression_tree_mutator(node, mutator_conn, (void *) ctx);
    if(ctx->to_remove){
      ctx->list_constraint = lappend(ctx->list_constraint,n);
      ctx->to_remove = false;
      return NULL;
    }
    return n;
  }
  if(IsA(node,Var)){
    Var * v = (Var *) node;
    if(v->varlevelsup == 0 && is_in_list(v->varno,ctx->list_varno)){
        ctx->to_remove = true;
    }
  }
  return expression_tree_mutator(node, mutator_conn, (void *) ctx);
}

Node * mutator_target(Node * node, temp_context_connex_rew2 * ctx){

  if (node == NULL)
    return NULL;
  if(IsA(node, Var)){
    Var *v = (Var *)copyObject(node);
    int i = is_in_list(v->varno,ctx->list_varno);
    if(!i)
      return NULL;
    v->varno = i;
    ctx->to_add = true;
    return (Node *)v;
  }

  if(IsA(node,List)){
    List * result = NULL;
    const ListCell *cell;
    int i = 1;
    foreach(cell, (List *)node)
	  { 
      Node * t = copyObject(lfirst(cell));
      TargetEntry * n = (TargetEntry *)mutator_target(t,ctx);
      if(ctx->to_add == true){
        n->resno = i;
        i++;
        result = lappend(result,n);
        ctx->to_add = false;
      }
    }
    /* i = 1; */
    return (Node *)result;
  }
  return expression_tree_mutator(node, mutator_target, (void *)ctx);
}

Node * create_from_query_list(List * list_rte, Node * constraint, List * listTargetEntry){
  Query * q = makeNode(Query);
  /* elog(LOG,"HERE 1 \n");   */
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
  /* elog(LOG,"HERE 1 \n"); */
  q->targetList = listTargetEntry;
  q->rtable = list_rte;

  /*LIST SIZE*/
  RangeTblRef * rtr = makeNode(RangeTblRef);
  rtr->rtindex = 1;
  q->jointree = makeFromExpr(lappend(NULL,rtr),constraint);
  
  /*Steal Target Entry From Query !*/ 
  /* elog(LOG,"Query : %s\n",nodeToString(q)); */
  return (Node *)q;
}

Node * mutator_varquals(Node * node, temp_context_connex_rew2 * ctx){
  if (node == NULL)
    return NULL;
   if(IsA(node, Var)){
    Var *v = (Var *)copyObject(node);
    if(v->varlevelsup == 0)
      v->varno = deacrease(v->varno,ctx->list_varno);
    return (Node *)v;
  }

  return expression_tree_mutator(node, mutator_varquals, (void *)ctx);
}

Node * mutator_varTarget(Node * node, temp_context_connex_rew2 * ctx){

  if (node == NULL)
    return NULL;
  if(IsA(node, Var)){
    Var *v = (Var *)copyObject(node);
    int i = is_in_list(v->varno,ctx->list_varno);
    if(i)
      return NULL;
    v->varno = deacrease(v->varno,ctx->list_varno);
    ctx->to_add = true;
    return (Node *)v;
  }

  if(IsA(node,List)){
    List * result = NULL;
    const ListCell *cell;
    int i = 1;
    foreach(cell, (List *)node)
	  { 
      Node * t = copyObject(lfirst(cell));
      TargetEntry * n = (TargetEntry *)mutator_varTarget(t,ctx);
      if(ctx->to_add == true){
        n->resno = i;
        i++;
        result = lappend(result,n);
        ctx->to_add = false;
      }
    }
    /* i = 1; */
    return (Node *)result;
  }
  return expression_tree_mutator(node, mutator_varTarget, (void *)ctx);
    
}

List * substract(List * input, int nb){
  List * result = NULL;
  int i = input->length - nb;
  const ListCell *cell;
  foreach(cell, input)
  { 
    result = lappend(result,lfirst(cell));
    if(i <= 1)
      break;
    i--;
  }
  return result;
}


void rewrite_connexe(Query *q, List * list_varno){
  /*Capture constrainte*/
  temp_context_connex_rew * ctx = make_temp_context_connex_rew(list_varno);
  q->jointree->quals = expression_tree_mutator((Node *)q->jointree->quals, mutator_conn, (void *) ctx);
  q->jointree->fromlist = substract(q->jointree->fromlist,list_varno->length);

  /*create get_RangleTbl from q, get ListTargetEntry*/
  /*create subquere Exists*/
  /*Modify previous query (modify var tar et rtbl + add exists)*/

  temp_context_connex_rew2 * ctx2 = make_temp_context_connex_rew2(list_varno);
  List * n = (List *)mutator_target((Node *)q->targetList,ctx2);
  List * lrte = NULL;
  const ListCell *cell;
  foreach(cell, list_varno)
  {
    int v = lfirst_int(cell);
    lrte =  lappend(lrte,list_nth(q->rtable,v - 1));
  }
  SubLink * ex = makeNode(SubLink);
  ex->subselect = create_from_query_list(lrte,(Node *)ctx->list_constraint,n);
  int i = 0;
  foreach(cell, list_varno)
  { 
    int v = lfirst_int(cell);
    q->rtable = list_delete_ptr(q->rtable,list_nth(q->rtable,v - 1-i));
    i++;
  }
  q->targetList = (List *)mutator_varTarget((Node *)q->targetList,ctx2);
  q->jointree = (FromExpr *)mutator_varquals((Node *)q->jointree,ctx2);
  List * tmp_list = lappend(NULL,q->jointree->quals);
  tmp_list = lappend(tmp_list,ex);
  q->jointree->quals = makeBoolExpr(AND_EXPR,tmp_list,-1);
  
}


Node * mutator_exist_deep(Node * node, void * context){
  if(node == NULL)
    return NULL;
  if(IsA(node,Query)){
    Query * q = (Query *)node;
    q->jointree = (FromExpr *)expression_tree_mutator((Node *)q->jointree,mutator_exist_deep,context);
    optimize_exist(q);
    return (Node *)q;
  }
  return expression_tree_mutator(node,mutator_exist_deep,context);
}

void optimize_exist_deep(Query *nq){
  mutator_exist_deep((Node *)nq->jointree,NULL);
}

void optimize_exist(Query *nq){
  /*Delete self_constraint*/
  optimize_self_constraint(nq);
  /*Create and init the contexte with the number of rel*/
  if(nq->rtable->length == 1) return;

  context_exists * ctx_e = make_context_exists(nq->rtable->length);
  /*Actually compute the connexe componante where rel are node and = are edges.*/
  create_connexe((Node *)nq->jointree,ctx_e);
  /*Here we rewrite the query component by component*/
  /*It's a bit tricky as we have to care of changing varno*/
  List * list = NULL;
  if(ctx_e ->composante_connexe_sub_level == NULL || ctx_e ->composante_connexe_sub_level->length == 0 )return;
  list = ctx_e->list_composantes_connexes;
  while(list != NULL && list->length != 0){
    rewrite_connexe(nq,list_nth(list,0));
    List * tmp = NULL;
    if(list->length < 1)
      break;
    for(int i = 1;i<list->length;i++){
      List * tm2 = NULL;
      const ListCell *cell;
      foreach(cell, list_nth(list,i))
      {
        tm2 = lappend_int(tm2,deacrease(lfirst_int(cell),list_nth(list,0)));
      }
      tmp = lappend(tmp,tm2);
    }
    list = copyObject(tmp);
  }
}
