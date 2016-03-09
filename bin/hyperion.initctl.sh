## Hyperion daemon initctl script

description "hyperion"
author "poljvd & tvdzwan"

start on (runlevel [2345])
stop on (runlevel [!2345])

respawn

pre-start script
#comment out the following 2 lines for x32/64
modprobe spidev 
/usr/bin/gpio2spi
end script

exec /usr/bin/hyperiond /etc/hyperion.config.json