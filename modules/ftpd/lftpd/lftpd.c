#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "lftpd.h"

#include "private/lftpd_status.h"
#include "private/lftpd_inet.h"
#include "private/lftpd_log.h"
#include "private/lftpd_string.h"
#include "private/lftpd_io.h"

// https://tools.ietf.org/html/rfc959
// https://tools.ietf.org/html/rfc2389#section-2.2
// https://tools.ietf.org/html/rfc3659
// https://tools.ietf.org/html/rfc5797
// https://tools.ietf.org/html/rfc2428#section-3 EPSV
// https://en.wikipedia.org/wiki/List_of_FTP_commands

typedef struct {
	char *command;
	int (*handler) (lftpd_client_t* client, const char* arg);
} command_t;

static int cmd_cwd();
static int cmd_dele();
static int cmd_epsv();
static int cmd_feat();
static int cmd_list();
static int cmd_nlst();
static int cmd_noop();
static int cmd_pass();
static int cmd_pasv();
static int cmd_pwd();
static int cmd_quit();
static int cmd_retr();
static int cmd_size();
static int cmd_stor();
static int cmd_syst();
static int cmd_type();
static int cmd_user();

static command_t commands[] = {
	{ "CWD", cmd_cwd },
	{ "DELE", cmd_dele },
	{ "EPSV", cmd_epsv },
	{ "FEAT", cmd_feat },
	{ "LIST", cmd_list },
	{ "NLST", cmd_nlst },
	{ "NOOP", cmd_noop },
	{ "PASS", cmd_pass },
	{ "PASV", cmd_pasv },
	{ "PWD", cmd_pwd },
	{ "QUIT", cmd_quit },
	{ "RETR", cmd_retr },
	{ "SIZE", cmd_size },
	{ "STOR", cmd_stor },
	{ "SYST", cmd_syst },
	{ "TYPE", cmd_type },
	{ "USER", cmd_user },
	{ NULL, NULL },
};

static int send_response(int socket, int code, bool include_code,
		bool multiline_start, const char* format, ...) {
	va_list args;
	va_start(args, format);
	char* message = NULL;
	int err = vasprintf(&message, format, args);
	va_end(args);
	if (err < 0) {
		return -1;
	}

	char* response = NULL;
	if (include_code) {
		if (multiline_start) {
			err = asprintf(&response, "%d-%s%s", code, message, CRLF);
		}
		else {
			err = asprintf(&response, "%d %s%s", code, message, CRLF);
		}
	}
	else {
		err = asprintf(&response, "%s%s", message, CRLF);
	}
	free(message);
	if (err < 0) {
		return -1;
	}

	err = lftpd_inet_write_string(socket, response);
	free(response);
	return err;
}

#define send_simple_response(socket, code, format, ...) send_response(socket, code, true, false, format, ##__VA_ARGS__)

#define send_multiline_response_begin(socket, code, format, ...) send_response(socket, code, true, true, format, ##__VA_ARGS__)

#define send_multiline_response_line(socket, format, ...) send_response(socket, 0, false, false, format, ##__VA_ARGS__)

#define send_multiline_response_end(socket, code, format, ...) send_response(socket, code, true, false, format, ##__VA_ARGS__)

static int send_list(int socket, const char* path) {
	// https://files.stairways.com/other/ftp-list-specs-info.txt
	// http://cr.yp.to/ftp/list/binls.html
	static const char* directory_format = "drw-rw-rw- 1 owner group %13llu Jan 01  1970 %s";
	static const char* file_format = "-rw-rw-rw- 1 owner group %13llu Jan 01  1970 %s";

	DIR* dp = opendir(path);
	if (dp == NULL) {
		return -1;
	}

	struct dirent *entry;
	while ((entry = readdir(dp))) {
		char* file_path = lftpd_io_canonicalize_path(path, entry->d_name);
		struct stat st;
		if (stat(file_path, &st) == 0) {
			unsigned long long size = st.st_size;
			if (S_ISDIR(st.st_mode)) {
				send_multiline_response_line(socket, directory_format, size, entry->d_name);
			}
			else if (S_ISREG(st.st_mode)) {
				send_multiline_response_line(socket, file_format, size, entry->d_name);
			}
		}
		free(file_path);
	}

	closedir(dp);

	return 0;
}

static int send_nlst(int socket, const char* path) {
	DIR* dp = opendir(path);
	if (dp == NULL) {
		return -1;
	}

	struct dirent *entry;
	while ((entry = readdir(dp))) {
		char* file_path = lftpd_io_canonicalize_path(path, entry->d_name);
		struct stat st;
		if (stat(file_path, &st) == 0) {
			if (S_ISREG(st.st_mode)) {
				send_multiline_response_line(socket, entry->d_name);
			}
		}
		free(file_path);
	}

	closedir(dp);

	return 0;
}

