#include "real_function.h"

int exec_cmd(char *cmd, char*result){
  FILE *fp;
  int status=-1;
  char tmpBuf[500];
  //memset(tmpBuf,500,0);
  /* Open the command for reading. */
  //fp = popen("/bin/ls /etc/", "r");
  //fp = popen(" cat /proc/stat |grep cpu |tail -1|awk \'{print ($5*100)/($2+$3+$4+$5+$6+$7+$8+$9+$10)}\'|awk \'{print 100-$1}\' ", "r");
  fp = popen(cmd, "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    pclose(fp);
    return status;
  }

  /* Read the output a line at a time - output it. */
  while (fgets(tmpBuf, sizeof(tmpBuf), fp) != NULL) {
    status = 0;
    memcpy(result, tmpBuf, sizeof(result)-1);
    printf("%s", result);
  }
  result[sizeof(result)] = 0;
  /* close */
  pclose(fp);
  printf(">>%s", result);
  return status;

}