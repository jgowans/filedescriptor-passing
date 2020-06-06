#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "shared-header.h"

static int extract_fd(struct msghdr *msgh) {
	struct cmsghdr *cmsg = CMSG_FIRSTHDR(msgh);

	/* This must be present, but next must be null;
	 * we expect one and only one cmesg. */
	if (!cmsg || CMSG_NXTHDR(msgh, cmsg) ||
			cmsg->cmsg_level != SOL_SOCKET ||
			cmsg->cmsg_type != SCM_RIGHTS ||
			cmsg->cmsg_len != CMSG_LEN(sizeof(int)))
		return -EINVAL;

	return *(int *)CMSG_DATA(cmsg);
}

static int read_fd_from_socket(int socket_fd) {
	struct msghdr msg = { 0 };
	struct iovec io;
	char iobuf[100];
	union {         /* Ancillary data buffer, wrapped in a union
			   in order to ensure it is suitably aligned */
		char buf[CMSG_SPACE(sizeof(int))];
		struct cmsghdr align;
	} u;
	io.iov_base = iobuf;
	io.iov_len = sizeof(iobuf);
	msg.msg_name = NULL;
	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = u.buf;
	msg.msg_controllen = sizeof(u.buf);

	recvmsg(4, &msg, 0);
	return extract_fd(&msg);
}

int main(int argc, char **argv) {

	printf("In receiver pid %i\n", getpid());
	for(;;) {
		printf("Receiver got fd number: %i\n",
				read_fd_from_socket(4));
	}
	return 0;
}
