/* system includes */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sqlite3.h>

/* internal includes */
#include "dbm_l.h"
#include "dbm_helper.h"
#include "common.h"
#include "env.h"

extern struct env_t env;
struct dbm_socket_info_t *t_info = NULL;
char *dbm_socket = ".dbm_socket";
/* database connection handler */
sqlite3 *db;

pid_t init_database_manager ()
{
  pid_t dbm_pid = fork ();
  switch (dbm_pid)
  {
    case -1:
      printf ("[%s][%d][%s] fork () %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
      break;
    case 0:
      /* this is the child process / database manager daemon */
#ifdef DBM_VERBOSE
      printf ("[%s][%d][%s] database manager started - dbm_pid : %d, parent pid : %d\n", __FILE__, __LINE__, __FUNCTION__, getpid (), getppid ());
#endif
      if (database_manager_loop () == -1)
      {
        return EXIT_FAILURE;
	  }
      break;
    default:
    /* this is the parent process of database manager daemon : do nothing */
      break;
  }
  return dbm_pid;
}

int database_manager_loop ()
{
  char client_message [SQLITE_QUERY_MAX_SIZE];
  fd_set readfds;
  int c, i, max_sd, sd, activity, new_socket,read_size;
    
  /* return code */
  int rc;
  
  /* we keep only the current monitoring session database */
  rc = unlink (env.db_file_path);
#ifdef DBM_VERBOSE
  if ( rc == -1)
  {
    printf("[%s][%d][%s] unlink : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
  }
#endif  
  
  /* open SQLite database file */
  rc = sqlite3_open (env.db_file_path, &db);
  if ( rc )
  {
    printf("[%s][%d][%s] Can't open database: %s\n", __FILE__, __LINE__, __FUNCTION__, sqlite3_errmsg(db));
    sqlite3_close (db);
    return -1;
  }
  
  /* free t_info residuals */
  if (t_info != NULL)
  {
    free (t_info);
  }
  
  t_info = malloc (sizeof (struct dbm_socket_info_t));
  if (t_info == NULL)
  {
    printf ("[%s][%d][%s] failed to allocate dynamic memory\n", __FILE__, __LINE__, __FUNCTION__);
    return -1;
  }
  
  /* initialise all client_socket[] to 0 so not checked */
  for (i = 0; i < MAX_CLIENTS; i++)
  {
    t_info->client_socket[i] = 0;
  }
  
  /* create unix socket */
  t_info->master_socket = socket (AF_UNIX, SOCK_STREAM, 0);
  if (t_info->master_socket == -1) 
  {
    printf("[%s][%d][%s] failed to create socket : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
    return -1;
  }
  
  /* set master socket to allow multiple connections ,
   * this is just a good habit, it will work without this  
   * */
   if( setsockopt (t_info->master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&t_info->opt, sizeof (t_info->opt)) < 0 )   
  {   
    printf("[%s][%d][%s] failed to set options on socket : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
    return -1;
  }  
  
  /* init socket context structure */
  memset (&t_info->server, 0, sizeof (struct sockaddr_un));
  t_info->server.sun_family = AF_UNIX;
  strncpy(t_info->server.sun_path, dbm_socket, sizeof (t_info->server.sun_path)-1);
  
  /* remove socket residuals */
  if (unlink (dbm_socket) == -1)
  {
#ifdef DBM_VERBOSE
    printf("[%s][%d][%s] unlink : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
#endif
  }
  
  /* bind name to socket */
  if( bind (t_info->master_socket,(struct sockaddr *)&t_info->server , sizeof (t_info->server)) < 0) 
  {
    printf("[%s][%d][%s] failed to bind socket : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
    return -1;
  }
  
  /* set the maximum pending connections for the master socket */
  if (listen (t_info->master_socket , 5) < 0) 
  {
    printf("[%s][%d][%s] listen failed : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
    return -1;
  }
  
  /* accept the incoming connection */
  c = sizeof (struct sockaddr_un);  
  
  for (ever)
  {
    /* clear the socket set */
    FD_ZERO (&readfds);
    
    /* add master socket to set */
    FD_SET (t_info->master_socket, &readfds);
    max_sd = t_info->master_socket;
    
    /* add child sockets to set */
    for (i = 0; i < MAX_CLIENTS; i++)
    {
      /* socket descriptor */
      sd = t_info->client_socket[i];
      
      /* if valid socket descriptor then add to read list */
      if (sd > 0)
      {
        FD_SET (sd, &readfds);
	  }
	  
	  /* highest file descriptor number, need it for the select function */
	  if (sd > max_sd)
	  {
        max_sd = sd;
	  }
	}
	
	/* wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely */
	activity = select (max_sd + 1, &readfds, NULL, NULL, NULL);
	if ((activity < 0) && (errno != EINTR))   
    {
#ifdef DBM_VERBOSE
      printf("[%s][%d][%s] select error : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
#endif
    }
    
    /* If something happened on the master socket, then its an incoming connection */
    if (FD_ISSET (t_info->master_socket, &readfds))
    {
      if ((new_socket = accept (t_info->master_socket, (struct sockaddr *)&t_info->server, (socklen_t*)&c)) < 0)
      {
#ifdef DBM_VERBOSE
        printf("[%s][%d][%s] accept error : %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
#endif  
        return -1;
	  }
#ifdef DBM_VERBOSE
      printf ("[%s][%d][%s] New connection , socket fd is %d\n", __FILE__, __LINE__, __FUNCTION__, new_socket);
#endif 
	  
	  /* add new socket to array of sockets */
	  for (i = 0; i < MAX_CLIENTS; i++)
	  {
        /* if position is empty */
        if( t_info->client_socket[i] == 0 ) 
        {
          t_info->client_socket[i] = new_socket;
#ifdef DBM_VERBOSE
          printf("[%s][%d][%s] Adding to list of sockets as %d\n", __FILE__, __LINE__, __FUNCTION__, i);
#endif 
          break;
		}
	  }
	}
	
	/* IO operation on other socket  */
	for (i = 0; i < MAX_CLIENTS; i++) 
	{
      sd = t_info->client_socket[i];
      
      if (FD_ISSET (sd, &readfds))
      {
        /* Check if it was for closing , and also read the incoming message */
        if ((read_size = read (sd, &client_message, SQLITE_QUERY_MAX_SIZE)) == 0)
        {
          /* Somebody disconnected */
#ifdef DBM_VERBOSE
          printf("[%s][%d][%s] Host disconnected. \n", __FILE__, __LINE__, __FUNCTION__);
#endif           
          /* Close the socket and mark as 0 in list for reuse */
          close(sd);  
          t_info->client_socket[i] = 0;
		}
		else
		{
          switch (process_client_request (sd, client_message))
          {
            case 0:
              unlink(dbm_socket);
              close(t_info->master_socket);
              return 0;
            default:
              break;
		  }
		}
	  }
	}	
  }
  
  /* close SQLite database */
  sqlite3_close(db);
  printf("[%s][%d][%s] SQLite database closed.\n", __FILE__, __LINE__, __FUNCTION__);
  unlink(dbm_socket);
  close(t_info->master_socket);
  
  return 0;
}	

int process_client_request (int client_sock, char *client_message) 
{
  /* return code */
  int rc;
  /* pointer to an error string */
  char *zErrMsg = 0;
  
  if (strcmp (client_message, "done") == 0)
  {
    return 0;
  }
  
  /* execute the SQL query */
  rc = sqlite3_exec (db, client_message, callback, 0, &zErrMsg);
  if( rc != SQLITE_OK )
  {
    printf("[%s][%d][%s] SQL error: %s\n", __FILE__, __LINE__, __FUNCTION__, sqlite3_errmsg(db));
    sqlite3_free(zErrMsg);
  }
  
  return 1;
}

static int callback (void *NotUsed, int argc, char **argv, char **azColName)
{
  int i;
  for (i = 0; i < argc; i++)
  {
    printf("[%s][%d][%s] %s = %s", __FILE__, __LINE__, __FUNCTION__, azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}
