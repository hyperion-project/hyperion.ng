Fadecandy: Open Pixel Control Protocol
======================================

The Fadecandy Server (`fcserver`) operates as a bridge between LED controllers attached over USB, and visual effects that communicate via a TCP socket.

The primary protocol supported by `fcserver` is [Open Pixel Control](http://openpixelcontrol.org), a super simple way to send RGB values over a socket. We support the standard Open Pixel Control commands, as well as some Fadecandy extensions.

Socket
------

Open Pixel Control uses a TCP socket, by default on port 7890. For the best performance, remember to set TCP_NODELAY socket option.

Command Format
--------------

All OPC commands follow the same general format. All multi-byte values in Open Pixel Control are in network byte order, high byte followed by low byte.

Channel    | Command   | Length (N) | Data
---------- | --------- | ---------- | --------------------------
1 byte     | 1 byte    | 2 bytes    | N bytes of message data

Set Pixel Colors
----------------

Video data arrives in a **Set Pixel Colors** command:

Byte   | **Set Pixel Colors** command
------ | --------------------------------
0      | Channel Number
1      | Command (0x00)
2 - 3  | Data length
4      | Pixel #0, Red
5      | Pixel #0, Green
6      | Pixel #0, Blue
7      | Pixel #1, Red
8      | Pixel #1, Green
9      | Pixel #1, Blue
…      | …

As soon as a complete Set Pixel Colors command is received, a new frame of video will be broadcast simultaneously to all attached Fadecandy devices.

Set Global Color Correction
---------------------------

The color correction data (from the 'color' configuration key) can also be changed at runtime, by sending a new blob of JSON text in a Fadecandy-specific command. Fadecandy's 16-bit System ID for Open Pixel Control's System Exclusive (0xFF) command is **0x0001**.

Byte   | **Set Global Color Correction** command
------ | ------------------------------------------
0      | Channel Number (0x00, reserved)
1      | Command (0xFF, System Exclusive)
2 - 3  | Data length (JSON Length + 4)
4 - 5  | System ID (0x0001, Fadecandy)
6 - 7  | SysEx ID (0x0001, Set Global Color Correction)
8 - …  | JSON Text

Set Firmware Configuration
--------------------------

The firmware supports some runtime configuration options. Any OPC client can send a new firmware configuration packet using this command. If the supplied data is shorter than the firmware's configuration buffer, only the provided bytes will be changed.

Byte   | **Set Firmware Configuration** command
------ | ------------------------------------------
0      | Channel Number (0x00, reserved)
1      | Command (0xFF, System Exclusive)
2 - 3  | Data length (Configuration Length + 4)
4 - 5  | System ID (0x0001, Fadecandy)
6 - 7  | SysEx ID (0x0002, Set Firmware Configuration)
8 - …  | Configuration Data

Current firmwares support the following configuration options:

Byte Offset | Bits   | Description
----------- | ------ | ------------
0           | 7 … 4  | (reserved)
0           | 3      | Manual LED control bit
0           | 2      | 0 = LED shows USB activity, 1 = LED under manual control
0           | 1      | Disable keyframe interpolation
0           | 0      | Disable dithering
1 … 62      | 7 … 0  | (reserved)
