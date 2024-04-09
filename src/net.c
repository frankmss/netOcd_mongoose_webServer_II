#include "./sdl_thread_lib/netocd_config_json.h"
#include "./sdl_thread_lib/real_function.h"
#include "./sdl_thread_lib/sdl_thread.h"
#include "mongoose.h"

#define MQTT_SERVER "mqtt://broker.hivemq.com:1883"
#define MQTT_PUBLISH_TOPIC "mg/my_device"
#define MQTT_SUBSCRIBE_TOPIC "mg/#"
extern struct swap_config_cmd m2s_data;
extern struct swap_status s2m_data;

static time_t s_boot_timestamp = 0;               // Updated by SNTP
static struct mg_connection *s_sntp_conn = NULL;  // SNTP connection
// static const char *s_root_dir = "web_root";
static const char *s_root_dir = "/usr/local/mongoose_netocd/web_root";
extern char s_root_dir_user_asign[100];
// Define system time()
time_t time(time_t *tp) {
  time_t t = s_boot_timestamp + (time_t)(mg_millis() / 1000);
  if (tp != NULL) *tp = t;
  return t;
}

// Authenticated user.
// A user can be authenticated by:
//   - a name:pass pair
//   - a token
// When a user is shown a login screen, she enters a user:pass. If successful,
// a server returns user info which includes token. From that point on,
// client can use token for authentication. Tokens could be refreshed/changed
// on a server side, forcing clients to re-login.
struct user {
  const char *name, *pass, *token;
};

// This is a configuration structure we're going to show on a dashboard
static struct config {
  char *url, *pub, *sub;  // MQTT settings
} s_config;

static struct mg_connection *s_mqtt = NULL;  // MQTT connection
static bool s_connected = false;             // MQTT connection established

// Try to update a single configuration value
static void update_config(struct mg_str *body, const char *name, char **value) {
  char buf[256];
  if (mg_http_get_var(body, name, buf, sizeof(buf)) > 0) {
    free(*value);
    *value = strdup(buf);
  }
}

// Parse HTTP requests, return authenticated user or NULL
static struct user *getuser(struct mg_http_message *hm) {
  // In production, make passwords strong and tokens randomly generated
  // In this example, user list is kept in RAM. In production, it can
  // be backed by file, database, or some other method.
  static struct user users[] = {
      {"admin", "pass0", "admin_token"},
      {"user1", "pass1", "user1_token"},
      {"user2", "pass2", "user2_token"},
      {NULL, NULL, NULL},
  };
  char user[256], pass[256];
  struct user *u;
  mg_http_creds(hm, user, sizeof(user), pass, sizeof(pass));
  if (user[0] != '\0' && pass[0] != '\0') {
    // Both user and password is set, search by user/password
    for (u = users; u->name != NULL; u++)
      if (strcmp(user, u->name) == 0 && strcmp(pass, u->pass) == 0) return u;
  } else if (user[0] == '\0') {
    // Only password is set, search by token
    for (u = users; u->name != NULL; u++)
      if (strcmp(pass, u->token) == 0) return u;
  }
  return NULL;
}

// Notify all config watchers about the config change
static void send_notification(struct mg_mgr *mgr, const char *fmt, ...) {
  struct mg_connection *c;
  for (c = mgr->conns; c != NULL; c = c->next) {
    if (c->label[0] == 'W') {
      va_list ap;
      va_start(ap, fmt);
      mg_ws_vprintf(c, WEBSOCKET_OP_TEXT, fmt, &ap);
      va_end(ap);
    }
  }
}

gs_OcdCfgsList(struct mg_mgr *mgr) {
#define resultBufsize 4096 * 2
  char resultBuf[resultBufsize];
  memset(resultBuf, 0, resultBufsize);
  get_OcdCfgList(resultBuf);
  create_cfgJson(resultBuf, resultBufsize);
  send_notification(mgr, "%s", resultBuf);
  printf("ws:send:%s\n", resultBuf);
}

// Send simulated metrics data to the dashboard, for chart rendering

// Send simulated metrics data to the dashboard, for chart rendering
static void timer_metrics_fn(void *param) {
  send_notification(param, "{%Q:%Q,%Q:[%lu, %d]}", "name", "metrics", "data",
                    (unsigned long)time(NULL),
                    10 + (int)((double)rand() * 10 / RAND_MAX));
}

