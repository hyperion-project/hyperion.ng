#pragma once

#include <memory>

#include <xcb/xcb.h>

void check_error(xcb_generic_error_t * error)
{
	if (error) {
		Logger * LOGGER = Logger::getInstance("XCB");
		Error(LOGGER,
			"XCB request failed, event_error: response_type:%u error_code:%u "
			"sequence:%u resource_id:%u minor_code:%u major_code:%u.\n",
			error->response_type, error->error_code, error->sequence,
			error->resource_id, error->minor_code, error->major_code);

		free(error);
	}
}


// Requests with void response type
template<class Request, class ...Args>
	typename std::enable_if<std::is_same<typename Request::ResponseType, xcb_void_cookie_t>::value, void>::type
		static query(xcb_connection_t * connection, Args&& ...args)
{
	auto cookie = Request::RequestFunction(connection, std::forward<Args>(args)...);

	xcb_generic_error_t * error = Request::ReplyFunction(connection, cookie);

	check_error(error);
}

// Requests with non-void response type
template<class Request, class ...Args>
	typename std::enable_if<!std::is_same<typename Request::ResponseType, xcb_void_cookie_t>::value, std::unique_ptr<typename Request::ResponseType, decltype(&free)>>::type
		static query(xcb_connection_t * connection, Args&& ...args)
{
	auto cookie = Request::RequestFunction(connection, std::forward<Args>(args)...);

	xcb_generic_error_t * error = nullptr;
	std::unique_ptr<typename Request::ResponseType, decltype(&free)> xcbResponse(
		Request::ReplyFunction(connection, cookie, &error), free);

	check_error(error);

	return xcbResponse;
}
