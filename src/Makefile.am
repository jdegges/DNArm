bin_PROGRAMS = ctb

ctb_SOURCES = ctb.c
ctb_LDADD = $(LOOMLIB_LIBS) $(PTHREAD_LIBS)
ctb_CFLAGS = $(LOOMLIB_CFLAGS) $(PTHREAD_CFLAGS)

bin_PROGRAMS += memdb-test
memdb_test_SOURCES = db-test.c memdb.c
