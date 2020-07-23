#pragma once

#include <memory>

#include <xcb/xcb.h>

template<class Request, class ...Args>
	std::unique_ptr<typename Request::ResponseType, decltype(&free)>
		query(xcb_connection_t * connection, Args&& ...args)
{
	auto cookie = Request::RequestFunction(connection,args...);

	xcb_generic_error_t * error = nullptr;
	std::unique_ptr<typename Request::ResponseType, decltype(&free)> xcbResponse(
		Request::ReplyFunction(connection, cookie, &error), free);

	if (error) {
		Logger * LOGGER = Logger::getInstance("XCB");
		Error(LOGGER,
			"Cannot get the image data event_error: response_type:%u error_code:%u "
			"sequence:%u resource_id:%u minor_code:%u major_code:%u.\n",
			error->response_type, error->error_code, error->sequence,
			error->resource_id, error->minor_code, error->major_code);

		free(error);
		return {nullptr, nullptr};
	}

	return xcbResponse;
}
