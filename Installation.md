
# Installation

## Raspberry Pi OS (armhf) <br> Debian Stretch (9) and later (armhf/x64) <br> Ubuntu 16.04 and later (64-bit)

```bash
# Add the release GPG key to your system:
wget -qO- https://apt.hyperion-project.org/hyperion.pub.key | sudo apt-key add -

# Add Hyperion to your APT sources:
echo "deb https://apt.hyperion-project.org/ stable main" | sudo tee /etc/apt/sources.list.d/hyperion.list

# Update your local package index and install Hyperion:
sudo apt-get update
sudo apt-get install hyperion
```

## Windows:

For Windows 10 we provide a .exe file. A one click installation package that does the job for you.  
Download the file from the [Release page](https://github.com/hyperion-project/hyperion.ng/releases).
