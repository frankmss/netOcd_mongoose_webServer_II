#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h> 
int pipefds[2];
char *pin;
char buffer[5000];
int init_fork_process(void) {
  pid_t pid;

  if (pipe(pipefds) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  pid = fork();
  if (pid == 0) {
    // int out = fileno(stdout);
    char cmd[300];
    close(pipefds[0]);  // close read fd
    if (dup2(pipefds[1], fileno(stdout)) == -1) {
      perror("child[pre-exec]: Failed to redirect stdin for child");
      return -1;
    }
    // sprintf(cmd, "ping 192.168.0.1 > %d", pipefds[1]);
    // sprintf(cmd, "ping 192.168.0.1");
    sprintf(cmd, "tcpdump -l -i eth0 -e -n ");
    // sprintf(cmd, "tcpdump -i enp0s31f6 -l -e -n | ./netbps");
    // We are in the child process.
    system(cmd);
  } else {
    if (pid == -1) {
      printf("create fork process error");
    }
    fcntl(pipefds[0], F_SETFL, O_NONBLOCK); 
    close(pipefds[1]);  // close write fd
  }
}

void main(void) {
  init_fork_process();
  while (1) {
    read(pipefds[0], buffer, 500); // read PIN from pipe
    printf("main: %s\n",buffer);
    sleep(1);
  }
}