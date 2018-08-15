# contrib/only_encrypted_passwords/Makefile

MODULE_big = certain_answer_codd
OBJS = certain_answer_codd.o true_var.o self_optimize.o split_optimize.o exist_optimize.o

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/certain_answer_codd
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
