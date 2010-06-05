#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <cvdb.h>
#include <cldb.h>
#include <pthread.h>
/* File structures:
 * L1 block:
 *     <---- 2**8 = 256 items --->
 * +-----------------------------------------+
 * | L2 index ptr (64 bits)     | ...        |
 * | - - - - - - - - - - - - - - - - - - - - |
 * | size of L2 data (64 bits)  | ...        |
 * +-----------------------------------------+
 *
 * L2 index block:
 *     <---- 2**24 items -------->
 * +----------------------------------------------+
 * | uint32_t offset into L2 data (32 bits) | ... |
 * | - - - - - - - - - - - - - - - - - - - - - - -|
 * | Current # of data items (32 bits)      | ... |
 * +----------------------------------------------+
 *
 * L2 data segment:
 * +----------------------+
 * | item (32 bits) | ... |
 * +----------------------+
 *
 * Layout:
 * +----------+----------------------------+---------------+-----+
 * | L1 (4KB) | L2 index (128MB) | L2 data | L2 IB | L2 DB | ... |
 * +----------+----------------------------+---------------+-----+
 */

/* offset from start of file to L1 block */
#define L1_OFFSET   (0x00000000*1)  /* 0B */
#define L1_SIZE     (0x00000100)    /* 2**8 = 256 */
#define L1_BYTES    (L1_SIZE*8*2)   /* 4KB */

/* offset from start of file to the first L2 block: 2KB */
#define L2_OFFSET   (L1_OFFSET+L1_BYTES)  /* 4KB */
#define L2_SIZE     (0x01000000)          /* 2**24 */
#define L2_BYTES    (L2_SIZE*4*2)         /* 128MB */

/* offset from the start of the current L2 block to the start of the DATA
 * block: 128MB */
#define DATA_OFFSET (0x01000000*8)
#define DATA_SIZE   (40535064)      /* maximum number of items in any section */
#define DATA_BYTES  (DATA_SIZE*4)  /* size of cache */

struct cldb
{
  /* cache storage */
  uint64_t *l1;       /* 2KB */
  uint32_t *l2;       /* 128MB */
  uint32_t *l2_switch;
  uint32_t *data2;
  uint32_t *data;     /* CACHE_BYTES bytes */
  uint32_t l1_index;  /* currently cached L2 block */

  /* file stream info */
  FILE *fp;
};
pthread_mutex_t cache_lock;
pthread_mutex_t backup_lock;
pthread_cond_t cache_switch;
pthread_t precache;
int num_threads;
int num_wait=0;




#define print_error(msg){ \
  fprintf (stderr, "[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
  fprintf (stderr, "%s\n", (msg)); \
}

/* write the bytes specified to the offset specified in the file stream. this
 * function shall not return upon failure. */
static void
xfseekwrite (const void *ptr, size_t size, size_t nmemb, long offset,
             int whence, FILE *stream)
{
  if (0 != fseek (stream, offset, whence))
    {
      print_error (strerror (errno));
      exit (EXIT_FAILURE);
    }
  if (nmemb != fwrite (ptr, size, nmemb, stream))
    {
      if (feof (stream))
        print_error ("EOF");
      if (ferror (stream))
        print_error ("Stream error");
      exit (EXIT_FAILURE);
    }
}

/* read the number of bytes specified from the offset specified in the file
 * stream. this function shall not return upon failure. */
static void
xfseekread (void *ptr, size_t size, size_t nmemb, long offset, int whence,
            FILE *stream)
{
  if (0 != fseek (stream, offset, whence))
    {
      print_error (strerror (errno));
      exit (EXIT_FAILURE);
    }

  if (nmemb != fread (ptr, size, nmemb, stream))
    {
      if (feof (stream))
        print_error ("EOF");
      if (ferror (stream))
        print_error ("Stream error");
      exit (EXIT_FAILURE);
    }
}

