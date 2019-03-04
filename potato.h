#ifndef __POTATO_H__
#define __POTATO_H__

#include <stdlib.h>

typedef struct potato_t{
  int player_num;
  int hop_num;
  int start_player;
  int next_player;
  int game_ended;
  int trace[512];
}potato;
  

#endif
