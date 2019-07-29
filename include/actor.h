#ifndef __ACTOR_H__
#define __ACTOR_H__

#define MAX_IO  8

typedef struct actor_io_t
{
  int in[MAX_IO];
  int out[MAX_IO];
} actor_io_t;

#endif /* __ACTOR_H__ */
