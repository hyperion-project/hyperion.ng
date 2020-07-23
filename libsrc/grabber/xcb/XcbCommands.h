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

struct RandRQueryVersion
{
	typedef xcb_randr_query_version_reply_t ResponseType;

	static constexpr auto RequestFunction = xcb_randr_query_version;
	static constexpr auto ReplyFunction = xcb_randr_query_version_reply;
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

