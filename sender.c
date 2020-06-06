#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "shared-header.h"

int main(int argc, char **argv) {
	int fd;
	struct iovec io;
	char *iobuf = "hello world header";
	struct msghdr msg = { 0 };
	union {         /* Ancillary data buffer, wrapped in a union
			   in order to ensure it is suitably aligned */
		char buf[CMSG_SPACE(sizeof(int))];
		struct cmsghdr align;
	} u;


	printf("In sender pid %i\n", getpid());
	fd = open("/dev/zero", O_RDWR);

	io.iov_base = iobuf;
	io.iov_len = strlen(iobuf);
	printf("sizeof iobuf: %lu\n", strlen(iobuf));
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = u.buf;
	msg.msg_controllen = sizeof(u.buf);

	struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
	*((int *) CMSG_DATA(cmsg)) = fd;
	msg.msg_controllen = CMSG_SPACE(sizeof(fd));

	for(;;) {
		if (sendmsg(3, &msg, 0) < 0)
			printf("Failed to send message\n");
		sleep(1);
		//dprintf(3, "hello from sender\n");
	}
	return 0;
}
