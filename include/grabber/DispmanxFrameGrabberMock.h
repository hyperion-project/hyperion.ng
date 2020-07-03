#pragma once
#ifndef PLATFORM_RPI

#include <QRect>
#include <utils/Image.h>
#include <utils/ColorRgba.h>

typedef int DISPMANX_DISPLAY_HANDLE_T;
typedef Image<ColorRgba> DISPMANX_RESOURCE;
typedef DISPMANX_RESOURCE* DISPMANX_RESOURCE_HANDLE_T;
const int VC_IMAGE_RGBA32 = 1;
const int DISPMANX_SNAPSHOT_FILL = 1;
typedef int DISPMANX_TRANSFORM_T;


struct DISPMANX_MODEINFO_T {
	int width;
	int height;
};

struct VC_RECT_T {
	int left;
	int top;
	int width;
	int height;
};

void bcm_host_init();
void bcm_host_deinit();
int  vc_dispmanx_display_open(int);
void vc_dispmanx_display_close(int);
int  vc_dispmanx_display_get_info(int, DISPMANX_MODEINFO_T *vc_info);
DISPMANX_RESOURCE_HANDLE_T vc_dispmanx_resource_create(int,int width,int height, uint32_t *);
void vc_dispmanx_resource_delete(DISPMANX_RESOURCE_HANDLE_T resource);
int  vc_dispmanx_resource_read_data(DISPMANX_RESOURCE_HANDLE_T vc_resource, VC_RECT_T *rectangle, void* capturePtr, unsigned capturePitch);
void vc_dispmanx_rect_set(VC_RECT_T *rectangle, int left, int top, int width, int height);
int vc_dispmanx_snapshot(int, DISPMANX_RESOURCE_HANDLE_T resource, int vc_flags);

#endif
