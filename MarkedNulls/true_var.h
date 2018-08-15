#ifndef TRUE_VAR_H
#define TRUE_VAR_H

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

// typedef enum typeAdd{
//   QUERY_ADD,
//   AND_ADD,
//   FIRSTOR_ADD,
//   OR_ADD
// }typeAdd;

typedef struct context_walker_set_constraint{
  /*usefull for true_var*/
  List * trueVars;
  List * current_trueVar;
  // int current_varlevelsup;
  // int current_varattno;
  /*Not null constraint*/
  List * list_of_not_null_att_schem;
  List * list_of_not_null_in_op;
  List * list_of_not_null_to_be_true;
  List * list_of_not_null_to_be_false;

  /*Do i have to add the var*/
  /*Operation needed to add*/
  bool ready;

  /*Where i am*/
  List * list_list_true;
  List * list_list_false;
  int where_i_am_querry;

}context_walker_set_constraint;

typedef struct context_modifier {
  bool positive;
  bool ready;
  List * constraint_to_add;
  List * trueVars;
  List * current_trueVar;
  

  List * list_of_not_null_in_current;
  // List * list_of_not_null_att_schem;
  List * list_list_true;
  List * list_list_false;

  int where_i_am;
  int where_i_am_querry;
  // int current_varlevelsup;

}context_modifier;


context_walker_set_constraint* make_context_walker_set_constraint(void);
context_modifier* make_context_modifier(bool positive, context_walker_set_constraint * context);
void free_context_walker_set_constraint(context_walker_set_constraint * ctx);
void free_context_modifier(context_modifier * ctx);

bool
get_pg_att_not_null(Oid relid, AttrNumber attnum);

int
get_relnatts(Oid relid);

Var * getTrueVar(List* trueVars,Var * v);
bool isInListTrueVar(List * trueVar, Var * v);
// bool isInListNullVar(List * trueVars, Var * v);
List * union_list(List * l);
List * inter_list(List * l,int nb_arg);



#endif 
