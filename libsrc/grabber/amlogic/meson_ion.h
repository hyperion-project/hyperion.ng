/*
 * include/linux/amlogic/dev_ion.h
 *
 * Copyright (C) 2014 Amlogic, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __LINUX_AMLOGIC_ION_H__
#define __LINUX_AMLOGIC_ION_H__

//#include <linux/types.h>
//#include <ion/ion.h>

/**
* CUSTOM IOCTL - CMD
*/

#define ION_IOC_MESON_PHYS_ADDR             8

struct meson_phys_data {
	int handle;
	unsigned int phys_addr;
	unsigned int size;
};


#if 0

/**
 * meson_ion_client_create() -  allocate a client and returns it
 * @heap_type_mask:	mask of heaps this client can allocate from
 * @name:		used for debugging
 */
struct ion_client *meson_ion_client_create(unsigned int heap_mask,
		const char *name);

/**
 * meson_ion_share_fd_to_phys -
 * associate with a share fd
 * @client:	the client
 * @share_fd: passed from the user space
 * @addr point to the physical address
 * @size point to the size of this ion buffer
 */

int meson_ion_share_fd_to_phys(struct ion_client *client,
		int share_fd, ion_phys_addr_t *addr, size_t *len);
#endif

#endif

