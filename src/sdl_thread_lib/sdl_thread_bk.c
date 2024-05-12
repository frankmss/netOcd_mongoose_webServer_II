// compile cmd: gcc test_sdl_thread_lock.c -o test_sdl_thread_lock -lSDL2
// -std=gnu99
//
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_timer.h>

#include "real_function.h"
#include "sdl_thread.h"
#include "mgcf_openocd.h"

struct swap_config_cmd m2s_data;
struct swap_status s2m_data;
extern char s_openocd_dir_user_asign[100];
void init_mutex(void) {
  m2s_data.gBufferLock = SDL_CreateMutex();
  s2m_data.gBufferLock = SDL_CreateMutex();
}

int from_swap_get_dat(struct swap_config_cmd *swap,
                      struct swap_config_cmd *data) {
  SDL_LockMutex(swap->gBufferLock);
  if (swap->this_available == 1) {
    memcpy(data, swap, sizeof(struct swap_config_cmd));
    swap->this_available = 0;
    SDL_UnlockMutex(swap->gBufferLock);
    return 1;
  } else {
    SDL_UnlockMutex(swap->gBufferLock);
    return 0;
  }

}

// push swap_config_cmd data to globle m2s_data;
int push_cmd_to_swap(struct swap_config_cmd *data) {
  struct swap_config_cmd *swap;
  swap = &m2s_data;

  SDL_LockMutex(swap->gBufferLock);

  memcpy(swap, data, sizeof(struct swap_config_cmd));
  SDL_UnlockMutex(swap->gBufferLock);
  return 0;
}

void push_status_data_to_mst(struct swap_status *dest,
                             struct swap_status *src) {
  // always wait
  SDL_LockMutex(dest->gBufferLock);

  src->this_available = 1;
  // printf("thread push:%d\n", src->cpusage);
  // dest->cpusage = src->cpusage;
  memcpy(dest, src, sizeof(struct swap_status));
  SDL_UnlockMutex(dest->gBufferLock);
}

void push_staCmd_to_client(struct swap_status *dest, struct swap_status *src) {
  // always wait
  SDL_LockMutex(dest->gBufferLock);

  src->this_available = 1;
  // printf("thread push:%d\n", src->cpusage);
  // dest->cpusage = src->cpusage;
  memcpy(dest, src, sizeof(struct swap_status));
  SDL_UnlockMutex(dest->gBufferLock);
}

void get_status_data_from_bk(struct swap_status *dest,
                             struct swap_status *src) {
  // always wait
  SDL_LockMutex(src->gBufferLock);

  src->this_available = 0;
  memcpy(dest, src, sizeof(struct swap_status));
  SDL_UnlockMutex(src->gBufferLock);
}