static void timer_getInterface_sta(void *param) {
  struct swap_status main_sta_data;
  int i;
  char *pstr;
  char msgBuf[2048];
  char ocdBuf[4096 + 2048];
  int cpusage = 0;   //
  int memusage = 0;  //

  memset(msgBuf, 0, sizeof(msgBuf));
  memset((char *)(&main_sta_data), 0, sizeof(struct swap_status));
  get_status_data_from_bk(&main_sta_data, &s2m_data);

  // generate interface status json
  create_ifcJson(&main_sta_data, msgBuf);
  send_notification(param, "%s", msgBuf);

  // generate ocd status json

  memset(ocdBuf, 0, 4096 + 2048);
  create_ocdJson(&main_sta_data, ocdBuf);
  send_notification(param, "%s", ocdBuf);
  printf("%s\n", ocdBuf);

  cpusage = main_sta_data.cpusage;
  memusage = main_sta_data.memusage;
  printf("real == cpusage:%d,mem:%d\n", cpusage, memusage);
  cpusage = cpusage + (int)((double)rand() * 3 / RAND_MAX);
  memusage = memusage + (int)((double)rand() * 3 / RAND_MAX);
  // printf("vir == cpusage:%d,mem:%d\n", cpusage, memusage);
  if (cpusage >= 105) {
    cpusage = 105;
  }
  if (memusage >= 105) {
    memusage = 105;
  }
  // send cpu mem json
  send_notification(param, "{%Q:%Q,%Q:[%lu, %d],%Q:[%lu, %d]}", "name",
                    "cpumem", "cpu", (unsigned long)time(NULL), cpusage, "mem",
                    (unsigned long)time(NULL), memusage);
}

// MQTT event handler function
static void mqtt_fn(struct mg_connection *c, int ev, void *ev_data, void *fnd) {
  if (ev == MG_EV_CONNECT && mg_url_is_ssl(s_config.url)) {
    struct mg_tls_opts opts = {.ca = "ca.pem",
                               .srvname = mg_url_host(s_config.url)};
    mg_tls_init(c, &opts);
  } else if (ev == MG_EV_MQTT_OPEN) {
    s_connected = true;
    c->is_hexdumping = 1;
    mg_mqtt_sub(s_mqtt, mg_str(s_config.sub), 2);
    send_notification(c->mgr, "{%Q:%Q,%Q:null}", "name", "config", "data");
  } else if (ev == MG_EV_MQTT_MSG) {
    struct mg_mqtt_message *mm = ev_data;
    send_notification(c->mgr, "{%Q:%Q,%Q:{%Q: %.*Q, %Q: %.*Q, %Q: %d}}", "name",
                      "message", "data", "topic", (int)mm->topic.len,
                      mm->topic.ptr, "data", (int)mm->data.len, mm->data.ptr,
                      "qos", (int)mm->qos);
  } else if (ev == MG_EV_MQTT_CMD) {
    struct mg_mqtt_message *mm = (struct mg_mqtt_message *)ev_data;
    MG_DEBUG(("cmd %d qos %d", mm->cmd, mm->qos));
  } else if (ev == MG_EV_CLOSE) {
    s_mqtt = NULL;
    if (s_connected) {
      s_connected = false;
      send_notification(c->mgr, "{%Q:%Q,%Q:null}", "name", "config", "data");
    }
  }
  (void)fnd;
}

// Keep MQTT connection open - reconnect if closed
static void timer_mqtt_fn(void *param) {
  struct mg_mgr *mgr = (struct mg_mgr *)param;
  if (s_mqtt == NULL) {
    struct mg_mqtt_opts opts = {0};
    s_mqtt = mg_mqtt_connect(mgr, s_config.url, &opts, mqtt_fn, NULL);
  }
}

// SNTP connection event handler. When we get a response from an SNTP server,
// adjust s_boot_timestamp. We'll get a valid time from that point on
static void sfn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_SNTP_TIME) {
    uint64_t t = *(uint64_t *)ev_data;
    MG_INFO(("%lu SNTP: %lld ms from epoch", c->id, t));
    s_boot_timestamp = (time_t)((t - mg_millis()) / 1000);
    c->is_closing = 1;
  } else if (ev == MG_EV_CLOSE) {
    s_sntp_conn = NULL;
  }
  (void)fn_data;
}

static void timer_sntp_fn(void *param) {  // SNTP timer function. Sync up time
  struct mg_mgr *mgr = (struct mg_mgr *)param;
  if (s_sntp_conn == NULL && s_boot_timestamp == 0) {
    s_sntp_conn = mg_sntp_connect(mgr, NULL, sfn, NULL);
  }
}

