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

## run

Is a container for one or more streams which are isochronous, same system, and same sampling rate. <br>

## station

Is a container for one or more runs which are at the same location. <br>
A station can have multiple runs, but each run is recorded at a different time. <br>
Often runs have different sampling rates.<br>
If you only have one station, you process run by run. <br>

Different stations can have runs that are isochronous or partly overlapping compared to a different station. <br>

## survey

Is a container for one or more stations which are at the same geographical area. <br>
A survey can have multiple stations, but each station is recorded at a different location. <br>

## local

As above == station.
We can later calculate Ex/(Hy * Hyr*) or similar in the spectral domain.

## EMAP

Electrical local, but maps Magnetic field from a nearby station. <br>
So at this place, we have e.g. Ex, Ey only or want to use them only.<br>
We take Hx, Hy, Hz from a nearby station, call them Hxr, Hyr, Hzr. <br>
We later calculate Ex/(Hy * Hyr*) or similar in the spectral domain. <br>

## Remote Reference (RR)
Is a station that is far away from the local station. <br>
We use the RR station to remove noise from the local station. <br>
We take Ex, Ey, Hx, Hy, Hz from the local station. <br>
Additionally, we take Hx, Hy, Hz from the RR station, call them Hxr, Hyr, Hzr. <br>
Instead of using auto-spectra (Hx * Hx*), we use cross-spectra (Hx * Hxr*) to remove noise. <br>
Hx * conjugate(Hx) is a real number, the imaginary part is a numerical artefact. <br>
Hx * conjugate(Hxr) is a complex number, but maybe we make a double of the real part <br>

We later calculate Ex/(Hy * Hyr*) in the spectral domain. <br>

In many cases, the RR station runs much longer than the local station, and is used for many local stations. <br>
So you record a local station for 8 hours, but the RR station runs for 10 days. <br>

## EMAP RR
Electrical local, but maps Magnetic field from a nearby station and additionally from a RR station. <br>

We later calculate Ex/(Hy * Hyr*) or similar in the spectral domain. <br>

## Hanning / Hann Window / Hamming Window
Hanning or Hann Window is a sine like window function, applied in the *time domain* to achieve smooth spectra later. <br>
Because of data loss at the edges of the window, you want to overlap the windows, e.g. 40%. <br>
So simply spoken, we slide back the read pointer in the .atss file by 40% of the window size, and read the next window. <br>


# Data Frequency Domain

## Spectra 

## ASD

Amplitude Spectral Density, the amplitude of the spectra, in units of V/m/Hz or T/Hz. <br>

## Calibration

## MT Equations

Various MT equations multiplying Ex, Ey, Hx, Hy, Hz with each other and with the conjugate of each other. <br>
And mixing with the remote components Hxr, Hyr, Hzr in case. <br>

## Parzening

Parzening or Parzen Window is a sine like window function to smooth the spectra in the *frequency domain*. <br>
You average the spectra over a spectral lines, with an integral of 1.0. <br>
Trivial example: 0.25, 0.5, 0.25, integral = 1.0; 2,6,8 Hz, 6 is the target frequency, bandwidth of data = 2 Hz, bandwidth of window > 2 Hz, but "not really" 6 Hz as used here. <br>
You also can select 5 as target frequency: than you take +/- 3 Hz, and the available spectral lines (which are still 2,6,8) but you get "odd" weight factors. <br>

## Temporary Storage

After Parzening, we have much less data. <br>
At 1024 Hz with a bandwidth of 1 Hz, we have 1024 spectral lines, but after Parzening with 20 target frequencies, we have only 20 spectral lines, so 50 times less data. <br>


