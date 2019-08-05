/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* sleep (), fork () */
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include <getopt.h>
#include <sys/un.h>
#include <sys/socket.h>

/* internal includes */
#include "common.h"
#include "memory.h"
#include "actor.h"
#include "env.h"
#include "dbm_helper.h"

#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

extern char *dbm_socket;
struct sys_mem_info_t mem_info; 
struct env_t env;

void init ();
void parse_input_args (int argc, char **argv);

int thread_pool_init (pthread_t thread_pool[THREAD_POOL_SIZE], int thread_pool_pipe [THREAD_POOL_PIPES_COUNT][2], struct actor_io_t actor_io [THREAD_POOL_SIZE]);
int thread_pool_join (pthread_t thread_pool[THREAD_POOL_SIZE]);
int thread_pool_itc_init (int thread_pool_pipe [THREAD_POOL_PIPES_COUNT][2]);

void *meminfo_proc_listener_loop (void *arg);
void *meminfo_computer_thread_loop (void *arg);
void *meminfo_database_manager_loop (void *arg);

int send_sql_request_to_dbm (char *sql_request);

int main (int argc, char **argv)
{  
  /* database management system pid */
  pid_t dbm_pid;
  
  /* vector that contains all working threads */
  pthread_t thread_pool[THREAD_POOL_SIZE];
  
  /* vector that contains all communication pipes */
  int thread_pool_pipe [THREAD_POOL_PIPES_COUNT][2];
  
  /* vector that contains arguments for each thread */
  struct actor_io_t actor_io [THREAD_POOL_SIZE];

  /* step 1 : parse input arguments */
  parse_input_args (argc, argv);
  
  init ();
  
  /* fork () and create a database manager as an independant process */
  dbm_pid = init_database_manager ();
  if (dbm_pid == -1)
  {
    return EXIT_FAILURE;
  }
  
#ifdef VERBOSE
  printf ("[%s][%d][%s] mem-cpu-daemon pid : %d, parent pid : %d, dbm_pid : %d\n", __FILE__, __LINE__, __FUNCTION__, getpid (), getppid (), dbm_pid);
#endif
  
  /* prepare communication pipes between daemon threads */
  if (thread_pool_itc_init (thread_pool_pipe) == -1)
  {
    return EXIT_FAILURE;
  }
  
  /* create daemon threads */
  if (thread_pool_init (thread_pool, thread_pool_pipe, actor_io) == -1)
  {
    return EXIT_FAILURE;
  }
  
  /* wait */
  if (thread_pool_join (thread_pool) == -1)
  {
    return EXIT_FAILURE;
  }
#ifdef VERBOSE
  printf ("\n[%s][%d][%s] monitoring stopped \n", __FILE__, __LINE__, __FUNCTION__);
#endif
  return EXIT_SUCCESS;
}
void parse_input_args (int argc, char **argv)
{
  int opt;
  
  /* init environment variables */
  
  env.sensitivity = DEFAULT_SENSITIVITY;
  env.interval = DEFAULT_MONITORING_INTERVAL;
  strcpy (env.db_file_path, DEFAULT_DATABASE_FILE);
  
  /* parse input arguments */
  while ((opt = getopt(argc, argv, "d:i:s:")) != -1) 
  {
    switch (opt)
    {
      case 'd':
        strcpy (env.db_file_path, optarg);
        break;
      case 'i':
        env.interval = atoi (optarg);
        break;
      case 's':
        env.sensitivity = atoi (optarg);
        break;
	}
  }
  
#ifdef VERBOSE
  printf ("[%s][%d][%s] database file : %s\n", __FILE__, __LINE__, __FUNCTION__, env.db_file_path);
  printf ("[%s][%d][%s] montoring interval : %d\n", __FILE__, __LINE__, __FUNCTION__, env.interval);
  printf ("[%s][%d][%s] montoring sensitivity : %d\n", __FILE__, __LINE__, __FUNCTION__, env.sensitivity);
#endif
  
}

void init ()
{
  time_t t;
  
  /* step 1 : get the system total memory */
  mem_info.MemTotal = get_mem_info (MEM_TOTAL);
  assert (mem_info.MemTotal > 0);

  /* step 2 : get the system swap total */
  mem_info.SwapTotal = get_mem_info (SWAP_TOTAL);
  assert (mem_info.SwapTotal >= 0);
  
  /* step 3 : get monitoring start time */
  t = time(NULL);
  env.start_time = *localtime(&t);
  
  /* step 4 : set monitoring end time */
  env.end_time = env.start_time;
  //~ env.end_time.tm_hour = env.end_time.tm_hour + env.interval;
  env.end_time.tm_sec = env.end_time.tm_sec + env.interval;
  mktime (&env.end_time);
  
  
#ifdef VERBOSE
  printf ("[%s][%d][%s] MemTotal = %d MB\n", __FILE__, __LINE__, __FUNCTION__, mem_info.MemTotal / 1024);  
  printf ("[%s][%d][%s] SwapTotal = %d MB\n", __FILE__, __LINE__, __FUNCTION__, mem_info.SwapTotal / 1024);  
  printf ("[%s][%d][%s] monitoring start time : %s", __FILE__, __LINE__, __FUNCTION__, asctime (&env.start_time));
  printf ("[%s][%d][%s] monitoring end time : %s", __FILE__, __LINE__, __FUNCTION__, asctime (&env.end_time));
#endif
}



