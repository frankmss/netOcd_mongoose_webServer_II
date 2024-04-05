// compile cmd: gcc test_sdl_thread_lock.c -o test_sdl_thread_lock -lSDL2
// -std=gnu99
//
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_thread.h>

#include "sdl_thread.h"

struct swap_config_cmd m2s_data;
struct swap_status s2m_data;

void init_mutex(void) {
  m2s_data.gBufferLock = SDL_CreateMutex();
  s2m_data.gBufferLock = SDL_CreateMutex();
}

int from_swap_get_dat(struct swap_config_cmd *swap,
                      struct swap_config_cmd *data) {
  int lock_status;
  lock_status = SDL_TryLockMutex(swap->gBufferLock);
  if (lock_status == 0) {  // get lock
    // SDL_Log("Locked mutex\n");
    if (swap->this_available == 1) {
      memcpy(data, swap, sizeof(struct swap_config_cmd));
      swap->this_available = 0;
    }
    SDL_UnlockMutex(swap->gBufferLock);
    return 1;
  } else if (lock_status == SDL_MUTEX_TIMEDOUT) {
    /* Mutex not available for locking right now */
    return 0;
  } else {
    // SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't lock mutex\n");
    printf("ERROR: get from_swap_get_dat lock failed!!!\n");
    return -1;
  }
}

void push_status_data_to_mst(struct swap_status *dest,
                             struct swap_status *src) {
  // always wait
  SDL_LockMutex(dest->gBufferLock);

  src->this_available = 1;
  //printf("thread push:%d\n", src->cpusage);
  //dest->cpusage = src->cpusage;
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
                                    struct swap_status *src){
   int lock_status;
   printf("main src.lock:%x\n",src->gBufferLock);
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

void exec_cmds_queue(struct swap_config_cmd *cb) {}

// every time,the function must do be done
void runtine(struct swap_status *sta_dat) {
  char result_buf[100], cmd_buf[100];
  sprintf(cmd_buf, " ps -eo pcpu | sort -k 1 -rn | head -10 | xargs echo -n|awk \'{print $1+$2+$3+$4+$5+$6+$7+$8}\'");
  memset(result_buf,0,sizeof(result_buf));
  exec_cmd(cmd_buf,result_buf);
  //sta_dat->cpusage = rand() % 100;
  //printf("thread:%s  float:%f\n", result_buf, atof(result_buf));
  sta_dat->cpusage =  atof(result_buf);
}

// this funciton is end thread,
// check sys status
// exec cmd
// exec sqlite
// and so on ...
int sdl_thread_bk(void *data) {
  struct swap_config_cmd cmd_data;
  struct swap_status sta_data;
  int get_data = 0;
  init_mutex();

  while (1) {
    get_data = from_swap_get_dat(&m2s_data, &cmd_data);
    if (get_data == 1) {
      exec_cmds_queue((struct swap_config_cmd *)(&cmd_data));
    }

    runtine((struct swap_status *)(&sta_data));
    push_status_data_to_mst(&s2m_data, &sta_data);
    SDL_Delay(80);
    //printf("thread s2m_data.lock %x\n", s2m_data.gBufferLock);
  }
}

void start_init_thread_bk(void) {
  //m2s_data.gBufferLock = SDL_CreateMutex();
  //s2m_data.gBufferLock = SDL_CreateMutex();
  SDL_CreateThread(sdl_thread_bk, "sdl_thread_bk", NULL);
}

// for test this lib function
void main(void) {
  struct swap_status main_sta_data;
  //struct swap_config_cmd m2s_data;
  //struct swap_status s2m_data;
  start_init_thread_bk();
  while (1) {
    SDL_Delay(1);
    get_status_data_from_bk(&main_sta_data, &s2m_data);
    //get_status_data_from_bk_noblock(&main_sta_data, &s2m_data);
    printf("cpusage: %d\n", main_sta_data.cpusage);
    //main_sta_data.cpusage = 0;
  }
}