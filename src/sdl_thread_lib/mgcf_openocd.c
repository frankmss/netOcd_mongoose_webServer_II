

#include "mgcf_openocd.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "real_function.h"

int fork_exec_cmd(char *cmd, char *result) {
  FILE *fp;
  int status = -1;
  char tmpBuf[4096];
  char *presult = result;
  int cnt = 0;
  memset(tmpBuf, 0, 4096);
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

int cRm_msg(int fd, struct m2c_msg *m2cbuf) {
  
  char *pm2cbuf = (char *)m2cbuf;
  memset(pm2cbuf, 0, sizeof(struct m2c_msg));
  int x = read(fd, pm2cbuf, sizeof(struct m2c_msg));
  
  if (x != sizeof(struct m2c_msg)) {
    printf("fork real read %d bytes != %d bytes\n", x, sizeof(struct m2c_msg));
    return -1;
  }
  if ((m2cbuf->head == M2CMSG_HEAD) && (m2cbuf->tail == M2CMSG_TAIL)) {
    printf("fork real read %d bytes ok \n", sizeof(struct m2c_msg));
    return 0;
  } else {
   
    return -1;
  }
}

int parese_pid(char *resultBuf) {
  // printf("%s \n", resultBuf);
  if (strlen(resultBuf) == 0) {
    printf("fork parse_pid resultBuf:%s(len=%d)\n", resultBuf, strlen(resultBuf));
    return -1;
  }
  int openocdpid = 0;
  float tmp = 0;
  sscanf(resultBuf, "root %d %f *", &openocdpid, &tmp);
  // printf("--->openocdPid=%d\n",openocdpid);
  return openocdpid;
}

int readOcdLog(int fr, union openocd_log *log) {
  int rflag = 0;
  char oneLineLog[OPENOCD_LOG_RWO_SIZES + 1];
  int oneLineIndex = 0;
  char aChar[1];
  char *infoBegainCp, *errorBegainCp;
  int logIndex = 0;
  memset(oneLineLog, 0, OPENOCD_LOG_RWO_SIZES + 1);
  while (1) {
    rflag = read(fr, aChar, 1);
    if (rflag == 1) {        // one available char
      if (aChar[0] == 10) {  // a new line
        // parse new line
        // printf("orign->oneLinelog:%s\n",oneLineLog);
        infoBegainCp = strstr(oneLineLog, "Info :");
        errorBegainCp = strstr(oneLineLog, "Error:");
        if (infoBegainCp == oneLineLog || errorBegainCp == oneLineLog) {
          if (strlen(oneLineLog) >= OPENOCD_LOG_RWO_SIZES) {
            oneLineLog[OPENOCD_LOG_RWO_SIZES] = '\0';
          }
          memcpy(log->logBuf[logIndex], oneLineLog, OPENOCD_LOG_RWO_SIZES);

          // printf("oneLinelog:%s\n",oneLineLog);
          // printf("log:%s\n",log->logBuf[logIndex]);
          logIndex++;
        }

        if (logIndex > (OPENOCD_LOG_LINES - 2)) {
          sprintf(log->logBuf[OPENOCD_LOG_LINES - 1], "syswarn: too many logs");
          while (1) {  // read all error
            rflag = read(fr, aChar, 1);
            if (rflag != 1) {
              return -1;
            }
          }
          break;
        }

        memset(oneLineLog, 0, OPENOCD_LOG_RWO_SIZES + 1);
        oneLineIndex = 0;
      } else {
        oneLineLog[oneLineIndex] = aChar[0];

        if (oneLineIndex >= OPENOCD_LOG_RWO_SIZES) {
          oneLineIndex = OPENOCD_LOG_RWO_SIZES;
        } else {
          oneLineIndex++;
        }
      }

    } else if ((rflag == -1) || (rflag == 0)) {
      break;
    } else {
      return -1;
    }
  }
  return 0;
}

void initFp(struct forkProcessDes *fpDes) {
  pid_t pid;
  int pipeStd[2];
  if (pipe(fpDes->rpipefds) == -1) {
    perror("create r pipe error");
    exit(EXIT_FAILURE);
  }
  if (pipe(fpDes->wpipefds) == -1) {
    perror("create w pipe error");
    exit(EXIT_FAILURE);
  }
  if (pipe(pipeStd) == -1) {
    perror("create std pipe error");
    exit(EXIT_FAILURE);
  }

  pid = fork();
  if (pid == 0) {  // child process
    do {
      int wf = 0;
      int rf = 0;
      int fstd = 0;
      int i = 0;
      //int k = 0;
      char pidSaveCfg[64];
      char pidinterfaceCfg[64];
      struct c2m_msg c2m_msg;
      struct m2c_msg m2c_msg;
      char *pchar_c2m_msg = (char *)(&c2m_msg);

      close(fpDes->rpipefds[0]);  // close read fd
      fcntl(fpDes->wpipefds[0], F_SETFL, O_NONBLOCK);
      close(fpDes->wpipefds[1]);
      fcntl(pipeStd[0], F_SETFL, O_NONBLOCK);

      rf = fpDes->wpipefds[0];
      wf = fpDes->rpipefds[1];

      if (dup2(pipeStd[1], fileno(stderr)) == -1) {
        perror("child[pre-exec]: Failed to redirect stdin for child");
        return;
      }
      close(pipeStd[1]);
      fstd = pipeStd[0];
      while (1) {  // child main loop;
        sleep(1);
        printf("fork run loop while(1) pid:%d\n", pid);
        memset(pchar_c2m_msg, 0, sizeof(struct c2m_msg));
        if (cRm_msg(rf, &m2c_msg) == 0) {
          printf("fork get msg cmd:%s, searchKey:%s\n", m2c_msg.cmd, m2c_msg.searchKey);
          if (strlen(m2c_msg.cmd) != 0) {  // exec cmd
            printf("fork get cmd:%s\n", m2c_msg.cmd);
            system(m2c_msg.cmd);
            strcpy(pidSaveCfg, m2c_msg.cfg);
            strcpy(pidinterfaceCfg, m2c_msg.interfacecfg);
            read(pipeStd[0], c2m_msg.buf, M2C_CMD_SIZE);
            //sleep(1);
            printf("**child startloop:%s\n",c2m_msg.buf);
          }  // else {  // ps cmdkey, get its pid and openocd log

          if (strlen(m2c_msg.searchKey) != 0) {
            char resultBuf[512];
            char cmd[512];
            memset(resultBuf, 0, 512);
            memset(cmd, 0, 512);
            sprintf(cmd, "ps -aux | grep %s | grep -v grep", m2c_msg.searchKey);
            fork_exec_cmd(cmd, resultBuf);
            int openocd_pid = parese_pid(resultBuf);
            c2m_msg.openocd_pid = openocd_pid;
            printf("fork get openocd_pid %d\n", openocd_pid);
            readOcdLog(fstd, (union openocd_log *)(&(c2m_msg.ocdlog)));
          }
          // printf("child:%d\n", i++);
          i++;
          c2m_msg.head = M2CMSG_HEAD;
          c2m_msg.tail = M2CMSG_TAIL;
          // sprintf(c2m_msg.buf, "child msg %d", i);
          strcpy(c2m_msg.cfg, pidSaveCfg);  // always send this item
          strcpy(c2m_msg.interfacecfg, pidinterfaceCfg);
          printf("fork xxxx c2m_msg.interfacecfg:%s\n", c2m_msg.interfacecfg);
          write(wf, &c2m_msg, sizeof(struct c2m_msg));
        }else{
          printf("fork pid:%d cant get message from master\n", fpDes->pid);
        }
        
      }
    } while (0);

  } else if (pid > 0) {  // parent process
    fcntl(fpDes->rpipefds[0], F_SETFL, O_NONBLOCK);
    close(fpDes->rpipefds[1]);
    close(fpDes->wpipefds[0]);
    fpDes->pid = pid;

  } else {
    exit(EXIT_FAILURE);
  }
}

int mRc_msg(int fd, struct c2m_msg *c2mbuf) { 
  int k = 0;
  char *pc2mbuf = (char *)c2mbuf;
  memset(pc2mbuf, 0, sizeof(struct c2m_msg));
  int x = read(fd, pc2mbuf, sizeof(struct c2m_msg));
  if (x != sizeof(struct c2m_msg)) {
    for (k = 0; k < 10; k++) {
      read(fd, pc2mbuf, sizeof(struct c2m_msg));
    }
    // printf("mRc_msg:read fork thread error 1\n");
    return -1;
  }

  if ((c2mbuf->head == M2CMSG_HEAD) && (c2mbuf->tail == M2CMSG_TAIL)) {
    return 0;
  } else {
    // printf("mRc_msg:read fork thread error 2\n");
    for (k = 0; k < 10; k++) {
      read(fd, pc2mbuf, sizeof(struct c2m_msg));
    }
    return -1;
  }
}

// for openocd 0 and 1
struct forkProcessDes fpDes[2];
void intHandler(int dummy) {
  printf("the signal num:%d(%d), pid:%d\n", dummy, SIGKILL, fpDes[0].pid);
  printf("the signal num:%d(%d), pid:%d\n", dummy, SIGKILL, fpDes[1].pid);
  if (dummy == 2) {
    kill(fpDes[0].pid, SIGKILL);
    kill(fpDes[1].pid, SIGKILL);
  }
  if (fpDes[0].c2m_msg.openocd_pid > 0) {
    kill(fpDes[0].c2m_msg.openocd_pid, SIGKILL);
  }
  if (fpDes[1].c2m_msg.openocd_pid > 0) {
    kill(fpDes[1].c2m_msg.openocd_pid, SIGKILL);
  }
  exit(0);
}

void init_mgcf_openocd(void) {
  signal(SIGINT, intHandler);
  initFp(&(fpDes[0]));
  initFp(&(fpDes[1]));
  if (fpDes[0].pid < 0 || fpDes[1].pid < 0) {
    printf("init_mgcf_openocd: fork thread error!\n");
    exit(EXIT_FAILURE);
  }
  fpDes[0].rf = fpDes[0].rpipefds[0];
  fpDes[0].wf = fpDes[0].wpipefds[1];
  fpDes[1].rf = fpDes[1].rpipefds[0];
  fpDes[1].wf = fpDes[1].wpipefds[1];
  printf("init_mgcf_openocd init ok ...\n");
}
void forcKillFpProcess(void){
  kill(fpDes[0].pid, SIGTERM);
  kill(fpDes[1].pid, SIGTERM);
}

// only for opr openocd thread,
//  cmd contain start, check, stop
int send_cmd_to_forkThread(int ocdId, char *cmd, char *cfgFile, char *interfaceFile) {
  char realcmd[512];
  char searchKey[32];
  static char nowConfigFile[2][OPENOCD_LOG_RWO_SIZES];
  static char nowOcdInterfaceFile[2][OPENOCD_LOG_RWO_SIZES];
  if (!(ocdId != 0 || ocdId != 1)) {  // there is no this ocd
    printf("sendCmdToForkThread:ocdId(%d) err", ocdId);
    return -1;
  }

  memset(realcmd, 0, 512);
  memset(searchKey, 0, 32);
  if (strstr(cmd, "start") != NULL) {
    // sprintf(realcmd,
    // "/home/cahill/gitClone/openocd-code-withaxi/src/openocd_%d -f
    // /home/cahill/gitClone/openocd-code-withaxi/tcl/cahill_cfg/axiBlaster_tcl_%d.tcl
    // -f /home/cahill/gitClone/openocd-code-withaxi/tcl/target/%s.cfg -s
    // /home/cahill/gitClone/openocd-code-withaxi/tcl -d2 &",
    //         ocdId,ocdId,cfgFile);
    if (strstr(cfgFile, "Xilinx_XVC") != NULL) {
      sprintf(realcmd, "%s/xilinx_xvc_%d &", OPENOCD_PATH, ocdId);
    } 
    else if (strstr(cfgFile, "usbip_CMSISDAP") !=
               NULL) {  // 2024 add usbip_cmsisDap
      sprintf(realcmd, "%s/webOcd_usbip_cmsisDap_%d &", OPENOCD_PATH, ocdId);
    } 
    else {
      sprintf(realcmd,
              "%s/openocd_%d -f %s/tcl/cahill_cfg/%s_%d.tcl -f "
              "%s/tcl/target/%s.cfg -s %s/tcl -d2 &",
              OPENOCD_PATH, ocdId, OPENOCD_PATH,interfaceFile, ocdId, OPENOCD_PATH, cfgFile,
              OPENOCD_PATH);
      memset(nowOcdInterfaceFile[ocdId],0, OPENOCD_LOG_RWO_SIZES);
      sprintf(nowOcdInterfaceFile[ocdId], "%s", interfaceFile);
    }
    printf("send_cmd_to_forkThread:%s\n", realcmd);
    // for xvc cfgFile
    memset(nowConfigFile[ocdId], 0, OPENOCD_LOG_RWO_SIZES);
    sprintf(nowConfigFile[ocdId], "%s", cfgFile);
  } else if (strstr(cmd, "stop") != NULL) {
    if (fpDes[ocdId].c2m_msg.openocd_pid > 0) {
      sprintf(realcmd, "kill -9 %d", fpDes[ocdId].c2m_msg.openocd_pid);
      // for xvc
      memset(nowConfigFile[ocdId], 0, OPENOCD_LOG_RWO_SIZES);
    }
  } else if (strstr(cmd, "check") != NULL) {
    if (strstr(nowConfigFile[ocdId], "Xilinx_XVC") != NULL) {
      sprintf(searchKey, "xilinx_xvc_%d", ocdId);
    }
    else if(strstr(nowConfigFile[ocdId], "usbip_CMSISDAP")){
      sprintf(searchKey, "webOcd_usbip_cmsisDap_%d", ocdId);
    }
    else {
      sprintf(searchKey, "openocd_%d", ocdId);
    }
  } else {
    printf("sendCmdToForkThread: there is no this cmd(%s)\n", cmd);
    return -2;
  }

  struct m2c_msg m2c_msg;
  memset(&m2c_msg, 0, sizeof(struct m2c_msg));
  strcpy(m2c_msg.cmd, realcmd);
  strcpy(m2c_msg.searchKey, searchKey);
  if (cfgFile != NULL) {
    memset(m2c_msg.cfg, 0, M2C_SEARCHKEY_SIZE);
    strcpy(m2c_msg.cfg, cfgFile);
    printf("m2c_msg.cfg, cfgFile: %s\n", m2c_msg.cfg);
  } else {
    strcpy(m2c_msg.cfg, nowConfigFile[ocdId]);
  }
  if(interfaceFile != NULL){
    memset(m2c_msg.interfacecfg, 0, M2C_SEARCHKEY_SIZE);
    printf("sendCmdToForkThread interfaceFile:%s\n",interfaceFile);
    strcpy(m2c_msg.interfacecfg, interfaceFile);
  }else{
    strcpy(m2c_msg.interfacecfg, nowOcdInterfaceFile[ocdId]);
  }

  printf("send_cmd_to_forkThread cfg(%d):%s\n", ocdId, m2c_msg.cfg);
  printf("send_cmd_to_forkThread interfacecfg(%d):%s\n", ocdId, m2c_msg.interfacecfg);

  m2c_msg.head = M2CMSG_HEAD;
  m2c_msg.tail = M2CMSG_TAIL;

  ssize_t wn = write(fpDes[ocdId].wf, &m2c_msg, sizeof(struct m2c_msg));
  printf("sendCmdToForkThread %d write %d bytes over current.cmd=%s\n", ocdId, wn,m2c_msg.searchKey);
  return 0;
}

int get_status_from_forkThread(int ocdId, struct _ocd_status *ocdSta) {
  int rflag = -1;
  struct c2m_msg c2m_msg;
  if (!(ocdId != 0 || ocdId != 1)) {  // there is no this ocd
    return -1;
  }
  rflag = mRc_msg(fpDes[ocdId].rf, &c2m_msg);
  if (rflag != 0) {
    return -2;
  } else {  // get openocd sta
    memset(ocdSta, 0, sizeof(struct _ocd_status));
    if (c2m_msg.openocd_pid != 0) {
      ocdSta->run = 1;
      ocdSta->pid = c2m_msg.openocd_pid;
      fpDes[ocdId].c2m_msg.openocd_pid =
          c2m_msg.openocd_pid;  // save this pid for kill ocd thread
      sprintf(ocdSta->name, "OCD%d", ocdId);
      memcpy(ocdSta->ocdlog.cbuf, c2m_msg.ocdlog.cbuf,
             sizeof(union openocd_log));
      strcpy(ocdSta->configFile, c2m_msg.cfg);
      strcpy(ocdSta->interfaceFile, c2m_msg.interfacecfg);
      printf("get_status_from_forkThread ocdSta(%d)->pid:%d\n",ocdId,ocdSta->pid);
    } else {
      ocdSta->run = 0;
      ocdSta->pid = 0;
      sprintf(ocdSta->name, "OCD%d", ocdId);
    }
  }

  return 0;
}

// void main() {
//   struct m2c_msg m2c_msg;
//   struct c2m_msg c2m_msg;
//   int i = 0, k=0;
//   int rf, wf;
//   signal(SIGINT, intHandler);
//   initFp(&fpDes);
//   if (fpDes.pid < 0) {
//     exit(EXIT_FAILURE);
//   }
//   rf = fpDes.rpipefds[0];
//   wf = fpDes.wpipefds[1];

//   fpDes.c2m_msg = &c2m_msg;
//   memset(&m2c_msg, 0, sizeof(struct m2c_msg));
//   m2c_msg.head = M2CMSG_HEAD;
//   m2c_msg.tail = M2CMSG_TAIL;
//   sprintf(m2c_msg.cmd, "%s",
//   "/home/cahill/gitClone/openocd-code-withaxi/src/openocd -f
//   /home/cahill/gitClone/openocd-code-withaxi/tcl/cahill_cfg/axiBlaster_tcl_1.tcl
//   -f /home/cahill/gitClone/openocd-code-withaxi/tcl/target/stm32f4x.cfg -s
//   /home/cahill/gitClone/openocd-code-withaxi/tcl -d2 &"); write(wf, (char
//   *)&m2c_msg, sizeof(struct m2c_msg));

//   while (1) {
//     memset(&m2c_msg, 0, sizeof(struct m2c_msg));
//     m2c_msg.head = M2CMSG_HEAD;
//     m2c_msg.tail = M2CMSG_TAIL;
//     sprintf(m2c_msg.searchKey, "src/openocd", i);
//    // if ((i % 2) == 1) {
//       write(wf, (char *)&m2c_msg, sizeof(struct m2c_msg));
//    // }

//     printf("parent(%d,%d):%d\n", fpDes.pid, fpDes.rpipefds[0], i++);
//     if (mRc_msg(rf, &c2m_msg) == 0) {
//       printf("main get openocd pid:%d\n",c2m_msg.openocd_pid);
//       printf("----mainget ocd log start----\n");
//       for(k=0;k<OPENOCD_LOG_LINES;k++){
//         if(strlen(c2m_msg.ocdlog.logBuf[k]) != 0){
//           printf("%s\n", c2m_msg.ocdlog.logBuf[k]);
//         }
//       }
//       printf("-------------------------------end---\n");
//     }

//     sleep(1);
//   }
// }