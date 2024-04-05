#ifndef __SDL_THREAD_H__
#define __SDL_THREAD_H__

#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_thread.h>
#include <stdio.h>
#include <stdlib.h>

// com port config data and cmd
struct _com_cd {
#define COM_CD_NAME_LEN 10
  char name[COM_CD_NAME_LEN];
  int port;
  int cmd;  // 1=on, 0=off
  int bps;
  int status;
};

struct _can_cd {
#define CAN_CD_NAME_LEN 10
  char name[CAN_CD_NAME_LEN];
  int port;
  int cmd;  // 1=on, 0=off
  int bps;
  int status;
};

struct _ocd_cd {
#define OCD_CD_NAME_LEN 10
#define OCD_CONFIG_FILE_LEN 20
  char name[OCD_CD_NAME_LEN];
  char configFile[OCD_CONFIG_FILE_LEN];
  int port;
  int cmd;  // 1=on, 0=off
  int bps;
  int status;
};

struct swap_config_cmd {
  struct _com_cd com_cd[6];
  struct _can_cd can_cd[2];
  struct _ocd_cd ocd_cd[2];
  int this_available;  // 1=there is useage data, 0=there is no
  int num;
  SDL_mutex *gBufferLock;
};

void init_mutex(void);
int from_swap_get_dat(struct swap_config_cmd *swap,
                      struct swap_config_cmd *data);

struct _com_status {
  char name[COM_CD_NAME_LEN];
  int run;  //=0,no; =1 run
  int bps;
  int port;
  int netStatus;  // should save this port data/s
  int buf[10];    // should save some data
};
struct _can_status {
  char name[CAN_CD_NAME_LEN];
  int run;  //=0,no; =1 run
  int bps;
  int port;
  int netStatus;  // should save this port data/s
  int buf[10];    // should save some data
};

struct _ocd_status {
  char name[OCD_CD_NAME_LEN];
  char configFile[OCD_CONFIG_FILE_LEN];
  int run;  //=0,no; =1 run
  int bps;
  int port;
  int netStatus;  // should save this port data/s
  int buf[10];    // should save some data
};

struct swap_status {
  struct _com_status com_status[6];
  struct _can_status can_status[2];
  struct _ocd_status ocd_status[2];
  int cpusage;
  int memusage;
  int this_available;  // 1=there is useage data, 0=there is no
  int num;
  SDL_mutex *gBufferLock;
};
#endif  //__SDL_THREAD_H__