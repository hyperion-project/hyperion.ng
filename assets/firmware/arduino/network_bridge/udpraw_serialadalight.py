#!/usr/bin/env python
#

# Simple UDP to Serial redirector.
# Author: https://github.com/penfold42

# raw udp packets to raw serial:
# python.exe udp_adalight.py -P 2801 COM4 115200

# raw udp packets to adalight serial protocol:
# python.exe udp_adalight.py -a -P 2801 COM4 115200

# Derived from: https://github.com/pyserial/pyserial/blob/master/examples/tcp_serial_redirect.py
#
# (C) 2002-2016 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause

import sys
import socket
import serial
import serial.threaded

from __future__ import division

class SerialToNet(serial.threaded.Protocol):
    """serial->socket"""

    def __init__(self):
        self.socket = None

    def __call__(self):
        return self

    def data_received(self, data):
        if self.socket is not None:
            self.socket.sendall(data)


if __name__ == '__main__':  # noqa
    import argparse

    parser = argparse.ArgumentParser(
        description='Simple UDP to Serial redirector.',
        epilog="""\
NOTE: no security measures are implemented. Anyone can remotely connect
to this service over the network.
""")

    parser.add_argument(
        'SERIALPORT',
        help="serial port name")

    parser.add_argument(
        'BAUDRATE',
        type=int,
        nargs='?',
        help='set baud rate, default: %(default)s',
        default=115200)

    parser.add_argument(
        '-q', '--quiet',
        action='store_true',
        help='suppress non error messages',
        default=False)

    parser.add_argument(
        '-a', '--ada',
        action='store_true',
        help='prepend adalight header to serial packets',
        default=False)

    parser.add_argument(
        '--develop',
        action='store_true',
        help='Development mode, prints Python internals on errors',
        default=False)

    group = parser.add_argument_group('serial port')

    group.add_argument(
        "--parity",
        choices=['N', 'E', 'O', 'S', 'M'],
        type=lambda c: c.upper(),
        help="set parity, one of {N E O S M}, default: N",
        default='N')

    group.add_argument(
        '--rtscts',
        action='store_true',
        help='enable RTS/CTS flow control (default off)',
        default=False)

    group.add_argument(
        '--xonxoff',
        action='store_true',
        help='enable software flow control (default off)',
        default=False)

    group.add_argument(
        '--rts',
        type=int,
        help='set initial RTS line state (possible values: 0, 1)',
        default=None)

    group.add_argument(
        '--dtr',
        type=int,
        help='set initial DTR line state (possible values: 0, 1)',
        default=None)

    group = parser.add_argument_group('network settings')

    exclusive_group = group.add_mutually_exclusive_group()

    exclusive_group.add_argument(
        '-P', '--localport',
        type=int,
        help='local UDP port',
        default=2801)

    args = parser.parse_args()

    # connect to serial port
    ser = serial.serial_for_url(args.SERIALPORT, do_not_open=True)
    ser.baudrate = args.BAUDRATE
    ser.parity = args.parity
    ser.rtscts = args.rtscts
    ser.xonxoff = args.xonxoff

    if args.rts is not None:
        ser.rts = args.rts

    if args.dtr is not None:
        ser.dtr = args.dtr

    if not args.quiet:
        sys.stderr.write(
            '--- UDP to Serial redirector\n'
            '--- listening on udp port {a.localport}\n'
            '--- sending to {p.name}  {p.baudrate},{p.bytesize}{p.parity}{p.stopbits}\n'
            '--- type Ctrl-C / BREAK to quit\n'.format(p=ser, a=args))

    try:
        ser.open()
    except serial.SerialException as e:
        sys.stderr.write('Could not open serial port {}: {}\n'.format(ser.name, e))
        sys.exit(1)

    ser_to_net = SerialToNet()
    serial_worker = serial.threaded.ReaderThread(ser, ser_to_net)
    serial_worker.start()

    srv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(('0.0.0.0', args.localport))

    try:
        while True:
            try:
                while True:
                    try:
                        data,addr = srv.recvfrom(1024)
                        if not data:
                            break

                        if args.ada:
                            numleds = len(data)/3
                            hi = (numleds-1)/256
                            lo = (numleds-1)&255
                            sum = hi^lo^0x55
                            ser.write ("Ada"+ chr(hi) + chr(lo) + chr(sum))

                        ser.write(data)  # get a bunch of bytes and send them
                    except socket.error as msg:
                        if args.develop:
                            raise
                        sys.stderr.write('ERROR: {}\n'.format(msg))
                        # probably got disconnected
                        break
            except KeyboardInterrupt:
                # intentional_exit
                raise
            except socket.error as msg:
                if args.develop:
                    raise
                sys.stderr.write('ERROR: {}\n'.format(msg))
            finally:
                ser_to_net.socket = None
                sys.stderr.write('Disconnected\n')
    except KeyboardInterrupt:
        # do not handle exceptions
        pass

    sys.stderr.write('\n--- exit ---\n')
    serial_worker.stop()