static int send_file(int socket, const char* path) {
	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		lftpd_log_error("failed to open file for read");
		return -1;
	}
	unsigned char buffer[1024];
	int read_len;
	while ((read_len = fread(buffer, 1, 1024, file)) > 0) {
		unsigned char* p = buffer;
		while (read_len) {
			int write_len = write(socket, p, read_len);
			if (write_len < 0) {
				lftpd_log_error("write error");
				fclose(file);
				return -1;
			}
			p += write_len;
			read_len -= write_len;
		}
	}

	fclose(file);

	return 0;
}

static int receive_file(int socket, const char* path) {
	FILE* file = fopen(path, "wb");
	if (file == NULL) {
		lftpd_log_error("failed to open file for write");
		return -1;
	}

	unsigned char buffer[1024];
	int err;
	while ((err = read(socket, buffer, 1024)) > 0) {
		if (fwrite(buffer, err, 1, file) != 1) {
			err = -1;
			break;
		}
	}

	fclose(file);

	if (err < 0) {
		return err;
	}

	return 0;
}

static int cmd_cwd(lftpd_client_t* client, const char* arg) {
	if (arg == NULL || strlen(arg) == 0) {
		send_simple_response(client->socket, 550, STATUS_550);
	}

	char* path = lftpd_io_canonicalize_path(client->directory, arg);

	// make sure the path exists
	struct stat st;
	if (stat(path, &st) != 0) {
		send_simple_response(client->socket, 550, STATUS_550);
		free(path);
		return -1;
	}

	// make sure the path is a directory
	if (!S_ISDIR(st.st_mode)) {
		send_simple_response(client->socket, 550, STATUS_550);
		free(path);
		return -1;
	}

	free(client->directory);
	client->directory = path;
	send_simple_response(client->socket, 250, STATUS_250);

	return 0;
}

static int cmd_dele(lftpd_client_t* client, const char* arg) {
	if (arg == NULL || strlen(arg) == 0) {
		send_simple_response(client->socket, 550, STATUS_550);
	}

	char* path = lftpd_io_canonicalize_path(client->directory, arg);

	// make sure the path exists
	struct stat st;
	if (stat(path, &st) != 0) {
		send_simple_response(client->socket, 550, STATUS_550);
		free(path);
		return -1;
	}

	// make sure the path is a file
	if (!S_ISREG(st.st_mode)) {
		send_simple_response(client->socket, 550, STATUS_550);
		free(path);
		return -1;
	}

	remove(path);
	free(path);
	send_simple_response(client->socket, 250, STATUS_250);

	return 0;
}

static int cmd_epsv(lftpd_client_t* client, const char* arg) {
	// open a data port
	int listener_socket = lftpd_inet_listen(0);
	if (listener_socket < 0) {
		send_simple_response(client->socket, 425, STATUS_425);
		return -1;
	}

	// get the port from the new socket, which is random
	int port = lftpd_inet_get_socket_port(listener_socket);

	// format the response
	send_simple_response(client->socket, 229, STATUS_229, port);

	// wait for the connection to the data port
	lftpd_log_debug("waiting for data port connection on port %d...", port);
	int client_socket = accept(listener_socket, NULL, NULL);
	if (client_socket < 0) {
		lftpd_log_error("error accepting client socket");
		close(listener_socket);
		return -1;
	}
	lftpd_log_debug("data port connection received...");

	// close the listener
	close(listener_socket);

	client->data_socket = client_socket;

	return 0;
}

static int cmd_feat(lftpd_client_t* client, const char* arg) {
	send_multiline_response_begin(client->socket, 211, STATUS_211);
	send_multiline_response_line(client->socket, "EPSV");
	send_multiline_response_line(client->socket, "PASV");
	send_multiline_response_line(client->socket, "SIZE");
	send_multiline_response_line(client->socket, "NLST");
	send_multiline_response_end(client->socket, 211, STATUS_211);
	return 0;
}

static int cmd_list(lftpd_client_t* client, const char* arg) {
	if (client->data_socket == -1) {
		send_simple_response(client->socket, 425, STATUS_425);
		return -1;
	}

	send_simple_response(client->socket, 150, STATUS_150);
	int err = send_list(client->data_socket, client->directory);
	close(client->data_socket);
	client->data_socket = -1;
	if (err == 0) {
		send_simple_response(client->socket, 226, STATUS_226);
	}
	else {
		send_simple_response(client->socket, 550, STATUS_550);
	}
	return 0;
}

