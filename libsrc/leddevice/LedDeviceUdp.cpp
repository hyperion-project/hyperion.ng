
// Local-Hyperion includes
#include "LedDeviceUdp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>

struct addrinfo hints, *servinfo, *p;
//char udpbuffer[1024];
int sockfd;
int ledprotocol;
int leds_per_pkt;
int update_number;
int fragment_number;

LedDeviceUdp::LedDeviceUdp(const std::string& output, const unsigned baudrate, const unsigned protocol, const unsigned maxPacket) 
//LedDeviceUdp::LedDeviceUdp(const std::string& output, const unsigned baudrate) :
//	_ofs(output.empty()?"/home/pi/LedDevice.out":output.c_str())
{

    std::string hostname;
    std::string port;
    ledprotocol = protocol;
    leds_per_pkt = ((maxPacket-4)/3);
    if (leds_per_pkt <= 0) {
	    leds_per_pkt = 200;
    }

//printf ("leds_per_pkt is %d\n", leds_per_pkt);
    int got_colon=0;
    for (unsigned int i=0; i<output.length(); i++) {
	if (output[i] == ':') {
		got_colon++;
	} else if (got_colon == 0) {
		hostname+=output[i];
	} else {
		port+=output[i];
	}
    }
//std::cout << "output " << output << " hostname " << hostname << " port " << port <<std::endl;
    assert(got_colon==1);

    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(hostname.c_str() , port.c_str(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        assert(rv==0);
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        assert(p!=NULL);
    }
}

LedDeviceUdp::~LedDeviceUdp()
{
	// empty
}

int LedDeviceUdp::write(const std::vector<ColorRgb> & ledValues)
{

	char udpbuffer[4096];
	int udpPtr=0;

	update_number++;
	update_number &= 0xf;

	if (ledprotocol == 0) {
		int i=0;
		for (const ColorRgb& color : ledValues)
		{
			if (i<4090) {
				udpbuffer[i++] = color.red;
				udpbuffer[i++] = color.green;
				udpbuffer[i++] = color.blue;
			}
	//printf ("c.red %d sz c.red %d\n", color.red, sizeof(color.red));
		}
		sendto(sockfd, udpbuffer, i, 0, p->ai_addr, p->ai_addrlen);
	}
	if (ledprotocol == 1) {
#define MAXLEDperFRAG 450
		int mLedCount = ledValues.size();

		for (int frag=0; frag<4; frag++) {
			udpPtr=0;
			udpbuffer[udpPtr++] = 0;
			udpbuffer[udpPtr++] = 0;
			udpbuffer[udpPtr++] = (frag*MAXLEDperFRAG)/256;	// high byte
			udpbuffer[udpPtr++] = (frag*MAXLEDperFRAG)%256;	// low byte
			int ct=0;
			for (int this_led = frag*300; ((this_led<mLedCount) && (ct++<MAXLEDperFRAG)); this_led++) {
				const ColorRgb& color = ledValues[this_led];
				if (udpPtr<4090) {
					udpbuffer[udpPtr++] = color.red;
					udpbuffer[udpPtr++] = color.green;
					udpbuffer[udpPtr++] = color.blue;
				}
			}
			if (udpPtr > 7)
				sendto(sockfd, udpbuffer, udpPtr, 0, p->ai_addr, p->ai_addrlen);
		}
	}
	if (ledprotocol == 2) {
		udpPtr = 0;
		unsigned int ledCtr = 0;
		fragment_number = 0;
		udpbuffer[udpPtr++] = update_number & 0xf;
		udpbuffer[udpPtr++] = fragment_number++;
		udpbuffer[udpPtr++] = ledCtr/256;	// high byte
		udpbuffer[udpPtr++] = ledCtr%256;	// low byte

		for (const ColorRgb& color : ledValues)
		{
			if (udpPtr<4090) {
				udpbuffer[udpPtr++] = color.red;
				udpbuffer[udpPtr++] = color.green;
				udpbuffer[udpPtr++] = color.blue;
			}
			ledCtr++;
			if ( (ledCtr % leds_per_pkt == 0) || (ledCtr == ledValues.size()) ) {
				sendto(sockfd, udpbuffer, udpPtr, 0, p->ai_addr, p->ai_addrlen);
				memset(udpbuffer, 0, sizeof udpbuffer);
				udpPtr = 0;
				udpbuffer[udpPtr++] = update_number & 0xf;
				udpbuffer[udpPtr++] = fragment_number++;
				udpbuffer[udpPtr++] = ledCtr/256;	// high byte
				udpbuffer[udpPtr++] = ledCtr%256;	// low byte
			}
		}
		
	}

	if (ledprotocol == 3) {
		udpPtr = 0;
		unsigned int ledCtr = 0;
		unsigned int fragments = 1;
		unsigned int datasize = ledValues.size() * 3;
		if (ledValues.size() > leds_per_pkt) {
			fragments = (ledValues.size() / leds_per_pkt) + 1;
		}
		fragment_number = 1;
		udpbuffer[udpPtr++] = 0x9C;
		udpbuffer[udpPtr++] = 0xDA;
		udpbuffer[udpPtr++] = datasize/256;	// high byte
		udpbuffer[udpPtr++] = datasize%256;	// low byte
		udpbuffer[udpPtr++] = fragment_number++;
		udpbuffer[udpPtr++] = fragments;


		for (const ColorRgb& color : ledValues)
		{
			if (udpPtr<4090) {
				udpbuffer[udpPtr++] = color.red;
				udpbuffer[udpPtr++] = color.green;
				udpbuffer[udpPtr++] = color.blue;
			}
			ledCtr++;
			if ( (ledCtr % leds_per_pkt == 0) || (ledCtr == ledValues.size()) ) {
				udpbuffer[udpPtr++] = 0x36;
				sendto(sockfd, udpbuffer, udpPtr, 0, p->ai_addr, p->ai_addrlen);
				memset(udpbuffer, 0, sizeof udpbuffer);
				udpPtr = 0;
				udpbuffer[udpPtr++] = 0x9C;
				udpbuffer[udpPtr++] = 0xDA;
				udpbuffer[udpPtr++] = datasize/256;	// high byte
				udpbuffer[udpPtr++] = datasize%256;	// low byte
				udpbuffer[udpPtr++] = fragment_number++;
				udpbuffer[udpPtr++] = fragments;
			}
		}
	}

	return 0;
}

int LedDeviceUdp::switchOff()
{
//        return write(std::vector<ColorRgb>(mLedCount, ColorRgb{0,0,0}));
	return 0;
}