/* allocate an empty cldb object with associated caches */
static struct cldb *
cldb_alloc (void)
{
  struct cldb *db;

  if (NULL == (db = calloc (1, sizeof *db)))
    {
      print_error ("Out of memory");
      return NULL;
    }

  if (NULL == (db->l1 = calloc (1, L1_BYTES)))
    {
      print_error ("Out of memory");
      goto error;
    }

  if (NULL == (db->l2 = calloc (1, L2_BYTES)))
    {
      print_error ("Out of memory");
      goto error;
    }

  if (NULL == (db->data = calloc (1, DATA_BYTES)))
    {
      print_error ("Out of memory");
      goto error;
    }
  if (NULL == (db->l2_switch = calloc (1, L2_BYTES)))
    {
      print_error ("Out of memory");
      goto error;
    }

  if (NULL == (db->data2 = calloc (1, DATA_BYTES)))
    {
      print_error ("Out of memory");
      goto error;
    }

  return db;

error:
  free (db->data);
  free (db->data2);
  free (db->l2_switch);
  free (db->l2);
  free (db->l1);
  free (db);
  return NULL;
}

/* free cldb object. it wont try to free any null ptrs */
static void
cldb_free (struct cldb *db)
{
  if (NULL == db)
    return;
  if (NULL != db->data)
    free (db->data);
  if (NULL != db->l2)
    free (db->l2);
  if (NULL != db->data2)
    free (db->data2);
  if (NULL != db->l2_switch)
    free (db->l2_switch);
  if (NULL != db->l1)
    free (db->l1);
  free (db);
}


