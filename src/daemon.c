/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> /* sleep () */
#include <assert.h>
#include <pthread.h>

/* internal includes */
#include "common.h"
#include "memory.h"


struct sys_mem_info_t mem_info;    

void init ();
void *check_available_memory (void *arg);

int main (int argc, char **argv)
{  
  int s;
  pthread_t memory_check_thread;
  
  init ();
  
  s = pthread_create (&memory_check_thread, NULL, check_available_memory, NULL);
  if (s != 0)
  {
    printf ("%s:%d:%s: pthread_create failed (%d)\n", __FILE__, __LINE__, __FUNCTION__, s);
    return EXIT_FAILURE;
  }
  
  s = pthread_join (memory_check_thread, NULL);
  if (s != 0)
  {
    printf ("%s:%d:%s: pthread_oin failed (%d)\n", __FILE__, __LINE__, __FUNCTION__, s);
    return EXIT_FAILURE;
  }
  
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


void *check_available_memory (void *arg)
{
  time_t t;
  struct tm tm;
  struct sys_mem_free_info_t mem_free_info;
  int mem_free_percent;
  
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
    /* compute free memory percentage */
    mem_free_percent = (mem_free_info.MemFree + mem_free_info.Buffers + mem_free_info.Cached) * 100 / mem_info.MemTotal ;
    /* step 5 : get system time */
    t = time(NULL);
    tm = *localtime(&t);
    /* step 6 : print data */
    printf ("\r%d-%d-%d %d:%d:%d, available memory : %d%%", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, mem_free_percent);
    
    fflush (stdout);
    sleep (1);
  } 
}