int thread_pool_itc_init (int thread_pool_pipe [THREAD_POOL_PIPES_COUNT][2])
{
  int i;
  
  for (i = 0; i < THREAD_POOL_PIPES_COUNT; i++)
  {
    if (pipe (thread_pool_pipe[i]) == -1)
    {
      printf ("[%s][%d][%s] pipe(%d) %s\n", __FILE__, __LINE__, __FUNCTION__, i, strerror (errno));
      return -1;
	}
  } 
  return 0;
}

int thread_pool_init (pthread_t thread_pool[THREAD_POOL_SIZE], 
                       int thread_pool_pipe [THREAD_POOL_PIPES_COUNT][2],
                       struct actor_io_t actor_io [THREAD_POOL_SIZE])
{
  int s;  
  
  /* step 1-1 : prepare input args to listener thread */
  actor_io[MEMINFO_LISTENER].out[0] = thread_pool_pipe[LISTENER_TO_COMPUTER][WRITE];
  
  /* step 1-2 : create a listener thread to /proc/meminfo file */
  s = pthread_create (&thread_pool[MEMINFO_LISTENER], NULL, meminfo_proc_listener_loop, (void *) &actor_io[MEMINFO_LISTENER]);
  if (s != 0)
  {
    printf ("[%s][%d][%s] pthread_create failed (%d)\n", __FILE__, __LINE__, __FUNCTION__, s);
    return -1;
  }
  
  /* step 2-1 : prepare input args to computer thread */
  actor_io[MEMINFO_COMPUTER].in[0]  = thread_pool_pipe[LISTENER_TO_COMPUTER][READ];
  actor_io[MEMINFO_COMPUTER].out[0] = thread_pool_pipe[COMPUTER_TO_PRINTER][WRITE];
  
  /* step 2-2 : create the computer thread */
  s = pthread_create (&thread_pool[MEMINFO_COMPUTER], NULL, meminfo_computer_thread_loop, (void *) &actor_io[MEMINFO_COMPUTER]);
  if (s != 0)
  {
    printf ("[%s][%d][%s] pthread_create failed (%d)\n", __FILE__, __LINE__, __FUNCTION__, s);
    return -1;
  }
  
  /* step 3-1 : prepare input args for printer thread */
  actor_io[MEMINFO_DBM].in[0]  = thread_pool_pipe[COMPUTER_TO_PRINTER][READ];
  
  /* step 3-2 : create printer thread */
  /* to do :
   * maybe it's more efficient to fork a independant process for data management
   * */
  s = pthread_create (&thread_pool[MEMINFO_DBM], NULL, meminfo_database_manager_loop, (void *) &actor_io[MEMINFO_DBM]);
  if (s != 0)
  {
    printf ("[%s][%d][%s] pthread_create failed (%d)\n", __FILE__, __LINE__, __FUNCTION__, s);
    return -1;
  }
  
  return 0;
}

int thread_pool_join (pthread_t thread_pool[THREAD_POOL_SIZE])
{
  int s, i;
  
  for (i = 0; i < THREAD_POOL_SIZE; i++)
  {
    s = pthread_join (thread_pool[i], NULL);
    if (s != 0)
    {
      printf ("[%s][%d][%s] pthread_join(%d) failed (%d)\n", __FILE__, __LINE__, __FUNCTION__, i, s);
      return -1;
    }
  }
  
  return 0;
}

void *meminfo_proc_listener_loop (void *arg)
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
    
    if (difftime (mktime (&mem_free_info.tm), mktime (&env.end_time)) > 0)
    {
      mem_free_info.MemFree = -1;
    }
    
    /* spte 5 : put it to listener to computer pipe */
    nbytes = write (io_t->out[0], &mem_free_info, sizeof (struct sys_mem_free_info_t));
    switch (nbytes)
    {
      case  0 :
        printf ("[%s][%d][%s] write : nothing was written\n", __FILE__, __LINE__, __FUNCTION__);
        break; 
      case -1 :
        printf ("[%s][%d][%s] write failed : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
        exit (EXIT_FAILURE);
      default :
        break;
	}
    
    /* to do : this should be a real time thread and the following line have to be removed */
    if (mem_free_info.MemFree == -1)
    {
      pthread_exit (NULL);
	}
    else
    {
      sleep (env.sensitivity);
    }
  } 
}

