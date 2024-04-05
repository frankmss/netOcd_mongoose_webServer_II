#include "mgcf_openocd.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int fork_exec_cmd(char *cmd, char *result) {
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

int cRm_msg(int fd, struct m2c_msg *m2cbuf) {
  char oneChar[1];
  int aIndex = 0;
  char *pm2cbuf = (char *)m2cbuf;
  memset(pm2cbuf, 0, sizeof(struct m2c_msg));
  int x = read(fd, pm2cbuf, sizeof(struct m2c_msg));
  // printf("cRm_msg:%d(%d)\n", x, sizeof(struct m2c_msg));
  // printf("cRm_msg.head:0x%x, tail:0x%x\n",m2cbuf->head, m2cbuf->tail);
  if (x != sizeof(struct m2c_msg)) {
    // printf("cRm_msg: read fork thread error 1\n");
    return -1;
  }
  if ((m2cbuf->head == M2CMSG_HEAD) && (m2cbuf->tail == M2CMSG_TAIL)) {
    // printf("cRm_msg: read ok\n");
    return 0;
  } else {
    // printf("cRm_msg: read fork thread error 2\n");
    return -1;
  }
}

int parese_pid(char *resultBuf) {
  //printf("%s \n", resultBuf);
  if(strlen(resultBuf) == 0){
    return -1;
  }
  int openocdpid=0;
  float tmp =0;
  sscanf(resultBuf, "root %d %f *", &openocdpid, &tmp);
  // printf("--->openocdPid=%d\n",openocdpid);
  return openocdpid;
  
}

