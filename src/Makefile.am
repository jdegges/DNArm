bin_PROGRAMS =

#if BUILD_CGM
##bin_PROGRAMS += cgm
#cgm_SOURCES = cgm.c memdb.c
#endif

if BUILD_CTB
bin_PROGRAMS += ctb
ctb_SOURCES = ctb.c
ctb_LDADD = $(CTB_LDADD)
ctb_CFLAGS = $(CTB_CFLAGS)
endif

if BUILD_GENOME_GEOMETRY
bin_PROGRAMS += genome-geometry
genome_geometry_SOURCES = genome-geometry.c cvdb.c
endif

if BUILD_GENOME_INDEX
bin_PROGRAMS += genome-index
genome_index_SOURCES = genome-index.c cvdb.c cldb.c
genome_index_LADD = $(GENOME_INDEX_LADD)
genome_index_CFLAGS = $(GENOME_INDEX_CFLAGS)
endif

if BUILD_INDEX
bin_PROGRAMS += index
index_SOURCES = index.c tchdb.c
index_LDADD = $(INDEX_LDADD)
index_CFLAGS = $(INDEX_CFLAGS)
endif

if BUILD_MEMDB_TEST
bin_PROGRAMS += memdb-test
memdb_test_SOURCES = db-test.c memdb.c
endif

if BUILD_PINDEX
bin_PROGRAMS += pindex
pindex_SOURCES = pindex.c memdb.c
pindex_LDADD = $(PINDEX_LDADD)
pindex_CFLAGS = $(PINDEX_CFLAGS)
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

if BUILD_TCADB_TEST
bin_PROGRAMS += tcadb-test
tcadb_test_SOURCES = db-test.c tcadb.c
tcadb_test_LDADD = $(TCADB_TEST_LDADD)
tcadb_test_CFLAGS = $(TCADB_TEST_CFLAGS)
endif

if BUILD_TCHDB_TEST
bin_PROGRAMS += tchdb-test
tchdb_test_SOURCES = db-test.c tchdb.c
tchdb_test_LDADD = $(TCHDB_TEST_LDADD)
tchdb_test_CFLAGS = $(TCHDB_TEST_CFLAGS)
endif
