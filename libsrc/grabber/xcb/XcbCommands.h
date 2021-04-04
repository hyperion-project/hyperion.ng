#pragma once

#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

struct GetImage
{
	typedef xcb_get_image_reply_t ResponseType;

	static constexpr auto RequestFunction = xcb_get_image;
	static constexpr auto ReplyFunction = xcb_get_image_reply;
};

struct GetGeometry
{
	typedef xcb_get_geometry_reply_t ResponseType;

	static constexpr auto RequestFunction = xcb_get_geometry;
	static constexpr auto ReplyFunction = xcb_get_geometry_reply;
};

struct GetProperty
{
	typedef xcb_get_property_reply_t  ResponseType;

	static constexpr auto RequestFunction = xcb_get_property;
	static constexpr auto ReplyFunction = xcb_get_property_reply;
};

struct ShmQueryVersion
{
	typedef xcb_shm_query_version_reply_t ResponseType;

	static constexpr auto RequestFunction = xcb_shm_query_version;
	static constexpr auto ReplyFunction = xcb_shm_query_version_reply;
};

struct RenderQueryVersion
{
	typedef xcb_render_query_version_reply_t ResponseType;

	static constexpr auto RequestFunction = xcb_render_query_version;
	static constexpr auto ReplyFunction = xcb_render_query_version_reply;
};

struct ShmGetImage
{
	typedef xcb_shm_get_image_reply_t ResponseType;

	static constexpr auto RequestFunction = xcb_shm_get_image;
	static constexpr auto ReplyFunction = xcb_shm_get_image_reply;
};

struct RenderQueryPictFormats
{
	typedef xcb_render_query_pict_formats_reply_t ResponseType;

	static constexpr auto RequestFunction = xcb_render_query_pict_formats;
	static constexpr auto ReplyFunction = xcb_render_query_pict_formats_reply;
};

struct ShmCreatePixmap
{
	typedef xcb_void_cookie_t ResponseType;

	static constexpr auto RequestFunction = xcb_shm_create_pixmap_checked;
	static constexpr auto ReplyFunction = xcb_request_check;
};

struct ShmAttach
{
	typedef xcb_void_cookie_t ResponseType;

	static constexpr auto RequestFunction = xcb_shm_attach_checked;
	static constexpr auto ReplyFunction = xcb_request_check;
};

struct ShmDetach
{
	typedef xcb_void_cookie_t ResponseType;

	static constexpr auto RequestFunction = xcb_shm_detach_checked;
	static constexpr auto ReplyFunction = xcb_request_check;
};

struct CreatePixmap
{
	typedef xcb_void_cookie_t ResponseType;

	static constexpr auto RequestFunction = xcb_create_pixmap_checked;
	static constexpr auto ReplyFunction = xcb_request_check;
};

struct RenderCreatePicture
{
	typedef xcb_void_cookie_t ResponseType;

	static constexpr auto RequestFunction = xcb_render_create_picture_checked;
	static constexpr auto ReplyFunction = xcb_request_check;
};

struct RenderSetPictureFilter
{
	typedef xcb_void_cookie_t ResponseType;

	static constexpr auto RequestFunction = xcb_render_set_picture_filter_checked;
	static constexpr auto ReplyFunction = xcb_request_check;
};

struct RenderSetPictureTransform
{
	typedef xcb_void_cookie_t ResponseType;

	static constexpr auto RequestFunction = xcb_render_set_picture_transform_checked;
	static constexpr auto ReplyFunction = xcb_request_check;
};

struct RenderComposite
{
	typedef xcb_void_cookie_t ResponseType;

	static constexpr auto RequestFunction = xcb_render_composite_checked;
	static constexpr auto ReplyFunction = xcb_request_check;
};

struct RenderFreePicture
{
	typedef xcb_void_cookie_t ResponseType;

	static constexpr auto RequestFunction = xcb_render_free_picture_checked;
	static constexpr auto ReplyFunction = xcb_request_check;
};

struct FreePixmap
{
	typedef xcb_void_cookie_t ResponseType;

	static constexpr auto RequestFunction = xcb_free_pixmap_checked;
	static constexpr auto ReplyFunction = xcb_request_check;
};

