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

static int lua_respond(lua_State *L)
{
	const char *resp = luaL_checkstring(L, 1);
	lua_pushstring(L, "response");
	lua_pushstring(L, resp);
	lua_settable(L, LUA_REGISTRYINDEX);
	return 0;
}

void handle_connection(int fd)
{
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	read(fd, buffer, sizeof(buffer));
	lua_State *L;
	L = luaL_newstate();
	luaL_openlibs(L);
	lua_pushcfunction(L, lua_respond);
	lua_setglobal(L, "respond");
	lua_pushstring(L, buffer);
	lua_setglobal(L, "request");
	luaL_dofile(L, "main.lua");
	const char *resp;
	lua_pushstring(L, "response");
	lua_gettable(L, LUA_REGISTRYINDEX);
	resp = luaL_checkstring(L, 1);
	printf("%s\n", resp);
	sprintf(buffer, "HTTP 200 OK\nContent-Length: %i\n\n%s\n\n", (int)strlen(resp), resp);
	write(fd, buffer, strlen(buffer));
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
		if (!fork()) {
			handle_connection(new_socket);
		}
	}

	close(sd);

	return 0;
}
