#ifndef __DBM_L_H__
#define __DBM_L_H__

/* system includes */
#include <sys/un.h>

#define MAX_CLIENTS       10
#define SOCK_MSG_MAX_SIZE 2000

typedef struct dbm_socket_info_t
{
  int sock;
  int opt;
  int master_socket;
  struct sockaddr_un server;
  int client_socket [MAX_CLIENTS];
} dbm_socket_info_t;

int database_manager_loop ();
int process_client_request (int client_sock, char *client_message) ;
static int callback (void *NotUsed, int argc, char **argv, char **azColName);


#endif /* __DBM_L_H__ */