void *meminfo_computer_thread_loop (void *arg)
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
      printf ("[%s][%d][%s] read failed : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
      exit (EXIT_FAILURE);
    }
    
    if (mem_free_info.MemFree == -1)
    {
      nbytes = write (io_t->out[0], &mem_free_info, sizeof (struct sys_mem_free_info_t));
      switch (nbytes)
      {
        case  0 :
          printf ("[%s][%d][%s] write : nothing was written\n", __FILE__, __LINE__, __FUNCTION__);
          break; 
        case -1 :
          printf ("[%s][%d][%s] write failed : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
          exit (EXIT_FAILURE);
        default :
          break;
	  }
      pthread_exit (NULL);
	}
    
    /* compute free memory percentage */
    mem_free_info.free_memory = mem_free_info.MemFree + mem_free_info.Buffers + mem_free_info.Cached;
    
    /* pass data to printer thread */
    nbytes = write (io_t->out[0], &mem_free_info, sizeof (struct sys_mem_free_info_t));
    switch (nbytes)
    {
      case  0 :
        printf ("[%s][%d][%s] write : nothing was written\n", __FILE__, __LINE__, __FUNCTION__);
        break; 
      case -1 :
        printf ("[%s][%d][%s] write failed : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
        exit (EXIT_FAILURE);
      default :
        break;
	}
  }
}

void *meminfo_database_manager_loop (void *arg)
{
  long unsigned int nbytes;
  struct sys_mem_free_info_t mem_free_info;
  /* SQLite query */
  char *query;
  
  struct actor_io_t * io_t = (struct actor_io_t *) arg;
  assert (io_t != NULL);
  
  /* Create a table to store free memory percentage and cpu usage */
  query = "create table general_info (free_mem_percentage integer, cpu_usage integer, date DATETIME)";
  send_sql_request_to_dbm (query);
  
  for (ever)
  {
    /* get memory info file from input pipe */
    nbytes = read (io_t->in[0], &mem_free_info, sizeof (struct sys_mem_free_info_t)) ;
    if (mem_free_info.MemFree == -1)
    if (nbytes == -1)
    {
      printf ("[%s][%d][%s] read failed : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
      exit (EXIT_FAILURE);
    }
    
    if (mem_free_info.MemFree == -1)
    {
      strcpy (query, "done");
      send_sql_request_to_dbm (query);
      pthread_exit (NULL);
	}
    
    /* to do : put data to a csv file - sqlite may be optimal */
    asprintf (&query, "insert into general_info (free_mem_percentage, cpu_usage, date) values (%d, 0, '%d-%d-%d %d:%d:%d')", (mem_free_info.free_memory * 100) / mem_info.MemTotal, mem_free_info.tm.tm_year + 1900, mem_free_info.tm.tm_mon + 1, mem_free_info.tm.tm_mday, mem_free_info.tm.tm_hour, mem_free_info.tm.tm_min, mem_free_info.tm.tm_sec);
    send_sql_request_to_dbm (query);
    
#ifdef VERBOSE
    /* print data */
    printf ("\r%d-%d-%d %d:%d:%d, available memory : %d%%", mem_free_info.tm.tm_year + 1900, mem_free_info.tm.tm_mon + 1, mem_free_info.tm.tm_mday, mem_free_info.tm.tm_hour, mem_free_info.tm.tm_min, mem_free_info.tm.tm_sec, (mem_free_info.free_memory * 100) / mem_info.MemTotal);
    fflush (stdout);
#endif
  }
}

int send_sql_request_to_dbm (char *sql_request)
{
  int sock;
  struct sockaddr_un server;
  
  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock == -1) 
  {
    printf ("[%s][%d][%s] Could not create socket : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
    return -1;
  }
  
  memset (&server, 0, sizeof (struct sockaddr_un));
  server.sun_family = AF_UNIX;
  strncpy (server.sun_path, dbm_socket, sizeof (server.sun_path) - 1);
  
  while (connect (sock , (struct sockaddr *)&server , sizeof (server)) == -1) 
  {
    //~ printf ("[%s][%d][%s] connection failed : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
    //~ return -1;
  }
  
  if(send (sock , sql_request , strlen (sql_request) + 1, 0) == -1)
  {
    printf ("[%s][%d][%s] connection failed : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
    close (sock);
    return -1;
  }
  
  close(sock);
  return 1;  
}