int get_status_data_from_bk_noblock(struct swap_status *dest,
                                    struct swap_status *src) {
  int lock_status;
  // printf("main src.lock:%x\n",src->gBufferLock);
  lock_status = SDL_TryLockMutex(src->gBufferLock);
  if (lock_status == 0) {  // get lock
    // SDL_Log("Locked mutex\n");
    if (src->this_available == 1) {
      memcpy(dest, src, sizeof(struct swap_status));
      src->this_available = 0;
    }
    SDL_UnlockMutex(src->gBufferLock);
    return 1;
  } else if (lock_status == SDL_MUTEX_TIMEDOUT) {
    /* Mutex not available for locking right now */
    return 0;
  } else {
    // SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't lock mutex\n");
    printf("ERROR: get get_status_data_from_bk_noblock lock failed!!!\n");
    return -1;
  }
}
int parseArgs_ser2net(char *psrc, char *pid, char *port, char *realDevName,
                      char *bps) {
  if (strlen(psrc) < 2) {
    return -1;
  }  // this line is null
  else {
    char devBps[300];

    int ipid = 0;
    int iport = 0;

    sscanf(psrc,
    "root%d/usr/local/openocd-withaxi/netOcd_bin/third3_bin/ser2net-C%draw600%s8DATABITSNONE1STOPBITbanner",
    &ipid, &iport, devBps);


    // printf("%d, %d, %s\n", ipid,iport,devBps);
    // return -3;
    if (ipid != 0 && iport != 0 && strlen(devBps) > 10) {
      sprintf(pid, "%d", ipid);
      sprintf(port, "%d", iport);

      // /dev/ttyS1; index10;
      memcpy(realDevName, devBps, 10);
      realDevName[10] = '\0';
      // int tmplen = strlen(strtok(devBps+strlen(realDevName),
      // "8DATABITSNONE1STOPBITbanner")); printf("<%s>",
      // strtok(devBps+strlen(realDevName), "8DATABITSNONE1STOPBITbanner"));
      int shift = 0;
      char *p = devBps + strlen(realDevName);
      while (1) {
        if ((*(p + shift)) == '8' && (*(p + 1 + shift)) == 'D' &&
            (*(p + 2 + shift)) == 'A') {
          break;
        } else {
          shift++;
        }
        if (shift >= 10) {
          return -3;
        }
      }
      memcpy(bps, devBps + strlen(realDevName), shift);
      bps[shift] = 0;
      // printf("%s,%s,%s,%s\n",pid,port,realDevName,bps);
      return 0;
    } else {
      return -2;
    }
  }
}
int getCanBps(char *canName) {
  char result_buf[2024], cmd_buf[100];

  // ifconfig | grep can0
  memset(cmd_buf, 0, sizeof(cmd_buf));
  memset(result_buf, 0, sizeof(result_buf));
  sprintf(cmd_buf, "/usr/sbin/ifconfig | grep %s", canName);
  // printf("cmd_buf:%s\n",cmd_buf);
  exec_cmd(cmd_buf, result_buf);
  if ((result_buf[0] == 0) && (result_buf[1] == 0)) {
    return -2;
  }

  memset(cmd_buf, 0, sizeof(cmd_buf));
  memset(result_buf, 0, sizeof(result_buf));
  sprintf(cmd_buf, "/usr/bin/ip -det link show %s", canName);
  // printf("cmd_buf:%s\n",cmd_buf);
  exec_cmd(cmd_buf, result_buf);
  // printf("%s\n", result_buf);
  int realBps = 0;

  if ((result_buf[0] == 0) && (result_buf[1] == 0)) {
    return -1;
  }
  char *startBegin = strstr(result_buf, "bitrate");
  sscanf(startBegin, "bitrate%dsample", &realBps);
  // printf("get bps:%d\n", realBps);
  return realBps;
}
// int parseArgs_can(struct swap_status *canStatus) {
//   int canBps[2];
//   canBps[0] = 0;
//   canBps[1] = 0;

//   canBps[0] = getCanBps("can0");
//   canBps[1] = getCanBps("can1");
//   for (int i = 0; i < 2; i++) {
//     canStatus->can_status[i].bps = canBps[i];
//   }

//   sprintf(canStatus->can_status[0].name, "can0");
//   sprintf(canStatus->can_status[1].name, "can1");
// }

void exec_cmds_queue(struct swap_config_cmd *cb) {
  cb = cb;
  if (cb->this_available != 1) {
    goto end;
  }
  printf("********exec_cmds_queue ");
  // now only for openocd, so
  if (strstr(cb->ocd_cd[0].name, "OCD0") != NULL) {
    if (cb->ocd_cd[0].cmd == 1) {
      printf("should start openocd %s->%s \n", cb->ocd_cd[0].name,
             cb->ocd_cd[0].configFile);
      // int send_cmd_to_forkThread(int ocdId, char *cmd, char *cfgFile);
      send_cmd_to_forkThread(0, "start", cb->ocd_cd[0].configFile, cb->ocd_cd[0].interfaceFile);
    } else {
      printf("should stop openocd 0\n");
      send_cmd_to_forkThread(0, "stop", NULL, NULL);
    }
  }

  if (strstr(cb->ocd_cd[1].name, "OCD1") != NULL) {
    if (cb->ocd_cd[1].cmd == 1) {
      printf("should start openocd %s->%s \n", cb->ocd_cd[1].name,
             cb->ocd_cd[1].configFile);
      send_cmd_to_forkThread(1, "start", cb->ocd_cd[1].configFile, cb->ocd_cd[1].interfaceFile);
    } else {
      printf("should stop openocd 1\n");
      send_cmd_to_forkThread(1, "stop", NULL, NULL);
    }
  }

end:
  memset(cb, 0, sizeof(struct swap_config_cmd));
  return;
}