bool
cldb_new (char *path, char *geometry, char *genome)
{
  struct cldb *db = NULL;
  struct cvdb *gdb = NULL;
  uint32_t *l2_data_count = NULL;
  uint32_t *buf = NULL;
  uint32_t l1;
  uint32_t l2;
  FILE *fp = NULL;
  uint64_t db_offset = L2_OFFSET;
  bool retval = false;

  if (NULL == (db = cldb_alloc ()))
    {
      print_error ("cldb_alloc");
      return false;
    }

  if (NULL == (db->fp = fopen (path, "wb")))
    {
      print_error (strerror (errno));
      goto exit;
    }

  if (NULL == (gdb = cvdb_open (geometry, DB_MODE_READ)))
    {
      print_error ("cvdb_open");
      goto exit;
    }

  if (NULL == (l2_data_count = calloc (sizeof (uint32_t), L2_SIZE)))
    {
      print_error ("Out of memory");
      goto exit;
    }

  if (NULL == (buf = calloc (sizeof (uint32_t), L1_SIZE)))
    {
      print_error ("Out of memory");
      goto exit;
    }

  /* populate the db using the geometry db and the genome */
  for (l1 = 0; l1 < L1_SIZE; l1++)
    {
      uint64_t l2_data_size[2] = {0};
      size_t amount_read;
      uint32_t last;
      uint32_t genome_position = 0;

      fprintf (stderr, "At L1 index: %d\n", l1);

      for (l2 = 0; l2 < L2_SIZE; l2++)
        {
          uint32_t key = (l1 << 24) | l2;
          uint32_t value;

          /* query geometry db for a value count (this is to side step any
           * fragmentation issues) */
          if (false == cvdb_get (gdb, key, &value))
            {
              print_error ("cvdb_get");
              goto exit;
            }

          /* populate the L2 data pointer segment (if this key has no values
           * then set a convenient error code) */
          if (0 == value)
            db->l2[0+l2*2] = 0xFFFFFFFF;
          else
            db->l2[0+l2*2] = l2_data_size[0];

          /* zero out the data count */
          db->l2[1+l2*2] = 0;

          /* compute size of this L2 data block */
          l2_data_size[0] += value;

          /* record how many values should be associated with each key */
          l2_data_count[l2] = value;
        }

      /* go through genome filling out the L2 data count and data segments */
      if (NULL == (fp = fopen (genome, "rb")))
        {
          print_error (strerror (errno));
          goto exit;
        }

      if (0 != posix_fadvise (fileno (fp), 0, 0, POSIX_FADV_SEQUENTIAL))
        {
          print_error ("posix_fadvise");
          goto exit;
        }

      if (1 != fread (&last, sizeof (uint32_t), 1, fp))
        {
          print_error ("fread");
          goto exit;
        }

      while (L1_SIZE-1 == (amount_read = fread (buf+1, sizeof (uint32_t),
                                                L1_SIZE-1, fp))
             || (0 == ferror (fp) && 0 < amount_read))
        {
          buf[0] = last;
          {
            size_t length = amount_read + 1;
            uint32_t *bases = buf;

            while (--length)
              {
                int i;
                uint64_t whole = *bases;
                uint64_t p = *bases++;
                whole = (whole << 32) | p;
                for (i = 0; i < 32; i += 2)
                  {
                    uint32_t key = (whole << i) >> 32;

                    /* make sure the key falls within the current L1 block */
                    if (key >> 24 == l1)
                      {
                        uint32_t l2_index = key & 0x00FFFFFF;
                        /* sanity check to make sure geometry and genome match up */
                        if (db->l2[0+l2_index*2] == 0xFFFFFFFF)
                          {
                            print_error ("A key was found in genome that was not reported in the geometry");
                            goto exit;
                          }
                        db->data[db->l2[1+l2_index*2]++
                                 + db->l2[0+l2_index*2]] = genome_position;
                        l2_data_size[1]++;
                      }
                    genome_position++;
                  }
              }
          }
          last = buf[amount_read];
        }

      if (0 != ferror (fp))
        {
          print_error ("Stream error");
          goto exit;
        }

      if (last >> 24 == l1)
        {
          uint32_t l2_index = last & 0x00FFFFFF;
          db->data[db->l2[1+l2_index*2]++
                   + db->l2[0+l2_index*2]] = genome_position++;
          l2_data_size[1]++;
        }

      if (0 != fclose (fp) && (fp = NULL))
        {
          print_error (strerror (errno));
          goto exit;
        }
      fp = NULL;

      /* sanity check: make sure the size reported by geometry match up with
       * the actual size */
      for (l2 = 0; l2 < L2_SIZE; l2++)
        {
          if (l2_data_count[l2] != db->l2[1+l2*2])
            {
              print_error ("L2 data counts do not match up!");
              fprintf (stderr, "l2_data_count[%03u]:%u\n", l2,
                       l2_data_count[l2]);
              fprintf (stderr, "db->l2[1+%03u*2]:   %u\n", l2, db->l2[1+l2*2]);
              goto exit;
            }
        }
      if (l2_data_size[0] != l2_data_size[1])
        {
          print_error ("L2 data sizes do not match up!");
          fprintf (stderr, "l2_data_size[0]: %lu\n", l2_data_size[0]);
          fprintf (stderr, "l2_data_size[1]: %lu\n", l2_data_size[1]);
          goto exit;
        }

      /* update L1 pointers and write the L2 index and data blocks to disk */
      db->l1[0+l1*2] = db_offset;
      db->l1[1+l1*2] = l2_data_size[0] * sizeof (uint32_t);

      /* write out L2 index block */
      xfseekwrite (db->l2, 1, L2_BYTES, db_offset, SEEK_SET, db->fp);

      /* write out L2 data block */
      xfseekwrite (db->data, 1, db->l1[1+l1*2], db_offset + L2_BYTES, SEEK_SET,
                   db->fp);

      /* increment file offset */
      db_offset += L2_BYTES + db->l1[1+l1*2];
    }

  /* write out the L1 block */
  xfseekwrite (db->l1, 1, L1_BYTES, L1_OFFSET, SEEK_SET, db->fp);

  retval = true;

exit:
  if (NULL != fp && 0 != fclose (fp))
    print_error (strerror (errno));
  free (buf);
  free (l2_data_count);
  if (NULL != gdb && false == cvdb_close (gdb))
    print_error ("cvdb_close");
  if (NULL != db->fp && 0 != fclose (db->fp))
    print_error (strerror (errno));
  cldb_free (db);
  return retval;
}



