ODAS Lisa
=======
A receiver and decoder of ODAS signal which is able to feed a python  (based on kaldi or deep speech) and an intent recognition to trigger specific actions (e.g sending a ROS event).
Future roadmap
- ~~Add configuration for receiver via ini file~~
- ROS publisher for messages SST, SSL and SSS

```bash
sudo apt-get install libfftw3-dev
sudo apt-get install libconfig-dev
sudo apt-get install libasound2-dev

git clone https://github.com/ndevilla/iniparser
cd iniparser
make

```

ODAS 
=======

ODAS stands for Open embeddeD Audition System. This is a library dedicated to perform sound source localization, tracking, separation and post-filtering. ODAS is coded entirely in C, for more portability, and is optimized to run easily on low-cost embedded hardware. ODAS is free and open source.

The [ODAS wiki](https://github.com/introlab/odas/wiki) describes how to build and run the software. 

```bash
# Create a folder to build the project:

cd odas
mkdir build
cd build

# Run CMake (in the build directory) :

cmake ../

# Compile the project:

make

```

# Paper
You can find more information about the methods implemented in ODAS in this paper: 

* F. Grondin and F. Michaud, [Lightweight and Optimized Sound Source Localization and Tracking Methods for Opened and Closed Microphone Array Configurations](https://arxiv.org/pdf/1812.00115), Robotics and Autonomous Systems, 2019 


History
=======
Ver.1 
This repos was intended as a playground, try to extend ODAS with a matrix voice hw and connect with a python environment. Due to the evolution of the thesis, I have decided to use this as a main page where

Starting from a sketch on [hackster](https://www.hackster.io/matrix-labs/direction-of-arrival-for-matrix-voice-creator-using-odas-b7a15b)

The target is have a minimum interaction with LED, prints of:
* ssl: Sound Source Localization (potential), JSON
* sst: Sound Source Tracking (Tracked), JSON
* sss: Sound Source Stream (Separated, postfiltered), bytestream? JSON?

Wished:
* Speech Threshold Detection (in ODAS? In the sw)
* ROS Messages for DOA

Ver.2
Implementing a basic pipeline in a python revceiver. Able to use sphinx and google online
This is now in another branch (unmantained with the test for local speech recognition with DeepSpeech and Kaldi)
See [lisa-odas speech recognition](https://github.com/lawrence-iviani/lisa-odas/tree/speech_recognition)

Ver.3
The speech recognition is removed (available in the branch)
The actual, transformation in a Sensor with DOA and activity identification plus a Robot Automation based on Command Speech.
It is now imported as submodule in the project [rhasspy-lisa-odas-hermes](https://github.com/lawrence-iviani/rhasspy-lisa-odas-hermes/)

License
=======
ODAS: https://github.com/introlab/odas/blob/master/LICENSE (used as external dynamic library)
iniparser: https://github.com/ndevilla/iniparser/blob/master/LICENSE (used as a static library)


