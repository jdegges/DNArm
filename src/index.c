#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <endian.h>

#define BUFLEN 1024
#define INPUTLEN 1024
#define KEYLEN 32

#include <db.h>

static struct db *db;

int gpos = 0;

void findkeys(uint32_t *input, int inlen){
  while (--inlen) {
    int j;
    uint32_t p[2] = {input[1], input[0]};
    uint64_t whole = htole64 (* (uint64_t *) p);
    input++;

		for (j = 0; j < KEYLEN; j+=2){
			uint32_t key = (whole << j) >> 32;
			assert (db_insert(db, key, gpos++));
		}
	}
}

static void test_index(const char *path)
{
  FILE *fp = fopen (path, "rb");
  uint32_t *input;

  for (;;) {
    int i;
    input = malloc ((sizeof *input) * INPUTLEN);
    if (NULL == input)
    {
      printf ("Out of memory\n");
      exit (EXIT_FAILURE);
    }

    int curr_pos = ftell (fp);
    int numRead = fread (input, sizeof *input, INPUTLEN, fp);
    if (0 == numRead)
      break;

    for (i = 0; i < numRead; i++)
    {
      int j;
      uint32_t pos = curr_pos*4 + i*16;
      uint32_t *values = NULL;
      int32_t count = db_query (db, input[i], &values);

      if (count <= 0)
      {
        printf ("Error querying. Got %d hits for key=%u\n", count, input[i]);
        exit (EXIT_FAILURE);
      }

      for (j = 0; j < count; j++)
      {
        if (pos == values[j])
          break;
      }
      if (count <= j)
      {
        printf ("didnt find (k,v) = (%u,%u)\n", input[i], pos);
      }

      free (values);
    }
    free (input);
  }
  free (input);
  
  fclose (fp);
}

int main(int argc, char **argv){
	uint32_t last;
	uint32_t *buf;
  int more_input;

	assert (db = db_open ("db.bin", DB_MODE_WRITE_ONLY));

	// Open file 
	if (argc != 2)
	{
		printf ("Not enough arguments.\n");
	}

	FILE *fp = fopen(argv[1], "rb");
	if (NULL == fp)
	{
		printf("File open error!\n");
		return -1;
	}

	// allocate space to hold input from file
	if (sizeof last != fread (&last, 1, sizeof last, fp))
	{
		printf ("couldnt read first 32bits\n");
		exit (EXIT_FAILURE);
	}

	if (NULL == (buf = malloc ((sizeof *buf) * INPUTLEN)))
	{
		printf ("out of memory");
		exit (EXIT_FAILURE);
	}

	// store last int of each iteration

  more_input = 1;
	while (more_input){
		size_t amount_read;

		buf[0] = last;
		amount_read = fread (buf+1, sizeof *buf, INPUTLEN-1, fp) + 1;
		if (amount_read != INPUTLEN)
		{
      if (ferror (fp))
        {
          printf ("error on stream\n");
          break;
        }
      more_input = 0;
		}

		findkeys (buf, amount_read);
		last = buf[amount_read-1];
	}

  db_insert (db, last, gpos++);

	free (buf);
	fclose (fp);

  assert (db_close (db));

//  assert (db = db_open ("db.bin", DB_MODE_READ_ONLY));
//	test_index (argv[1]);
//  assert (db_close (db));

	return 0;
}
