import hyperion
import time
import colorsys
import socket
import errno
import struct 

# Get the parameters
ListenPort = int(hyperion.args.get('ListenPort', 2801))
ListenIP = hyperion.args.get('ListenIP', "")
octets = ListenIP.split('.');

UDPSock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM, socket.IPPROTO_UDP)
UDPSock.setblocking(False)

listen_addr = (ListenIP,ListenPort)
UDPSock.bind(listen_addr)

if ListenIP == "":
	print "udp.py: Listening on *.*.*.*:"+str(ListenPort)
else:
	print "udp.py: Listening on "+ListenIP+":"+str(ListenPort)

if len(octets) == 4 and int(octets[0]) >= 224 and int(octets[0]) < 240:
	print "ListenIP is a multicast address\n"
	# Multicast handling
	try:
		mreq = struct.pack("4sl", socket.inet_aton(ListenIP), socket.INADDR_ANY)
		UDPSock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
	except socket.error:
		print "ERROR enabling multicast\n"

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

