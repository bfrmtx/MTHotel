# MT Hotel

The MT hotel contains a a collection of scripts and codes for processing MT data.
All code is license free - you can do what you want.

*Hope to find some guests...*

# ATSS

atss is a **simplified** ats file (the old data format of metronix since 1998)

For today's applications two features are missing:
* a complete channel description **including** the calibration
* possibility to stream the data

*(Actually the old ats can include the calibration but the ADU-06 at that time with the MSDOS operating system was not able to do this. Later the correct decision was to keep the header as it was with 1024 bytes length. With all E-series coils and fluxgates the calibration is today available during recording)*


The ADU-08e, ADU-10e and ADU-07e got a **WebDAV server** implemented.
The web service will be used for data streaming, and is accessed by https://192.168.0.203/data for example.


Streaming data can only be *updated at the end*.
(the old ats format was updated at the beginning *and* the end when appending data).


The streaming format has bee broken down into two trivial files:

* a binary data stream (int_32 ending with *.bin* **or** double 64bit ending with *.atss*) which appends data at the end (hence that a int_32 can not be converted into a 32bit float without possible loss of precision)
* a simple JSON header which is created on start and does not need an update
* the samples are calculated from file size (/4 for int, /8 for double)


For **permanent stations** (observatories) this allows a **rsync** daemon.
The data streaming can be simplified that way, that the rsync *appends* the data at **the end only**.
In other words: you don't copy the data when the recording is finished, you transfer the data continuously *while* recording.



#### A new filename convention ...

can be implemented now ats "**.atss*" - or ats simplified

* Most users will convert the data into their proprietary formats. With a readable header and a good filename this can be done easily.
* Users using Python can now access the data directly
* the simple layout allows re-write and other file operations

Example:

084_V01_C02_R001_THx_BL_2S.ats   ... the old binary data

084_V85_C02_R001_THx_BL_2S.atss  ... the binary data stream

084_V85_C02_R001_THx_BL_2S.json  ... the JSON header

084_V85_C02_R001_THx_BL_2S_meta.json  ... the JSON meta data (e.g. data not used for processing, like system logs)


## ... and the Power Users

Creating this kind of files is easy. That can be up-sampled IMU data from airborne surveys, temperature from the fluxgate (FGS-03e and FGS-04e support temperature reading) ... or seismic data.

That reminds me that I may find a person helping me to create *miniseed* data.

....HDF5 is possible but in this case all of your tools would work out of the box ... havn't seen this desire yet.

A **directory tree** which is exactly the same for a survey (procmt creates one) would solve this problem as well (and much better)

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


