EMBEDDED
=======

Raspbian 
---------------

Go in [Raspberry Pi OS (Former Raspbian) page](https://www.raspberrypi.org/downloads/raspberry-pi-os/)
And download the latest Raspberry Pi OS (32-bit) Lite Minimal image based on Debian Buster

Flash it with a tool like [Balena Etcher](https://www.balena.io/etcher/) or use the (new) imager tool
from [Raspberry](https://www.raspberrypi.org/documentation/installation/installing-images/README.md)

You need to start Pi Headless with **SSH**, place a file named “ssh” (without any extension) onto the boot partition of the SD card:
![From hackernoon.com](https://hackernoon.com/hn-images/0*z9-QmlW-rVcKeWCq.png)
[raspberry-pi-headless-install](https://hackernoon.com/raspberry-pi-headless-install-462ccabd75d0 )

First Run, configure the Raspberry
---------------
**Find the Pi**
Well, there can be a number of possibility. 
I use a tool like [Advanced IP Scanner](https://www.advanced-ip-scanner.com/)

**Configure Pi**
SSH to the IP found in the step before, with your preferred tool.
Default credentials for a raspberry
*User:* pi
*password:* raspbian

**Configure the raspberry**
[sudo raspi-config](https://www.raspberrypi.org/documentation/configuration/raspi-config.md )

* Resized SD
* wlan

Update the embedded OS
---------------

```batch
sudo apt-get update
sudo apt-get upgrade

# install git
sudo apt-get install git

# Building tools
sudo apt-get install cmake

# other dependecies
?????Installed python-dev 
```

AUDIO HAT 
=======

* [MATRIX Voice Standard Version](https://store.matrix.one/products/matrix-voice)
* [ReSpeaker 4-Mic Array for Raspberry Pi](https://respeaker.io/4_mic_array/)


Direction of Arrival for MATRIX Voice/Creator Using ODAS
---------------

**Install Matrix Software**

Note: As first snippet I have started from this hack on [hackster](https://www.hackster.io/matrix-labs/direction-of-arrival-for-matrix-voice-creator-using-odas-b7a15b)


```batch
# Add repo and key
curl https://apt.matrix.one/doc/apt-key.gpg | sudo apt-key add -
echo "deb https://apt.matrix.one/raspbian $(lsb_release -sc) main" | sudo tee /etc/apt/sources.list.d/matrixlabs.list

# Update packages and install
sudo apt-get update
sudo apt-get upgrade

# Installation
sudo apt install matrixio-creator-init
sudo apt install libmatrixio-creator-hal
sudo apt install libmatrixio-creator-hal-dev
sudo reboot

```

* After reboot, install the MATRIX Kernel Modules as follows:

```batch
sudo apt install matrixio-kernel-modules
sudo reboot
```

**TODO: alsa file in /etc/asound.conf should be modified for non blocking acquisition**

ReSpeaker 4-Mic Array for Raspberry Pi
---------------
[getting-started](https://wiki.seeedstudio.com/ReSpeaker_4_Mic_Array_for_Raspberry_Pi/#getting-started)


```batch
sudo apt-get update 
sudo apt-get upgrade 
git clone https://github.com/respeaker/seeed-voicecard.git
cd seeed-voicecard
sudo ./install.sh
sudo reboot
```

**TODO: alsa file in /etc/asound.conf should be modified for non blocking acquisition**

RHASSPY
=======

TODO: here at the moment there are two ideas.

1. Develop a service compatible with [rhasspy voltron](https://rhasspy.github.io/rhasspy-voltron/) a collection of [rhasspy service managed with supervisord](https://github.com/rhasspy/rhasspy-voltron#rhasspy-voltron)
2. Start with the usual [rhasspy guide](https://rhasspy.readthedocs.io/en/latest/) and check the [lisa-ODAS-receiver README](https://github.com/lawrence-iviani/lisa-odas/blob/master/demo/lisa-ODAS-receiver/README.md)

