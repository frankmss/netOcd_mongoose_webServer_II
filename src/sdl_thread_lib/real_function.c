#include "real_function.h"

#include "sdl_thread.h"
extern char s_openocd_dir_user_asign[100];

int exec_cmd(char *cmd, char *result) {
  FILE *fp;
  int status = -1;
  char tmpBuf[4096];
  char *presult = result;
  int cnt = 0;
  memset(tmpBuf, 4096, 0);
  /* Open the command for reading. */
  // fp = popen("/bin/ls /etc/", "r");
  // fp = popen(" cat /proc/stat |grep cpu |tail -1|awk \'{print
  // ($5*100)/($2+$3+$4+$5+$6+$7+$8+$9+$10)}\'|awk \'{print 100-$1}\' ", "r");
  fp = popen(cmd, "r");
  if (fp == NULL) {
    printf("Failed to run command\n");
    pclose(fp);
    return status;
  }

  while (fgets(tmpBuf, sizeof(tmpBuf), fp) != NULL) {
    //  printf("<%d>%s\n",strlen(tmpBuf),tmpBuf);
    memcpy(presult, tmpBuf, strlen(tmpBuf));
    cnt = strlen(tmpBuf);
    presult = presult + cnt;
    memset(tmpBuf, 0, sizeof(tmpBuf));
    //  SDL_Delay(1000);
  }

  /* close */
  pclose(fp);
  // printf(">%d>%s", sizeof(result),result);
  return status;
}

struct interfaceMap interfaceMap;
struct canBpsMap canBpsMap;
void init_interfaceMap(void) {
  sprintf(interfaceMap.pair[0].webName, "UART0");
  sprintf(interfaceMap.pair[1].webName, "UART1");
  sprintf(interfaceMap.pair[2].webName, "Rs4220");
  sprintf(interfaceMap.pair[3].webName, "Rs4221");
  sprintf(interfaceMap.pair[4].webName, "Rs4850");
  sprintf(interfaceMap.pair[5].webName, "Rs4851");
  sprintf(interfaceMap.pair[6].webName, "CAN0");
  sprintf(interfaceMap.pair[7].webName, "CAN1");

  sprintf(interfaceMap.pair[0].realName, "/dev/ttyS5");
  sprintf(interfaceMap.pair[1].realName, "/dev/ttyS4");
  sprintf(interfaceMap.pair[2].realName, "/dev/ttyS0");
  sprintf(interfaceMap.pair[3].realName, "/dev/ttyS1");
  sprintf(interfaceMap.pair[4].realName, "/dev/ttyS2");
  sprintf(interfaceMap.pair[5].realName, "/dev/ttyS3");
  sprintf(interfaceMap.pair[6].realName, "can0");
  sprintf(interfaceMap.pair[7].realName, "can1");
}
void init_canBpsMap(void) {
  sprintf(canBpsMap.pair[0].webName, "125k");
  sprintf(canBpsMap.pair[1].webName, "250k");
  sprintf(canBpsMap.pair[2].webName, "500k");
  sprintf(canBpsMap.pair[3].webName, "750k");
  sprintf(canBpsMap.pair[4].webName, "1000k");

  sprintf(canBpsMap.pair[0].realName, "124999");
  sprintf(canBpsMap.pair[1].realName, "249999");
  sprintf(canBpsMap.pair[2].realName, "499999");
  sprintf(canBpsMap.pair[3].realName, "751879");
  sprintf(canBpsMap.pair[4].realName, "999999");
}

int converCanBpsw2r(char *wBps, char *realBps) {
  int i = 0;
  for (i = 0; i < 5; i++) {
    if (strcmp(canBpsMap.pair[i].webName, wBps) == 0) {
      strcpy(realBps, canBpsMap.pair[i].realName);
      return 0;
    }
  }
  return -1;
}

int converCanBpsr2w(char *realBps, char *wBps) {
  int i = 0;
  for (i = 0; i < 8; i++) {
    if (strcmp(canBpsMap.pair[i].realName, realBps) == 0) {
      strcpy(wBps, canBpsMap.pair[i].webName);
      return 0;
    }
  }
  return -1;
}

int converInterfacew2r(char *wName, char *realName) {
  int i = 0;
  for (i = 0; i < 8; i++) {
    if (strcmp(interfaceMap.pair[i].webName, wName) == 0) {
      strcpy(realName, interfaceMap.pair[i].realName);
      return 0;
    }
  }
  return -1;
}

int converInterfacer2w(char *realName, char *wName) {
  int i = 0;
  for (i = 0; i < 8; i++) {
    if (strcmp(interfaceMap.pair[i].realName, realName) == 0) {
      strcpy(wName, interfaceMap.pair[i].webName);
      return 0;
    }
  }
  return -1;
}

int start_can(char *wName, char *port, char *bps) {
  char realName[MAPNAME_SIZE];
  if (converInterfacew2r(wName, realName) != 0) {
    return -1;
  }
  char strCmd[100];
  memset(strCmd, 0, 100);
  sprintf(strCmd, "/sbin/ip link set down %s", realName);
  char exec_result[100];
  exec_cmd(strCmd, exec_result);
  printf("cmd: %s\n", strCmd);

  memset(strCmd, 0, 100);
  memset(exec_result, 0, 100);
  char realBps[20];
  memset(realBps, 0, 20);
  converCanBpsw2r(bps, realBps);
  sprintf(strCmd, "/sbin/ip link set %s up type can bitrate %s", realName,
          realBps);
  exec_cmd(strCmd, exec_result);
  printf("cmd; %s\n", strCmd);
}