void opr_interface(struct mg_http_message *hm) {
  char bufx[50];
  char interfaceBuf[20];
  char portBuf[20];
  char bpsBuf[20];
  char actionBuf[20];

  int getInterfacei = 0;
  int getBpsi = 0;
  int getActioni = 0;
  int getPorti = 0;
  getInterfacei = mg_http_get_var(&hm->body, "interface", interfaceBuf,
                                  sizeof(interfaceBuf));
  getBpsi = mg_http_get_var(&hm->body, "bps", bpsBuf, sizeof(bpsBuf));
  getActioni =
      mg_http_get_var(&hm->body, "action", actionBuf, sizeof(actionBuf));
  getPorti = mg_http_get_var(&hm->body, "port", portBuf, sizeof(portBuf));
  // printf("%d,%d,%d,%d\n",getInterfacei, getBpsi, getActioni,getPorti);
  printf("(interface)%s.(bps)%s.(port)%s.(action)%s\n", interfaceBuf, bpsBuf,
         portBuf, actionBuf);
  if ((getInterfacei > 0) && (getBpsi > 0) && (getActioni > 0) &&
      (getPorti > 0)) {
    if (strcmp(actionBuf, "enable") == 0) {
      if (strcmp(interfaceBuf, "CAN0") == 0 ||
          strcmp(interfaceBuf, "CAN1") == 0) {
        printf("can %s should be enable\n", interfaceBuf);
        int tmp = start_can(interfaceBuf, portBuf, bpsBuf);
      } else {
        printf("com %s should be enable\n", interfaceBuf);
        int tmp = start_ser2net(interfaceBuf, portBuf, bpsBuf);
        printf("start_ser2net return %d\n", tmp);
      }
    } else {
      if (strcmp(interfaceBuf, "CAN0") == 0 ||
          strcmp(interfaceBuf, "CAN1") == 0) {
        printf("can %s should be disable\n", interfaceBuf);
        int tmp = stop_can(interfaceBuf);
      } else {
        struct swap_status main_sta_data;

        memset((char *)(&main_sta_data), 0, sizeof(struct swap_status));
        get_status_data_from_bk(&main_sta_data, &s2m_data);
        int tmp = stop_ser2net(interfaceBuf, &main_sta_data);
        printf("com %s should be disable\n", interfaceBuf);
      }
    }
  }
}

void opr_openocd(struct mg_http_message *hm) {
  char ocdName[64];
  char cfg[64];
  char action[16];
  int ocdNamei = 0, cfgi = 0, actioni;
  ocdNamei = mg_http_get_var(&hm->body, "ocd", ocdName, sizeof(ocdName));
  cfgi = mg_http_get_var(&hm->body, "cfg", cfg, sizeof(cfg));
  actioni = mg_http_get_var(&hm->body, "action", action, sizeof(action));
  if ((ocdNamei > 0) && (cfgi > 0) && (actioni > 0)) {
    printf("opr_openocd: %s->%s->%s\n", ocdName, cfg, action);
    if (strstr(action, "start") != NULL) {
      // printf("%s should start\n",ocdName);
      // int start_ocd(char *ocdName, char *cfgFile)
      start_ocd(ocdName, cfg);
    } else if (strstr(action, "stop") != NULL) {
      stop_ocd(ocdName);
    }
  }
}



void manageOcdFile(struct mg_http_message *hm){
  char cfg[64];
  char action[16];
  int actioni = 0, cfgi = 0;
  cfgi = mg_http_get_var(&hm->body, "cfg", cfg, sizeof(cfg));
  actioni = mg_http_get_var(&hm->body, "action", action, sizeof(action));
  if((cfgi>0) && (actioni>0)){
    printf("manageOcdFile: action(%s)->file(%s)", action, cfg);
    if(strstr(action, "delete") != NULL){
     del_AocdCfgFile(cfg);
    }
  }
}

void catOcdFileContext(struct mg_http_message *hm, char *rsp){
  char cfg[64];
  int cfgi = 0;
  cfgi = mg_http_get_var(&hm->body, "cfg", cfg, sizeof(cfg));
  if(cfgi>0){
    cat_cfgFileContext(cfg, rsp);
  }
}

