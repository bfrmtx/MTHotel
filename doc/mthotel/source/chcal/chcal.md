# chcal
```{index} Software; chcal
```

chcal (change calibration) reads different calibration formats and converts them into others.

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

An old metronix file `MFS06022.TXT` is split into `MFS-06_0022_chopper_off.json` and `MFS-06_0022_chopper_on.json`.
