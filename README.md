HYPERION
========

An opensource 'AmbiLight' implementation controlled using the RaspBerry Pi running Raspbmc.
The intention is to replace BobLight. The replacement includes several 'improvements':
* Frame capture of screen included in deamon. This reduces the processor usages for image based led control to less than 1%.
* Priority channel can specificy a timeout on their 'command'. This allows a client or remote control to specify a fixed color and then close the connection. 
* Json IP-control interface. Easy to use interface based on json format for control over TCP/IP.
* Protobuf IP-control interface. Well documented and efficient interface for control over TCP/IP.

The source is released under MIT-License (see http://opensource.org/licenses/MIT).
