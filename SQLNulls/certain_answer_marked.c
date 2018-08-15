#include "postgres.h"
#include "parser/parse_node.h"
#include "parser/analyze.h"
#include "nodes/nodeFuncs.h"
#include "nodes/makefuncs.h"

#include <ctype.h>

#include "commands/user.h"
#include "fmgr.h"
#include "libpq/md5.h"

#include "access/hash.h"
#include "access/htup_details.h"
#include "access/nbtree.h"
#include "bootstrap/bootstrap.h"
#include "catalog/namespace.h"
#include "catalog/pg_am.h"
#include "catalog/pg_amop.h"
#include "catalog/pg_amproc.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_constraint.h"
#include "catalog/pg_language.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_opclass.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_range.h"
#include "catalog/pg_statistic.h"
#include "catalog/pg_transform.h"
#include "catalog/pg_type.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/datum.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/typcache.h"

#include "true_var.h"
#include "self_optimize.h"
#include "split_optimize.h"
#include "exist_optimize.h"
/* #include "pretty_printer.h" */



PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

bool my_walker(Node *node, context_walker_set_constraint *context);
Node * my_mutator (Node *node, context_modifier *context);
static void my_post_parse_analyse_hook(ParseState *pstate, Query *query);

static post_parse_analyze_hook_type prev_post_parse_analyze_hook = NULL;

void
_PG_init(void)
{
  prev_post_parse_analyze_hook = post_parse_analyze_hook;
	post_parse_analyze_hook = my_post_parse_analyse_hook;
}

void _PG_fini(void)
{
  /* elog(LOG,"\n GET IN \n"); */
  post_parse_analyze_hook = prev_post_parse_analyze_hook;
}



bool my_walker(Node *node, context_walker_set_constraint *context){
  if (node == NULL)
 			return false;
  if(IsA(node,BoolExpr)){
    BoolExpr * bExpr =(BoolExpr *) node;
    bool retour = false;
    List * l_true_save = NULL;
    List * l_false_save = NULL;
    List * l_true = NULL;
    List * l_false = NULL;
    int nb_arg = bExpr->args->length;
    switch(bExpr->boolop){
      case NOT_EXPR:
        /*(Unicité des valeur) Setisation sur le true*/
        /*Pre Traitement*/
        l_true_save = context->list_of_not_null_to_be_true;
        l_false_save = context->list_of_not_null_to_be_false;
        context->list_of_not_null_to_be_true = NULL;
        context->list_of_not_null_to_be_false = NULL;

        /*Traitement*/
        retour = expression_tree_walker(node, my_walker, (void *) context);

        /*Post Traitement*/
        l_true = context->list_of_not_null_to_be_true;
        l_false = context->list_of_not_null_to_be_false;
        context->list_of_not_null_to_be_true = list_concat(l_true_save,l_false);
        context->list_of_not_null_to_be_false = list_concat(l_false_save,l_true);

        return retour;

        break;
      case AND_EXPR:
        /*(Unicité des valeur) Setisation sur le true*/
        /*Pre Traitement*/
        l_true_save = context->list_of_not_null_to_be_true;
        l_false_save = context->list_of_not_null_to_be_false;
        context->list_of_not_null_to_be_true = NULL;
        context->list_of_not_null_to_be_false = NULL;

        /*Traitement*/
        retour = expression_tree_walker(node, my_walker, (void *) context);

        /*Post traitement*/

        l_true = union_list(context->list_of_not_null_to_be_true);
        l_false = inter_list(context->list_of_not_null_to_be_false,nb_arg);
        context->list_list_true = lappend(context->list_list_true,l_true);
        context->list_list_false = lappend(context->list_list_false,l_false);
        context->list_of_not_null_to_be_true = list_concat(l_true_save,l_true);
        context->list_of_not_null_to_be_false = list_concat(l_false_save,l_false);

        return retour;

        break;
      case OR_EXPR:
        /*(Unicité des valeur) Setisation sur le true*/
        /*Pre Traitement*/
        l_true_save = context->list_of_not_null_to_be_true;
        l_false_save = context->list_of_not_null_to_be_false;
        context->list_of_not_null_to_be_true = NULL;
        context->list_of_not_null_to_be_false = NULL;

        /*Traitement*/
        retour = expression_tree_walker(node, my_walker, (void *) context);

        /*Post traitement*/
        l_true = inter_list(context->list_of_not_null_to_be_true,nb_arg);
        l_false = union_list(context->list_of_not_null_to_be_false);
        context->list_list_true = lappend(context->list_list_true,l_true);
        context->list_list_false = lappend(context->list_list_false,l_false);
        context->list_of_not_null_to_be_true = list_concat(l_true_save,l_true);
        context->list_of_not_null_to_be_false = list_concat(l_false_save,l_false);
        return retour;

        break;
    }

  }

  if(context->ready && IsA(node,Var)){
    Var * v = getTrueVar(context->current_trueVar,(Var *)node);
    if(!isInListTrueVar(context->list_of_not_null_in_op,v)){
      context->list_of_not_null_in_op = lappend(context->list_of_not_null_in_op,v);
    }
    return expression_tree_walker(node, my_walker, (void *) context);
  }

  if(IsA(node,OpExpr)){
     OpExpr * oExpr = (OpExpr *)node;
     if(oExpr->opno == 518 /*<>*/){
        context->ready = true;
        context->list_of_not_null_in_op = NULL;
        bool retour = expression_tree_walker(node, my_walker, (void *) context);
        context->list_of_not_null_to_be_true = list_concat(context->list_of_not_null_to_be_true,context->list_of_not_null_in_op);
        return retour;
     }
     else{
       return expression_tree_walker(node, my_walker, (void *) context);
     }
  }

  if(IsA(node,Query)){
    Query * q = (Query *) node;
    
    /*Prétraitement*/
    List * save_current_trueVar = context->current_trueVar;
    context->current_trueVar = NULL;
    context->current_trueVar = lappend(context->current_trueVar,save_current_trueVar);
    
    /*Traitement*/
    /*Create trueVars and not_null schema*/
    bool result =  expression_tree_walker((Node *)q->rtable, my_walker, (void *) context);
    context->trueVars = lappend(context->trueVars,context->current_trueVar);
    /*Propagate*/
    result = expression_tree_walker((Node *)q->jointree, my_walker, (void *) context) | result;
    /*Post Traitement*/
    context->current_trueVar = save_current_trueVar;
    return result;
  }
  if(IsA(node,RangeTblEntry)){
    RangeTblEntry * rte = (RangeTblEntry *)node;
    Oid relid= rte->relid;
    int number_col = get_relnatts(relid);
    AttrNumber i = 0;
    List * relation = NULL;
    for(i=1;i<=number_col;i++){
      /*Create True_var*/
      Var * v = makeNode(Var);
      v->varattno = i;
      v->vartype = relid;
      relation =  lappend(relation,v);
      /*Add to not_null if needed*/
      if(get_pg_att_not_null(relid,i)){
        context->list_of_not_null_att_schem = lappend(context->list_of_not_null_att_schem,v);
      }
    }
    context->current_trueVar = lappend(context->current_trueVar,relation);
    return false;
  }
  return expression_tree_walker(node, my_walker, (void *) context);
}



