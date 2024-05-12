#ifndef __NETOCD_CONFIG_JSON__
#define __NETOCD_CONFIG_JSON__

#define JSON_NAME_SIZE 20
#define JSON_VAL_SIZE 50
struct jsonOne{
  char name[JSON_NAME_SIZE];
  char val[JSON_VAL_SIZE];
};

struct jsonOneInterface{
  char nodeName[JSON_NAME_SIZE];
  struct jsonOne name;
  struct jsonOne bps;
  struct jsonOne sta;
};

struct jsonInterface{
  char nodeName[JSON_NAME_SIZE];
  struct jsonOneInterface com[6];
  struct jsonOneInterface can[2];
  // struct jsonOneInterface uart0;
  // struct jsonOneInterface uart1;
  // struct jsonOneInterface rs4220;
  // struct jsonOneInterface rs4221;
  // struct jsonOneInterface rs4850;
  // struct jsonOneInterface rs4851;
  // struct jsonOneInterface can0;
  // struct jsonOneInterface can1;
};

struct jsonOneOcd{
  char nodeName[JSON_NAME_SIZE];
  // struct jsonOne ocdName;
  struct jsonOne configFileName;
  struct jsonOne cpu;
  struct jsonOne sta;
};

struct jsonOcd{
  char nodeName[JSON_NAME_SIZE];
  struct jsonOneOcd ocd0;
  struct jsonOneOcd ocd1;
};

struct json_netocd_config{
  struct jsonOne config;
  struct jsonInterface interface;
  struct jsonOcd ocd;
};
#include "sdl_thread.h"
// void create_ifcJson(struct swap_status *sstatus);
void create_ifcJson(struct swap_status *sstatus, char *returnBuffer);
int create_ocdJson(struct swap_status *sstatus, char *jOcdInfc);
void create_cfgJson(char *cfgList_str, char *interFace_str, int bufSize) ;
void create_svfJson(char *cfgList_str,int bufSize);
#endif //__NETOCD_CONFIG_JSON__