static int cmd_nlst(lftpd_client_t* client, const char* arg) {
	if (client->data_socket == -1) {
		send_simple_response(client->socket, 425, STATUS_425);
		return -1;
	}

	send_simple_response(client->socket, 150, STATUS_150);
	int err = send_nlst(client->data_socket, client->directory);
	close(client->data_socket);
	client->data_socket = -1;
	if (err == 0) {
		send_simple_response(client->socket, 226, STATUS_226);
	}
	else {
		send_simple_response(client->socket, 550, STATUS_550);
	}
	return 0;
}

static int cmd_noop(lftpd_client_t* client, const char* arg) {
	send_simple_response(client->socket, 200, STATUS_200);
	return 0;
}

static int cmd_pass(lftpd_client_t* client, const char* arg) {
	send_simple_response(client->socket, 230, STATUS_230);
	return 0;
}

static int cmd_pasv(lftpd_client_t* client, const char* arg) {
	// open a data port
	int listener_socket = lftpd_inet_listen(0);
	if (listener_socket < 0) {
		send_simple_response(client->socket, 425, STATUS_425);
		return -1;
	}

	// get the port from the new socket, which is random
	int port = lftpd_inet_get_socket_port(listener_socket);

	// get our IP by reading our side of the client's control channel
	// socket connection
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(struct sockaddr_in);
	int err = getsockname(client->socket, (struct sockaddr*) &client_addr, &client_addr_len);
	if (err != 0) {
		lftpd_log_error("error getting client IP info");
		send_simple_response(client->socket, 425, STATUS_425);
		close(listener_socket);
		return -1;
	}

	// format the response
	in_addr_t ip = htonl(client_addr.sin_addr.s_addr);
	send_simple_response(client->socket, 227, STATUS_227,
			(ip >> 24) & 0xff,
			(ip >> 16) & 0xff,
			(ip >> 8) & 0xff,
			(ip >> 0) & 0xff,
			(port >> 8) & 0xff, (port >> 0) & 0xff);

	// wait for the connection to the data port
	lftpd_log_debug("waiting for data port connection on port %d...", port);
	int client_socket = accept(listener_socket, NULL, NULL);
	if (client_socket < 0) {
		lftpd_log_error("error accepting client socket");
		close(listener_socket);
		return -1;
	}
	lftpd_log_debug("data port connection received...");

	// close the listener
	close(listener_socket);

	client->data_socket = client_socket;

	return 0;
}

static int cmd_pwd(lftpd_client_t* client, const char* arg) {
	send_simple_response(client->socket, 257, "\"%s\"", client->directory);
	return 0;
}

static int cmd_quit(lftpd_client_t* client, const char* arg) {
	send_simple_response(client->socket, 221, STATUS_221);
	return -1;
}

static int cmd_retr(lftpd_client_t* client, const char* arg) {
	if (client->data_socket == -1) {
		send_simple_response(client->socket, 425, STATUS_425);
		return -1;
	}

	send_simple_response(client->socket, 150, STATUS_150);
	char* path = lftpd_io_canonicalize_path(client->directory, arg);
	lftpd_log_debug("send '%s'", path);
	int err = send_file(client->data_socket, path);
	free(path);
	close(client->data_socket);
	client->data_socket = -1;
	if (err == 0) {
		send_simple_response(client->socket, 226, STATUS_226);
	}
	else {
		send_simple_response(client->socket, 450, STATUS_450);
	}
	return 0;
}

static int cmd_size(lftpd_client_t* client, const char* arg) {
	if (!arg) {
		send_simple_response(client->socket, 550, STATUS_550);
		return 0;
	}

	char* path = lftpd_io_canonicalize_path(client->directory, arg);
	lftpd_log_debug("size %s", path);
	struct stat st;
	if (stat(path, &st) == 0) {
		send_simple_response(client->socket, 213, "%llu", st.st_size);
	}
	else {
		send_simple_response(client->socket, 550, STATUS_550);
	}
	free(path);
	return 0;
}

static int cmd_stor(lftpd_client_t* client, const char* arg) {
	if (client->data_socket == -1) {
		send_simple_response(client->socket, 425, STATUS_425);
		return -1;
	}

	send_simple_response(client->socket, 150, STATUS_150);
	char* path = lftpd_io_canonicalize_path(client->directory, arg);
	lftpd_log_debug("receive '%s'", path);
	int err = receive_file(client->data_socket, path);
	free(path);
	close(client->data_socket);
	client->data_socket = -1;
	if (err == 0) {
		send_simple_response(client->socket, 226, STATUS_226);
	}
	else {
		send_simple_response(client->socket, 450, STATUS_450);
	}
	return 0;
}

static int cmd_syst(lftpd_client_t* client, const char* arg) {
	send_simple_response(client->socket, 215, "UNIX Type: L8");
	return 0;
}

