<a href="https://scan.coverity.com/projects/snake-sanders-sanders-sld">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/15762/badge.svg"/>
</a>

# SDL Skeleton
This a simple SDL2 skeleton that compiles on OSX and Ubuntu. It provides a simple comon base for a programming excerise in C++ to be sent to potential hires. 

## Task
Our customer has a sensor connected to some equipment and it publishes sensor data over [mqtt](http://mqtt.org) on `amee.interaktionsbyran.se`. The data arrives on the topic `recruitment/ciot/sensor1` in a comma separated format epoch timestamp and value.

The equipment needs to be woken up every now and then in order to send valid data. You can wake it up by posting `1` to the topic `recruitment/ciot/wake`. The wake status is published by the equipment on topic `recruitment/ciot/state`.

We need an application that visualize the incoming data in a nice way with a moving window. Whenever the sensor falls asleep, the app should wake it up again.

## Setup
Setting up the project for building should be straight forward if you're familiar with package management on your chosen platform, but just to make sure the instructions are dirt simple.

### OSX
1. Install [homebrew](http://brew.sh/).
2. Install sdl2 by opening the terminal and typing `brew install sdl2`.
3. Install mosquitto by opening the terminal and typing `brew install mosquitto`

### Ubuntu
1. Open a terminal and install the following packages using `sudo apt-get install` (you will be prompted for your password):
	* libsdl2-dev
	* libsdl2-gfx-dev
	* libsdl2-image-dev
	* libsdl2-mixer-dev
	* libsdl2-net-dev
	* libsdl2-ttf-dev
2. You might also need to install `build-essential` if you have not configured your build environment earlier.
3. Install libmosquitto-dev by doing the following:
	1. Open /etc/apt/sources.list with sudo permissions (i.e `sudo nano /etc/apt/sources.list`).
	2. Add `deb http://us.archive.ubuntu.com/ubuntu vivid main universe` to the bottom of the file and save.
	3. Run `sudo apt-get update`.
	4. Run `sudo apt-get install libmosquitto-dev`

## Building
Open a terminal, navigate to the project root and type `make`.

## Running
Simply run the output main file `./main`

## Extending and developing
The make file automatically builds all cpp files in the src directory. To keep things simple simply add your additional source files in this directory. Should you need to extend the make file for whatever reason feel free to do so, but please make sure the changes work on all platforms or do not affect the default target.

##Documentation 
SDL2 documenation can be found on the [SDL Wiki](https://wiki.libsdl.org/) and libmosquitto documenation is available on [mosquitto.org](http://mosquitto.org/api/files/mosquitto-h.html).
