# ATSS

atss is a **stream** ats file (the successor of the old data format of metronix since 1998, "**a**dvanced **t**ime **s**eries").

An .atss file contains a stream of *IEEE 754* double numbers. That is the default format on most computers
and is also the default format of C/C++, Python numpy and Matlab. A double is always 64bit.


The doubles representing units as given in the header, e.g. **mV/km** for electric field and
**mV** for the magnetic.

## JSON

RFC 8259 and ECMA-404  [European Computer Manufacturers Association](https://www.ecma-international.org/) specified [JSON](https://json.org/) format
is an UTF-8 encoded text file, used for describing the contents of the stream.

## Directory Tree

The directory tree [HDF5](../HDF5-Tree/HDF5-Tree.md#hdf5-tree) is the same as in the *MTH5 survey*.



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

- serial number > 0
- System Name (without space and underscore)
- C NN channel number >= 0
- T channel Type
- Sample Rate with Unit (can be a double); Units: Hz and s

### Entries

- datetime: ISO 8601, UTC time the stream started, can contain fractions of seconds like 13:22:01.5
- latitude: ISO 6709, North latitude is positive, decimal fractions
- longitude: ISO 6709, East longitude is positive, decimal fractions
- elevation: in meter, e.g. WGS84 referenced
- angle: North to East of the measured direction; 0 = North, 90 = East
- dip: positive downwards; 0 = horizontal, 90 positive down
- resistance: in Ohm, e.g. the contact resistance of the electrodes or internal resistance of the sensor
- units: text string, e.g. mV/km or mV
- filter: text string, e.g. like ADB-LF,LF-RF-4 -> filter used during recording
- source: text string, e.g. empty or CSAMT
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

It is the common and fatal mistake in programming to have redundant information.
$$samples == file \ size \ [bytes] \big/ 8$$
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
"angle": 0.0,
"dip": 0.0,
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
"latitude": 39.026196666666664,
"longitude": 29.123953333333333,
"elevation": 1088.31,
"angle": 0.0,
"dip": 0.0,
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
      0.19953,
      0.25119,
      0.31623,
      0.39811,
      0.50119,
      0.63095,
      0.7943,
      1.0,
      1.2589,
      1.5849,
      1.9952,
      2.5119,
      3.1623,
      3.981,
      5.0118,
      6.3095,
      7.943,
      10.0,
      12.589,
      15.849,
      19.952,
      25.119,
      31.622,
      39.81,
      50.118,
      63.095,
      79.43,
      100.0,
      125.89,
      158.49,
      199.52,
      251.18,
      316.22,
      398.1,
      501.18,
      630.94,
      794.3,
      1000.0,
      1258.9,
      1584.9,
      1995.2,
      2511.8,
      3162.2,
      3981.0,
      5011.7,
      6309.4,
      7943.0,
      9999.5,
      10000.0
   ],
   "a": [
      20.027,
      25.170446600000002,
      31.751886599999995,
      39.8980188,
      50.2405119,
      63.1068588,
      79.1203814,
      99.68167910000001,
      124.92179050000001,
      155.881375,
      193.79,
      241.38148599999997,
      297.517428,
      360.03384,
      429.38418599999994,
      502.8373230000001,
      575.0156400000001,
      635.796948,
      688.87121,
      734.25092,
      766.36,
      785.780202,
      801.1669499999999,
      810.35048,
      816.9452370000001,
      822.0455119999999,
      825.89826,
      770.714604,
      826.0397399999999,
      827.81946,
      825.9499999999999,
      828.0288859999999,
      830.17062,
      828.8659359999999,
      829.044708,
      826.75719,
      822.7532699999999,
      822.6368520000001,
      821.862444,
      820.03532,
      818.2,
      815.477653,
      813.814452,
      840.637616,
      784.636084,
      749.12518,
      698.46645,
      596.542651,
      528.601532,
      551.911412,
      517.094144,
      517.0699999999999
   ],
   "p": [
      88.566,
      88.187,
      87.711,
      87.177,
      86.459,
      85.487,
      84.447,
      82.992,
      81.233,
      79.055,
      76.345,
      72.954,
      69.03,
      64.201,
      58.69,
      52.571,
      46.212,
      39.399,
      33.074,
      27.567,
      22.507,
      18.198,
      14.529,
      11.635,
      9.0507,
      7.133,
      5.5026,
      3.6917,
      3.2486,
      2.2856,
      1.3903,
      0.8116,
      0.21197,
      -0.5109,
      -1.217,
      -2.3402,
      -2.8624,
      -3.7097,
      -4.8118,
      -6.1635,
      -7.8127,
      -9.8044,
      -12.075,
      -14.807,
      -23.519,
      -27.61,
      -34.118,
      -40.321,
      -36.267,
      -40.999,
      -51.017,
      -51.019
   ]
}
}
```

# ATMM

Files ending with .atmm contain a vector of boolean. In the software this is realized using a char or uint8_t.
As a consequence the size of the vector\<char> contains samples/8 chars (in case it fits) or (samples/8 + 1) chars else.

The implementation returns true in case the sample is excluded from processing.

You use .atmm files for excluding data from processing; this is the case when a cable was chopped.
You also can use it to mask spikes.

