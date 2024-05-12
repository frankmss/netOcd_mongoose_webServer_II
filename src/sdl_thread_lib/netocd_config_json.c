
#include <stdio.h>

#include "cJSON.h"
#include "real_function.h"
#include "sdl_thread.h"
#include "netocd_config_json.h"

//char *create_json_netocd_config(struct json_netocd_config *config) {}

int eInterfaceOne(struct jsonOneInterface *oneIfc, char *bps, char *sta) {
  sprintf((oneIfc->bps.name), "bps");
  sprintf((oneIfc->bps.val), bps);
  sprintf((oneIfc->sta.name), "sta");
  sprintf((oneIfc->sta.val), sta);
  return 0;
}

int eOcdOne(struct jsonOneOcd *oneOcd, char *fileName, char *cpu, char *sta) {
  sprintf((oneOcd->configFileName.name), "cfg");
  sprintf((oneOcd->configFileName.val), fileName);
  sprintf((oneOcd->cpu.name), "cpuName");
  sprintf((oneOcd->cpu.val), cpu);
  sprintf((oneOcd->sta.name), "sta");
  sprintf((oneOcd->sta.val), sta);
  return 0;
}

cJSON *createOneIfc(cJSON *jOneIfc, struct jsonOneInterface *oneIfc) {
  // cJSON *jOneIfc = NULL;
  cJSON *jtmp = NULL;
  //cJSON *jbps = NULL;
  //cJSON *jsta = NULL;

  // jOneIfc=cJSON_CreateObject();
  cJSON_AddItemToObject(jOneIfc, oneIfc->nodeName, jtmp = cJSON_CreateObject());
  cJSON_AddStringToObject(jtmp, oneIfc->name.name, oneIfc->name.val);
  cJSON_AddStringToObject(jtmp, oneIfc->bps.name, oneIfc->bps.val);
  cJSON_AddStringToObject(jtmp, oneIfc->sta.name, oneIfc->sta.val);
  return jOneIfc;
}

cJSON *createIfc(struct jsonInterface *pinterface) {
  cJSON *jIfc = NULL;
  cJSON *jtmp = NULL;

  jIfc = cJSON_CreateObject();
  cJSON_AddItemToObject(jIfc, pinterface->nodeName,
                        jtmp = cJSON_CreateObject());
  for (int i = 0; i < 6; i++) {
    createOneIfc(jtmp, &(pinterface->com[i]));
  }
  for (int i = 0; i < 2; i++) {
    createOneIfc(jtmp, &(pinterface->can[i]));
  }
  // createOneIfc(jtmp, &(pinterface->uart0));
  // createOneIfc(jtmp, &(pinterface->uart1));
  // createOneIfc(jtmp, &(pinterface->rs4220));
  // createOneIfc(jtmp, &(pinterface->rs4221));
  // createOneIfc(jtmp, &(pinterface->rs4850));
  // createOneIfc(jtmp, &(pinterface->rs4851));
  // createOneIfc(jtmp, &(pinterface->can0));
  // createOneIfc(jtmp, &(pinterface->can1));

  return jIfc;
}

// "UART0");
// "UART1");
// "Rs4220");
// "Rs4221");
// "Rs4850");
// "Rs4851");
// "CAN0");
// "CAN1");

// this is a static name map struct in real_function.c
extern struct interfaceMap interfaceMap;

