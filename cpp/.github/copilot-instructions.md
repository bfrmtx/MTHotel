# cpp directory

In the cpp directory we only accept C++ source and header files.
Users without Qt libraries installed shall be able to use this part.

Also here takes place all the "number crushing" , computations, and low-level data manipulations.

# qt6 directory

Here we use the Qt6 framework for visualizing and interacting with the data.

We absolutely don't care about performance issues in that way, that we receive std::vector, std::filesystem::path or std::string. Qt will majorly drive classes from the cpp directory, set and get parameters, and handle the user interface. The performance is buried in the cpp directory. Even classes like qt_survey_tree are just interfaces to the underlying high-performance C++ code, and conversions between Qt types and standard C++ types are expected and acceptable.

# Data Time Domain
An example to scan is here:<br>
cpp/doc/new_survey_structure/Northern_Mining
Data consists of two files:<br>
cpp/doc/new_survey_structure/Northern_Mining/stations/Sarıçam/run_001/084_ADU-07e_C000_TEx_128Hz.atss<br>
and<br>
cpp/doc/new_survey_structure/Northern_Mining/stations/Sarıçam/run_001/084_ADU-07e_C000_TEx_128Hz.json<br>
The .atss file is a binary file that contains doubles ONLY. It is a stream.<br>
The .json file is a text file that contains metadata about the .atss file, especially the start time and date.
The filename of the .atss file is a string that contains the following information:<br>

- 084: the serial number of the system<br>
- ADU-07e: the model of the system<br>
- C000: the channel number<br>
- TEx: the type of the stream, like Ex, Ey, Ez, Hx, Hy, Hz and others<br>
- 128Hz: the sampling rate of the stream, this can also be 2s (== 0.5Hz)<br>

## Isochronous streams

When we convert or down filter, we treat streams independently. <br>
ANYTHING ELSE: we must ensure that we pick up isochronous segments of all streams. <br>
For example, Ex of station A starts at 10:00 and station B starts at 10:01, we must ensure that we pick up the segment of Ex of station A that starts at 10:01. In case the sampling rate was 1024Hz, we have skip 60 * 1024 samples of station A. <br>
And vice versa, if station B stops at 11:05 and station A stops at 11:03, we stop the processing at 11:03. <br>
**We always work on the INTERSECTION of the time intervals of all streams of SAME sampling rate.** <br>
start date and time of a stream is in the .json file, stop time is calculated from the ((stream size / 8) / sampling rate) == (samples / sampling rate). <br>

## slot

A slot is a *physical connection hardware*, e.g. 0 to 7 on a data acquisition system. 

## channel

A channel is a *logical data stream* from a specific sensor, e.g., Ex, Ey, Hx, Hy, Hz. We can have Hx on slot 2 and Hx on slot 5 simultaneously.<br>
BUT we definitively have two different sensors connected to different slots. Even if the channel labels are the same, the physical sensors are distinct.  ref. channel.hpp

## run

Is a container for one or more streams which are **isochronous**, same system, and **same sampling rate**, but will have multiple channels. <br>
It is the leaf node in the hierarchy of the data structure and handled differently from higher-level nodes (like station or survey). <br>
When downsampling (filtering), we must ensure that all channels within the run are downsampled consistently to maintain isochrony AND logically we must create a new run for the downsampled data. <br>
Filtering with 4x decimation a) creates a new sampling rate for the downsampled run, and b) a new start time for the downsampled run (filter runtime), e.g. shifted to a new full second. <br>
When using threads for processing, we must first ensure that all data of a run, lets say run_001, is loaded, and b) is written to the **SAME** output location, lets say run_002. so 1. load all data, 2. create a new run for all channels of run_001 (e.g. next free run number), 3) copy and modify the JSON data as needed, 4) start the filter threads for the atss data. <br>
channel->set_dir("/path/to/output/location") should be used to specify the output location for the new run.


## station

