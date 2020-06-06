#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "shared-header.h"

int main(int argc, char **argv) {
	int sv[2];
	int ret;

	ret = socketpair(AF_UNIX, SOCK_DGRAM, 0, sv);
	printf("socket pair returned: %i\n", ret);
	printf("got socket fds: %i and %i\n", sv[0], sv[1]);

	if (!fork()) {
		execl("./sender", "sender", NULL);
	}

	if (!fork()) {
		execl("./receiver", "receiver", NULL);
	}

	printf("Init's pid is %i\n", getpid());
	printf("Init's job is done. Kill me whenever\n");
	for(;;) {
		sleep(1);
	}
}