int start_trstPinLow(int32_t pinI) {
  char strCmd[100];
  memset(strCmd, 0, sizeof(strCmd));
  sprintf(strCmd, "%s/netOcd_setTrstPin_%d", s_openocd_dir_user_asign, pinI);
  char exec_result[100];
  exec_cmd(strCmd, exec_result);
  printf("cmd: %s\n", strCmd);
  return 0;
}

int start_ser2net(char *wName, char *port, char *bps) {
  char realName[MAPNAME_SIZE];
  if (converInterfacew2r(wName, realName) != 0) {
    return -1;
  }
  char strCmd[100];
  // ser2net -C "6005:raw:600:/dev/ttyS5:115200 8DATABITS NONE 1STOPBIT banner"
  sprintf(strCmd,
          "%s/ser2net -C \"%s:raw:600:%s:%s 8DATABITS NONE 1STOPBIT banner\"",
          s_openocd_dir_user_asign, port, realName, bps);
  char exec_result[100];
  exec_cmd(strCmd, exec_result);
  printf("cmd: %s\n", strCmd);
  return 0;
}

int stop_ser2net(char *wName, struct swap_status *pswap) {
  int i = 0;
  char tmpWebName[30];
  char strCmd[100];
  for (i = 0; i < 6; i++) {
    memset(tmpWebName, 0, 30);
    converInterfacer2w(pswap->com_status[i].name, tmpWebName);
    if (strcmp(tmpWebName, wName) == 0) {
      printf("kill -9 %s(%d)", wName, pswap->com_status[i].pid);
      sprintf(strCmd, "kill -9 %d", pswap->com_status[i].pid);
      char exec_result[100];
      exec_cmd(strCmd, exec_result);
      printf("cmd: %s\n", strCmd);
      return 0;
    }
  }
  return -1;
}

int stop_can(char *wName) {
  char realName[MAPNAME_SIZE];
  if (converInterfacew2r(wName, realName) != 0) {
    return -1;
  }
  char strCmd[100];
  memset(strCmd, 0, 100);
  sprintf(strCmd, "/sbin/ip link set down %s", realName);
  char exec_result[100];
  exec_cmd(strCmd, exec_result);
  printf("cmd: %s\n", strCmd);
}

int get_OcdCfgList(char *resultBuf) {
  char cfgPath[100];
  memset(cfgPath, 0, 100);
  // sprintf(cfgPath, "ls /usr/local/openocd-code-withaxi/tcl/target");
  sprintf(cfgPath, "ls %s/tcl/target", OPENOCD_PATH);
  exec_cmd(cfgPath, resultBuf);
  // printf("cfgList:%s\n",resultBuf);
}

int del_AocdCfgFile( char *fileName){
  char cfgPath[100];
  memset(cfgPath, 0, 100);
  // sprintf(cfgPath, "ls /usr/local/openocd-code-withaxi/tcl/target");
  sprintf(cfgPath, "rm %s/tcl/target/%s.cfg -rf", OPENOCD_PATH, fileName);
  char exec_result[100];
  exec_cmd(cfgPath, exec_result);
}

int cat_cfgFileContext(char* fileName, char* rsp){
  char cfgPath[100];
  memset(cfgPath, 0, 100);
  sprintf(cfgPath, "cat %s/tcl/target/%s.cfg", OPENOCD_PATH, fileName);
  exec_cmd(cfgPath, rsp);
  //printf("------->%s", rsp);
}

// openocd shoud run in fork thread, the thread start stop and check ocd sta
// so all cmd shoud exed int this fork
int start_ocd(char *ocdName, char *cfgFile) {
  struct swap_config_cmd swap_status;
  memset((char *)(&swap_status), 0, sizeof(struct swap_config_cmd));
  if (strstr(ocdName, "OCD0") != 0) {
    swap_status.ocd_cd[0].cmd = 1;
    strcpy(swap_status.ocd_cd[0].name, ocdName);
    strcpy(swap_status.ocd_cd[0].configFile, cfgFile);
  } else if (strstr(ocdName, "OCD1") != 0) {
    swap_status.ocd_cd[1].cmd = 1;
    strcpy(swap_status.ocd_cd[1].name, ocdName);
    strcpy(swap_status.ocd_cd[1].configFile, cfgFile);
  } else {
    return -1;
  }

  swap_status.this_available = 1;
  push_cmd_to_swap(&swap_status);
  // int push_cmd_to_swap(struct swap_config_cmd *data);
}

int stop_ocd(char *ocdName) {
  struct swap_config_cmd swap_status;
  memset((char *)(&swap_status), 0, sizeof(struct swap_config_cmd));
  if (strstr(ocdName, "OCD0") != 0) {
    swap_status.ocd_cd[0].cmd = 0;
    strcpy(swap_status.ocd_cd[0].name, ocdName);
  } else if (strstr(ocdName, "OCD1") != 0) {
    swap_status.ocd_cd[1].cmd = 0;
    strcpy(swap_status.ocd_cd[1].name, ocdName);
  } else {
    return -1;
  }
  swap_status.this_available = 1;
  push_cmd_to_swap(&swap_status);
}