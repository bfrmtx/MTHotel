# ATSS

atss is a **stream** ats file (the successor of the old data format of metronix since 1998, "**a**dvanced **t**ime **s**eries").

An .atss file contains a stream of *IEEE 754* double numbers. That is the default format on most computers
and is also the default format of C/C++, Python NumPy and Matlab. A double is always 64 bit (4 byte).


The doubles representing units as given in the header, e.g. **mV/km** for electric field and
**mV** for the magnetic. The system scales the data to the units given in the header.

## JSON

RFC 8259 and ECMA-404  [European Computer Manufacturers Association](https://www.ecma-international.org/) specified [JSON](https://json.org/) format
is an UTF-8 encoded text file, used for describing the contents of the stream.

## Directory Tree

The directory tree [HDF5](../HDF5-Tree/HDF5-Tree.md#hdf5-tree) is the same / similar as in the *MTH5 survey*. <br>
The purpose of the directory tree is to organize the data in a way that it can be easily found and processed,
as well to make it easy into the [MTH5](https://mth5.readthedocs.io/en/latest/) format.



## Filename

The run number is obtained from the parent directory.

The filename (and run number) are split by underscores "\_".

nnnSystem_SystemName_ChannelNo_TypeOfData_SamplingRate .atss or .json

As a **limitation** you can **NOT** provide names with underscores or spaces like "ADU  _  08e".

- run_001/084_ADU-08e_C02_THx_2s.atss  ... the binary data stream
- run_001/084_ADU-08e_C02_THx_2s.json  ... the JSON header
- run_002/084_ADU-08e_C02_THx_1024Hz.atss  ... the binary data stream
- run_002/084_ADU-08e_C02_THx_1024Hz.json  ... the JSON header

additional there may appear [atmm](#atmm)  mask files:


- run_001/084_ADU-08e_C02_THx_2s.atmm  ... the mask file
- run_002/084_ADU-08e_C02_THx_1024Hz.atmm  ... the mask file

### File Entries

- serial number > 0 of the logger
- System Name (without underscore, without spaces: protect shell scripts from splitting the filename)
- C NN channel number >= 0
- T channel Type
- Sample Rate with Unit (can be a double); Units: Hz and s

### JSON Entries

- datetime: ISO 8601, UTC time the stream started, can contain fractions of seconds like 13:22:01.5
- latitude: ISO 6709, North latitude is positive, decimal fractions
- longitude: ISO 6709, East longitude is positive, decimal fractions
- elevation: in meter, e.g. WGS84 referenced
- azimuth: North to East of the measured direction; 0 = North, 90 = East
- tilt: positive downwards; 0 = horizontal, 90 positive down
- resistance: in Ohm, e.g. the contact resistance of the electrodes or internal resistance of the sensor
- units: text string, e.g. mV/km or mV; the systems scales E to mV/km automatically
- filter: text string, e.g. like ADB-LF,LF-RF-4 -> filter used during recording
- source: text string, e.g. empty or CSAMT

in the sensor_calibration section:

- sensor: name of sensor like EFP-06, MFS-06e and so on
- serial: positive number
- chopper: 1 chopper is on, 0 chopper is off or nothing
- units_frequency: Hz in Hertz, do not use other units
- units_amplitude: mV, should be mV since the timeseries are in mV by default
- units_phase: degrees
- datetime: date of calibration, use "1970-01-01T00:00:00" when unknown
- Operator: person or institute carried out the calibration
- f: \[\]: frequency data
- a: \[\]: amplitude data
- p: \[\]: phase data

<span style="color:red">... **and where are stop date & samples ???**</span>

A stream persists as long as the logger records **OR** a receiver collects. <br>
The later one is undefined, the (network) receiver may stop at any time.<br>
The number of samples can be calculated from the file size and the present stream end with the sample rate.
*AND* the system acts as a streamer; the json header is written **once** at the beginning of the stream. <br>
A synchronization client (syncthing, rsync) *appends* the stream to the file; this process may be terminated either by
the sender **or** the receiver. In case the receiver terminates, you would have an inconsistent header - avoided by not
using a stop time info.[^catref] <br>

$samples == file \ size \ [bytes] \big/ 8$

$$stop date == \frac{samples}{sample \ rate} + datetime == \frac{file \ size \ [bytes]}{8 \cdot sample \ rate} + datetime$$

It is obvious that the system can write (append) data to the stream *without* touching the JSON header: a stream has not stop date, he exists as long recorded. 

When the system splits the recording into (for example) hourly segments you simply concatenate the streams and use the *first* header for the newly assembled stream. *Hence* that the system writes *scaled* data, so mV/km for the electric field, and therefore the even the electric field can be concatenated.

## Header

For the electric field *without* calibration the entries for f, a, p are there but empty.

```json
{
"datetime": "2009-08-20T13:22:01",
"latitude": 39.026196666666664,
"longitude": 29.123953333333333,
"elevation": 1088.31,
"azimuth": 0.0,
"tilt": 0.0,
"resistance": 572.3670043945313,
"units": "mV/km",
"filter": "ADB-LF,LF-RF-4",
"source": "",
"sensor_calibration": {
   "sensor": "EFP-06",
   "serial": 0,
   "chopper": 1,
   "units_frequency": "Hz",
   "units_amplitude": "mV",
   "units_phase": "degrees",
   "datetime": "1970-01-01T00:00:00",
   "Operator": "",
   "f": [],
   "a": [],
   "p": []
}
}
```

For a magnetic file *with* calibration:

```json
{
"datetime": "2009-08-20T13:22:01",
"latitude": 39.02,
"longitude": 29.12,
"elevation": 1088.3,
"azimuth": 0.0,
"tilt": 0.0,
"resistance": 684052.0,
"units": "mV",
"filter": "ADB-LF,LF-RF-4",
"source": "",
"sensor_calibration": {
   "sensor": "MFS-06",
   "serial": 26,
   "chopper": 1,
   "units_frequency": "Hz",
   "units_amplitude": "mV/nT",
   "units_phase": "degrees",
   "datetime": "2006-12-01T11:23:02",
   "Operator": "",
   "f": [
      0.1,
      0.12589,
      0.15849,
      10000.0
   ],
   "a": [
      20.027,
      25.170446600000002,
      31.751886599999995,
      517.07
   ],
   "p": [
      88.566,
      88.187,
      87.711,
      -51.019
   ]
}
}
```
## Dipole Length

The dipole length is not given in the header. (see also [^catref])<br>
The systems calculate the electric field in mV/km. The position of the electrodes is given in a *meta file* (JSON).
This allows you to calculate the "real" mV input to the logger.<br>
**WHY?** 99.5% of all users use a North - East setup. If done so, you simply read the data from the disk. <br>
You can concatenate the data from different runs and systems without further transformation. <br>
Likely that other data formats force an entry: use +/- 1000 m for the dipole length.



# ATMM

Files ending with .atmm contains pairs of start and stop indices of the stream.
The indices are 0-based and the stop index is exclusive (0, 1024 means the first 1024 samples).

You use .atmm files for excluding data from processing; this is the case when a cable was chopped.
You also can use it to mask spikes.

[^catref]: For remote observatory networks you may stream hourly segments. 
On the receiving server you concatenate the hourly segments using the ">" or ">>" operator. <br>
This always works, even if the station has been setup newly with a different dipole length. <br>
Additionally for the magnetic field (assuming the same sensor type) you can concatenate 
the data without any transformation in case the sampling rate is <= 512 Hz. <br>