struct json_netocd_config sct_config;
// char *create_ifcJson(struct swap_status *sstatus, char *returnBuffer) {
void create_ifcJson(struct swap_status *sstatus, char *returnBuffer) {
  struct jsonInterface *pinterface;
  struct jsonOneInterface *ptmpOneIfc;
  memset((char *)(&sct_config), 0, sizeof(struct json_netocd_config));

  // init json struct orignal name;
  for (int i = 0; i < 6; i++) {
    ptmpOneIfc = &(sct_config.interface.com[i]);
    sprintf(ptmpOneIfc->name.name, "%s", "name");

    strcpy(ptmpOneIfc->name.val, interfaceMap.pair[i].webName);
    strcpy(ptmpOneIfc->nodeName, interfaceMap.pair[i].webName);
    sprintf(ptmpOneIfc->sta.name, "sta");
    sprintf(ptmpOneIfc->sta.val, "disable");
  }

  for (int i = 0; i < 6; i++) {
    // convert realName to webName;
    char *tmpName = sstatus->com_status[i].name;
    struct _com_status *pcom_status = &(sstatus->com_status[i]);
    // int converInterfacer2w(char *realName, char *wName)
    // printf("test name: tmpName%s(%d)
    // [9]=%d[10]=%d\n",tmpName,strlen(tmpName),*(tmpName+9),*(tmpName+10));
    if (converInterfacer2w(tmpName, tmpName) != 0) {
      continue;
    }
    // printf("<%s(%d)>",tmpName,strlen(tmpName));
    for (int k = 0; k < 6; k++) {
      ptmpOneIfc = &(sct_config.interface.com[k]);
      // printf("ptmpOneIfc->name.val:%s(%d)\n",ptmpOneIfc->name.val,strlen(ptmpOneIfc->name.val));
      if (strcmp(tmpName, ptmpOneIfc->name.val) == 0) {
        // printf("**get it<%s>\n",tmpName);
        // sprintf(ptmpOneIfc->bps.name,"bps");
        // sprintf(ptmpOneIfc->bps.val,"%d",pcom_status->bps);
        char bpsVal[30];
        sprintf(bpsVal, "%d", pcom_status->bps);

        eInterfaceOne(ptmpOneIfc, bpsVal, "enable");
      } else {
        // printf("val:
        // %s(%d)",ptmpOneIfc->name.val,strlen(ptmpOneIfc->name.val) );
        // printf("tmpName:%s(%d)\n",tmpName,strlen(tmpName));
      }
    }
  }

  // init json can struct orignal name;
  for (int i = 0; i < 2; i++) {
    ptmpOneIfc = &(sct_config.interface.can[i]);
    sprintf(ptmpOneIfc->name.name, "%s", "name");

    strcpy(ptmpOneIfc->name.val, interfaceMap.pair[i + 6].webName);  // can0
    strcpy(ptmpOneIfc->nodeName, interfaceMap.pair[i + 6].webName);  // can1
    sprintf(ptmpOneIfc->sta.name, "sta");
    sprintf(ptmpOneIfc->sta.val, "disable");
  }
  for (int i = 0; i < 2; i++) {
    ptmpOneIfc = &(sct_config.interface.can[i]);
    // convert realName to webName;
    char *tmpName = sstatus->can_status[i].name;
    // struct _can_status *pcan_status = &(sstatus->can_status[i]);
    // int converInterfacer2w(char *realName, char *wName)
    // printf("test name: tmpName%s(%d)
    // [9]=%d[10]=%d\n",tmpName,strlen(tmpName),*(tmpName+9),*(tmpName+10));
    if (converInterfacer2w(tmpName, tmpName) != 0) {
      continue;
    }
    // printf("%s.%d\n",sstatus->can_status[i].name,sstatus->can_status[i].bps);
    if ((sstatus->can_status[i].bps) > 0) {  // can is running
      char bpsVal[30];
      sprintf(bpsVal, "%d", sstatus->can_status[i].bps);
      converCanBpsr2w(bpsVal, bpsVal);
      eInterfaceOne(ptmpOneIfc, bpsVal, "enable");
      printf(" running can:%s.%s\n", sstatus->can_status[i].name, bpsVal);
    }
  }

  // create json format
  pinterface = &(sct_config.interface);
  sprintf((pinterface->nodeName), "interface");
  cJSON *jIfc = createIfc(pinterface);
  printf("create_ifcJson function:\n");
  char *pstr = cJSON_PrintUnformatted(jIfc);
  // char *pstr = cJSON_Print(jIfc);
  cJSON_Delete(jIfc);
  // printf("\n%s\n", pstr);
  sprintf(returnBuffer, "%s", pstr);
  cJSON_free(pstr);

  // return pstr;
}

