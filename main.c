#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT (8081)

void handle_connection(int fd)
{
	printf("Handling connection on %d\n", getpid());
	char buffer[1024] = {0};

	read(fd, buffer, sizeof(buffer));

	lua_State *L;
	L = luaL_newstate();
	luaL_openlibs(L);

	luaL_dofile(L, "main.lua");

	lua_getglobal(L, "main");
	lua_pushstring(L, buffer);
	lua_pcall(L, 1, 1, 0);
	const char *response = luaL_checkstring(L, -1);

	write(fd, response, strlen(response));
	shutdown(fd, SHUT_RDWR);
	close(fd);
	lua_close(L);
}

int main (int argc, const char *argv[])
{
	int sd;
	int new_socket;
	int buffersize = 4096;
	char *buffer = malloc(buffersize * sizeof(char));
	struct sockaddr_in addr;
	socklen_t addrlen;

	int enable = 1;

	sd = socket(AF_INET, SOCK_STREAM, 0);

	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	bind(sd, (struct sockaddr*) &addr, sizeof(addr));

	while (1) {
		if (listen(sd, 0) < 0) {
			printf("Error on listen()");
			close(sd);
			exit(1);
		}
		new_socket = accept(sd, (struct sockaddr*) &addr, &addrlen);
		pid_t pid = fork();
		if (pid == 0) {
			printf("Forking with pid %d\n", getpid());
			handle_connection(new_socket);
		}
	}

	close(sd);

	return 0;
}