Is a container for one or more runs which are at the same location. <br>
A station can have multiple runs, but each run is recorded at a different time. <br>
Often runs have different sampling rates.<br>
If you only have one station, you process run by run. <br>

Different stations can have runs that are isochronous or partly overlapping compared to a different station. <br>
If runs from different stations overlap in time (intersection), we can use EMAP or Remote Reference (RR) techniques to handle the overlapping data.

## survey

Is a container for one or more stations which are at the same geographical area. <br>
A survey can have multiple stations, but each station is recorded at a different location. <br>
ref. survey_tree.hpp for the structure of surveys and their contained stations.

## experiment

Is a container for one or more surveys.  <br>
This is NOT part of this software. We start from survey as top node. <br>
In a later stage we may use experiment as top node for **MTH5** (aka HDF5) containers *only*.

## Channel labelling

As the time of recording you only have Ex, Ey, Hx, Hy, Hz.<br>
When processing, you may add a suffix _e for EMAP channel or _r for Remote Reference channel. <br>

## local

As above == station. Local is also called "single station", so only data from here is used. <br>
E.g. Ex, Ey, Hx, Hy, Hz are recorded at this station. <br>
Position is from the .json file, E channel.<br>
We calculate Ex * Ex<sup>\*</sup> / (Hy * Hy<sup>\*</sup>) or similar in the spectral domain. <br>

## EMAP

Electrical local, but maps Magnetic field from a nearby station. <br>
So at this place, we have or take e.g. Ex<sub>e</sub>, Ey<sub>e</sub> only or want to use them only.<br>
We take Hx, Hy, Hz from a nearby station, call them Hx, Hy, Hz. <br>
We later calculate Ex<sub>e</sub> * Ex<sub>e</sub><sup>\*</sup> / (Hy * Hy<sup>\*</sup>) or similar in the spectral domain. <br>
We take the position of the EMAP station from the .json file, E<sub>e</sub> channel. <br>

## Remote Reference (RR)
Is a station that is far away from the local station. <br>
We use the RR station to remove noise from the local station. <br>
We take Ex, Ey, Hx, Hy, Hz from the local station. <br>
Additionally, we take Hx, Hy, Hz from the RR station, call them Hx<sub>r</sub>, Hy<sub>r</sub>, Hz<sub>r</sub>. <br>
Instead of using auto-spectra (Hx * Hx<sup>\*</sup>), we use cross-spectra (Hx * Hx<sub>r</sub><sup>\*</sup>) to remove noise. <br>
Hx * conjugate(Hx) is a real number, the imaginary part is a numerical artefact. <br>
Hx * conjugate(Hx<sub>r</sub>) is a complex number, but maybe we make a double of the real part <br>

We later calculate Ex * Ex<sup>\*</sup> / (Hy * Hy<sub>r</sub><sup>\*</sup>) in the spectral domain. <br>
We take the position of the local station from the .json file, E channel. <br>

In many cases, the RR station runs much longer than the local station, and is used for many local stations. <br>
So you record a local station for 8 hours, but the RR station runs for 10 days. <br>

## EMAP RR
Electrical local, but maps Magnetic field from a nearby station and additionally from a RR station. <br>

We later calculate Ex<sub>e</sub> * Ex<sub>e</sub><sup>\*</sup> / (Hy * Hy<sub>r</sub><sup>\*</sup>) or similar in the spectral domain. <br>

We take the position of the EMAP station from the .json file, E<sub>e</sub> channel. <br>

## Parallel Sensor Test (PST)

