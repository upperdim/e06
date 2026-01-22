#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

int max_fd = 0, client_count = 0;
fd_set afds, rfds, wfds;
char buf_read[1001], buf_write[42];
int ids[65000];
char *msgs[65000];

int extract_message(char **buf, char **msg) {
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add) {
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void err(char *s) {
	write(2, s, strlen(s));
}

// 6
void register_client(int fd) {
	max_fd = fd > max_fd ? fd : max_fd;
	ids[fd] = client_count++;
	msgs[fd] = NULL;
	FD_SET(fd, &afds);
	sprintf(buf_write, "server: client %d just arrived\n", ids[fd]);
	notify_other(fd, buf_write);
}

// 5
void remove_client(int fd) {
	sprintf(buf_write, "server: client %d just left\n", ids[fd]);
	notify_other(fd, buf_write);
	free(msgs[fd]);
	FD_CLR(fd, &afds);
	close(fd);
}

// 6
void send_msg(int fd) {
	char *msg;
	while (extract_message(&msgs[fd], &msg)) {
		sprintf(buf_write, "client %d: ", ids[fd]);
		notify_other(fd, buf_write);
		notify_other(fd, msgs[fd]);
		free(msg);
	}
}

// 3
void notify_other(int author, char *str) {
	for (int fd = 0; fd <= max_fd; ++fd) {
		if (FD_ISSET(fd, &wfds) && fd != author) {
			send(fd, str, strlen(str), 0);
		}
	}
}

int main(int argc, char **argv) {
	if (argc !=2) {
		err("Wrong number of arguments\n");
		exit(1);
	}

	int sockfd, connfd, len;
	struct sockaddr_in servaddr;
	
	FD_ZERO(&afds);
	
	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		err("Fatal error\n");
		exit(1);
	}
	max_fd = sockfd;
	FD_SET(max_fd, &afds);
	
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(8081);

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
		err("Fatal error\n");
		exit(1);
	}
	if (listen(sockfd, 10) != 0) {
		err("Fatal error\n");
		exit(1);
	}
	// len = sizeof(servaddr);
	// connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	// if (connfd < 0) {
	// 	printf("server acccept failed...\n");
	// 	exit(0);
	// }
	// else
	// 	printf("server acccept the client...\n");
	while (1) {
		wfds = rfds = afds;
		if (select(max_fd + 1, &rfds, &wfds, NULL, NULL) < 0) {
			err("Fatal error\n");
			exit(1);
		}

		for (int fd = 0; fd <= max_fd; ++fd) {
			if (!FD_ISSET(fd, &rfds))
				continue;
			if (fd == sockfd) {
				// Accept, 5
				socklen_t len = sizeof(servaddr);
				int client_fd = accept(sockfd, (struct sockaddr *)&servaddr, &len);
				if (client_fd >= 0) {
					register_client(client_fd);
					break;
				}
			}
			// Remove, 3
			ssize_t bytes_read = recv(fd, buf_read, 1000, 0);
			if (bytes_read <= 0) {
				remove_client(fd);
				break;
			}
			// Send msg, 3
			buf_read[bytes_read] = '\0';
			msgs[fd] = str_join(msgs[fd], buf_read);
			send_msg(fd);
		}
	}
}