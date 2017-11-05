#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
	pid_t spawnpid = fork();
	switch(spawnpid)
	{
		case -1:exit(1); break;
		case 0: exit(0); break;
		default: break;
	}
	printf("XYZZY\n");
}