void*
cldb_precache (struct cldb* db)
{
  while (1)
    {
      uint32_t l1_switch_index;

      pthread_mutex_lock (&backup_lock);

      l1_switch_index = db->l1_index + 1;

      /* read in the adjacent L2 index segment */
      xfseekread (db->l2_switch, 1, L2_BYTES, db->l1[0+l1_switch_index*2],
                  SEEK_SET, db->fp);

      /* read in the adjacent L2's data segment */
      xfseekread (db->data2, 1, db->l1[1+l1_switch_index*2],
                  db->l1[0+l1_switch_index*2] + L2_BYTES, SEEK_SET, db->fp);

      pthread_mutex_unlock (&backup_lock);
      pthread_cond_wait (&cache_switch, &backup_lock);
    }
}



struct cldb *
cldb_open (char *path, int threads)
{
  struct cldb *db;
  struct stat st;

  pthread_cond_init (&cache_switch, NULL);
  pthread_mutex_init (&cache_lock, NULL);
  pthread_mutex_init (&backup_lock, NULL);
  num_threads = threads;

  if (NULL == (db = cldb_alloc ()))
    {
      print_error ("Out of memory");
      return NULL;
    }

  /* make sure file exists */
  if (0 != stat (path, &st))
    {
      print_error (strerror (errno));
      goto error;
    }
  /* open existing file */
  else
    {
      if (NULL == (db->fp = fopen (path, "rb")))
        {
          print_error (strerror (errno));
          goto error;
        }
    }

  /* tell OS that we are going to be accessing data sequentially */
  if (0 != posix_fadvise (fileno (db->fp), 0, 0, POSIX_FADV_SEQUENTIAL))
    {
      print_error ("posix_fadvise");
      goto error;
    }

  /* load L1 cache block */
  xfseekread (db->l1, 1, L1_BYTES, L1_OFFSET, SEEK_SET, db->fp);

  /* read in the first L2 index segment */
  xfseekread (db->l2, 1, L2_BYTES, db->l1[0+0*2], SEEK_SET, db->fp);

  /* read in the first L2's data segment */
  xfseekread (db->data, 1, db->l1[1+0*2], db->l1[0+0*2] + L2_BYTES, SEEK_SET,
              db->fp);

  pthread_create(&precache, NULL, cldb_precache, (void*) &db);

  return db;

error:
  if (NULL != db->fp && 0 != fclose (db->fp))
    print_error (strerror (errno));
  cldb_free (db);
  return NULL;
}

bool
cldb_close (struct cldb *db)
{
  bool retval = true;

  if (NULL == db)
    return true;

  if (NULL != db->fp && 0 != fclose (db->fp))
    {
      print_error (strerror (errno));
      retval = false;
    }

  cldb_free (db);
  return retval;
}

void
cldb_wait (struct cldb *db, uint32_t l1)
{
  //assuming no parallelization of reads
  //lock so cache_blocks wont be accessed
  pthread_mutex_lock (&backup_lock);
  //switch current cache to the backup cache
  db->l2 = db->l2_switch;
  db->data = db->data2;
  //update L1 index
  db->l1_index = l1;
  pthread_mutex_unlock (&backup_lock);
  //get precache thread to get next precache
  pthread_cond_broadcast(&cache_switch);
}

int32_t
cldb_get (struct cldb *db, uint32_t key, uint32_t **values)
{
  uint32_t l1_index = key >> 24;        /* upper 8 bits */
  uint32_t l2_index = key & 0x00FFFFFF; /* lower 24 bits */

  if (NULL == db)
    {
      print_error ("Invalid argument `db'");
      return -1;
    }

  /* wait at a barrier upon cache a cache miss */
  if (db->l1_index != l1_index)
    cldb_wait(db, l1_index);

  /* if the requested key is in the currently loaded cache block */
  if (db->l1_index == l1_index)
    {
      *values = &db->data[db->l2[0+l2_index*2]];
      return db->l2[1+l2_index*2];
    }

  /* the above barrier should guarantee that this never happens */
  print_error("Cache error that shouldn't have occured");
  return -1;
}
