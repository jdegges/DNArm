#include <stdlib.h>
#include <stdio.h>

#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <pthread.h>
#include <loomlib/thread_pool.h>

#define BUF_LEN 1024

#define print_error(format, ...) { \
    fprintf (stderr, "[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
    fprintf (stderr, format, ##__VA_ARGS__); \
    fprintf (stderr, "\n"); \
}

struct convert_args
{
  char *data;
  size_t id;
};

static FILE *outfp;
static size_t outid;
static pthread_mutex_t outfp_mutex;
static pthread_cond_t outfp_cond;

/*
 * Base | lower case | upper case
 * ------------------------------
 * A    | 0110 0001  | 0100 0001
 * C    | 0110 0011  | 0100 0011
 * G    | 0110 0111  | 0100 0111
 * T    | 0111 0100  | 0101 0100
 *              ^^           ^^
 *               
 * Only the highlighted bits are used:
 * `(char & 0000 0110)` zero's out the other bits
 * `(char & 0000 0110) << 5` shifts the two bits we care about to the MSB
 */
static void
convert (void *vptr)
{
  struct convert_args *args = vptr;
  char *in = args->data;
  char *inp;
  char *out;
  char *outp;
  size_t len = BUF_LEN;
  size_t id = args->id;

  free (args);

  if (NULL == (out = malloc (len / 4)))
    {
      print_error ("Out of memory");
      return;
    }

  inp = in;
  outp = out;

  while (inp <= in+len-4)
    {
      *outp    = ((*inp++ & 6) << 5); // XX00 0000
      *outp   |= ((*inp++ & 6) << 3); // 00XX 0000
      *outp   |= ((*inp++ & 6) << 1); // 0000 XX00
      *outp++ |= ((*inp++ & 6) >> 1); // 0000 00XX
    }

  free (in);

  /* lock output file pointer */
  pthread_mutex_lock (&outfp_mutex);

  /* make sure its our turn to write */
  while (id != outid)
    pthread_cond_wait (&outfp_cond, &outfp_mutex);

  /* actually write */
  assert (len/4 == fwrite (out, 1, len / 4, outfp));

  /* tell everyone else to wake up and write */
  outid++;
  pthread_cond_broadcast (&outfp_cond);

  pthread_mutex_unlock (&outfp_mutex);

  free (out);
}

static void
convert_inv (void *vptr)
{
  struct convert_args *args = vptr;
  char *in = args->data;
  char *inp;
  char *out;
  char *outp;
  size_t len = BUF_LEN;
  size_t id = args->id;

  free (args);

  if (NULL == (out = malloc (len * 4)))
    {
      print_error ("Out of memory");
      return;
    }

  inp = in;
  outp = out;

  while (inp < in+len)
    {
      *outp = ((*inp >> 5) & 6) | 65;
      *outp = *outp == 69 ? 84 : *outp;
      outp++;

      *outp = ((*inp >> 3) & 6) | 65;
      *outp = *outp == 69 ? 84 : *outp;
      outp++;

      *outp = ((*inp >> 1) & 6) | 65;
      *outp = *outp == 69 ? 84 : *outp;
      outp++;

      *outp = ((*inp++ << 1)&6) | 65;
      *outp = *outp == 69 ? 84 : *outp;
      outp++;
    }

  free (in);

  /* lock output file pointer */
  pthread_mutex_lock (&outfp_mutex);

  /* make sure its our turn to write */
  while (id != outid)
    pthread_cond_wait (&outfp_cond, &outfp_mutex);

  /* actually write */
  assert (len*4 == fwrite (out, 1, len * 4, outfp));

  /* tell everyone else to wake up and write */
  outid++;
  pthread_cond_broadcast (&outfp_cond);

  pthread_mutex_unlock (&outfp_mutex);

  free (out);
}

static int
ctb (FILE *infp, const size_t parallel, bool inverse)
{
  char *data = NULL;
  struct thread_pool *pool;
  size_t id;
  int ret_val;

  assert (pool = thread_pool_new (parallel));

  if (0 != (ret_val = pthread_mutex_init (&outfp_mutex, NULL)))
    {
      print_error ("Failed to init mutex: %d", ret_val);
      return 1;
    }

  if (0 != (ret_val = pthread_cond_init (&outfp_cond, NULL)))
    {
      print_error ("Failed to init condvar: %d", ret_val);
      return 1;
    }

  if (NULL == (data = malloc ((sizeof *data) * BUF_LEN)))
    {
      print_error ("Out of memory");
      return 1;
    }


  id = 0;
  while (BUF_LEN == fread (data, sizeof *data, BUF_LEN, infp))
    {
      struct convert_args *args;

      if (NULL == (args = malloc (sizeof *args)))
        {
          print_error ("Out of memory");
          return 1;
        }

      args->data = data;
      args->id = id++;
      if (inverse)
        assert (thread_pool_push (pool, convert_inv, args));
      else
        assert (thread_pool_push (pool, convert, args));

      if (NULL == (data = malloc ((sizeof *data) * BUF_LEN)))
        {
          print_error ("Out of memory");
          return 1;
        }
    }

  free (data);
  assert (thread_pool_terminate (pool));
  thread_pool_free (pool);

  return 0;
}

static void
usage (void)
{
  printf ("\n");
  printf ("ctb -i input_file -o output_file\n");
  printf ("\n");
  printf ("Sample usage:\n");
  printf ("$ wget ftp://ftp.ncbi.nlm.nih.gov/genomes/H_sapiens/Assembled_chromosomes/hs_alt_Celera_chr2.fa.gz\n");
  printf ("$ gunzip hs_alt_Celera_chr2.fa.gz\n");
  printf ("$ grep -v \">\" hs_alt_Celera_chr2.fa | tr -d '\\n' > sample_input.txt\n");
  printf ("$ cut -c 1-20480 sample_input.txt | sed 's/N/A/g' | tr -d '\\n' > small_input.txt\n");
  printf ("$ ctb -i small_input.txt -o packed.txt\n");
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
  FILE *infp = outfp = NULL;
  size_t parallel = 1;
  bool inverse = false;

  static struct option long_options[] = {
    {"input",     required_argument, 0, 'i'},
    {"output",    required_argument, 0, 'o'},
    {"parallel",  required_argument, 0, 'p'},
    {"inverse",   no_argument      , 0, 'n'}
  };

  for (;;)
    {
      int option_index = 0;
      int c = getopt_long (argc, argv, "i:o:p:n", long_options, &option_index);

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
          case 'p':
            parallel = strtoul (optarg, NULL, 10);
            if (EINVAL == errno || ERANGE == errno)
              {
                usage ();
                return 1;
              }
            break;
          case 'n':
            inverse = true;
            break;
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

  if (0 != ctb (infp, parallel, inverse))
    print_error ("Failed to convert input to binary.");

  fclose (infp);
  fclose (outfp);

  return 0;
}