void create_svfJson(char *cfgList_str,int bufSize) {
  char tmpFileName[100], availableName[100];
  cJSON *js_cfglist = NULL;
  cJSON *jsOneFile = NULL;
  cJSON *name = NULL;
  cJSON *cfgConfig = cJSON_CreateObject();
  if (cfgConfig == NULL) {
    goto end;
  }
  name = cJSON_CreateString("svfConfig");
  if (name == NULL) {
    goto end;
  }
  cJSON_AddItemToObject(cfgConfig, "svfConfig", name);

  js_cfglist = cJSON_CreateArray();
  if (js_cfglist == NULL) {
    goto end;
  }
  cJSON_AddItemToObject(cfgConfig, "list", js_cfglist);
  jsOneFile = cJSON_CreateObject();
  if (jsOneFile == NULL) {
    goto end;
  }
  
  int aIndex = 0, tIndex = 0, nshift = 0;
  while (1) {
    if (cfgList_str[aIndex] == '\0') {
      break;
    }
    if (cfgList_str[aIndex] == 10) {  // check lf ,is one filename;
      
      //int tmp = 0;
      char *found;
      found = strstr(tmpFileName, ".svf");
      if (found != NULL) {
        memcpy(availableName, tmpFileName, (found - tmpFileName));
        //printf("-(%d)-\n%s\n---\n", nshift, availableName);
        
        tIndex = 0;
        nshift++;
        cJSON_AddItemReferenceToArray(js_cfglist, cJSON_CreateString(availableName));
        memset(tmpFileName, 0, 100);
        memset(availableName, 0, 100);
      }
  
      // jsOneFile = cJSON_CreateObject();
      // if (jsOneFile == NULL) {
      //   goto end;
      // }

    } else {  // the context is file name;
      tmpFileName[tIndex++] = cfgList_str[aIndex];
    }
    // if (nshift > 10) {
    //   break;
    // }
    aIndex++;
  }
  char *pstr = cJSON_Print(cfgConfig);
  // char *pstr = cJSON_PrintUnformatted(cfgConfig);
  printf("create svf file list ok\n");
  printf("%s\n",pstr);
  memset(cfgList_str,0,bufSize);
  strcpy(cfgList_str,pstr );
  cJSON_free(pstr);
  cJSON_Delete(cfgConfig);
  return ;
end:
  printf("create ocdCfg fail !!!\n");
  cJSON_Delete(cfgConfig);
  return;
}

//2024.5.10 add interface_str for jtag swd and speed config
void create_cfgJson(char *cfgList_str, char *interFace_str, int bufSize) {
  char tmpFileName[100], availableName[100];
  cJSON *js_cfglist = NULL;
  cJSON *jsOneFile = NULL;
  cJSON *name = NULL;
  cJSON *cfgConfig = cJSON_CreateObject();
  if (cfgConfig == NULL) {
    goto end;
  }
  name = cJSON_CreateString("cfgConfig");
  if (name == NULL) {
    goto end;
  }
  cJSON_AddItemToObject(cfgConfig, "cfgConfig", name);

  js_cfglist = cJSON_CreateArray();
  if (js_cfglist == NULL) {
    goto end;
  }
  cJSON_AddItemToObject(cfgConfig, "list", js_cfglist);
  jsOneFile = cJSON_CreateObject();
  if (jsOneFile == NULL) {
    goto end;
  }
  
  int aIndex = 0, tIndex = 0, nshift = 0;
  while (1) {
    if (cfgList_str[aIndex] == '\0') {
      break;
    }
    if (cfgList_str[aIndex] == 10) {  // check lf ,is one filename;
      
      //int tmp = 0;
      char *found;
      found = strstr(tmpFileName, ".cfg");
      if (found != NULL) {
        memcpy(availableName, tmpFileName, (found - tmpFileName));
        tIndex = 0;
        nshift++;
        cJSON_AddItemReferenceToArray(js_cfglist, cJSON_CreateString(availableName));
        memset(tmpFileName, 0, 100);
        memset(availableName, 0, 100);
      }
      //for check .tcl file 2024-0409 add
      found = strstr(tmpFileName, ".tcl");
      if (found != NULL) {
        memcpy(availableName, tmpFileName, (found - tmpFileName));       
        tIndex = 0;
        nshift++;
        cJSON_AddItemReferenceToArray(js_cfglist, cJSON_CreateString(availableName));
        memset(tmpFileName, 0, 100);
        memset(availableName, 0, 100);
      }
    } else {  // the context is file name;
      tmpFileName[tIndex++] = cfgList_str[aIndex];
    }
    aIndex++;
  }

  //add this while(1) for add interFace_str
  cJSON *js_interFacelist = cJSON_CreateArray();
  if (js_interFacelist == NULL) {
    goto end;
  }
  cJSON_AddItemToObject(cfgConfig, "interFacelist", js_interFacelist);
  aIndex = 0;
  tIndex = 0;
  while(1){
    if(interFace_str[aIndex] == '\0'){
      break;
    }
    if(interFace_str[aIndex] == 10){ //check lf, new line
      char *found;
      found = strstr(tmpFileName, ".tcl");
      if(found != NULL){
        memcpy(availableName, tmpFileName, (found - tmpFileName-2)); //-2 for -0 or -1 in file_name
        tIndex = 0;
        cJSON_AddItemReferenceToArray(js_interFacelist, cJSON_CreateString(availableName));
        memset(tmpFileName, 0, 100);
        memset(availableName, 0, 100);
      }
    }else {  // the context is file name;
      tmpFileName[tIndex++] = interFace_str[aIndex];
    }
    aIndex++;
  }

  char *pstr = cJSON_Print(cfgConfig);
  // char *pstr = cJSON_PrintUnformatted(cfgConfig);
  printf("create ocdCfg ok\n");
  printf("%s",pstr);
  memset(cfgList_str,0,bufSize);
  strcpy(cfgList_str,pstr );
  cJSON_free(pstr);
  cJSON_Delete(cfgConfig);
  return ;
end:
  printf("create ocdCfg fail !!!\n");
  cJSON_Delete(cfgConfig);
  return;
}