static int cmd_type(lftpd_client_t* client, const char* arg) {
	send_simple_response(client->socket, 200, STATUS_200);
	return 0;
}

static int cmd_user(lftpd_client_t* client, const char* arg) {
	send_simple_response(client->socket, 230, STATUS_230);
	return 0;
}

static int handle_control_channel(lftpd_client_t* client) {
	int err = send_simple_response(client->socket, 220, STATUS_220);
	if (err != 0) {
		lftpd_log_error("error sending welcome message");
		goto cleanup;
	}

	size_t read_buffer_len = 512;
	char* read_buffer = malloc(read_buffer_len);
	while (err == 0) {
		int line_len = lftpd_inet_read_line(client->socket, read_buffer, read_buffer_len);
		if (line_len != 0) {
			lftpd_log_error("error reading next command");
			goto cleanup;
		}

		// find the index of the first space
		int index;
		char* p = strchr(read_buffer, ' ');
		if (p != NULL) {
			index = p - read_buffer;
		}
		// if no space, use the whole string
		else {
			index = strlen(read_buffer);
		}

		// if the index is 5 or greater the command is too long
		if (index >= 5) {
			err = send_simple_response(client->socket, 500, STATUS_500);
			continue;
		}

		// copy the command into a temporary buffer
		char command_tmp[4 + 1];
		memset(command_tmp, 0, sizeof(command_tmp));
		memcpy(command_tmp, read_buffer, index);

		// upper case the command
		for (int i = 0; command_tmp[i]; i++) {
			command_tmp[i] = (char) toupper((int) command_tmp[i]);
		}

		// see if we have a matching function for the command, and if
		// so, dispatch it
		bool matched = false;
		for (int i = 0; commands[i].command; i++) {
			if (strcmp(commands[i].command, command_tmp) == 0) {
				char* arg = NULL;
				if (index < strlen(read_buffer)) {
					arg = strdup(read_buffer + index + 1);
					arg = lftpd_string_trim(arg);
				}
				err = commands[i].handler(client, arg);
				free(arg);
				matched = true;
				break;
			}
		}
		if (!matched) {
			send_simple_response(client->socket, 502, STATUS_502);
		}
	}

	cleanup:
	close(client->socket);

	return 0;
}

int lftpd_start(const char* directory, int port, lftpd_t* lftpd) {
	memset(lftpd, 0, sizeof(lftpd_t));

	lftpd->directory = directory;
	lftpd->port = port;
	lftpd->server_socket = lftpd_inet_listen(port);
	if (lftpd->server_socket < 0) {
		lftpd_log_error("error creating listener");
		return -1;
	}

	struct sockaddr_in6 server_addr;
	socklen_t server_addr_len = sizeof(struct sockaddr_in6);
	int err = getsockname(lftpd->server_socket, (struct sockaddr*) &server_addr, &server_addr_len);
	if (err != 0) {
		lftpd_log_error("error getting server IP info");
	}
	else {
		char ip[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &server_addr.sin6_addr, ip, INET6_ADDRSTRLEN);
		int port = lftpd_inet_get_socket_port(lftpd->server_socket);
		lftpd_log_info("listening on [%s]:%d...", ip, port);
	}

	while (true) {
		lftpd_log_info("waiting for connection...");

		int client_socket = accept(lftpd->server_socket, NULL, NULL);
		if (client_socket < 0) {
			lftpd_log_error("error accepting client socket");
			break;
		}

		struct sockaddr_in6 client_addr;
		socklen_t client_addr_len = sizeof(struct sockaddr_in6);
		int err = getpeername(client_socket, (struct sockaddr*) &client_addr, &client_addr_len);
		if (err != 0) {
			lftpd_log_error("error getting client IP info");
			lftpd_log_info("connection received...");
		}
		else {
			char ip[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &client_addr.sin6_addr, ip, INET6_ADDRSTRLEN);
			int port = lftpd_inet_get_socket_port(client_socket);
			lftpd_log_info("connection received from [%s]:%d...", ip, port);
		}

		lftpd_client_t client = {
				.directory = strdup(directory),
				.socket = client_socket,
				.data_socket = -1,
		};
		lftpd->client = &client;
		handle_control_channel(&client);
		free(client.directory);
		lftpd->client = NULL;
	}

	close(lftpd->server_socket);

	return 0;
}

int lftpd_stop(lftpd_t* lftpd) {
	close(lftpd->server_socket);
	if (lftpd->client) {
		close(lftpd->client->socket);
	}
	return 0;
}

int main( int argc, char *argv[] ) {
	char* cwd = getcwd(NULL, 0);
	lftpd_t lftpd;
	lftpd_start(cwd, 2121, &lftpd);
	free(cwd);
}