Parallel Sensor Test is used to compare sensor data from two or more stations running simultaneously. <br>
We take the same channels from each station and analyze them in parallel to check for consistency and noise correlation. <br>
When we indicate a parallel test: two or more stations have the exact same amount of runs, exact same same start times for the runs and exact same duration for each run (file size). <br>
The station name is typically MFS-06e-123_MFS-06e-992_MFS-12e-456, so the (mostly) the three sensors types and their serial numbers. <br>
This let us easily find the sensors. We ignore azimuth and tilt angles from the JSON file. <br>
In our evaluation (GUI: tabs) we compare:
- The stacked amplitude density spectra for each sensor (for multiple runs, e.g. different sampling rates, large spectral coverage joined together), log x frequency, y log magnitude).
- The coherence between the sensors (so a vs b, a vs c, b vs c and so on, so a *combination* of all possible pairs, where a vs b is the same as b vs a and the later one is ignored). e.g. x log frequency, y linear coherence (mostly rescaled to 0.8 and 1.0).
- The noise correlation between the sensors.
- And the calibration functions for each sensor, two plots: one for amplitude and one for phase. <br>



## Hanning / Hann Window / Hamming Window
Hanning or Hann Window is a sine like window function, applied in the *time domain* to achieve smooth spectra later. <br>
Because of data loss at the edges of the window, you want to overlap the windows, e.g. 40%. <br>
So simply spoken, we slide back the read pointer in the .atss file by 40% of the window size, and read the next window. <br>

# MT (Magnetotellurics)

Except for single station analysis (where we can group by sampling rate, so two runs at different time can be stacked together), for multi station analysis (aka EMAP, RR, EMAP RR):

- group by station.
- group the data by sampling rate.
- group by intersection of time intervals (start & stop time, stop time calculated from the file size).

We only process multi station data where the intersection of sampling rate and time intervals is non-empty. <br>
So station A run_005 and station B run_007 must have overlapping time intervals and the same sampling rate to be processed. <br>
So if we calculate cross spectra of station A Ex and station B Hy with a window length of 1024 samples, these 1024 samples are taken at the same time. <br>

## Spectra (MT)

- slide to the intersection of time intervals (if multiple stations are involved).
- fft (Fast Fourier Transform, FFTW library) to convert the time-domain data into the frequency domain.
- apply calibration function of the sensor
- apply calibration function of the ADC (analog-to-digital converter).
- apply resistance calibration (if applicable, for E fields).

Set a number of spc_buffers like 64. <br>
For each channel we start now processing perfectly in thread. <br>
After spc_buffers are filled with the calibrated spectra, we must pause, because memory is limited. <br>
we have now n spc_buffers filled with the calibrated spectra or more precisely ASD, Amplitude Spectral Density, the amplitude of the spectra, in units of V/m/Hz or T/Hz. <br>

## MT Equations

Various MT equations multiplying Ex, Ey, Hx, Hy, Hz with each other and with the conjugate of each other. <br>
And mixing with the remote components Hxr, Hyr, Hzr in case. <br>
We have now a matrix of auto and cross spectra. <br>
This matrix will be used for further processing, such as Parzening and temporary storage. <br>

## Parzening

Parzening or Parzen Window is a sine like window function to smooth the spectra in the *frequency domain*. <br>
You average the spectra over a spectral lines, with an integral of 1.0. <br>
Trivial example: 0.25, 0.5, 0.25, integral = 1.0; 2,6,8 Hz, 6 is the target frequency, bandwidth of data = 2 Hz, bandwidth of window > 2 Hz, but "not really" 6 Hz as used here. <br>
You also can select 5 as target frequency: than you take +/- 3 Hz, and the available spectral lines (which are still 2,6,8) but you get "odd" weight factors. <br>

## Temporary Storage

After Parzening, we have much less data. <br>
At 1024 Hz with a bandwidth of 1 Hz, we have 1024 spectral lines, but after Parzening with 20 target frequencies, we have only 20 spectral lines, so 50 times less data. <br>
This reduction in data size allows for more efficient storage and faster subsequent processing. <br>

## Repeating the Process

The spc_buffers can now be reused for the next batch of data. <br>
This process of filling spc_buffers, performing FFT, calibration, Parzening, and temporary storage is repeated until all data is processed. <br>




