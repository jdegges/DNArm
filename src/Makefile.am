bin_PROGRAMS =

if BUILD_CGM
bin_PROGRAMS += cgm
cgm_SOURCES = cgm.c
endif

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

if BUILD_READGEN
bin_PROGRAMS += readgen
readgen_SOURCES = readgen.c
endif

if BUILD_SAMPLECL
bin_PROGRAMS += samplecl
samplecl_SOURCES = samplecl.c
samplecl_LDADD = $(SAMPLECL_LDADD)
samplecl_CFLAGS = $(SAMPLECL_CFLAGS)
endif