Node * my_mutator (Node *node, context_modifier *context)
  {
    if (node == NULL)
 			return NULL;
    if(IsA(node,BoolExpr)){
      BoolExpr * bExpr =(BoolExpr *) node;
      if(bExpr->boolop == NOT_EXPR){
        context->positive = !context->positive;
        Node * retour =  expression_tree_mutator(node, my_mutator, (void *) context);
        context->positive = !context->positive;
        return retour;
      }
      if(bExpr->boolop == OR_EXPR || bExpr->boolop == AND_EXPR){
        /*Pré traitement*/
        List * l_save = context->list_of_not_null_in_current;
        context->list_of_not_null_in_current = list_copy(l_save);
        if(context->positive){
          context->list_of_not_null_in_current = list_concat(context->list_of_not_null_in_current,list_nth(context->list_list_true,context->list_list_true->length - context->where_i_am - 1));
        }
        else{
          context->list_of_not_null_in_current = list_concat(context->list_of_not_null_in_current,list_nth(context->list_list_false,context->list_list_true->length - context->where_i_am - 1));
        }
        context->where_i_am ++;
        Node * retour =  expression_tree_mutator(node, my_mutator, (void *) context);
        /*Post traitement*/
        context->list_of_not_null_in_current = l_save;

        return retour;
      }
    }
    if(IsA(node,Query)){
      Query * q = (Query *) node;
      List * save_current_trueVar = context->current_trueVar;
      context->current_trueVar = list_nth(context->trueVars,context->where_i_am_querry);
      context->where_i_am_querry = context->where_i_am_querry + 1;
      q->jointree = expression_tree_mutator((Node *) q->jointree, my_mutator, (void *) context);
      context->current_trueVar = save_current_trueVar;
      return node;
    }
    if(IsA(node,OpExpr)){
      OpExpr * oExpr = (OpExpr *) node;
      if(context->positive && oExpr->opno == 32820){ /*<>*/
        context->ready = true;
      }
      if(!context->positive && oExpr->opno == 32821){ /* = */
        context->ready = true;
      }
      /* if(context->positive && oExpr->opno == 521){ #<{(| > |)}># */
      /*   context->ready = true; */
      /* } */
      /* if(oExpr->opno == 525){ #<{(| >= |)}># */
      /*   OpExpr * equa = makeNode(OpExpr); */
      /*   equa->opno = 96; #<{(|=|)}># */
      /*   equa->args = list_concat(equa->args,oExpr->args); */
      /*   OpExpr * stric = makeNode(OpExpr); */
      /*   stric->opno = 521; #<{(|>|)}># */
      /*   stric->args = list_concat(stric->args,oExpr->args); */
      /*   List * tmp = NULL; */
      /*   tmp = lappend(tmp,equa); */
      /*   tmp = lappend(tmp,stric); */
      /*   Node * result = expression_tree_mutator((Node *)makeBoolExpr(OR_EXPR,tmp,-1),my_mutator,context); */
      /*   return result; */
      /*  */
      /* } */
      /* if(oExpr->opno == 523){ #<{(| <= |)}># */
      /*   OpExpr * equa = makeNode(OpExpr); */
      /*   equa->opno = 96; #<{(|=|)}># */
      /*   equa->args = list_concat(equa->args,oExpr->args); */
      /*   OpExpr * stric = makeNode(OpExpr); */
      /*   stric->opno = 97; #<{(|<|)}># */
      /*   stric->args = list_concat(stric->args,oExpr->args); */
      /*   List * tmp = NULL; */
      /*   tmp = lappend(tmp,equa); */
      /*   tmp = lappend(tmp,stric); */
      /*   Node * result = expression_tree_mutator((Node *)makeBoolExpr(OR_EXPR,tmp,-1),my_mutator,context); */
      /*   return result; */
      /* } */
      /* if(!context->positive && oExpr->opno == 1209){ #<{(| LIKE |)}># */
      /*   context->ready = true; */
      /* } */


      Node * result = expression_tree_mutator(node, my_mutator, (void *) context);
      if(context->constraint_to_add != NULL){
        if(!context->positive /* && oExpr->opno == 518 */){
          context->constraint_to_add = lappend(context->constraint_to_add,result);
          Node * to_return = makeBoolExpr(OR_EXPR,context->constraint_to_add,-1);
          context->constraint_to_add = NULL;
          context->ready = false;
          return to_return;
        }
        if(context->positive /* && oExpr->opno == 96 */){
          context->constraint_to_add = lappend(context->constraint_to_add,result);
          Node * to_return = makeBoolExpr(AND_EXPR,context->constraint_to_add,-1);
          context->constraint_to_add = NULL;
          context->ready = false;
          return to_return;
        }
      }
      else
        return result;

    }
 		if(context->ready && IsA(node, Var))
 		{
      Var * v = (Var *) node;
      Var * rv = getTrueVar(context->current_trueVar,v);
      if(!context->positive && !isInListTrueVar(context->list_of_not_null_in_current,rv)){
        FuncExpr *is_null = makeNode(FuncExpr);
        is_null->funcid = 32814;
        is_null->funcresulttype = 16;
        is_null->args = lappend(is_null->args,node);
        /* OpExpr * inf0 = makeNode(OpExpr); */
        /* inf0->opno = 97; #<{(|<|)}># */
        /* inf0->args = lappend(inf0->args,node); */
        /* Const * c0 = makeConst(23,-1,0,4,Int16GetDatum(0),false, true); */
        /* inf0->args = lappend(inf0->args,c0); */
        context->constraint_to_add = lappend(context->constraint_to_add,is_null);
      }
      if(context->positive){
        FuncExpr *is_null = makeNode(FuncExpr);
        is_null->funcid = 32815;
        is_null->funcresulttype = 16;
        is_null->args = lappend(is_null->args,node);
        /* OpExpr * inf0 = makeNode(OpExpr); */
        /* inf0->opno = 97; #<{(|<|)}># */
        /* inf0->args = lappend(inf0->args,node); */
        /* Const * c0 = makeConst(23,-1,0,4,Int16GetDatum(0),false, true); */
        /* inf0->args = lappend(inf0->args,c0); */
        context->constraint_to_add = lappend(context->constraint_to_add,is_null);
      }

 		}
 		return expression_tree_mutator(node, my_mutator, (void *) context);
  }



