#ifndef __MGCF_OPENOCD_H__
#define __MGCF_OPENOCD_H__

struct mgcfOcd{

};

#define M2CMSG_HEAD 0xeb902014
#define M2CMSG_TAIL 0x410209be
#define M2C_CMD_SIZE 256
#define M2C_SEARCHKEY_SIZE 64


struct m2c_msg{
  unsigned int head; //0xeb902014
  char cmd[M2C_CMD_SIZE];
  char searchKey[M2C_SEARCHKEY_SIZE];
  unsigned int tail; //0x410209be
};

#define OPENOCD_LOG_BUF 4096
#define OPENOCD_LOG_LINES 32
#define OPENOCD_LOG_RWO_SIZES 128
  union openocd_log 
  {
    char cbuf[OPENOCD_LOG_BUF];
    char logBuf[OPENOCD_LOG_LINES][OPENOCD_LOG_RWO_SIZES];
  };

struct c2m_msg{
  unsigned int head; //0xeb902014
  char name[4];
  int openocd_pid;
  union openocd_log ocdlog;
  
  char buf[M2C_CMD_SIZE];
  char searchKey[M2C_SEARCHKEY_SIZE];
  int sta;
  // char searchKey[M2C_SEARCHKEY_SIZE];
  unsigned int tail; //0x410209be
};

struct forkProcessDes{
  int pid;
  char forkName[12];
  int rpipefds[2]; //main read
  int wpipefds[2]; //main write
  struct c2m_msg *c2m_msg;
};
#endif //__MGCF_OPENOCD_H__