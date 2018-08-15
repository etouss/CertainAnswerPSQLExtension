#ifndef EXIST_OPTIMIZE_H
#define EXIST_OPTIMIZE_H

#include "postgres.h"
#include "parser/parse_node.h"
#include "parser/analyze.h"
#include "nodes/nodeFuncs.h"
#include "nodes/makefuncs.h"

#include <ctype.h>
#include <sys/resource.h>

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

#include "self_optimize.h"


void optimize_exist(Query *q);
void optimize_exist_deep(Query *q);


#endif
