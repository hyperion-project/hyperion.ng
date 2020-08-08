#ifndef PLATFORM_RPI

#include <grabber/DispmanxFrameGrabberMock.h>

unsigned  __bcm_frame_counter = 0;
const int __screenWidth  = 800;
const int __screenHeight = 600;

void bcm_host_init()
{
}

void bcm_host_deinit()
{
}

int vc_dispmanx_display_open(int)
{
	return 1;
}

void vc_dispmanx_display_close(int)
{
}

int vc_dispmanx_display_get_info(int, DISPMANX_MODEINFO_T *vc_info)
{
	vc_info->width  = __screenWidth;
	vc_info->height = __screenHeight;
	return 0;
}

DISPMANX_RESOURCE_HANDLE_T vc_dispmanx_resource_create(int, int width, int height, uint32_t *)
{
	return new DISPMANX_RESOURCE(width,height);
}

void vc_dispmanx_resource_delete(DISPMANX_RESOURCE_HANDLE_T resource)
{
	delete resource;
}

int  vc_dispmanx_resource_read_data(DISPMANX_RESOURCE_HANDLE_T resource, VC_RECT_T *rectangle, void* capturePtr, unsigned capturePitch)
{
	memcpy(capturePtr, resource->memptr(), resource->height()*resource->width() * sizeof(ColorRgba));
	return 0;
}

void vc_dispmanx_rect_set(VC_RECT_T *rectangle, int left, int top, int width, int height)
{
	rectangle->width  = width;
	rectangle->height = height;
	rectangle->left   = left;
	rectangle->top    = top;
}

int vc_dispmanx_snapshot(int, DISPMANX_RESOURCE_HANDLE_T resource, int vc_flags)
{
	__bcm_frame_counter++;
	if (__bcm_frame_counter > 100)
	{
		__bcm_frame_counter = 0;
	}

	ColorRgba color[4] = {ColorRgba::RED, ColorRgba::BLUE, ColorRgba::GREEN, ColorRgba::WHITE};
	if (__bcm_frame_counter < 25)
	{
		color[0] = ColorRgba::WHITE;
0		color[1] = ColorRgba::RED;
		color[2] = ColorRgba::BLUE;
		color[3] = ColorRgba::GREEN;
	}
	else if(__bcm_frame_counter < 50)
	{
		color[1] = ColorRgba::WHITE;
		color[2] = ColorRgba::RED;
		color[3] = ColorRgba::BLUE;
		color[0] = ColorRgba::GREEN;
	}
	else if(__bcm_frame_counter < 75)
	{
		color[2] = ColorRgba::WHITE;
		color[3] = ColorRgba::RED;
		color[0] = ColorRgba::BLUE;
		color[1] = ColorRgba::GREEN;
	}

	unsigned w = resource->width();
	unsigned h = resource->height();
	for (unsigned y=0; y<h; y++)
	{
		for (unsigned x=0; x<w; x++)
		{
			unsigned id = 0;
			if (x  < w/2 && y  < h/2) id = 1;
			if (x  < w/2 && y >= h/2) id = 2;
			if (x >= w/2 && y  < h/2) id = 3;

			resource->memptr()[y*w + x] = color[id];
		}
	}

	return 0;
}

#endif
 
