#pragma once

/// Constants and utility functions related to WebSocket opcodes
/**
 * WebSocket Opcodes are 4 bits. See RFC6455 section 5.2.
 */
namespace OPCODE
{
	enum value
	{
		CONTINUATION = 0x0,
		TEXT = 0x1,
		BINARY = 0x2,
		RSV3 = 0x3,
		RSV4 = 0x4,
		RSV5 = 0x5,
		RSV6 = 0x6,
		RSV7 = 0x7,
		CLOSE = 0x8,
		PING = 0x9,
		PONG = 0xA,
		CONTROL_RSVB = 0xB,
		CONTROL_RSVC = 0xC,
		CONTROL_RSVD = 0xD,
		CONTROL_RSVE = 0xE,
		CONTROL_RSVF = 0xF
	};

	/// Check if an opcode is reserved
	/**
	 * @param v The opcode to test.
	 * @return Whether or not the opcode is reserved.
	 */
	inline bool reserved(value v)
	{
		return (v >= RSV3 && v <= RSV7) || (v >= CONTROL_RSVB && v <= CONTROL_RSVF);
	}

	/// Check if an opcode is invalid
	/**
	 * Invalid opcodes are negative or require greater than 4 bits to store.
	 *
	 * @param v The opcode to test.
	 * @return Whether or not the opcode is invalid.
	 */
	inline bool invalid(value v)
	{
		return (v > 0xF || v < 0);
	}

	/// Check if an opcode is for a control frame
	/**
	 * @param v The opcode to test.
	 * @return Whether or not the opcode is a control opcode.
	*/
	inline bool is_control(value v)
	{
		return v >= 0x8;
	}
}

namespace CLOSECODE
{
	enum value
	{
		NORMAL    = 1000,
		AWAY      = 1001,
		TERM      = 1002,
		INV_TYPE  = 1003,
		INV_DATA  = 1007,
		VIOLATION = 1008,
		BIG_MSG   = 1009,
		UNEXPECTED= 1011
	};
}
