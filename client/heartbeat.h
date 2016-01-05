
/* heartbeat - heartbeat.h
 *
 * Copyright (C) 2016 - 2018 Zengxianbo. All Rights Reserved. 
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

#ifndef HEARBEART_H_
#define HEARTBEAT_H_

struct heartbeat_operations {
	void (*broken_callback)(void); /* he function will be called when dectected the
					 connection has broken */
	ssize_t (*probe)(void); 	/* probe function */

};

struct heartbeat {
	int servfd; 	/* server socket */
	int nsec; 	/* second between each alarm */
	int maxprobes; 	/* the maxmum probe times. */
	int nprobes; 	/* probes since last server response */
	struct heartbeat_operations *ops;
};


int heartbeat_register(int serverfd_arg, int sec_arg,  int maxprobes_arg, struct heartbeat_operations *ops);
void heartbeat_unregister();
void heartbeat_reset();

#endif
