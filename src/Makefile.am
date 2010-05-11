bin_PROGRAMS =

if BUILD_CTB
bin_PROGRAMS += ctb
ctb_SOURCES = ctb.c
ctb_LDADD = $(CTB_LDADD)
ctb_CFLAGS = $(CTB_CFLAGS)
endif

if BUILD_MEMDB_TEST
bin_PROGRAMS += memdb-test
memdb_test_SOURCES = db-test.c memdb.c
endif
