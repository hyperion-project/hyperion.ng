# Detect Hyperion
Hyperion announces it's services on the network, via ZeroConf and SSDP.

[[toc]]

## SSDP
**S**imple**S**ervice**D**iscovery**P**rotocol
([SSDP](https://en.wikipedia.org/wiki/Simple_Service_Discovery_Protocol)) is the
discovery subset of UPnP. The implementation is lighter than ZeroConf as it just needs a
UDP Socket without any further dependencies.


### SSDP-Client Library
Here are some example client libraries for different programming languages (many others available):
  * [NodeJS](https://github.com/diversario/node-ssdp#usage---client)
  * [Java](https://github.com/resourcepool/ssdp-client#jarpic-client)

### Usage
With a given SSDP-client library, you can use the following USN / service type:

`urn:hyperion-project.org:device:basic:1`

Some headers from the response will include:
  * **Location**: The URL of the webserver
  * **USN**: The unique id for this Hyperion instance, it will remain the same after system restarts or Hyperion updates
  * **HYPERION-FBS-PORT**: The port of the flatbuffers server
  * **HYPERION-JSS-PORT**: The port of the JsonServer
  * **HYPERION-NAME**: The user defined name for this server

As the data changes (e.g. network adapter IP address change), new updates will be automatically announced.

## Zeroconf
Also known as [Apple Bonjour](https://en.wikipedia.org/wiki/Bonjour_(software)) or [Avahi](https://en.wikipedia.org/wiki/Avahi_(software)). Hyperion is detectable through zeroconf.

**Hyperion publishes the following informations:**
  * **_hyperiond-http._tcp**: Hyperion Webserver (HTTP+Websocket)
  * **_hyperiond-json._tcp**: Hyperion JSON Server (TcpSocket)
  * **_hyperiond-flatbuf._tcp**: Hyperion Flatbuffers server (Google Flatbuffers)

You get the IP address, hostname, port and the Hyperion instance name (before the @ for
the full name). As this works realtime you can always have an up to date list of available
Hyperion servers.

### TXT RECORD
Each published entry contains at least the following data in the txt field:
  * **id**: A static unique id to identify an Hyperion instance.
  * **version**: Hyperion version.


### Test Clients
There are several clients available for testing like the
[avahi-browse](http://manpages.ubuntu.com/manpages/bionic/man1/avahi-browse.1.html) a
commandline tool for Ubuntu/Debian. Example command 
``` bash
sudo apt-get install avahi-browse && avahi-browse -r _hyperiond-http._tcp
```
<ImageWrap src="/images/en/avahi-browse.jpg" alt="Searching for Hyperion Server with Avahi cli" />