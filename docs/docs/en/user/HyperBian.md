# HyperBian
Is a ready to use image for your Raspberry Pi. Based on the original Raspberry Pi Foundation image "Raspbian lite". Hyperion is already pre installed. So simply
1. Download
2. Burn image on SD 
3. Power on your Pi
4. Visit with your Browser `http://IpOfYourPi:8090` for configuration
 

## Requirements
  * SD card with at least 2GB size
  * Raspberry Pi
  * Linux/Mac/Windows + SD card read/writer

## Installation
  * Download the image here: [HyperBian Download](https://github.com/Hyperion-Project/HyperBian/releases)
  * Extract HyperBian-XXXX.img out of the HyperBian.zip
  * Burn the extracted HyperBian-XXXX.img to your SD card. Below 3 instructions for the specific system
  * On Windows: [INSTALLING OPERATING SYSTEM IMAGES USING WINDOWS](https://www.raspberrypi.org/documentation/installation/installing-images/windows.md)
  * On Mac: [INSTALLING OPERATING SYSTEM IMAGES ON MAC OS](https://www.raspberrypi.org/documentation/installation/installing-images/mac.md)
  * On Linux: [INSTALLING OPERATING SYSTEM IMAGES ON LINUX](https://www.raspberrypi.org/documentation/installation/installing-images/linux.md)
  * In case your Raspberry Pi has WLAN or you want to use a WLAN stick, you could pre-configure the WLAN SSID and password before you plugin the SD in your Pi. See [HyperBian WLAN](#hyperbian-wlan)
  * Optional: Enable SSH [HyperBian SSH](#HyperBian-SSH)

::: tip
As a image is already outdated the moment you create it. We update it once a week with latest sources from the Raspberry Pi Foundation. Or upon a new Hyperion release. 
:::

### HyperBian WLAN
In case you want to use WLAN with your Raspberry Pi, you can include the WLAN SSID and password after you burned the HyperBian-XXXX.img to your SD card for auto configuration on first boot.
Open the SD card with a file explorer. It's called "boot".
 - Create a new  text file
<ImageWrap src="/images/en/user_hyperbian_wpa_suppli1.jpg" alt="Create a new textfile" />
 - Rename it to "wpa_supplicant.conf"
<ImageWrap src="/images/en/user_hyperbian_wpa_suppli2.jpg" alt="Rename to wpa_supplicant.conf"/>
 - Add your WLAN credentials, replace **YOUR_SSID** and **YOUR_PASSWORD** with your values
```
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country=GB

  network={
    ssid="YOUR_SSID"
    psk="YOUR_PASSWORD"
    key_mgmt=WPA-PSK
  }
```
<ImageWrap src="/images/en/user_hyperbian_wpa_suppli3.jpg" alt="Add your WLAN config"/>
 - Save it! You are done, the WLAN will be configured during next boot

### HyperBian SSH
You should never need it for daily usage, but for completion: To enable SSH access, plugin your sd card with HyperBian already installed into your computer, and open the "boot" partition with a file explorer. Create a empty file named `ssh` in the root directory (without file extension). SSH will be enabled now on next boot. \
**SSH LOGIN**
 - User: pi
 - Password: raspberry
<ImageWrap src="/images/en/user_hyperbian_ssh.jpg" alt="Enable SSH"/>
