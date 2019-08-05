#ifndef __ENV_H__
#define __ENV_H__

/* system includes */
#include <time.h>

/* internal includes */
#include "common.h"

#define DEFAULT_DATABASE_FILE        "database.sq3"
#define DEFAULT_SENSITIVITY           2
#define DEFAULT_MONITORING_INTERVAL   1

typedef struct env_t
{
  /* monitoring sensitivity in seconds */
  int sensitivity;
  /* monitoring time interval in hours */
  int interval;
  /* SQLite database path : R/Wr privilege needed */
  char db_file_path [MAX_INFORMATION_LEN];
  /* monitoring start time */
  struct tm start_time;
  struct tm end_time;
} env_t;

#endif /* __ENV_H__ */
