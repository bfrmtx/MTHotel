from pathlib import Path
import sys
import os
import matplotlib.pyplot as plt
import importlib
import numpy as np
import sqlite3
base_dir = Path(__file__).resolve().parent if "__file__" in globals() else Path.cwd()
if str(base_dir) not in sys.path:
    sys.path.insert(0, str(base_dir))

import atss.atsslib as atsslib_module
atsslib_module = importlib.reload(atsslib_module)

channel = atsslib_module.channel
resolve_file_path = atsslib_module.resolve_file_path
plot_ts = atsslib_module.plot_ts

# 2 times 4x filter
# test Hx
raw =    "/survey/4x4xatss/stations/65k/run_003/014_ADU-11e_C002_THx_4096Hz.atss"
out_4 =  "/survey/4x4xpy/stations/65k/run_005/"
out4_4 = "/survey/4x4xpy/stations/65k/run_006/"
out16 = "/survey/4x4xpy/stations/65k/run_007/"

channel_raw = channel(raw)
print(f"Path      : {channel_raw.path}")
# print(f"Header    : {channel_raw.header}")
print(f"Cal sensor: {channel_raw.calibration['sensor']}")
print(f"Timing    : {channel_raw.timing_info()}")
print(f"Samples   : {channel_raw.get_samples()}")
# load filter coefficients in a np array
filter_path = resolve_file_path(base_dir / 'filter' / 'filter.db')
conn = sqlite3.connect(filter_path)
cursor = conn.cursor() # table is "mtx4", one column only "coeff", col ist text, we read all rows and convert to a numpy array
cursor.execute("SELECT coeff FROM mtx4")
rows = cursor.fetchall()
coeffs_4x = np.array([float(row[0]) for row in rows])

cursor.execute("SELECT coeff FROM mtx16")
rows = cursor.fetchall()
coeffs_16x = np.array([float(row[0]) for row in rows])
conn.close()

#coeffs_4x = np.loadtxt(resolve_file_path(base_dir / 'filter' / 'mtx4x.txt'))
# filter Hx with 4x
channel_4x = channel_raw.decimate(coeffs_4x, 4, out_4)
# filter Hx with 4x again
channel_4x4 = channel_4x.decimate(coeffs_4x, 4, out4_4)
# filter Hx with 16x
channel_16x = channel_raw.decimate(coeffs_16x, 16, out16)
channel_16x.rm_seconds_from_start(1)
# plot the results
plot_ts([channel_4x4, channel_16x])