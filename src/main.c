// Copyright (c) 2021 Cesanta Software Limited
// All rights reserved
//
// Example that demonstrates how to send a large responses with limited memory.
// We're going to send a JSON array of many integer values, s_data.
// The idea is to send a response in small chunks, and let the client request
// the next chunk.
// Periodically, s_data changes, which is tracked by s_version.
// Client requests a range and a version, to ensure data integrity.
//
//  1. Start this server, type `make`
//  2. Open http://localhost:8000 in your browser

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "./get_cpuage.h"
#include "./sdl_thread_lib/netbps_fork.h"
#include "./sdl_thread_lib/sdl_thread.h"
#include "sdl_thread_lib/real_function.h"
#include "mongoose.h"
#include "net.h"
#include "./sdl_thread_lib/mgcf_openocd.h"

static const char *s_listen_on = "http://0.0.0.0:10000";
// static const char *s_root_dir = "/web_root";
char s_root_dir_user_asign[100];
char s_openocd_dir_user_asign[100];
extern struct swap_config_cmd m2s_data;
extern struct swap_status s2m_data;
extern int pipefds[2];

#define DATA_SIZE 10000        // Total number of elements
#define CHUNK_SIZE 100         // Max number returned in one API call
static int s_data[DATA_SIZE];  // Simulate some complex big data
static long s_version = 0;     // Data "version"


#if 0 //for void warning
static long getparam(struct mg_http_message *hm, const char *json_path) {
  double dv = 0;
  mg_json_get_num(hm->body, json_path, &dv);
  return dv;
}


static size_t printdata(mg_pfn_t out, void *ptr, va_list *ap) {
  unsigned start = va_arg(*ap, unsigned);
  unsigned max = start + CHUNK_SIZE;
  const char *comma = "";
  size_t n = 0;
  if (max > DATA_SIZE) max = DATA_SIZE;
  while (start < max) {
    n += mg_xprintf(out, ptr, "%s%d", comma, s_data[start]);
    comma = ",";
    start++;
  }
  return n;
}
#endif

// static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
//   if (ev == MG_EV_HTTP_MSG) {
//     struct mg_http_message *hm = ev_data;
//     if (mg_http_match_uri(hm, "/api/data")) {
//       const char *headers = "content-type: text/json\r\n";
//       long start = getparam(hm, "$.start");
//       long version = getparam(hm, "$.version");
//       MG_DEBUG(("%.*s", (int)hm->body.len, hm->body.ptr));
//       if (version > 0 && version != s_version) {
//         // Version mismatch: s_data has changed while client fetches it
//         // Tell client to restart
//         mg_http_reply(c, 200, headers, "{%Q:%Q, %Q:%ld}", "error",
//                       "wrong version", "version", version);
//       } else {
//         // Return data, up to CHUNK_SIZE elements
//         mg_http_reply(c, 200, headers, "{%Q:%ld,%Q:%ld,%Q:[%M]}", "version",
//                       s_version, "start", start, "data", printdata, start);
//       }
//     } else if (mg_http_match_uri(hm, "/websocket")) {
//       // Upgrade to websocket. From now on, a connection is a full-duplex
//       // Websocket connection, which will receive MG_EV_WS_MSG events.
//       // mg_timer_add(c->mgr, 1000, MG_TIMER_REPEAT, timer_metrics_fn, c);
//       mg_ws_upgrade(c, hm, NULL);
//     } else {
//       struct mg_http_serve_opts opts = {0};
//       opts.root_dir = s_root_dir;
//       mg_http_serve_dir(c, hm, &opts);
//     }
//   } else if (ev == MG_EV_WS_MSG) {
//     struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;

//     printf("get json %s : %s\n", "page", mg_json_get_str(wm->data, "$.page"));
//     printf("get json %s : %s\n", "time", mg_json_get_str(wm->data, "$.time"));
//     printf("get ws mes(%d): %s\n", wm->data.len, wm->data.ptr);

//     // mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
//     // char result[20];
//     struct swap_status main_sta_data;
//     get_status_data_from_bk(&main_sta_data, &s2m_data);
//     // getCpuage(result);
//     // printf("main len(%d) :%s  \n",sizeof(result), result);
//     char *buf;
//     char buffer[500];
//     int hours, min, sec;
//     float bps;
//     read(pipefds[0], buffer, 50);
//     sscanf( buffer, "%d:%d:%d %f Bps", &hours, &min, &sec, &bps );
//     buf = mg_mprintf("{ %Q: \"%s\", %Q: \"%d\", %Q:\"%d\" ,%Q:\"%d\"}", "page",
//                      "status_dashboard", "cpusage", main_sta_data.cpusage,
//                      "memusage", main_sta_data.memusage,
//                      "netbps", (int)(bps/8));
//     // if(result[0] != 0){

//     // buf = mg_mprintf("{ %Q: \"%s\", %Q: \"%s\" }", "page",
//     // "status_dashboard", "cpusage", result); }else{ buf = mg_mprintf("{ %Q:
//     // \"%s\", %Q: \"%s\" }", "page", "status_dashboard", "cpusage", "100");
//     // }
//     printf("buf:%s\n", buf);
//     struct mg_str buf_str = mg_strstrip(mg_str(buf));
//     mg_ws_send(c, buf_str.ptr, buf_str.len, WEBSOCKET_OP_TEXT);
//   }
//   (void)fn_data;
// }

static void timer_fn(void *arg) {
  for (int i = 0; i < DATA_SIZE; i++) {
    s_data[i] = rand();
  }
  s_version++;
  (void)arg;
}


// /home/cahill/gitClone/openocd-code-withaxi
// ./src/openocd -f tcl/cahill_cfg/axiBlaster_tcl_0.tcl -f tcl/target/stm32f4x.cfg -s tcl

int main(int argc, char *argv[]) {
  struct mg_mgr mgr;
// netocd init//
  if(argc != 3){
    printf("usage: netOcd_mongoose_webServer rootPath openocdPath\n");
    return 0;
  }else{
    sprintf(s_root_dir_user_asign, "%s", argv[1]);
    sprintf(s_openocd_dir_user_asign, "%s", argv[2]);
    printf("rootPath:%s\n",s_root_dir_user_asign );
    printf("execPath:%s\n",s_openocd_dir_user_asign );
    //return 0;
  }

  init_interfaceMap();
  init_canBpsMap();
  start_init_thread_bk();
  init_mgcf_openocd();
// netocd init over

  mg_log_set(MG_LL_INFO);
  // start_init_thread_bk();
  //init_fork_process();
  mg_mgr_init(&mgr);
  srand(time(NULL));
  mg_timer_add(&mgr, 5000, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, timer_fn, NULL);
  sleep(1);
  mg_http_listen(&mgr, s_listen_on, device_dashboard_fn, &mgr);
  MG_INFO(("Listening on %s", s_listen_on));
  // for (;;) mg_mgr_poll(&mgr, 1000);
  while (mgr.conns != NULL) mg_mgr_poll(&mgr, 500);
  mg_mgr_free(&mgr);
  return 0;
}