int create_ocdJson(struct swap_status *sstatus, char *jOcdInfc){
  struct _ocd_status *ocdSta0;
  struct _ocd_status *ocdSta1;
  cJSON *jOcdInterface =NULL;
  cJSON *jocd0 = NULL;
  cJSON *jocd1 = NULL;
  cJSON *jocd0Log = NULL;
  cJSON *jocd1Log = NULL;


  cJSON *jtmpOne = NULL;
  int k=0;

  ocdSta0 = &(sstatus->ocd_status[0]);
  ocdSta1 = &(sstatus->ocd_status[1]);

  jOcdInterface = cJSON_CreateObject();
  jocd0 = cJSON_CreateObject();
  jocd1 = cJSON_CreateObject();

  if((jOcdInterface==NULL) || (jocd1==NULL) || (jocd0==NULL)){
    printf("create_ocdJson error 0\n");
    return -1;
  }
  cJSON_AddItemToObject(jOcdInterface, "OCD0", jocd0);
  cJSON_AddItemToObject(jOcdInterface, "OCD1", jocd1);

  
  
  
  // halt means not get msg from fork,
  // disable means openocd is not run,
  // enable means openocd is running,
  if(ocdSta0->pid == 0){

    cJSON_AddItemToObject(jocd0, "sta", cJSON_CreateString("halt"));
  }else if(ocdSta0->pid == -1){
    cJSON_AddItemToObject(jocd0, "sta", cJSON_CreateString("disable"));
  }else {
    cJSON_AddItemToObject(jocd0, "sta", cJSON_CreateString("enable"));
    cJSON_AddItemToObject(jocd0, "cfg", cJSON_CreateString(ocdSta0->configFile));
    cJSON_AddItemToObject(jocd0, "interfacecfg", cJSON_CreateString(ocdSta0->interfaceFile));
  }

  if(ocdSta1->pid == 0){
    cJSON_AddItemToObject(jocd1, "sta", cJSON_CreateString("halt"));
  }else if(ocdSta1->pid == -1){
    cJSON_AddItemToObject(jocd1, "sta", cJSON_CreateString("disable"));
  }else {
    cJSON_AddItemToObject(jocd1, "sta", cJSON_CreateString("enable"));
    cJSON_AddItemToObject(jocd1, "cfg", cJSON_CreateString(ocdSta1->configFile));
    cJSON_AddItemToObject(jocd1, "interfacecfg", cJSON_CreateString(ocdSta1->interfaceFile));
  }


  jocd0Log = cJSON_CreateArray();
  jocd1Log = cJSON_CreateArray();
  
  cJSON_AddItemToObject(jocd0, "log", jocd0Log);
  cJSON_AddItemToObject(jocd1, "log", jocd1Log);
  for(k=0;k<OPENOCD_LOG_LINES;k++){
   // printf("****for****<%c(%d)>\n", ocdSta0->ocdlog.logBuf[k][0],ocdSta0->ocdlog.logBuf[k][0]);
    if(strlen(ocdSta0->ocdlog.logBuf[k]) != 0){
      jtmpOne  = cJSON_CreateObject(); //tmp one;
      switch(ocdSta0->ocdlog.logBuf[k][0]){
        case 'I':
          cJSON_AddItemToObject(jtmpOne, "Info",cJSON_CreateString(ocdSta0->ocdlog.logBuf[k])); 
        break;
        case 'E':
          cJSON_AddItemToObject(jtmpOne, "Error",cJSON_CreateString(ocdSta0->ocdlog.logBuf[k]));
        break;
        case 's':
          cJSON_AddItemToObject(jtmpOne, "Warn",cJSON_CreateString(ocdSta0->ocdlog.logBuf[k]));
        break;
        default:
          cJSON_AddItemToObject(jtmpOne, "Other",cJSON_CreateString(ocdSta0->ocdlog.logBuf[k]));
        break;
      }
      cJSON_AddItemToArray(jocd0Log,jtmpOne);
      
    }

    if(strlen(ocdSta1->ocdlog.logBuf[k]) != 0){
      //printf("********<%c(%d)%s>\n", ocdSta1->ocdlog.logBuf[k][0],ocdSta1->ocdlog.logBuf[k][0],ocdSta1->ocdlog.logBuf[k]);
      jtmpOne  = cJSON_CreateObject(); //tmp one;
      switch(ocdSta1->ocdlog.logBuf[k][0]){
        case 'I':
          cJSON_AddItemToObject(jtmpOne, "Info",cJSON_CreateString(ocdSta1->ocdlog.logBuf[k]));
        break;
        case 'E':
          cJSON_AddItemToObject(jtmpOne, "Error",cJSON_CreateString(ocdSta1->ocdlog.logBuf[k]));
        break;
        case 's':
          cJSON_AddItemToObject(jtmpOne, "Warn",cJSON_CreateString(ocdSta1->ocdlog.logBuf[k]));
        break;
        default:
          cJSON_AddItemToObject(jtmpOne, "Other",cJSON_CreateString(ocdSta1->ocdlog.logBuf[k]));
        break;
      }
      cJSON_AddItemToArray(jocd1Log,jtmpOne);
    }
  }

  char *pstr = cJSON_Print(jOcdInterface);
  // printf("%s\n", pstr);
  sprintf(jOcdInfc,"%s", pstr);
  cJSON_Delete(jOcdInterface);
  
  cJSON_free(pstr);
  return 0;
}

