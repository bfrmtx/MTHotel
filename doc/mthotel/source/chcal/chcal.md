# chcal
```{index} Software; chcal
```

chcal (change calibration) reads different calibration formats and converts them into others.


## Foreword

Over the years, the calibration files for the Metronix sensors have changed slightly.
Sometimes date was written as 12/24/99, sometimes as 24/12/99; sometimes MFS06e, sometimes MFS-06e.
In some cases, the frequency list contained duplicate entries.

Overall chcal tries to catch most of these cases and convert them into the new JSON format.
The JSON format is the default format for the Metronix sensors. <br>
You can easily plot these files, check for consistency and - eventually - convert them into other formats.

The old mtx format contains frequency [Hz], amplitude [V/(nT * Hz)], and phase [degrees].



The most common usage is

`chcal -outdir /home/newcal -old_to_new -tojson *.txt`

This converts old metronix calibration files into the new JSON format. <br>

For a magnetic sensor the .json should contain:

```json
"units_frequency": "Hz",
"units_amplitude": "mV/nT",
"units_phase": "degrees"
```

For a MFS-06xx chopper on you typically have `f, a, p: 0.1, 20.0, 89.0`

`chcal -help` gives you more options you don't need.

An old metronix file `MFS06022.TXT` is split into `MFS-06_0022_chopper_off.json` and `MFS-06_0022_chopper_on.json`. <br>
Hence that the atss / JSON file *only* contains the calibration data of either *chopper on* or *chopper off*. 
