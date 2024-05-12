#ifndef __MGCF_OPENOCD_H__
#define __MGCF_OPENOCD_H__

#include "sdl_thread.h"

struct mgcfOcd {};

#define M2CMSG_HEAD 0xeb902014
#define M2CMSG_TAIL 0x410209be
#define M2C_CMD_SIZE 512
#define M2C_SEARCHKEY_SIZE 64

struct m2c_msg {
  unsigned int head;  // 0xeb902014
  char cmd[M2C_CMD_SIZE];
  char searchKey[M2C_SEARCHKEY_SIZE];
  char cfg[M2C_SEARCHKEY_SIZE];
  char interfacecfg[M2C_SEARCHKEY_SIZE];
  unsigned int tail;  // 0x410209be
};



struct c2m_msg {
  unsigned int head;  // 0xeb902014
  char name[4];
  int openocd_pid;
  union openocd_log ocdlog;
  char cfg[M2C_SEARCHKEY_SIZE];
  char interfacecfg[M2C_SEARCHKEY_SIZE];
  char buf[M2C_CMD_SIZE];
  char searchKey[M2C_SEARCHKEY_SIZE];
  int sta;
  // char searchKey[M2C_SEARCHKEY_SIZE];
  unsigned int tail;  // 0x410209be
};

struct forkProcessDes {
  int pid;
  int rf, wf;
  char forkName[12];
  int rpipefds[2];  // main read
  int wpipefds[2];  // main write
  struct c2m_msg c2m_msg;
};

void init_mgcf_openocd(void);
void forcKillFpProcess(void);
int get_status_from_forkThread(int ocdId, struct _ocd_status *ocdSta);
// only for opr openocd thread,
// cmd contain start, check, stop
int send_cmd_to_forkThread(int ocdId, char *cmd, char *cfgFile, char *interfaceFile);
#endif  //__MGCF_OPENOCD_H__