// void main(void){
//   struct jsonOne *ptmp;
//   struct jsonInterface *pinterface;
//   struct jsonOcd *pocd;
//   struct json_netocd_config sct_config;
//   struct jsonOneInterface *ptmpOneIfc;
//   struct jsonOneOcd *ptmpOneOcd;
//   pinterface = &(sct_config.interface);
//   pocd = &(sct_config.ocd);

//   // sct_config.config = &tmp;
//   // sct_config.interface = &interface;
//   // sct_config.ocd = &ocd;
// do{
//   sprintf((sct_config.config.name),"config");
//   sprintf((sct_config.config.val),"netocd");

//   sprintf((pinterface->nodeName), "interface");
//   ptmpOneIfc = &(sct_config.interface.uart0);
//   sprintf((ptmpOneIfc->nodeName),"uart0");
//   eInterfaceOne(ptmpOneIfc, "115200", "enable");

//   ptmpOneIfc = &(sct_config.interface.uart1);
//   sprintf((ptmpOneIfc->nodeName),"uart1");
//   eInterfaceOne(ptmpOneIfc, "115200", "enable");

//   ptmpOneIfc = &(sct_config.interface.rs4850);
//   sprintf((ptmpOneIfc->nodeName),"rs4850");
//   eInterfaceOne(ptmpOneIfc, "115200", "enable");

//   ptmpOneIfc = &(sct_config.interface.rs4851);
//   sprintf((ptmpOneIfc->nodeName),"rs4851");
//   eInterfaceOne(ptmpOneIfc, "115200", "enable");

//   ptmpOneIfc = &(sct_config.interface.rs4220);
//   sprintf((ptmpOneIfc->nodeName),"rs4220");
//   eInterfaceOne(ptmpOneIfc, "115200", "enable");

//   ptmpOneIfc = &(sct_config.interface.rs4221);
//   sprintf((ptmpOneIfc->nodeName),"rs4221");
//   eInterfaceOne(ptmpOneIfc, "115200", "enable");

//   ptmpOneIfc = &(sct_config.interface.can0);
//   sprintf((ptmpOneIfc->nodeName),"can0");
//   eInterfaceOne(ptmpOneIfc, "500kbps", "enable");

//   ptmpOneIfc = &(sct_config.interface.can1);
//   sprintf((ptmpOneIfc->nodeName),"can1");
//   eInterfaceOne(ptmpOneIfc, "125kbps", "enable");

//   sprintf((sct_config.ocd.nodeName),"ocd");
//   ptmpOneOcd = &(sct_config.ocd.ocd0);
//   sprintf((ptmpOneOcd->nodeName),"ocd0");
//   eOcdOne(ptmpOneOcd,"stm32f4.cfg","stm32f","enable");

//   ptmpOneOcd = &(sct_config.ocd.ocd1);
//   sprintf((ptmpOneOcd->nodeName),"ocd1");
//   eOcdOne(ptmpOneOcd,"esp32.cfg","esp32","enable");
// }while(0);
// cJSON *jIfc = createIfc(pinterface);
// char *pstr = cJSON_Print(jIfc);
// cJSON_Delete(jIfc);
// //eOcdOne(struct jsonOneOcd *oneOcd, char *fileName,char *cpu, char *sta)
//   printf("%s:%s\n",(sct_config.interface.uart0.bps.name),(sct_config.interface.uart0.bps.val));
//   printf("\n%s\n", pstr);
// }