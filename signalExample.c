#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void sigquit(int signo) {
    printf("CHILD (%d): termination request from PARENT (%d)...ta ta for now...\n", getpid(), getppid());
    exit(0);
}

int main(void) {

    struct sigaction sigact;
    sigact.sa_flags = 0;
    sigemptyset(&sigact.sa_mask);

    sigact.sa_handler = sigquit;
    if (sigaction(SIGQUIT, &sigact, NULL) < 0) {
        perror("sigaction()");
        exit(1);
    }

    pid_t pid;
    /* get child process */
    if ((pid = fork()) < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        /* child */
	printf ( "CHILD: my process id is %d\n", getpid() );
        for(;;); /* loop forever */
    } else {
        /* parent */
        sigact.sa_handler = SIG_DFL;
        sigaction(SIGQUIT, &sigact, NULL);
        /* pid holds the id of child */
        sleep(1); /* pause for 1 secs */
        printf("PARENT (%d): sending SIGQUIT/kill to %d\n", getpid(), pid);
        kill(pid,SIGQUIT);
    }

    return 0;
}