static void
my_post_parse_analyse_hook(ParseState *pstate, Query *query)
{
  char * result = malloc(128);
  result = GetConfigOptionByName	(	"certain_answer_marked.certain",&result,false);	
  int certain = atoi(result);
  /* elog(LOG,"%d --- %s \n",certain,result); */
  if(certain == 0){
    return;
  }
  if(certain == 1){
  context_walker_set_constraint * ctx = make_context_walker_set_constraint();
  my_walker((Node *)query,ctx);
  context_modifier * ctx_m = make_context_modifier(true,ctx);
  my_mutator((Node *)query,ctx_m);
  optimize_self_constraint_deep(query);
  elog(LOG,"1\n");
  optimize_split_deep(query);
  elog(LOG,"2\n");
  optimize_exist_deep(query);
  elog(LOG,"3\n");
  }
  if(certain == 2){
  context_walker_set_constraint * ctx = make_context_walker_set_constraint();
  my_walker((Node *)query,ctx);
  context_modifier * ctx_m = make_context_modifier(true,ctx);
  ctx_m->positive = false;
  my_mutator((Node *)query,ctx_m);
  optimize_self_constraint_deep(query);
  optimize_split_deep(query);
  /* elog(LOG,"SPLIT::: %s \n",nodeToString(query->jointree)); */
  optimize_exist_deep(query);

  }
    /* elog(LOG,"EXISTS:: %s \n",nodeToString(query->jointree)); */
}
