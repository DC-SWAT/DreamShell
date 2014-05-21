/****************************
 * DreamShell ##version##   *
 * telnetd.c                *
 * DreamShell telnet server *
 * Created by SWAT          *
 ****************************/     

#include <kos.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdio.h>
#include <sys/queue.h>
#include "ds.h"

struct tel_state;
typedef TAILQ_HEAD(tel_state_list, tel_state) tel_state_list_t;

typedef struct tel_state {
	TAILQ_ENTRY(tel_state)		list;

	int			socket;
	struct sockaddr_in	client;
	socklen_t		client_size;

	kthread_t		* thd;

	int			ptyfd;
} tel_state_t;


static tel_state_list_t states;
#define st_foreach(var) TAILQ_FOREACH(var, &states, list)
static mutex_t list_mutex = MUTEX_INITIALIZER;

static int st_init() {
	TAILQ_INIT(&states);
	return 0;
}

static tel_state_t * st_create() {
	tel_state_t * ns;

	ns = calloc(1, sizeof(tel_state_t));
	mutex_lock(&list_mutex);
	TAILQ_INSERT_TAIL(&states, ns, list);
	mutex_unlock(&list_mutex);

	return ns;
}

static void st_destroy(tel_state_t *st) {
	mutex_lock(&list_mutex);
	TAILQ_REMOVE(&states, st, list);
	mutex_unlock(&list_mutex);
	free(st);
}

static int st_add_fds(fd_set * fds, int maxfd) {
	tel_state_t * st;

	mutex_lock(&list_mutex);
	st_foreach(st) {
		FD_SET(st->socket, fds);
		if (maxfd < (st->socket+1))
			maxfd = st->socket + 1;
	}
	mutex_unlock(&list_mutex);
	return maxfd;
}

/**********************************************************************/

// This thread will grab input from the PTY and stuff it into the socket
static void *client_thread(void *param) {
	int rc, o;
	char buf[256];
	tel_state_t * ts = (tel_state_t *)param;

	ds_printf("telnetd: client thread started, sock %d\n", ts->socket);

	// Block until we have something to write out
	for ( ; ; ) {
		rc = fs_read(ts->ptyfd, buf, 256);
		if (rc <= 0)
			break;

		for (o=0; rc; ) {
			int i = write(ts->socket, buf, rc);
			rc -= i;
			o += i;
		}
	}

	ds_printf("telnetd: client thread exited, sock %d\n", ts->socket);
	return NULL;
}

// Grab any input from the socket and stuff it into the PTY
static int handle_read(tel_state_t * ts) {
	char buffer[256];
	int rc, o, d;

	for (d = 0; !d; ) {
		rc = read(ts->socket, buffer, 256);
		if (rc == 0)
			return -1;
		if (rc < 256)
			d = 1;

		for (o=0; rc; ) {
			int i = fs_write(ts->ptyfd, buffer + o, rc);
			rc -= i;
			o += i;
		}
	}

	return 0;
}

void *telnetd(void *p);

typedef struct telnetd_params {
	int port;
	int state;
} telnetd_params_t;

static telnetd_params_t tdp;


void telnetd_shutdown() {
	
	tdp.state = 0;
	/*
	if(wait) {
		while(tdp.state > -1) thd_sleep(100);
	}*/
}

int telnetd_init(int port) {
	tdp.port = port ? port : 23;
	tdp.state = 1;
	thd_create(1, telnetd, NULL);
	return 0;
}

void *telnetd(void *p) {
	int listenfd;
	struct sockaddr_in saddr;
	fd_set readset;
	fd_set writeset;
	int i, maxfdp1;
	tel_state_t *ts;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {     
		ds_printf("telnetd: socket create failed\n");
		return NULL;                 
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	saddr.sin_port = htons(23);

	if (bind(listenfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		ds_printf("telnetd: bind failed\n");
		close(listenfd);
		return NULL;
	}

	if (listen(listenfd, 10) < 0) {
		ds_printf("telnetd: listen failed\n");
		close(listenfd);
		return NULL;
	}

	st_init();
	ds_printf("telnetd: listening for connections on socket %d\n", listenfd);

	while(tdp.state > 0) {
		maxfdp1 = listenfd + 1;

		FD_ZERO(&readset);
		FD_ZERO(&writeset);
		FD_SET(listenfd, &readset);
		maxfdp1 = st_add_fds(&readset, maxfdp1);

		i = select(maxfdp1, &readset, &writeset, 0, 0);

		if (i == 0)
			continue;

		// Check for new incoming connections
		if (FD_ISSET(listenfd, &readset)) {
			ts = st_create();
			ts->client_size = sizeof(ts->client);
			ts->socket = accept(listenfd, (struct sockaddr *)&ts->client, &ts->client_size);
			
			ds_printf("telnetd: connect from %08lx, port %d, socket %d\n",
				ts->client.sin_addr.s_addr, ts->client.sin_port, ts->socket);
				
			if (ts->socket < 0) {
				st_destroy(ts);
			} else {
				file_t master, slave;

				if (fs_pty_create(NULL, 0, &master, &slave) < 0) {
					ds_printf("telnetd: can't create pty for shell\n");
				} else {

					ts->ptyfd = master;
					//assert( ts->ptyfd >= 0 );

					fs_dup2(slave, 0);
					fs_dup2(slave, 1);
					fs_dup2(slave, 2);

					ts->thd = thd_create(1, client_thread, ts);
				}
			}
		}

		// Process data from connected clients
		st_foreach(ts) {
			if (FD_ISSET(ts->socket, &readset)) {
				if (handle_read(ts) < 0) {
					ds_printf("telnetd: disconnected socket %d\n", ts->socket);
					close(ts->socket);
					st_destroy(ts);
					break;
				}
			}
		}
	}
	
	close(listenfd);
	return NULL;
}

