#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <linux/userfaultfd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "shared-header.h"

#define errExit(msg) do { perror(msg); exit(EXIT_FAILURE); \
		        } while (0)

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

void handle_userfault(int uffd) {
	static struct uffd_msg msg;   /* Data read from userfaultfd */
	static int fault_cnt = 0;     /* Number of faults so far handled */
	static char *page = NULL;
	struct uffdio_copy uffdio_copy;
	ssize_t nread;
	int page_size = sysconf(_SC_PAGE_SIZE);

	/* Create a page that will be copied into the faulting region */

	if (page == NULL) {
		page = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (page == MAP_FAILED)
			errExit("mmap");
	}

	/* Loop, handling incoming events on the userfaultfd
	   file descriptor */

	for (;;) {

		/* See what poll() tells us about the userfaultfd */

		struct pollfd pollfd;
		int nready;
		pollfd.fd = uffd;
		pollfd.events = POLLIN;
		nready = poll(&pollfd, 1, -1);
		if (nready == -1)
			errExit("poll");

		printf("\nfault_handler_thread():\n");
		printf("    poll() returns: nready = %d; "
				"POLLIN = %d; POLLERR = %d\n", nready,
				(pollfd.revents & POLLIN) != 0,
				(pollfd.revents & POLLERR) != 0);

		/* Read an event from the userfaultfd */

		nread = read(uffd, &msg, sizeof(msg));
		if (nread == 0) {
			printf("EOF on userfaultfd!\n");
			exit(EXIT_FAILURE);
		}

		if (nread == -1)
			errExit("read");

		/* We expect only one kind of event; verify that assumption */

		if (msg.event != UFFD_EVENT_PAGEFAULT) {
			fprintf(stderr, "Unexpected event on userfaultfd\n");
			exit(EXIT_FAILURE);
		}

		/* Display info about the page-fault event */

		printf("    UFFD_EVENT_PAGEFAULT event: ");
		printf("flags = %llx; ", msg.arg.pagefault.flags);
		printf("address = %llx\n", msg.arg.pagefault.address);

		/* Copy the page pointed to by 'page' into the faulting
		   region. Vary the contents that are copied in, so that it
		   is more obvious that each fault is handled separately. */

		memset(page, 'A' + fault_cnt % 20, page_size);
		fault_cnt++;

		uffdio_copy.src = (unsigned long) page;

		/* We need to handle page faults in units of pages(!).
		   So, round faulting address down to page boundary */

		uffdio_copy.dst = (unsigned long) msg.arg.pagefault.address &
			~(page_size - 1);
		uffdio_copy.len = page_size;
		uffdio_copy.mode = 0;
		uffdio_copy.copy = 0;
		if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1)
			errExit("ioctl-UFFDIO_COPY");

		printf("        (uffdio_copy.copy returned %lld)\n",
				uffdio_copy.copy);
	}
}

int main(int argc, char **argv) {
	int uffd;

	printf("In receiver pid %i\n", getpid());
	uffd = read_fd_from_socket(4);
	printf("Receiver got uffd: %i\n", uffd);
	handle_userfault(uffd);
	return 0;
}
