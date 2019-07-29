/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* sleep () */
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

/* internal includes */
#include "common.h"
#include "memory.h"
#include <actor.h>

struct sys_mem_info_t mem_info; 

void init ();
void thread_pool_init (pthread_t thread_pool[THREAD_POOL_SIZE], int thread_pool_pipe [THREAD_POOL_PIPES_COUNT][2], struct actor_io_t actor_io [THREAD_POOL_SIZE]);
void thread_pool_join (pthread_t thread_pool[THREAD_POOL_SIZE]);
void thread_pool_itc_init (int thread_pool_pipe [THREAD_POOL_PIPES_COUNT][2]);

void *fetch_meminfo_file (void *arg);
void *compute_available_memory (void *arg);
void *print_and_save_date (void *arg);

int main (int argc, char **argv)
{  
  pthread_t thread_pool[THREAD_POOL_SIZE];               /* vector that contains all working threads */
  int thread_pool_pipe [THREAD_POOL_PIPES_COUNT][2];     /* vector that contains all communication pipes */
  struct actor_io_t actor_io [THREAD_POOL_SIZE];         /* vector that contains arguments for each thread */

  init ();
  
  thread_pool_itc_init (thread_pool_pipe);  
  thread_pool_init (thread_pool, thread_pool_pipe, actor_io);  
  thread_pool_join (thread_pool);
  
  return EXIT_SUCCESS;
}

void init ()
{
  /* step 1 : get the system total memory */
  mem_info.MemTotal = get_mem_info (MEM_TOTAL);
  assert (mem_info.MemTotal > 0);
  printf ("%s:%d:%s: MemTotal = %d MB\n", __FILE__, __LINE__, __FUNCTION__, mem_info.MemTotal / 1024);  
  
  /* step 2 : get the system swap total */
  mem_info.SwapTotal = get_mem_info (SWAP_TOTAL);
  assert (mem_info.SwapTotal > 0);
  printf ("%s:%d:%s: SwapTotal = %d MB\n", __FILE__, __LINE__, __FUNCTION__, mem_info.SwapTotal / 1024);  
}

void thread_pool_itc_init (int thread_pool_pipe [THREAD_POOL_PIPES_COUNT][2])
{
  int i;
  
  for (i = 0; i < THREAD_POOL_PIPES_COUNT; i++)
  {
    if (pipe (thread_pool_pipe[i]) == -1)
    {
      printf ("%s:%d:%s:pipe(%d) %s\n", __FILE__, __LINE__, __FUNCTION__, i, strerror (errno));
      exit (EXIT_FAILURE);
	}
  } 
}

void thread_pool_init (pthread_t thread_pool[THREAD_POOL_SIZE], 
                       int thread_pool_pipe [THREAD_POOL_PIPES_COUNT][2],
                       struct actor_io_t actor_io [THREAD_POOL_SIZE])
{
  int s;  
  
  /* step 1-1 : prepare input args to listener thread */
  actor_io[MEMINFO_LISTENER].out[0] = thread_pool_pipe[LISTENER_TO_COMPUTER][WRITE];
  
  /* step 1-2 : create a listener thread to /proc/meminfo file */
  s = pthread_create (&thread_pool[MEMINFO_LISTENER], NULL, fetch_meminfo_file, (void *) &actor_io[MEMINFO_LISTENER]);
  if (s != 0)
  {
    printf ("%s:%d:%s: pthread_create failed (%d)\n", __FILE__, __LINE__, __FUNCTION__, s);
    exit (EXIT_FAILURE);
  }
  
  /* step 2-1 : prepare input args to computer thread */
  actor_io[MEMINFO_COMPUTER].in[0]  = thread_pool_pipe[LISTENER_TO_COMPUTER][READ];
  actor_io[MEMINFO_COMPUTER].out[0] = thread_pool_pipe[COMPUTER_TO_PRINTER][WRITE];
  
  /* step 2-2 : create the computer thread */
  s = pthread_create (&thread_pool[MEMINFO_COMPUTER], NULL, compute_available_memory, (void *) &actor_io[MEMINFO_COMPUTER]);
  if (s != 0)
  {
    printf ("%s:%d:%s: pthread_create failed (%d)\n", __FILE__, __LINE__, __FUNCTION__, s);
    exit (EXIT_FAILURE);
  }
  
  /* step 3-1 : prepare input args for printer thread */
  actor_io[MEMINFO_PRINTER].in[0]  = thread_pool_pipe[COMPUTER_TO_PRINTER][READ];
  
  /* step 3-2 : create printer thread */
  s = pthread_create (&thread_pool[MEMINFO_PRINTER], NULL, print_and_save_date, (void *) &actor_io[MEMINFO_PRINTER]);
  if (s != 0)
  {
    printf ("%s:%d:%s: pthread_create failed (%d)\n", __FILE__, __LINE__, __FUNCTION__, s);
    exit (EXIT_FAILURE);
  }
}

