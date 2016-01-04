/* heartbeat - heartbeat.c
 *
 * Copyright (C) 2016 - 2017 Xianbo Zeng. All Rights Reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
/* Version : 	V1.0
 * Author  : 	Xianbo Zeng
 * Date    : 	2016-1-4 
 * Description : Used for decting whether the TCP socket connection has broken.
 * 
 * Change history: 
 * 1. 2016-1-4 finished V1.0 by Xianbo Zeng.
 *
 * Note : This heartbeat implement depend on alarm(3), as one process can just 
 * have one alarm in linux, so, before use these API, carefully ensure that 
 * there has no other alarm.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include "heartbeat.h"


static struct heartbeat heartbeat;

static struct sigaction old_action; /* save old action */
static void heartbeat_sig_handler(int); /* SIGURG SIGALRM */


/*
 * default_probe() - the default probe function.
 * As the socket will not send the data when the buffer has little
 * data, so we use MSG_OOB flag to set socket as out of band mode,
 * then just one byte length data can be quikly send from socket.
 *
 * NOTE: 
 * 	here not use any lock, add it if necessary.
 */
ssize_t default_probe(void)
{
	return  send(heartbeat.servfd, "1", 1, MSG_OOB);
}


/* 
 * heartbeat_register() - register heartbeat.
 * @serverfd_arg : server socket fd
 * @sec_arg: how long time send one heartbeat, 1 or bigger integer
 * @maxprobes_arg: maxmum probe times, 3 or other non-negative integer
 * @ops: heartbeat operations
 *
 * RETURN:
 *  0 : success. otherwise, failed.
 */
int heartbeat_register(int servfd_arg, int sec_arg,  int maxprobes_arg,
		struct heartbeat_operations *ops)
{
	int ret;
	struct sigaction heartbeat_sg;

	heartbeat.servfd = servfd_arg; 
	if ((heartbeat.nsec = sec_arg) < 1)
		heartbeat.nsec = 1;
	if ( (heartbeat.maxprobes = maxprobes_arg) < 1)
		heartbeat.maxprobes = 3;

	heartbeat.nprobes = 0;
	heartbeat.ops = ops;

	/* set default probe action */
	if (!heartbeat.ops->probe) {
		heartbeat.ops->probe = default_probe;		
	}

	heartbeat_sg.sa_handler = heartbeat_sig_handler;
	sigemptyset(&heartbeat_sg.sa_mask);
	heartbeat_sg.sa_flags = 0;

	sigaction(SIGURG, &heartbeat_sg, &old_action);

	/* Set the  process ID or process group ID that will receive SIGIO
	 * and SIGURG signals for events on file descriptor servfd  to  the  ID
	 * given in arg.
	 */
	ret = fcntl(heartbeat.servfd, F_SETOWN, getpid());
	if (ret < 0) {
		return ret;
	}

	sigaction(SIGALRM, &heartbeat_sg, NULL);
	alarm(heartbeat.nsec);

	return 0;
}

void heartbeat_unregister()
{
	bzero(&heartbeat, sizeof(heartbeat));
	alarm(0);

	/* restore signal hander */
	sigaction(SIGURG, &old_action, NULL);
	sigaction(SIGALRM, &old_action, NULL);
}

/*
 * heartbeat_reset() - let the heartbeat timer and conter reset to 0.
 */
void heartbeat_reset()
{
	heartbeat.nprobes = 0; 
	alarm(heartbeat.nsec);
}

static void heartbeat_sig_handler(int signum)
{
	int  n;
       	ssize_t	ret;
	char c;  

	if (signum == SIGURG) {
		if ((n = recv(heartbeat.servfd, &c, 1, MSG_OOB)) < 0) {
			if (errno != EWOULDBLOCK) {
				perror("recv()");
				return;
			}
		}
		heartbeat.nprobes = 0; 
		alarm(heartbeat.nsec);
	}

	if (signum == SIGALRM) {
		if (++heartbeat.nprobes > heartbeat.maxprobes) {
			heartbeat.ops->broken_callback();
			return;
		}
		ret = heartbeat.ops->probe();
		if (ret < 0) {
			perror("heartbeat.ops->probe()");
			heartbeat.ops->broken_callback();
		}
		alarm(heartbeat.nsec);
	}
}
