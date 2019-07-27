/* system includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* internal includes */
#include "file_io.h"
#include "common.h"


int get_string_value_from_path(const char *file_path, char *value, int value_len)
{
  int ret = EXIT_SUCCESS;
  if ((file_path == NULL) || (value == NULL))
  {
    if (file_path == NULL)
    {
       printf ("%d : file_path is NULL...\n", __LINE__);
    }
    if (value == NULL)
    {
       printf ("%d, value is NULL...\n", __LINE__);
    }
    ret = EXIT_FAILURE;
  }
  else
  {
    FILE *f = NULL;
    
    if ((f = fopen(file_path, "r")) == NULL)
    {
      printf ("%d, fopen (%s) failed... (%s)", __LINE__, file_path, strerror(errno));
      ret = EXIT_FAILURE;
    }
    else
    {
      char buf[MAX_INFORMATION_LEN];
      int len = 0;
      int max_len = sizeof(buf);
    
      memset(buf, 0, max_len);
      len = fread(buf, sizeof(char), max_len, f);
      buf[max_len-1]='\0';
      if (len < max_len)
      {
         if (len <= value_len)
         {
            char *p = strstr(buf, "\n");
            memset(value, 0, value_len);
            if (p != NULL)
            {
               *p = '\0';
            }
            strncpy(value, buf, value_len);
         }
      }
      fclose(f);
    }
  }
  return ret;
}

