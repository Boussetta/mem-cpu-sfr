#ifndef __REGEX_TOOLS_H__
#define __REGEX_TOOLS_H__

/* system includes */
#include <regex.h>

/* The following is the size of a buffer to contain any error messages
   encountered when the regular expression is compiled. */
#define MAX_ERROR_MSG 0x1000

int compile_regex (regex_t * r, const char * regex_text);
int match_regex (regex_t * r, const char * to_match, char **l, int *size, const int n_matches);

#endif /* __REGEX_TOOLS_H__ */