void thread_pool_join (pthread_t thread_pool[THREAD_POOL_SIZE])
{
  int s, i;
  
  for (i = 0; i < THREAD_POOL_SIZE; i++)
  {
    s = pthread_join (thread_pool[i], NULL);
    if (s != 0)
    {
      printf ("%s:%d:%s: pthread_join(%d) failed (%d)\n", __FILE__, __LINE__, __FUNCTION__, i, s);
      exit (EXIT_FAILURE);
    }
  }
}

void *fetch_meminfo_file (void *arg)
{
  time_t t;
  long unsigned int nbytes;
  struct sys_mem_free_info_t mem_free_info;
  
  struct actor_io_t * io_t = (struct actor_io_t *) arg;
  assert (io_t != NULL);
  
  for (ever)
  {
    /* step 1 : get system MemFree  */
    mem_free_info.MemFree = get_mem_info (MEM_FREE);
    assert (mem_free_info.MemFree > 0);
    /* step 2 : get system Buffers  */
    mem_free_info.Buffers = get_mem_info (BUFFERS);
    assert (mem_free_info.Buffers > 0); 
    /* step 3 : get system Cached  */
    mem_free_info.Cached  = get_mem_info (CACHED);
    assert (mem_free_info.Cached > 0);
    /* step 4 : get system time */
    t = time(NULL);
    mem_free_info.tm = *localtime(&t);
    /* spte 5 : put it to listener to computer pipe */
    nbytes = write (io_t->out[0], &mem_free_info, sizeof (struct sys_mem_free_info_t));
    switch (nbytes)
    {
      case  0 :
        printf ("%s:%d:%s: write : nothing was written\n", __FILE__, __LINE__, __FUNCTION__);
        break; 
      case -1 :
        printf ("%s:%d:%s: write failed : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
        exit (EXIT_FAILURE);
      default :
        break;
	}
    
    sleep (1);
  } 
}

void *compute_available_memory (void *arg)
{
  long unsigned int nbytes;
  struct sys_mem_free_info_t mem_free_info;
  
  struct actor_io_t * io_t = (struct actor_io_t *) arg;
  assert (io_t != NULL);
  
  for (ever)
  {
    /* get memory info file from input pipe */
    nbytes = read (io_t->in[0], &mem_free_info, sizeof (struct sys_mem_free_info_t)) ;
    if (nbytes == -1)
    {
      printf ("%s:%d:%s: read failed : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
      exit (EXIT_FAILURE);
    }
    
    /* compute free memory percentage */
    mem_free_info.mem_free_percent = (mem_free_info.MemFree + mem_free_info.Buffers + mem_free_info.Cached) * 100 / mem_info.MemTotal ;
    
    /* pass data to printer thread */
    nbytes = write (io_t->out[0], &mem_free_info, sizeof (struct sys_mem_free_info_t));
    switch (nbytes)
    {
      case  0 :
        printf ("%s:%d:%s: write : nothing was written\n", __FILE__, __LINE__, __FUNCTION__);
        break; 
      case -1 :
        printf ("%s:%d:%s: write failed : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
        exit (EXIT_FAILURE);
      default :
        break;
	}
    //~ /* print data */
    //~ printf ("\r%d-%d-%d %d:%d:%d, available memory : %d%%", mem_free_info.tm.tm_year + 1900, mem_free_info.tm.tm_mon + 1, mem_free_info.tm.tm_mday, mem_free_info.tm.tm_hour, mem_free_info.tm.tm_min, mem_free_info.tm.tm_sec, mem_free_percent);
    //~ 
    //~ fflush (stdout);
  }
}

void *print_and_save_date (void *arg)
{
  long unsigned int nbytes;
  struct sys_mem_free_info_t mem_free_info;
  
  struct actor_io_t * io_t = (struct actor_io_t *) arg;
  assert (io_t != NULL);
  
  for (ever)
  {
    /* get memory info file from input pipe */
    nbytes = read (io_t->in[0], &mem_free_info, sizeof (struct sys_mem_free_info_t)) ;
    if (nbytes == -1)
    {
      printf ("%s:%d:%s: read failed : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
      exit (EXIT_FAILURE);
    }
    
    /* print data */
    printf ("\r%d-%d-%d %d:%d:%d, available memory : %d%%", mem_free_info.tm.tm_year + 1900, mem_free_info.tm.tm_mon + 1, mem_free_info.tm.tm_mday, mem_free_info.tm.tm_hour, mem_free_info.tm.tm_min, mem_free_info.tm.tm_sec, mem_free_info.mem_free_percent);
    
    fflush (stdout);
  }
}
