#ifndef __REAL_FUNCTION_H__
#define __REAL_FUNCTION_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mongoose.h"
#include "sdl_thread.h"
#define MAPNAME_SIZE 20

struct onePair{
  char webName[MAPNAME_SIZE];
  char realName[MAPNAME_SIZE];
};
struct interfaceMap{
    struct onePair pair[8];
};
struct canBpsMap{
  struct onePair pair[5]; //125k,256k,500k,750k,1000k;
};

int exec_cmd(char *cmd, char*result);
int converInterfacew2r(char *wName, char *realName);
int converInterfacer2w(char *realName, char *wName);
int converCanBpsw2r(char *wBps, char *realBps);
int converCanBpsr2w(char *realBps, char *wBps);

void init_interfaceMap(void);
void init_canBpsMap(void);
int start_ser2net(char *wName, char *port, char * bps);
int start_can(char *wName, char *port, char *bps);
int stop_ser2net(char *wName, struct swap_status *pswap);
int stop_can(char *wName);
int get_OcdCfgList(char *resultBuf);
int start_ocd(char *ocdName, char *cfgFile);
int stop_ocd(char *ocdName);

#define OPENOCD_PATH "/usr/local/openocd-withaxi/012"
#endif
