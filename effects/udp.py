import hyperion
import time
import colorsys
import socket
import errno

# Get the parameters
udpPort = int(hyperion.args.get('udpPort', 2812))

UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
UDPSock.setblocking(False)

listen_addr = ("",udpPort)
print "udp.py: bind socket port:",udpPort
UDPSock.bind(listen_addr)

hyperion.setColor(hyperion.ledCount *  bytearray((int(0), int(0), int(0))) )

# Start the write data loop
while not hyperion.abort():
		try:
			data,addr = UDPSock.recvfrom(4500)
#			print data.strip(),len(data),addr
			if (len(data)%3 == 0):
#				print "numleds ",len(data)/3
				ledData = bytearray()
				for i in range(hyperion.ledCount):
					if (i<(len(data)/3)):
							ledData += data[i*3+0]
							ledData += data[i*3+1]
							ledData += data[i*3+2]
					else:
							ledData += 	bytearray((int(0), int(0), int(0)))

				hyperion.setColor(ledData)

			else:
				print "not div 3"
		except IOError as e:
			if e.errno == errno.EWOULDBLOCK:
				pass
			else:
				print "errno:", e.errno

print "udp.py: closing socket"
UDPSock.close()

