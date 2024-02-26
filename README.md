# MT Hotel

The MT hotel contains a a collection of scripts and codes for processing MT data.
All code is license free - you can do what you want.

*Hope to find some guests...*

More details shall follow here: https://mth5.geo-metronix.de/

# ATSS

atss is a **simple** binary array (data stream) of double numbers ( 64bit IEEE 754-2008), which is readable by any computer & software such as C/C++, Python and Matlab.

The ending has been chosen in order not to conflict with other file types. It is meant to be **a**dvanced **t**ime **s**eries **s**tream. Strictly speaking: it is not a format at all.

Together it comes with a *JSON* file (RFC 8259, ECMA-404) description, containing the header, needed to understand what the doubles are representing.

## Naming Convention

* 084_V01_C02_R001_THx_BL_2S.ats   ... the *old binary* data with binary header
* run_001/084_ADU-08e_C02_THx_2s.atss  ... the binary data stream
* run_001/084_ADU-08e_C02_THx_2s.json  ... the JSON header
* meta/run_001/084_ADU-08e_C02_THx_BL_2s.json  ... the JSON meta data **in a different tree!** (e.g. data not used for processing, like system logs)

SystemSerial_System_Channel_ChannelType_SamplingRate is a filename (or ending with 64Hz for example) where "s" indicate seconds and Hz indicate Hertz, for readability. <br>
The filename is split by underscores. You can not give a system name like abc_logger.

The run number is stored a directory name like run_NNN. This makes it possible to easily store the data later in the [USGS](https://www.usgs.gov/) -> [MTH5](https://mth5.readthedocs.io) format.

**Surprisingly** the header will not contain the amount of samples or stop time. Since atss is a *stream** format, the amount of samples is simply file_size/8 and the stop time is calculated from the amount for samples and the sampling rate.

The ADU-08e, ADU-10e / ADU-11e got a **WebDAV server** implemented.
The web service will be used for data streaming, and is accessed by https://192.168.0.203/data for example.

When streaming the data the JSON header is untouched. The atss file *only* appends at the end.

For **permanent stations** (observatories) this allows a **rsync** or other daemon. <br>
The data streaming can be simplified that way, that the rsync *appends* the data at **the end only**.
In other words: you don't copy the data when the recording is finished, you transfer the data continuously *while* recording.

### ... and the extended usage

Some channel types are defined like Ex, Ey, Hx, Hy, Hz or currents like Jx, Jy, Jz or Pitch, Roll, Yaw or x, y, z (coordinates) or T for temperature.

You can put the flight data from your IMU into Pitch, Roll, Yaw .atss and tack to the airborne EM data. <br>
The transmitter currents land up in Jx, Jy, Jz .atss in case you have semi airborne data. <br>

Some fluxgates (FGS-03e and FGS-04e support temperature reading) log the the temperature, use T .atss

## Compile

The cpp section require **C++ 20** (std::filesystem and jthreads); Cmake 3.20 and above. Both, clang and gcc shall work: clang is on Linux/MacOS, gcc on Windows / MinGW.

Presently (2022) the C++ 20 standard is new - at the end of the project it will be standard. Especially the jthreads (auto joined threads are of interest).

**BOOST** conatins many functions which will be used (e.g. math).

## OMG - Open Source!

Under Linux as well MacOS with Homebrew you simply add "boost", run cmake ... and that's it.

The libraries and executables are linked dynamically against open source libraries - as requested.

On Windows .... it is more difficult but possible.

Qt - which is used for GUI applications - is doing a great job.

## MinGW64

I have added a MinGW64 compiler to the project. It is the version which is used by Qt and Qt pages point to https://github.com/cristianadam/mingw-builds/releases and uses posix threads. Please follow the Readme in the compiler directory. You do **not** need to install it, if you install Qt! (recommended).

If you just want to use the basic tools located in the cpp directory, this or a similar compiler will do.

## Qt6

On Windows I use Qt6 *with* the built-in mingw and open-ssh and ninja (cmake target). Cmake does not need to be installed, it is part of Qt6 installation.

The Vulkan library should be installed because for somewhat reason the cmake wants it.

Environment

* BOOST_INCLUDEDIR=C:\boost\boost_1_78_0
* BOOST_ROOT=C:\boost\boost_1_78_0
* VK_SDK_PATH=C:\VulkanSDK\1.2.198.1
* VULKAN_INCLUDEDIR=C:\VulkanSDK\1.2.198.1\Include
* VULKAN_SDK=C:\VulkanSDK\1.2.198.1

PATH

* C:\VulkanSDK\1.2.198.1\Bin;
* C:\Windows\System32\OpenSSH\;
* C:\Qt\Tools\mingw900_64\bin;
* C:\Qt\6.2.3\mingw_64\bin;
* C:\Qt\Tools\CMake_64\bin;
* C:\Qt\Tools\Ninja;

Both above are set on Linux automatically and cmake finds them. Similar on MacOS.

# Settings for Build and Install

If you **don't** want to edit script files, locate the **source code** here:

* Linux/Mac `$HOME'/devel/github_mthotel/MTHotel/cpp/'`
* Windows `Z:\github_mthotel\MTHotel\cpp"` 
* or copy the later mentioned scripts and edit the drive letters

In my case I do `mkdir -p $HOME'/devel/github_mthotel`

cd  $HOME'/devel/github_mthotel

and `git clone https://github.com/bfrmtx/MTHotel.git`

The **output build directories** are outside the source code and located by default

* Linux/Mac `$HOME'/build/'$PROJ'_cpp/build'`
* Windows `%HOMEDRIVE%%HOMEPATH%\build\%PROJ%_cpp` 

The **install directories** are 

* Linux/Mac `/usr/local/mthotel`
* Windows `%HOMEDRIVE%%HOMEPATH%\install\%PROJ%` 

on Linux/Mac create this directory with your permissions like

* `sudo mkdir -p /usr/local/mthotel`
* `sudo chown -R $USER:$USER /usr/local/mthotel`

In order to run, set your **PATH** environment

WINDOWS

* `%HOMEDRIVE%%HOMEPATH%\install\MTHotel\bin` , in my case C:\Users\bfr\install\MTHotel\bin
* `%HOMEDRIVE%%HOMEPATH%\install\MTHotel\lib` , in my case C:\Users\bfr\install\MTHotel\lib

# Build on Linux

* you can skip the SQLite - it is part of your distribution
* cd `$HOME'/devel/github_mthotel/MTHotel`
* for the first tools, cd `cpp`
* bash or `zsh clang_build_release_bfr.sh` or `zsh gcc_build_release_bfr.sh`

*example:* `/usr/local/mthotel/bin/chcal -keep_name -toxml MFS06E0005.txt` creates MFS06E0005.xml (and for plotting MFS06E0005_plot.xml)

# Build on Windows

On Windows we need to build *SQLite* first.

* `cd oss\sqlite`
* `win_build_release_prj_bfr.bat`

then build the code

* `cd cpp`
* `win_build_release_prj_bfr.bat`


# MacOS FFTW3

Make your own build; the find_package is buggy;
I have included /usr/local .... for self-made FFTW3
