# Running ODAS using a MATRIX Creator/Voice board or 

We assumed you have installed the necessary sw for the relative board (see INSTALL)

## Install ODAS Prerequisites

You will need CMake, GCC and the following external libraries:

```batch
sudo apt-get install g++ git cmake
sudo apt-get install libfftw3-dev
sudo apt-get install libconfig-dev
sudo apt-get install libasound2-dev
sudo apt install libjson-c-dev
```

## Installing ODAS

Clone the Lisa-ODAS project:

```batch
git clone https://github.com/lawrence-iviani/lisa-odas.git
```

Create a folder to build the project and build it:

```batch
cd lisa-odas
mkdir build
cd build
cmake ..
make
```

## Run the demo!

You need to run two applications. The `odaslive` that performs all the cool audio processing and the `lisa_speech_recognition` that receives the result and draws it in the MATRIX Everloop, and perform a basic speech recognition (TODO: going to change, in evolution).

In one terminal, run one of the possivle demo. (TODO: a number of requirements should be satisfied)
```batch
cd odas-switcher/demo/lisa_py_processing_engine 
python3 lisa_speech_recognition.py
```

In a second terminal (order matter!)
Adapt for other board, 
```batch
cd ~/odas/bin
./matrix-odas &
  ./odaslive -vc ../config/matrix-lisa/matrix_voice_LISA_1.cfg
```

Make some noise! ... you should see a blue lights indicating where the sound is coming from.

![](./matrix-odas-running.gif)
