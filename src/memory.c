/* system includes */
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

/* internal includes */
#include "common.h"
#include "memory.h"
#include "file_io.h"
#include "regex_tools.h" 



int get_mem_info (char *info)
{
  int ret_val = 0;    /* in KBytes */
  FILE *meminfo;       /* /proc/meminfo */
  
  regex_t r;
  char *pattern = "[0-9]+";
  const int n_matches = 1;
  char to_match [MAX_INFORMATION_LEN];
  char **l = malloc (n_matches * sizeof (char*));
  int l_size = 0;  
  
  /* open /proc/meminfo file */
  meminfo = fopen (PROC_MEMINFO, "r");
  if (meminfo == NULL)
  {
    printf ("%s:%d:%s: %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
    return -1;
  }
  
  /* read /proc/meminfo file line by line */
  while (fgets (to_match, sizeof (to_match), meminfo) != NULL)
  {
    /* break once we get the data */
    if (strstr (to_match, info) != NULL)
    {
      break;
	}
  }
  
  /* close /proc/meminfo file */
  if (EOF ==fclose (meminfo))
  {
    printf ("%s:%d:%s: %s\n", __FILE__, __LINE__, __FUNCTION__, strerror (errno));
    return -1;
  }
  
  /* prepare the payload extraction */
  if (compile_regex (&r, pattern) == -1)
  {
    printf ("%s:%d:%s: regex compile error\n", __FILE__, __LINE__, __FUNCTION__);
    return -1;
  }  
  
  /* extract the payload */
  match_regex (& r, to_match, l, &l_size, n_matches);  
  
  /* the payload is unique */
  assert (l_size == 0);  
  
  /* cast payload from char* to int */
  ret_val = atoi (l[0]);
  
  free (l);  
  return ret_val;
}
