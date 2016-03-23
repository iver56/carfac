# What?

This utility takes in a wav file and outputs Neural Activity Pattern (NAP) to a text file

# How to use the command line utility

* For example: `nap.py drums.wav`
* The NAP data is now written to the text file drums.wavcochlear

# Setup (Ubuntu)

This repository includes an executable that is built on Ubuntu 14.04. You may be able to use that. If you need to build your own, keep reading.

## Install gcc 4.9
`sudo apt-get install build-essential`
`sudo apt-get install software-properties-common`
`sudo add-apt-repository ppa:ubuntu-toolchain-r/test`
`sudo apt-get update`
`sudo apt-get install gcc-4.9 g++-4.9 cpp-4.9`

If you have previously installed an older version of gcc, and you try to run `gcc --version` now, you'll see that it still points to the old version. Let's fix that:

`sudo cd /usr/bin`
`sudo rm gcc g++ cpp`
`sudo ln -s gcc-4.9 gcc`
`sudo ln -s g++-4.9 g++`
`sudo ln -s cpp-4.9 cpp`
`echo 'export CC=/usr/bin/gcc' >> ~/.bashrc`
`echo 'export CXX=/usr/bin/g++' >> ~/.bashrc`

## Get Eigen and SCons
`sudo apt-get install libeigen3-dev scons cmake libgtest-dev`
`echo 'export EIGEN_PATH=/usr/include/eigen3' >> ~/.bashrc`

## Get CARFAC
`cd /data`
`git clone https://github.com/iver56/carfac.git`
`echo 'CARFAC_PATH=/data' >> ~/.bashrc`

## Specify number of cores
`echo 'OMP_NUM_THREADS=2' >> ~/.bashrc`
(replace 2 with the number of physical cores if it is not 2)

## Activate all those new os environment variables
`source ~/.bashrc`

## Build
`scons`

## Install python dependencies

Install numpy, scipy and scikits.samplerate

# Setup (Windows)

This repository includes an executable that is built on Windows 10. You may be able to use that. If you need to build your own, keep reading.

* Make sure you have Visual Studio with C++ compiler and all that
* Install MinGW: http://www.mingw.org/
* Install SCons. Installer for Windows can be found here: http://scons.org/pages/download.html
* Get Eigen
    * Download http://bitbucket.org/eigen/eigen/get/3.2.8.zip and unzip it to C:\ (or somewhere else)
    * Add environment variable EIGEN_PATH with value C:\eigen-eigen-07105f7124f9 (or your custom path, if applicable)

## Get CARFAC (this repository)
* `git clone https://github.com/iver56/carfac.git`
* Add environment variable CARFAC_PATH. The value should be the path to the folder that contains the carfac folder.

## Specify number of cores

* Create environment variable OMP_NUM_THREADS with value 2 (or the number of physical cores if it is not 2)

## Install python dependencies

* Install numpy and scipy
* Download scikits.samplerate-0.3.3-cp27-none-win32.whl from http://www.lfd.uci.edu/~gohlke/pythonlibs/#scikits.samplerate
* `pip install scikits.samplerate-0.3.3-cp27-none-win32.whl`
