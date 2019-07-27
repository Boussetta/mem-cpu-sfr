/* system includes */
#include <stdlib.h>        // malloc ()
#include <string.h>        // memset ()
#include <stdio.h>

/* internal includes */
#include "regex_tools.h"

int compile_regex (regex_t * r, const char * regex_text)
{
  int status = regcomp (r, regex_text, REG_EXTENDED|REG_NEWLINE);
  if (status != 0) 
  {
    char error_message[MAX_ERROR_MSG];
    regerror (status, r, error_message, MAX_ERROR_MSG);
    return -1;
  }
  return 0;
}

int match_regex (regex_t * r, const char * to_match, char **l, int *size, const int n_matches)
{
  /* "P" is a pointer into the string which points to the end of the previous match. */
  const char * p = to_match;
  
  /* "M" contains the matches found. */
  regmatch_t m[n_matches];
  
  int s = *size;
  
  while (1) 
  {
    int i;
    int nomatch = regexec (r, p, n_matches, m, 0);
    if (nomatch) 
    {
      /* No more matches */
      *size = s - 1;
      return nomatch;
    }
    
    for (i = 0; i < n_matches; i++) 
    {
      int start;
      int finish;
      
      if (m[i].rm_so == -1) 
      {
        break;
      }
      start = m[i].rm_so + (p - to_match);
      finish = m[i].rm_eo + (p - to_match);
      
      l[s]     = malloc (sizeof(char)*10);
      memset (l[s], '\0', sizeof(char)*10);
      sprintf (l[s], "%.*s", (finish - start), to_match + start); 
    }
    s++;
    p += m[0].rm_eo;
  }
  return 0;
}

