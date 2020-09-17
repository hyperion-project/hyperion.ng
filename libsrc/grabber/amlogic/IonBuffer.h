/*
 *
 * Copyright (C) 2016 OtherCrashOverride@users.noreply.github.com.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
*/
#pragma once

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdexcept>

#include "ion.h"
#include "meson_ion.h"


class IonBuffer
{
	size_t bufferSize = 0;
	ion_user_handle_t handle = 0;
	int exportHandle = 0;
	size_t length = 0;
	unsigned long physicalAddress = 0;


	static int ion_fd; // = -1;


public:

	size_t BufferSize() const
	{
		return bufferSize;
	}

	ion_user_handle_t Handle() const
	{
		return handle;
	}

	int ExportHandle() const
	{
		return exportHandle;
	}

	size_t Length() const
	{
		return length;
	}

	unsigned long PhysicalAddress() const
	{
		return physicalAddress;
	}

	IonBuffer(size_t bufferSize)
		: bufferSize(bufferSize)
	{
		if (bufferSize < 1)
			throw std::runtime_error("bufferSize < 1");


		if (ion_fd < 0)
		{
			ion_fd = open("/dev/ion", O_RDWR);
			if (ion_fd < 0)
			{
				throw std::runtime_error("open ion failed.");
			}
		}


		int io;

		// Allocate a buffer
		ion_allocation_data allocation_data = { 0 };
		allocation_data.len = bufferSize;
		allocation_data.heap_id_mask = ION_HEAP_CARVEOUT_MASK;

#if defined(__aarch64__)
		allocation_data.flags = ION_FLAG_CACHED_NEEDS_SYNC; //ION_FLAG_CACHED;
#else
		allocation_data.flags = 0;
#endif

		io = ioctl(ion_fd, ION_IOC_ALLOC, &allocation_data);
		if (io != 0)
		{
			throw std::runtime_error("ION_IOC_ALLOC failed.");
		}

// 		fprintf(stderr, "ion handle=%d\n", allocation_data.handle);


		// Map/share the buffer
		ion_fd_data ionData = { 0 };
		ionData.handle = allocation_data.handle;

		io = ioctl(ion_fd, ION_IOC_SHARE, &ionData);
		if (io != 0)
		{
			throw std::runtime_error("ION_IOC_SHARE failed.");
		}

// 		fprintf(stderr, "ion map=%d\n", ionData.fd);


		// Get the physical address for the buffer
		meson_phys_data physData = { 0 };
		physData.handle = ionData.fd;

		ion_custom_data ionCustomData = { 0 };
		ionCustomData.cmd = ION_IOC_MESON_PHYS_ADDR;
		ionCustomData.arg = (long unsigned int)&physData;

		io = ioctl(ion_fd, ION_IOC_CUSTOM, &ionCustomData);
		if (io != 0)
		{
			throw std::runtime_error("ION_IOC_CUSTOM failed.");
		}

		// Assignment
		handle = allocation_data.handle;
		exportHandle = ionData.fd;
		length = allocation_data.len;
		physicalAddress = physData.phys_addr;

// 		fprintf(stderr, "ion phys_addr=%lu\n", physicalAddress);
	}

	virtual ~IonBuffer() noexcept(false)
	{
		ion_handle_data ionHandleData = { 0 };
		ionHandleData.handle = handle;

		int io = ioctl(ion_fd, ION_IOC_FREE, &ionHandleData);
		if (io != 0)
		{
			throw std::runtime_error("ION_IOC_FREE failed.");
		}
	}


	void Sync()
	{

#if defined(__aarch64__)
		ion_fd_data ionFdData = { 0 };
		ionFdData.fd = ExportHandle();

		int io = ioctl(ion_fd, ION_IOC_SYNC, &ionFdData);
		if (io != 0)
		{
			throw std::runtime_error("ION_IOC_SYNC failed.");
		}
#endif

	}

	void* Map()
	{
		void* result = mmap(NULL,
			Length(),
			PROT_READ | PROT_WRITE,
			MAP_FILE | MAP_SHARED,
			ExportHandle(),
			0);
		if (result == MAP_FAILED)
		{
			throw std::runtime_error("mmap failed.");
		}

		return result;
	}
};
