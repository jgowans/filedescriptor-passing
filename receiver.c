#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "shared-header.h"

int main(int argc, char **argv) {
	struct msghdr msg = { 0 };
	struct iovec io;
	char iobuf[100];
	union {         /* Ancillary data buffer, wrapped in a union
			   in order to ensure it is suitably aligned */
		char buf[CMSG_SPACE(sizeof(int))];
		struct cmsghdr align;
	} u;

	printf("In receiver pid %i\n", getpid());
	for(;;) {
		io.iov_base = iobuf;
		io.iov_len = sizeof(iobuf);
		printf("sizeof(iobuf) %li\n", sizeof(iobuf));

		msg.msg_name = NULL;
		msg.msg_iov = &io;
		msg.msg_iovlen = 1;
		msg.msg_control = u.buf;
		msg.msg_controllen = sizeof(u.buf);
		recvmsg(4, &msg, 0);
		printf("Receiver got: %s\n", (char *)msg.msg_iov->iov_base);
	}
	return 0;
}
