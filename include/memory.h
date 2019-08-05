#ifndef __MEMORY_H__
#define __MEMORY_H__

/* system includes */
#include <time.h>

/* internal includes */

#define PROC_MEMINFO "/proc/meminfo"

#define MEM_TOTAL    "MemTotal"
#define SWAP_TOTAL   "SwapTotal"
#define MEM_FREE     "MemFree"
#define BUFFERS      "Buffers"
#define CACHED       "Cached"

typedef struct sys_mem_info_t
{
  int MemTotal;    /* system total memory in KBytes */
  int SwapTotal;   /* system swap memory in KBytes */
} sys_mem_info_t;

typedef struct sys_mem_free_info_t
{
  int MemFree;
  int Buffers;
  int Cached;
  struct tm tm;
  int free_memory;
} sys_mem_free_info_t;

int get_mem_info (char *info);

#endif /* __MEMORY_H__ */
