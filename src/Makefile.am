bin_PROGRAMS =

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

if BUILD_PARALLEL_DB
bin_PROGRAMS += parallel-db
parallel_db_SOURCES = db_test.c cldb.c cvdb.c
parallel_db_LADD = $(PARALLEL_DB_LADD)
parallel_db_CFLAGS = $(PARALLEL_DB_CFLAGS)
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
