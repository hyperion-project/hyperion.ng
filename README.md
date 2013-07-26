HYPERION
========

An opensource 'AmbiLight' implementation controlled using the RaspBerry Pi running Raspbmc. 
The library is supplied with an implementation the client interface of the "Boblight" library (libbob2hyperion.so). The library is intended to boost the performance of the ambilight-led control, which can be an issue on the RaspBerry Pi. The current implementation does not use a client-server architecture and can be configured by overwritting libboblight with a symbolic link to libbob2hyperion.
The shell-script 'bin/install_hyperion.sh' should install the library and create the symbolic link required to use the library.