// every time,the function must do be done
void runtine(struct swap_status *sta_dat) {
  char result_buf[2024], cmd_buf[100];
  char args[4][30];
  char tmp1[6][200];
  // get cpusage:
  memset(cmd_buf, 0, sizeof(cmd_buf));
  memset(sta_dat, 0, sizeof(struct swap_status));
  sprintf(cmd_buf,
          " ps -eo pcpu | sort -k 1 -rn | head -10 | xargs echo -n|awk "
          "\'{print $1+$2+$3+$4+$5+$6+$7+$8}\'");
  memset(result_buf, 0, sizeof(result_buf));
  exec_cmd(cmd_buf, result_buf);
  // sta_dat->cpusage = rand() % 100;
  // printf("thread:%s  float:%f\n", result_buf, atof(result_buf));
  sta_dat->cpusage = atof(result_buf);

  // get memusage:
  memset(cmd_buf, 0, sizeof(cmd_buf));
  memset(result_buf, 0, sizeof(result_buf));
  sprintf(cmd_buf,
          " ps -eo pmem | sort -k 1 -rn | head -10 |xargs echo -n|awk \'{print "
          "$1+$2+$3+$4+$5+$6+$7+$8}\'");
  exec_cmd(cmd_buf, result_buf);
  sta_dat->memusage = atof(result_buf);

  // get interface coms status:
  do {
    memset(tmp1, 0, 200 * 6);
    printf("\n");
    memset(cmd_buf, 0, sizeof(cmd_buf));
    memset(result_buf, 0, sizeof(result_buf));
    sprintf(cmd_buf,
            " ps --no-headers -eo \"uname,pid,args\" |grep ser2net | grep -v "
            "grep | sed 's/ //g'");
    // printf("get interface coms status cmd_buf:%s\n",cmd_buf);
    exec_cmd(cmd_buf, result_buf);
    //printf("get interface coms ps result->%s\n", result_buf);
    // sscanf( buffer, "%d:%d:%d %f Bps", &hours, &min, &sec, &bps );
    sscanf(result_buf, "%s\n%s\n%s\n%s\n%s\n%s\n", tmp1[0], tmp1[1], tmp1[2],
           tmp1[3], tmp1[4], tmp1[5]);
    // printf("\n%s\n",result_buf);
    for (int i = 0; i < 6; i++) {
      memset(args, 0, 4 * 30);
      //printf("tmp1[%d]:%s\n", i, tmp1[i]);
      // parseArgs_ser2net(char *psrc, char *pid, char *port, char
      // *realDevName,char*bps){
      if (parseArgs_ser2net(tmp1[i], args[0], args[1], args[2], args[3]) == 0) {
        printf("pid:%s,port:%s, name:%s, bps:%s\n", args[0], args[1], args[2],
               args[3]);
        // printf("test name %s(%d)\n",args[2],strlen(args[2]));
        sta_dat->com_status[i].pid = atoi(args[0]);
        sta_dat->com_status[i].port = atoi(args[1]);
        // strcpy(sta_dat->com_status[i].name, args[2]);
        sprintf(sta_dat->com_status[i].name, "%s", args[2]);
        // printf("test sta_dat->name %s(%d)\n", sta_dat->com_status[i].name,
        // strlen(sta_dat->com_status[i].name));
        sta_dat->com_status[i].bps = atoi(args[3]);
        sta_dat->com_status[i].sta = true;
      } else {
        sta_dat->com_status[i].sta = false;
      }
    }
  } while (0);  // get coms status over

  do {  // get can bus status;
    int canBps[2];
    canBps[0] = 0;
    canBps[1] = 0;

    canBps[0] = getCanBps("can0");
    canBps[1] = getCanBps("can1");
    for (int i = 0; i < 2; i++) {
      sta_dat->can_status[i].bps = canBps[i];
    }

    sprintf(sta_dat->can_status[0].name, "can0");
    sprintf(sta_dat->can_status[1].name, "can1");
    // printf("%s:%d\n",sta_dat->can_status[0].name,
    // sta_dat->can_status[0].bps);
    // printf("%s:%d\n",sta_dat->can_status[1].name,
    // sta_dat->can_status[1].bps);
  } while (0);

  do {  // check openocd status
    // send_cmd_to_forkThread(int ocdId, char *cmd, char *cfgFile)
    // int get_status_from_forkThread(int ocdId, struct _ocd_status *ocdSta);
    int getOcdSta = 0;
    struct swap_config_cmd cmd_data;
    int get_data = from_swap_get_dat(&m2s_data, &cmd_data);
    if (get_data == 1) {
      exec_cmds_queue((struct swap_config_cmd *)(&cmd_data));
    } else {
      send_cmd_to_forkThread(0, "check", NULL, NULL);
      send_cmd_to_forkThread(1, "check", NULL, NULL);
    }

    struct _ocd_status *ocdSta;
    ocdSta = &(sta_dat->ocd_status[0]);
    getOcdSta = get_status_from_forkThread(0, ocdSta);
    if (getOcdSta == 0) {
      // should convert json;
      printf("threak_bk %s pid:%d \n", ocdSta->name, ocdSta->pid);
      //int k = 0;
      // for(k=0;k<OPENOCD_LOG_LINES;k++){
      //   if(strlen(ocdSta->ocdlog.logBuf[k])!=0){
      //     printf("ocd0 log:%s\n",ocdSta->ocdlog.logBuf[k]);
      //   }
      // }
    }
    ocdSta = &(sta_dat->ocd_status[1]);
    getOcdSta = get_status_from_forkThread(1, ocdSta);
    if (getOcdSta == 0) {
      // should convert json;
      printf("threak_bk %s pid:%d\n", ocdSta->name, ocdSta->pid);
      //int k = 0;
      // for(k=0;k<OPENOCD_LOG_LINES;k++){
      //   if(strlen(ocdSta->ocdlog.logBuf[k])!=0){
      //     printf("ocd1 log:%s\n",ocdSta->ocdlog.logBuf[k]);
      //   }
      // }
    }
  } while (0);
}

