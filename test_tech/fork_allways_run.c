#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>




int pipefds[2];
char *pin;

pid_t init_fork_process(void) {
  pid_t pid;

  if (pipe(pipefds) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  pid = fork();
  if (pid == 0) {  // child process
    // int out = fileno(stdout);
    char cmd[600];
    memset(cmd, 0, sizeof(cmd));
    close(pipefds[0]);  // close read fd stderr
    if (dup2(pipefds[1], fileno(stderr)) == -1) {
      perror("child[pre-exec]: Failed to redirect stdin for child");
      return -1;
    }

    // sprintf(cmd, "tcpdump -l -i eth0 -e -n  | netbps");
    sprintf(cmd,
            "/home/cahill/gitClone/openocd-code-withaxi/src/openocd -f /home/cahill/gitClone/openocd-code-withaxi/tcl/cahill_cfg/axiBlaster_tcl_1.tcl -f /home/cahill/gitClone/openocd-code-withaxi/tcl/target/stm32f4x.cfg -s /home/cahill/gitClone/openocd-code-withaxi/tcl &");
    // sprintf(cmd,
    //         "/home/cahill/gitClone/openocd-code-withaxi/src/openocd -h ");
    system(cmd);
   // char *args[3] = {"/home/cahill/gitClone/openocd-code-withaxi/src/openocd", "-h", NULL};
    char *args[3] = {"ls", "-al", NULL};
    // execvp(cmd,NULL);
    // execl("/home/cahill/gitClone/openocd-code-withaxi/src/openocd", 
    //       "openocd","-f","/home/cahill/gitClone/openocd-code-withaxi/tcl/cahill_cfg/axiBlaster_tcl_1.tcl",
    //       "-f", "/home/cahill/gitClone/openocd-code-withaxi/tcl/target/stm32f4x.cfg",
    //        "-s","/home/cahill/gitClone/openocd-code-withaxi/tcl",
    //       (char *)0);
    // execl("/bin/ls","ls","-al","/etc/passwd",(char * )0);
    printf("**********child execl end\n");
    //  execl("/bin/ls","ls","-al","/etc/passwd",(char * )0);
    //exit(0);
    // char *args[3] = {"ls", "-al", NULL};
    // execvp(cmd,NULL);
  } else {
    if (pid == -1) {
      printf("create fork process error");
    }
    fcntl(pipefds[0], F_SETFL, O_NONBLOCK);
    close(pipefds[1]);  // close write fd
  }
  return pid;
}

pid_t pid = -1;
void intHandler(int dummy) {
  printf("the signal num:%d(%d), pid:%d\n", dummy, SIGKILL, pid);
  if (dummy == 2) {
    kill(pid, SIGKILL);
  }
  exit(0);
}

void main(void) {
  int hours, min, sec;
  char buffer[500];
  float bps;
  int timeout=0;

  signal(SIGINT, intHandler);
  pid = init_fork_process();
  while (1) {
    memset(buffer, 0, sizeof(buffer));
    read(pipefds[0], buffer, 50);  // read PIN from pipe
    if (strlen(buffer) != 0) {
      printf("main(%d): %s", pid, buffer);
    }
    if((timeout++)>20){
      //kill(pid, SIGKILL);
    }
     sleep(1);
  }
}