void opr_trstPinLow(struct mg_http_message *hm) {
  char resetPin[20];
  int32_t pinI = -1;
  int yy = mg_http_get_var(&hm->body, "resetPin", resetPin, sizeof(resetPin));
  if ((yy > 0)) {
    printf("opr_trstPinLow->trstPin(%d)\n", resetPin);
    if (strstr(resetPin, "trst0") != NULL) {
      pinI = 0;
    } else if (strstr(resetPin, "trst1") != NULL) {  // trst1
      pinI = 1;
    } else {
      printf("opr_trstPinLow->trstPin pin name err xxx\n");
      return;
    }
  }
  start_trstPinLow(pinI);
}

// HTTP request handler function
void device_dashboard_fn(struct mg_connection *c, int ev, void *ev_data,
                         void *fn_data) {
  if (ev == MG_EV_OPEN && c->is_listening) {
    // mg_timer_add(c->mgr, 1000, MG_TIMER_REPEAT, timer_metrics_fn, c->mgr);
    // mg_timer_add(c->mgr, 1000, MG_TIMER_REPEAT, timer_mqtt_fn, c->mgr);
    // mg_timer_add(c->mgr, 1000, MG_TIMER_REPEAT, timer_sntp_fn, c->mgr);
    mg_timer_add(c->mgr, 1000, MG_TIMER_REPEAT, timer_getInterface_sta, c->mgr);
    s_config.url = strdup(MQTT_SERVER);
    s_config.pub = strdup(MQTT_PUBLISH_TOPIC);
    s_config.sub = strdup(MQTT_SUBSCRIBE_TOPIC);
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    struct user *u = getuser(hm);
    //  MG_INFO(("%p [%.*s] auth %s", c->fd, (int) hm->uri.len, hm->uri.ptr,
    //  u ? u->name : "NULL"));
    if (mg_http_match_uri(hm, "/api/hi")) {
      mg_http_reply(c, 200, "", "hi\n");  // Testing endpoint
    } else if (u == NULL && mg_http_match_uri(hm, "/api/#")) {
      // All URIs starting with /api/ must be authenticated
      mg_http_reply(c, 403, "", "Denied\n");
    } else if (mg_http_match_uri(hm, "/api/config/get")) {
      mg_http_reply(c, 200, NULL, "{%Q:%Q,%Q:%Q,%Q:%Q,%Q:%s}\n", "url",
                    s_config.url, "pub", s_config.pub, "sub", s_config.sub,
                    "connected", s_connected ? "true" : "false");
    } else if (mg_http_match_uri(hm, "/api/config/set")) {
      // Admins only
      if (strcmp(u->name, "admin") == 0) {
        update_config(&hm->body, "url", &s_config.url);
        update_config(&hm->body, "pub", &s_config.pub);
        update_config(&hm->body, "sub", &s_config.sub);
        if (s_mqtt) s_mqtt->is_closing = 1;  // Ask to disconnect from MQTT
        send_notification(fn_data, "{%Q:%Q,%Q:null}", "name", "config", "data");
        mg_http_reply(c, 200, "", "ok\n");
      } else {
        mg_http_reply(c, 403, "", "Denied\n");
      }
    } else if (mg_http_match_uri(hm, "/api/message/send")) {
      char buf[256];
      if (s_connected &&
          mg_http_get_var(&hm->body, "message", buf, sizeof(buf)) > 0) {
        mg_mqtt_pub(s_mqtt, mg_str(s_config.pub), mg_str(buf), 1, false);
      }
      mg_http_reply(c, 200, "", "ok\n");
    } else if (mg_http_match_uri(hm, "/api/watch")) {
      c->label[0] = 'W';  // Mark ourselves as a event listener
      mg_ws_upgrade(c, hm, NULL);
    } else if (mg_http_match_uri(hm, "/api/login")) {
      mg_http_reply(c, 200, NULL, "{%Q:%Q,%Q:%Q}\n", "user", u->name, "token",
                    u->token);
    } else if (mg_http_match_uri(hm, "/api/Navigation")) {
      char buf[256];
      if (mg_http_get_var(&hm->body, "page", buf, sizeof(buf)) > 0) {
        printf("/api/Navigation -> %s\n", buf);
        if (strcmp(buf, "click_sysSta") == 0) {
          send_notification(c->mgr, "{%Q:%Q}", "page", "sysSta");
        } else if (strcmp(buf, "click_Comconfig") == 0) {
          send_notification(c->mgr, "{%Q:%Q}", "page", "Comconfig");
        } else if (strcmp(buf, "click_OcdConfig") == 0) {
          send_notification(c->mgr, "{%Q:%Q}", "page", "OcdConfig");
          gs_OcdCfgsList(c->mgr);
        } else if (strcmp(buf, "click_editOcdOption") == 0) {
          send_notification(c->mgr, "{%Q:%Q}", "page", "editOcdCfg");
          gs_OcdCfgsList(c->mgr);
        } else if (strcmp(buf, "click_settingConfig") == 0) {
          send_notification(c->mgr, "{%Q:%Q}", "page", "settingConfig");
        }
      }
      mg_http_reply(c, 200, "", "ok\n");
    } else if (mg_http_match_uri(hm, "/api/interface")) {
      // char bufx[256];
      // int xx = mg_http_get_var(&hm->body, "uart0", bufx, sizeof(bufx));
      // printf("/api/interface(%d): %s\n",xx, bufx);
      // int yy = mg_http_get_var(&hm->body, "UART0", bufx, sizeof(bufx));
      // printf("/api/interface(%d): %s ", yy, bufx);
      // yy = mg_http_get_var(&hm->body, "port", bufx, sizeof(bufx));
      // printf("%s(%d)\n", bufx, yy);
      opr_interface(hm);
      mg_http_reply(c, 200, "", "ok\n");
    } else if (mg_http_match_uri(hm, "/api/ocd")) {
      // api/ocd config openocd
      opr_openocd(hm);
      mg_http_reply(c, 200, "", "ok\n");
    } else if (mg_http_match_uri(hm, "/api/manegeOcdCfgFile")){
      manageOcdFile(hm);
      mg_http_reply(c, 200, "", "ok\n");
      printf("after manageOcdFile should post fgsList\n");
      gs_OcdCfgsList(c->mgr);  //after delete a cfg file, should ws post cfg list
    } else if (mg_http_match_uri(hm, "/api/getOcdCfgFileContext")){
      #define CFGMAXSIZE (100*1024) //100K bytes
      printf("get view cfg context request!\n");
      const char *headers = "content-type: text/json\r\n";
      char *p_catRsp = malloc(CFGMAXSIZE);
      p_catRsp = (char*)calloc(CFGMAXSIZE, 1); //申请一块100k的内存，并且已经初始化为0
      catOcdFileContext(hm, p_catRsp);
      mg_http_reply(c, 200, headers, "{%Q:%Q}", "complexField", p_catRsp);
      free(p_catRsp);
      
    } else if (mg_http_match_uri(hm, "/api/trstExec")) {
      opr_trstPinLow(hm);
      mg_http_reply(c, 200, "", "ok\n");
    } else if (mg_http_match_uri(
                   hm, "/api/svfFileupload")) {  // "/api/svgfileupload"
      char path[80], name[64];

      mg_http_get_var(&hm->query, "name", name, sizeof(name));

      printf("/api/svgfileupload--name-->%s\n", name);
      if ((name[0] == '\0') || (strstr(name, "svf")==NULL) ) {
        mg_http_reply(c, 400, "", "%s", "file format is wrong");
        // fprintf(name, "xyz123.bin");
      } else {
        mg_snprintf(path, sizeof(path),
                    "/usr/local/openocd-withaxi/012/usrSvfDir/%s", name);
        mg_http_upload(c, hm, &mg_fs_posix, mg_remove_double_dots(path),
                       512000);
      }
    } else if (mg_http_match_uri(
                   hm, "/api/cfgFileupload")) {  // "/api/svgfileupload"
      char path[80], name[64];

      mg_http_get_var(&hm->query, "name", name, sizeof(name));

      printf("/api/cfgfileupload--name-->%s\n", name);
      if (name[0] == '\0') {
        mg_http_reply(c, 400, "", "%s", "name required");
        // fprintf(name, "xyz123.bin");
      } else {
        mg_snprintf(path, sizeof(path),
                    "/usr/local/openocd-withaxi/012/tcl/target/%s", name);
        mg_http_upload(c, hm, &mg_fs_posix, mg_remove_double_dots(path),
                       512000);
      }
    } else {
      struct mg_http_serve_opts opts = {0};
      // opts.root_dir = s_root_dir;
      opts.root_dir = s_root_dir_user_asign;
      mg_http_serve_dir(c, ev_data, &opts);
    }
    MG_DEBUG(("%.*s %.*s -> %.*s", (int)hm->method.len, hm->method.ptr,
              (int)hm->uri.len, hm->uri.ptr, (int)3, &c->send.buf[9]));
  }
}