// this funciton is end thread,
// check sys status
// exec cmd
// exec sqlite
// and so on ...
int sdl_thread_bk(void *data) {
  struct swap_status sta_data; 
  data = data;
  init_mutex();

  while (1) {
    runtine((struct swap_status *)(&sta_data));
    printf("sdl_thread_bk ocd(0).pid:%d\n", sta_data.ocd_status[0].pid);
    push_status_data_to_mst(&s2m_data, &sta_data);
    SDL_Delay(1000);

    // printf("thread s2m_data.lock %x\n", s2m_data.gBufferLock);
  }
}

void start_init_thread_bk(void) {
  // m2s_data.gBufferLock = SDL_CreateMutex();
  // s2m_data.gBufferLock = SDL_CreateMutex();
  SDL_CreateThread(sdl_thread_bk, "sdl_thread_bk", NULL);
}

// for test this lib function
// void main(void) {
//   struct swap_status main_sta_data;
//   //struct swap_config_cmd m2s_data;
//   //struct swap_status s2m_data;
//   start_init_thread_bk();
//   while (1) {
//     SDL_Delay(1);
//     get_status_data_from_bk(&main_sta_data, &s2m_data);
//     //get_status_data_from_bk_noblock(&main_sta_data, &s2m_data);
//     printf("cpusage: %d\n", main_sta_data.cpusage);
//     //main_sta_data.cpusage = 0;
//   }
// }