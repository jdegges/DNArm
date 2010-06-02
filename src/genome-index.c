#define _BSD_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <getopt.h>
#include <string.h>
#include <cldb.h>

#define print_error(msg){ \
    fprintf (stderr, "[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
    fprintf (stderr, "%s\n", (msg)); \
}

static void
usage (void)
{
  printf ("\n");
  printf ("genome-index -o <geometry_database> -n <packed_genome> -i <index_db>\n");
  printf ("\n");
  printf ("  --geometry, -o             Path to the geometry database file\n");
  printf ("                               (see genome-geometry).\n");
  printf ("  --genome, -n               Path to a packed genome (see ctb).\n");
  printf ("  --index, -i                Where to create genome index.\n");
  printf ("  --help, -h                 This help menu.\n");
  printf ("\n");
}

int
main (int argc, char **argv)
{
  char geometry[PATH_MAX] = {0};
  char genome[PATH_MAX] = {0};
  char index[PATH_MAX] = {0};

  static struct option long_options[] = {
    {"geometry", required_argument, 0, 'o'},
    {"genome", required_argument, 0, 'n'},
    {"index", required_argument, 0, 'i'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0},
  };

  for (;;)
    {
      int option_index = 0;
      int c = getopt_long (argc, argv, "o:n:i:h", long_options, &option_index);

      if (-1 == c)
        break;

      switch (c)
        {
          case 'o':
            if (PATH_MAX <= strlen (optarg))
              {
                fprintf (stderr, "Geometry path too long\n");
                usage ();
                return 1;
              }
            strcpy (geometry, optarg);
            break;
          case 'n':
            if (PATH_MAX <= strlen (optarg))
              {
                fprintf (stderr, "Genome path too long");
                usage ();
                return 1;
              }
            strcpy (genome, optarg);
            break;
          case 'i':
            if (PATH_MAX <= strlen (optarg))
              {
                fprintf (stderr, "Index path too long\n");
                usage ();
                return 1;
              }
            strcpy (index, optarg);
            break;
          case 'h':
            usage ();
            return 0;
          case '?':
          default:
            printf ("Unknown option\n");
            usage ();
            return 1;
        }
    }

  if ('\0' == *geometry)
    {
      fprintf (stderr, "You must specify a geometry path\n");
      usage ();
      return 1;
    }

  if ('\0' == *genome)
    {
      fprintf (stderr, "You must specify a valid genome path\n");
      usage ();
      return 1;
    }

  if ('\0' == *index)
    {
      fprintf (stderr, "You must specify an index path\n");
      usage ();
      return 1;
    }

  if (false == cldb_new (index, geometry, genome))
    {
      fprintf (stderr, "Error building genome index");
      return 1;
    }

  return 0;
}
