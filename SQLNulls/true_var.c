#include "true_var.h"

context_walker_set_constraint* make_context_walker_set_constraint(){
  context_walker_set_constraint * ctx = palloc(sizeof(context_walker_set_constraint));
  ctx->trueVars = NULL;
  ctx->current_trueVar = NULL;
  ctx->list_of_not_null_att_schem = NULL;
  ctx->list_of_not_null_in_op = NULL;
  ctx->list_of_not_null_to_be_true = NULL;
  ctx->list_of_not_null_to_be_false = NULL;

  ctx->ready = false;

  ctx->list_list_true = NULL;
  ctx->list_list_false = NULL;
  ctx->where_i_am_querry = 0;

  return ctx;
}


context_modifier* make_context_modifier(bool positive, context_walker_set_constraint * context){
   context_modifier * ctx = palloc(sizeof(context_modifier));
   ctx->positive = positive;
   ctx->ready = false;
   ctx->constraint_to_add = NULL;
   ctx->list_of_not_null_in_current = context->list_of_not_null_att_schem;
   ctx->trueVars = context->trueVars;
   ctx->current_trueVar = list_nth(context->trueVars,0);

   ctx->list_list_true = context->list_list_true;
   ctx->list_list_false = context->list_list_false;
   ctx->where_i_am = 0;
   ctx->where_i_am_querry = 0;
   return ctx;
}

void free_context_walker_set_constraint(context_walker_set_constraint * ctx){
  /* elog(LOG,"Here1"); */
  if(ctx->trueVars != NULL)
     list_free_deep(ctx->trueVars);
  /* elog(LOG,"Here2"); */
  if(ctx->current_trueVar != NULL)
     list_free_deep(ctx->current_trueVar);
  /* elog(LOG,"Here3"); */
  /* if(ctx->list_of_not_null_att_schem != NULL) */
     /* list_free_deep(ctx->list_of_not_null_att_schem); */
  /* elog(LOG,"Here4"); */
  /* if(ctx->list_of_not_null_in_op != NULL) */
     /* list_free_deep(ctx->list_of_not_null_in_op); */
  /* elog(LOG,"Here5"); */
  /* if(ctx->list_of_not_null_to_be_true != NULL) */
  /*    list_free_deep(ctx->list_of_not_null_to_be_true); */
  /* elog(LOG,"Here6"); */
  /* if(ctx->list_of_not_null_to_be_false != NULL) */
  /*    list_free_deep(ctx->list_of_not_null_to_be_false); */
  /* elog(LOG,"Here7"); */
  if(ctx->list_list_true != NULL)
     list_free_deep(ctx->list_list_true);
  /* elog(LOG,"Here8"); */
  if(ctx->list_list_false != NULL)
     list_free_deep(ctx->list_list_false);
  /* elog(LOG,"Here9"); */
  pfree(ctx);
  /* elog(LOG,"Here10"); */
}

void free_context_modifier(context_modifier * ctx){
   /* elog(LOG,"Here1"); */
   if(ctx->constraint_to_add != NULL)
     list_free_deep(ctx->constraint_to_add);
   /* elog(LOG,"Here2"); */
   if(ctx->list_of_not_null_in_current != NULL)
     list_free_deep(ctx->list_of_not_null_in_current);
   /* elog(LOG,"Here3"); */
   /* if(ctx->trueVars != NULL) */
   /*   list_free_deep(ctx->trueVars); */
   /* elog(LOG,"Here4"); */
   /* if(ctx->current_trueVar != NULL) */
   /*   list_free(ctx->current_trueVar); */
   /* elog(LOG,"Here5"); */
   /* if(ctx->list_list_true != NULL) */
   /*   list_free_deep(ctx->list_list_true); */
   /* elog(LOG,"Here6"); */
   /* if(ctx->list_list_false != NULL) */
   /*   list_free_deep(ctx->list_list_false); */
   /* elog(LOG,"Here7"); */
   pfree(ctx);
   /* elog(LOG,"Here8"); */
}


bool
get_pg_att_not_null(Oid relid, AttrNumber attnum)
{
	HeapTuple	tp;

	tp = SearchSysCache2(ATTNUM,
						 ObjectIdGetDatum(relid),
						 Int16GetDatum(attnum));
	if (HeapTupleIsValid(tp))
	{
		Form_pg_attribute att_tup = (Form_pg_attribute) GETSTRUCT(tp);
    bool result = att_tup->attnotnull;
    ReleaseSysCache(tp);
    return result;
		/* return att_tup; */
	}
	else
		return false;
}

int
get_relnatts(Oid relid)
{
	HeapTuple	tp;

	tp = SearchSysCache1(RELOID, ObjectIdGetDatum(relid));
	if (HeapTupleIsValid(tp))
	{
		Form_pg_class reltup = (Form_pg_class) GETSTRUCT(tp);
		int			result;

		result = reltup->relnatts;
		ReleaseSysCache(tp);
		return result;
	}
	else
		return InvalidAttrNumber;
}


Var * getTrueVar(List* current_trueVars,Var * v){
  List * temp = current_trueVars;
  for(int i = 0;i<v->varlevelsup;i++){
    temp = list_nth(temp,0);
  }
  return (Var *) list_nth(list_nth(temp,v->varno),v->varattno-1);
}

/* bool isInListNullVar(List * trueVars, Var * v){ */
/*   if(trueVars == NULL) return false; */
/*   const ListCell *cell; */
/*   	foreach(cell, trueVars) */
/* 	  { */
/*       Var * t = lfirst(cell); */
/*       if(t->varattno == v->varattno && t->vartype == v->vartype){ */
/*         if(t->varlevelsup == -1) */
/*           return true; */
/*       } */
/*     } */
/*     return false; */
/* } */


bool isInListTrueVar(List * trueVars, Var * v){
  if(trueVars == NULL) return false;
  const ListCell *cell;
  	foreach(cell, trueVars)
	  {
      Var * t = lfirst(cell);
      if(t->varattno == v->varattno && t->vartype == v->vartype &&t->varlevelsup == v->varlevelsup){
        return true;
      }
    }
    return false;
}

List * union_list(List * l){
  if(l == NULL) return l;
  List * new_list = NULL;
  const ListCell *cell;
	  foreach(cell, l)
	  {
      Var * t = lfirst(cell);
      if(!isInListTrueVar(new_list,t))
        new_list = lappend(new_list,t);
    }
  return new_list;
}

List * inter_list(List * l,int nb_arg){
  if(l == NULL) return l;
  List * new_list = NULL;
  const ListCell *cell;
	  foreach(cell, l)
	  {
      Var * v = lfirst(cell);
      int nb_in = 0;
      const ListCell *cell_in;
      foreach(cell_in, l)
	    {
        Var * t = lfirst(cell_in);
        if(v->varattno == t->varattno && v->vartype == t->vartype)
          nb_in ++;
      }
      if(nb_in == nb_arg && !isInListTrueVar(new_list,v))
      new_list = lappend(new_list,v);
    }
  return new_list;
}

