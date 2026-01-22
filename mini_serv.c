#include <sys/socket.h>
#include <>

void puts(char *s) {
	write(2, s, strlen(s));
}

int main(int argc, char **argv) {
	if (argc != 2) {
		puts("Wrong number of arguments\n");
		return 1;
	}

	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
}
