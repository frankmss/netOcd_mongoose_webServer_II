//cat /proc/stat |grep cpu |tail -1|awk '{print ($5*100)/($2+$3+$4+$5+$6+$7+$8+$9+$10)}'|awk '{print "CPU Usage: " 100-$1}'

//ps -eo pcpu | sort -k 1 -r | head -10 | awk 'NR>1{a[++k]=$0}END{for(i=1;i<k;i++)print a[i]}'|xargs echo -n|awk '{print $1+$2+$3+$4+$5+$6+$7+$8}'
// ps -eo pmem | sort -k 1 -r | head -10 | awk 'NR>1{a[++k]=$0}END{for(i=1;i<k;i++)print a[i]}'|xargs echo -n|awk '{print $1+$2+$3+$4+$5+$6+$7+$8}'
// ps -eo pmem | sort -k 1 -r | head -30 | awk 'NR>1{a[++k]=$0}END{for(i=1;i<k;i++)print a[i]}'|xargs echo -n|awk '{print $1+$2+$3+$4+$5+$6+$7+$8+$9+$10+$11+$12+$13+$14+$15+$16+$17+$18+$19+$20+$21+$22+$23+$24+$25+$26+$27+$28+$29}'
// $20+$21+$22+$23+$24+$25+$26+$27+$28+$29
// #include <iostream>

//now use ps -eo pcpu | sort -k 1 -rn | head -10 | xargs echo -n|awk '{print $1+$2+$3+$4+$5+$6+$7+$8}'
//now should use ps -eo pmem | sort -k 1 -rn | head -10 |xargs echo -n|awk '{print $1+$2+$3+$4+$5+$6+$7+$8}'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int getCpuage(char *result){
  FILE *fp;
  int status=-1;
  char tmpBuf[500];
  //memset(tmpBuf,500,0);
  /* Open the command for reading. */
  //fp = popen("/bin/ls /etc/", "r");
  //fp = popen(" cat /proc/stat |grep cpu |tail -1|awk \'{print ($5*100)/($2+$3+$4+$5+$6+$7+$8+$9+$10)}\'|awk \'{print 100-$1}\' ", "r");
  fp = popen(" ps -eo pcpu | sort -k 1 -rn | head -10 | xargs echo -n|awk \'{print $1+$2+$3+$4+$5+$6+$7+$8}\'", "r");
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
