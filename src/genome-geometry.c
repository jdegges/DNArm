#define _BSD_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <cvdb.h>

#define BUFFER_SIZE  0x80000  /* 2**11 * 32 bits = 2MB */
#define KEY_BITS 32

#define print_error(msg){ \
    fprintf (stderr, "[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
    fprintf (stderr, "%s\n", (msg)); \
}

/* its necessary to align our access pattern with the cache size to optimize
 * performance, here cache is 512M so we must only access values that will be
 * placed within 512M of eachother (2**27 * 32 bits == 512M)
 */
static const uint32_t key_space = 32; /* 2**5 == 32 */
static const uint32_t key_shift = 27; /* 32 - 5 = 27 */

static bool
compute_key_freq (uint32_t *bases, size_t length, uint32_t key_mask,
                  struct cvdb *db)
{
  while (--length)
    {
      uint64_t whole = *bases;
      uint64_t p = *bases++;
      int i;
      whole = (whole << 32) | p;

      for (i = 0; i < KEY_BITS; i += 2)
        {
          uint32_t key = (whole << i) >> 32;
          if (key >> key_shift == key_mask)
            {
              if (false == cvdb_increment (db, key))
                {
                  print_error ("db_increment");
                  return false;
                }
            }
        }
    }

  return true;
}

static bool
build_geometry_db (char *path, char *genome)
{
  FILE *fp = NULL;
  uint32_t *buf = NULL;
  uint32_t i;
  struct cvdb *db = NULL;
  bool retval = false;

  /* connect to the db */
  if (NULL == (db = cvdb_open (path, DB_MODE_WRITE)))
    {
      print_error ("Failed to open gdb");
      return false;
    }

  /* create 32MB buffer */
  if (NULL == (buf = calloc (sizeof (uint32_t), BUFFER_SIZE)))
    {
      print_error ("Out of memory");
      goto exit;
    }

  for (i = 0; i < key_space; i++)
    {
      size_t amount_read;
      uint32_t last;

      /* open packed genome file */
      if (NULL == (fp = fopen (genome, "rb")))
        {
          print_error (strerror (errno));
          goto exit;
        }

      if (1 != fread (&last, sizeof (uint32_t), 1, fp))
        {
          print_error ("fread");
          goto exit;
        }

      while (BUFFER_SIZE-1 == (amount_read = fread (buf+1, sizeof (uint32_t),
                                                    BUFFER_SIZE-1, fp))
             || (0 == ferror (fp) && 0 < amount_read))
        {
          buf[0] = last;
          if (false == compute_key_freq (buf, amount_read + 1, i, db))
            {
              print_error ("compute_key_freq");
              goto exit;
            }
          last = buf[amount_read];
        }

      if (0 != ferror (fp))
        {
          print_error ("Stream error");
          goto exit;
        }

      if (last >> key_shift == i && false == cvdb_increment (db, last))
        {
          print_error ("db_increment");
          goto exit;
        }

      if (0 != fclose (fp))
        {
          print_error (strerror (errno));
          goto exit;
        }
      fp = NULL;
    }

  retval = true;

exit:
  if (NULL != fp && 0 != fclose (fp))
    print_error (strerror (errno));
  free (buf);
  if (NULL != db && false == cvdb_close (db))
    print_error ("Failed to close cvdb");
  return retval;
}

static void
usage (void)
{
  printf ("\n");
  printf ("genome-geometry -o <geometry_database> -n <packed_genome>\n");
  printf ("\n");
  printf ("  --geometry, -o             Where the geometry database file will "
                                        "be created.\n");
  printf ("  --genome, -n               Path to a packed genome (see ctb).\n");
  printf ("  --help, -h                 This help menu.\n");
  printf ("\n");
}

int
main (int argc, char **argv)
{
  char geometry[PATH_MAX] = {0};
  char genome[PATH_MAX] = {0};

  static struct option long_options[] = {
    {"geometry", required_argument, 0, 'o'},
    {"genome", required_argument, 0, 'n'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0},
  };

  for (;;)
    {
      int option_index = 0;
      int c = getopt_long (argc, argv, "o:n:h", long_options, &option_index);

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
                printf ("Genome path too long");
                usage ();
                return 1;
              }
            strcpy (genome, optarg);
            break;
          case 'h':
            usage ();
            return 0;
          case '?':
          default:
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

  if (false == build_geometry_db (geometry, genome))
    {
      fprintf (stderr, "Error building geometry database");
      return 1;
    }

  return 0;
}