int readOcdLog(int fr, union openocd_log *log){
  int rflag = 0;
  char oneLineLog[OPENOCD_LOG_RWO_SIZES+1];
  int oneLineIndex = 0;
  char aChar[1];
  char *infoBegainCp, *errorBegainCp;
  int logIndex = 0;
  memset(oneLineLog, 0, OPENOCD_LOG_RWO_SIZES+1);
  while(1){
    rflag = read(fr, aChar, 1);
    if(rflag == 1){ // one available char
      if(aChar[0] == 10) {// a new line
        // parse new line
        //printf("orign->oneLinelog:%s\n",oneLineLog);
        infoBegainCp = strstr(oneLineLog, "Info :");
        errorBegainCp= strstr(oneLineLog, "Error:");
        if(infoBegainCp == oneLineLog || errorBegainCp == oneLineLog){
          if(strlen(oneLineLog) >= OPENOCD_LOG_RWO_SIZES){oneLineLog[OPENOCD_LOG_RWO_SIZES]='\0';}
          memcpy(log->logBuf[logIndex], oneLineLog, OPENOCD_LOG_RWO_SIZES);
          
          //printf("oneLinelog:%s\n",oneLineLog);
         // printf("log:%s\n",log->logBuf[logIndex]);
          logIndex++;
        }
        
        if(logIndex>(OPENOCD_LOG_LINES-2)){
          sprintf(log->logBuf[OPENOCD_LOG_LINES-1],"syswarn: too many logs");
          break;
        }
        
        memset(oneLineLog, 0, OPENOCD_LOG_RWO_SIZES+1);
        oneLineIndex=0;
      }else{
        oneLineLog[oneLineIndex] = aChar[0];
        
        if(oneLineIndex >= OPENOCD_LOG_RWO_SIZES){
          oneLineIndex = OPENOCD_LOG_RWO_SIZES;
        }else{
          oneLineIndex++;
        }
      }

    }else if((rflag == -1) || (rflag == 0)){
      break;
    }else {
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
  if(pipe(pipeStd) == -1){
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
      int k=0;
      
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
        memset(pchar_c2m_msg, 0, sizeof(struct c2m_msg));
        if (cRm_msg(rf, &m2c_msg) == 0) {
          // printf("child get msg :%s\n", m2c_msg.cmd);
          if (strlen(m2c_msg.cmd) != 0) {  // exec cmd
            system(m2c_msg.cmd);
            read(pipeStd[0], c2m_msg.buf, M2C_CMD_SIZE);
            //printf("**child startloop:%s\n",c2m_msg.buf);
          } else {  // ps cmdkey, get its pid and openocd log
            char resultBuf[512];
            char cmd[512];
            memset(resultBuf, 0, 512);
            memset(cmd, 0, 512);
            sprintf(cmd, "ps -aux | grep %s | grep -v grep", m2c_msg.searchKey);
            fork_exec_cmd(cmd, resultBuf);
            int openocd_pid = parese_pid(resultBuf);
            c2m_msg.openocd_pid = openocd_pid;
            //printf("\n--start---\n");
            readOcdLog(fstd, (union openocd_log *)(&(c2m_msg.ocdlog)));
            // for(k=0; k<OPENOCD_LOG_LINES; k++){
            //   if(strlen(c2m_msg.ocdlog.logBuf[k]) != 0){
            //     printf("%s\n", c2m_msg.ocdlog.logBuf[k]);
            //   }
            // }
            //printf("\n---end----\n");
            //printf("**child:%s\n",c2m_msg.buf);
          }
        }
        
        // printf("child:%d\n", i++);
        i++;
        c2m_msg.head = M2CMSG_HEAD;
        c2m_msg.tail = M2CMSG_TAIL;
        //sprintf(c2m_msg.buf, "child msg %d", i);
        
          write(wf, &c2m_msg, sizeof(struct c2m_msg));
        
        sleep(1);
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

struct forkProcessDes fpDes;
void intHandler(int dummy) {
  printf("the signal num:%d(%d), pid:%d\n", dummy, SIGKILL, fpDes.pid);
  if (dummy == 2) {
    kill(fpDes.pid, SIGKILL);
  }
  if(fpDes.c2m_msg->openocd_pid > 0){
    kill(fpDes.c2m_msg->openocd_pid, SIGKILL);
  }
  exit(0);
}

int mRc_msg(int fd, struct c2m_msg *c2mbuf) {
  char oneChar[1];
  int aIndex = 0;
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

void main() {
  struct m2c_msg m2c_msg;
  struct c2m_msg c2m_msg;
  int i = 0, k=0;
  int rf, wf;
  signal(SIGINT, intHandler);
  initFp(&fpDes);
  if (fpDes.pid < 0) {
    exit(EXIT_FAILURE);
  }
  rf = fpDes.rpipefds[0];
  wf = fpDes.wpipefds[1];

  fpDes.c2m_msg = &c2m_msg;
  memset(&m2c_msg, 0, sizeof(struct m2c_msg));
  m2c_msg.head = M2CMSG_HEAD;
  m2c_msg.tail = M2CMSG_TAIL;
  sprintf(m2c_msg.cmd, "%s", "/home/cahill/gitClone/openocd-code-withaxi/src/openocd -f /home/cahill/gitClone/openocd-code-withaxi/tcl/cahill_cfg/axiBlaster_tcl_1.tcl -f /home/cahill/gitClone/openocd-code-withaxi/tcl/target/stm32f4x.cfg -s /home/cahill/gitClone/openocd-code-withaxi/tcl -d2 &");
  write(wf, (char *)&m2c_msg, sizeof(struct m2c_msg));
  
  while (1) {
    memset(&m2c_msg, 0, sizeof(struct m2c_msg));
    m2c_msg.head = M2CMSG_HEAD;
    m2c_msg.tail = M2CMSG_TAIL;
    sprintf(m2c_msg.searchKey, "src/openocd", i);
   // if ((i % 2) == 1) {
      write(wf, (char *)&m2c_msg, sizeof(struct m2c_msg));
   // }

    printf("parent(%d,%d):%d\n", fpDes.pid, fpDes.rpipefds[0], i++);
    if (mRc_msg(rf, &c2m_msg) == 0) {
      printf("main get openocd pid:%d\n",c2m_msg.openocd_pid);
      printf("----mainget ocd log start----\n");
      for(k=0;k<OPENOCD_LOG_LINES;k++){
        if(strlen(c2m_msg.ocdlog.logBuf[k]) != 0){
          printf("%s\n", c2m_msg.ocdlog.logBuf[k]);
        }
      }
      printf("-------------------------------end---\n");
    }

    sleep(1);
  }
}