# Detect Hyperion
Hyperion pronounces it's services at the network. Currently with ZeroConf and SSDP.

[[toc]]

## SSDP
**S**imple**S**ervice**D**iscovery**P**rotocol ([SSDP](https://en.wikipedia.org/wiki/Simple_Service_Discovery_Protocol)), is the discovery subset of UPnP. The implementation is lighter than ZeroConf as it just needs a UdpSocket without further dependencies.


### SSDP-Client Library
Here are some client libraries for different programming languages. You can also search for "UPnP" client libraries. This list isn't complete, nor tested but will show you how simple it actually is!
  * [NodeJS](https://github.com/diversario/node-ssdp#usage---client)
  * [Java](https://github.com/resourcepool/ssdp-client#jarpic-client)

### Usage
If you found a ssdp-client library, all you need to do is searching for the following USN / service type:

`urn:hyperion-project.org:device:basic:1`

Some headers from the response.
  * Location: The URL of the webserver
  * USN: The unique id for this Hyperion instance, it will remain the same also after system restarts or Hyperion updates
  * HYPERION-FBS-PORT: The port of the flatbuffers server
  * HYPERION-JSS-PORT: The port of the JsonServer
  * HYPERION-NAME: The user defined name for this server
  * More may be added in future with additional data to other Hyperion network ports

You will receive further notifications when the data changes (Network adapter changed the IP Address, port change) or Hyperion shuts down.

## Zeroconf
Also known as [Apple Bonjour](https://en.wikipedia.org/wiki/Bonjour_(software)) or [Avahi](https://en.wikipedia.org/wiki/Avahi_(software)). Hyperion is detectable through zeroconf.

**Hyperion publishes the following informations:**
  * _hyperiond-http._tcp -> Hyperion Webserver (HTTP+Websocket)
  * _hyperiond-json._tcp -> Hyperion JSON Server (TcpSocket)
  * _hyperiond-flatbuf._tcp -> Hyperion Flatbuffers server (Google Flatbuffers)

So you get the IP address, hostname and port of the system. Also the Hyperion instance name is part of it (before the @ for the full name). As this works realtime you have always an up2date list of available Hyperion servers right to your hand. So check your development environment if you have access to it.

### TXT RECORD
Each published entry contains at least the following informations at the txt field
  * id = A static unique id to identify a hyperion instance (good value to sort between new and known instances)
  * version = Hyperion version


### Test Clients
There are several clients available for testing like the [avahi-browse](http://manpages.ubuntu.com/manpages/bionic/man1/avahi-browse.1.html) commandline tool for ubuntu/debian. Example command 
``` bash
sudo apt-get install avahi-browse &&
avahi-browse -r _hyperiond-http._tcp
```
<ImageWrap src="/images/en/avahi-browse.jpg" alt="Searching for Hyperion Server with Avahi cli" />