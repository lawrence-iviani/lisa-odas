ODAS Lisa
=======
A receiver and decoder of odas signal which is able to feed a speech recognition (based on kaldi or deep speech) and an intent recognition to trigger specific actions (e.g sending a ROS event).
It is one of the main implementation of my project and it contains some information about the embedded system as well.
The idea is to run this as service (docker? RHASSPY [Voltron](https://github.com/rhasspy/rhasspy-voltron)) inside a RHASSPY environment.

RHASSPY 
=======
Rhasspy (pronounced RAH-SPEE) is an open source, fully offline voice assistant toolkit for many languages that works well with Home Assistant, Hass.io, and Node-RED.

https://rhasspy.readthedocs.io/en/latest/

KALDI 
=======
[Kaldi](https://kaldi-asr.org/)
Kaldi is a toolkit for speech recognition, intended for use by speech recognition researchers and professionals. Find the code repository on [git](http://github.com/kaldi-asr/kaldi)

DEEPSPEECH
=======
[Git DeepSpeech](https://github.com/mozilla/DeepSpeech)

DeepSpeech is an open source Speech-To-Text engine, using a model trained by machine learning techniques based on Baidu's Deep Speech research paper. Project DeepSpeech uses Google's TensorFlow to make the implementation easier.

Documentation for installation, usage, and training models is available on [deepspeech.readthedocs.io](http://deepspeech.readthedocs.io/?badge=latest)

# Paper

https://arxiv.org/abs/1412.5567


ODAS 
=======

ODAS stands for Open embeddeD Audition System. This is a library dedicated to perform sound source localization, tracking, separation and post-filtering. ODAS is coded entirely in C, for more portability, and is optimized to run easily on low-cost embedded hardware. ODAS is free and open source.

The [ODAS wiki](https://github.com/introlab/odas/wiki) describes how to build and run the software. 

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

Ver.3
The actual, transformation in a Sensor with DOA and activity identification plus a Robot Automation based on Command Speech 
