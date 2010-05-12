#define _POSIX_SOURCE
#define _BSD_SOURCE

#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#define BUF_LEN 1024

#define print_error(format, ...) { \
    fprintf (stderr, "[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
    fprintf (stderr, format, ##__VA_ARGS__); \
    fprintf (stderr, "\n"); \
}

static FILE *infp;
static FILE *outfp;

static size_t read_length;
static size_t bpb;
static size_t mrate1;
static size_t mrate2;
static size_t mrate3;

static size_t
xrandom (void)
{
  return random ();
}

static int
readgen (void)
{
  char *data = NULL;
  struct stat buf;
  off_t start;

  if (NULL == (data = malloc ((sizeof *data) * read_length * bpb / 8)))
    {
      print_error ("Out of memory");
      return 1;
    }

  if (0 != fstat (fileno (infp), &buf))
    {
      print_error ("%s", strerror (errno));
      exit (EXIT_FAILURE);
    }

  /* pick a random starting point (this assumes that every base is
   * byte-aligned) */
  srandom (time (NULL));
  start = xrandom () % (buf.st_size - read_length*bpb/8);

  if (start < 0)
    {
      print_error ("Input does not contain enough bases.");
      exit (EXIT_FAILURE);
    }

  /* seek to the random position in the file */
  if (0 != fseek (infp, start, SEEK_SET))
    {
      print_error ("Failed to seek to random position in input file");
      exit (EXIT_FAILURE);
    }

  /* actually read from the input file */
  if (read_length*bpb/8 != fread (data, 1, read_length*bpb/8, infp))
    {
      print_error ("Error reading data");
      return 1;
    }

  /* apply any mutations */
  if (xrandom () % 100 < mrate1)
    data[xrandom () % (read_length*bpb/8)]++;

  if (xrandom () % 100 < mrate2)
    {
      data[xrandom () % (read_length*bpb/8)]++;
      data[xrandom () % (read_length*bpb/8)]++;
    }

  if (xrandom () % 100 < mrate3)
    {
      data[xrandom () % (read_length*bpb/8)]++;
      data[xrandom () % (read_length*bpb/8)]++;
      data[xrandom () % (read_length*bpb/8)]++;
    }

  fwrite (&start, sizeof start, 1, outfp);
  fwrite (data, 1, read_length*bpb/8, outfp);

  free (data);

  return 0;
}

static void
usage (void)
{
  printf ("\n");
  printf ("readgen -r 32 -b 8\n");
  printf ("\n");
  printf (" --input, -i           Input file (omit or - for stdin)\n");
  printf (" --output, -o          Output file (omit or - for stdout)\n");
  printf (" --read-length, -r     The number of bases per read\n");
  printf (" --bpb, -b             The number of bits per base in the input\n");
  printf (" --mrate1, -x          Probability of one mutation\n");
  printf (" --mrate2, -y          Probability of two mutations\n");
  printf (" --mrate3, -z          Probability of three mutations\n");
  printf (" --help, -h            This help menu.\n");
  printf ("\n");
}

static unsigned long int
xstrtoul (const char *nptr)
{
  unsigned long int r = strtoul (nptr, NULL, 10);
  if (errno == EINVAL || errno == ERANGE)
    {
      usage ();
      exit (EXIT_FAILURE);
    }
  return r;
}

static FILE*
xfopen (const char *path, const char *mode)
{ 
  FILE *fp = fopen (path, mode);

  if (NULL == fp)
    {
      print_error ("%s", strerror (errno));
      return NULL;
    }

  return fp;
}

int
main (int argc, char **argv)
{
  static struct option long_options[] = {
    {"input",       required_argument, 0, 'i'},
    {"output",      required_argument, 0, 'o'},
    {"read-length", required_argument, 0, 'r'},
    {"bpb",         required_argument, 0, 'b'},
    {"mrate1",      required_argument, 0, 'x'},
    {"mrate2",      required_argument, 0, 'y'},
    {"mrate3",      required_argument, 0, 'z'},
    {"help",        no_argument,       0, 'h'}
  };

  outfp = NULL;
  read_length = 48;
  bpb = 2;
  mrate1 = mrate2 = mrate3 = 0;

  for (;;)
    {
      int option_index = 0;
      int c = getopt_long (argc, argv, "i:o:p:r:n:b:x:y:z:h", long_options,
                           &option_index);

      if (-1 == c)
        break;

      switch (c)
        {
          case 'i':
            if (optarg[0] == '-' && optarg[1] == '\0')
              infp = stdin;
            else
              assert (NULL != (infp = xfopen (optarg, "r")));
            break;
          case 'o':
            if (optarg[0] == '-' && optarg[1] == '\0')
              outfp = stdout;
            else
              assert (NULL != (outfp = xfopen (optarg, "w")));
            break;
          case 'r':
            read_length = xstrtoul (optarg);
            break;
          case 'b':
            bpb = xstrtoul (optarg);
            break;
          case 'x':
            mrate1 = xstrtoul (optarg);
            break;
          case 'y':
            mrate2 = xstrtoul (optarg);
            break;
          case 'z':
            mrate3 = xstrtoul (optarg);
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

  if (NULL == infp)
    infp = stdin;
  if (NULL == outfp)
    outfp = stdout;

  if (0 != readgen ())
    print_error ("Failed to convert input to binary.");

  fclose (infp);
  fclose (outfp);

  return 0;
}
