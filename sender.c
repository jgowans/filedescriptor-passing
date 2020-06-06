#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <linux/userfaultfd.h>
#include <syscall.h>
#include <sys/ioctl.h>
#include "shared-header.h"

#define errExit(msg) do { perror(msg); exit(EXIT_FAILURE); \
		        } while (0)

static int send_fd_over_socket(int socket_fd, int fd_to_send) {
	struct iovec io;
	char *iobuf = "i'm sending you a file";
	struct msghdr msg = { 0 };
	union {         /* Ancillary data buffer, wrapped in a union
			   in order to ensure it is suitably aligned */
		char buf[CMSG_SPACE(sizeof(int))];
		struct cmsghdr align;
	} u;

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
	cmsg->cmsg_len = CMSG_LEN(sizeof(fd_to_send));
	*((int *) CMSG_DATA(cmsg)) = fd_to_send;
	msg.msg_controllen = CMSG_SPACE(sizeof(fd_to_send));

	return sendmsg(socket_fd, &msg, 0);
}

static void register_remote_userfaultfd(void *addr, unsigned long len) {
	int uffd;
	struct uffdio_api uffdio_api;
	struct uffdio_register uffdio_register;

	uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
	if (uffd == -1)
		errExit("userfaultfd");

	uffdio_api.api = UFFD_API;
	uffdio_api.features = 0;
	if (ioctl(uffd, UFFDIO_API, &uffdio_api) == -1)
		errExit("ioctl-UFFDIO_API");

	uffdio_register.range.start = (unsigned long) addr;
	uffdio_register.range.len = len;
	uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
	if (ioctl(uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
		errExit("ioctl-UFFDIO_REGISTER");


	if (send_fd_over_socket(3, uffd) < 0)
		printf("Failed to send message\n");
}

int main(int argc, char **argv) {
	char *addr;         /* Start of region handled by userfaultfd */
	unsigned long len;  /* Length of region handled by userfaultfd */

	printf("In sender pid %i\n", getpid());

	len = sysconf(_SC_PAGE_SIZE) * 4;

	addr = mmap(NULL, len, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (addr == MAP_FAILED)
		errExit("mmap");

	printf("Address returned by mmap() = %p\n", addr);


	register_remote_userfaultfd(addr, len);

	int l;
	l = 0xf;    /* Ensure that faulting address is not on a page
		       boundary, in order to test that we correctly
		       handle that case in fault_handling_thread() */
	while (l < len) {
		char c = addr[l];
		printf("Read address %p in main(): ", addr + l);
		printf("%c\n", c);
		l += 1024;
		usleep(100000);         /* Slow things down a little */
	}
	return 0;
}
