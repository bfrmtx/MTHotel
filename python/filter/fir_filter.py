import numpy as np
from scipy import signal
import matplotlib.pyplot as plt
import sqlite3
# check if filter.sql3 exists, else exit
import os
if not os.path.exists('filter.sql3'):
    print("filter.sql3 not found. Please make sure the database file is in the current directory.")
    exit(1)
conn = sqlite3.connect('filter.sql3')
cursor = conn.cursor()
# show all tables in the database
cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
tables = cursor.fetchall()
print(f"Tables in the database: {tables}")
cursor.execute("SELECT coeff FROM mtx4")
coeffs_from_db_4 = cursor.fetchall()
cursor.execute("SELECT coeff FROM mtx8")
coeffs_from_db_8 = cursor.fetchall()
cursor.execute("SELECT coeff FROM mtx25")
coeffs_from_db_25 = cursor.fetchall()



# make numbers from the possible string values in coeffs_from_db;
coeffs_from_db_4 = [float(c[0]) for c in coeffs_from_db_4]
coeffs_from_db_4 = np.array(coeffs_from_db_4).flatten()
coeffs_from_db_8 = [float(c[0]) for c in coeffs_from_db_8]
coeffs_from_db_8 = np.array(coeffs_from_db_8).flatten()
coeffs_from_db_25 = [float(c[0]) for c in coeffs_from_db_25]
coeffs_from_db_25 = np.array(coeffs_from_db_25).flatten()
sum_filter_4 = sum(coeffs_from_db_4)
sum_filter_8 = sum(coeffs_from_db_8)
sum_filter_25 = sum(coeffs_from_db_25)
print(f"Sum of FIR coefficients for N=4: {sum_filter_4}")
print(f"Sum of FIR coefficients for N=8: {sum_filter_8}")
print(f"Sum of FIR coefficients for N=25: {sum_filter_25}")

# we have a odd number of coefficients. We center the x-axis around zero, so we have negative and positive N values
N_4 = len(coeffs_from_db_4)
N_8 = len(coeffs_from_db_8)
x_4 = np.arange(-(N_4 // 2), N_4 // 2 + 1)
x_8 = np.arange(-(N_8 // 2), N_8 // 2 + 1)
# plot as blue and red dots, with grid
plt.figure(figsize=(12, 6))

# Use one y-scale for this figure.
if np.any(coeffs_from_db_4 <= 0) or np.any(coeffs_from_db_8 <= 0):
    plt.yscale('symlog')
else:
    plt.yscale('log')

plt.plot(x_4, coeffs_from_db_4, 'bo-', label='FIR Coefficients (N=4)')
plt.plot(x_8, coeffs_from_db_8, 'ro-', label='FIR Coefficients (N=8)')
plt.legend()

plt.title('FIR Filter Coefficients from Database')
plt.xlabel('N ')
plt.ylabel('Coefficient Value (log scale)')
plt.grid()
plt.show()
## now the big magic. Can we fit a sinc function to the coefficients? We can use scipy's curve_fit for this.
from scipy.optimize import curve_fit
def sinc_func(x, A, B):
    # sinc function centered at zero
    return A * np.sinc(B * x)
# Fit the sinc function to the coefficients for N=4
popt_4, _ = curve_fit(sinc_func, x_4, coeffs_from_db_4, p0=[1, 1])
# Fit the sinc function to the coefficients for N=8
popt_8, _ = curve_fit(sinc_func, x_8, coeffs_from_db_8, p0=[1, 1])
print(f"Fitted parameters for N=4: A={popt_4[0]}, B={popt_4[1]}")
print(f"Fitted parameters for N=8: A={popt_8[0]}, B={popt_8[1]}")
# Plot the original coefficients and the fitted sinc functions
plt.figure(figsize=(12, 6))
if np.any(coeffs_from_db_4 <= 0) or np.any(coeffs_from_db_8 <= 0):
    plt.yscale('symlog')
else:    plt.yscale('log')
plt.plot(x_4, coeffs_from_db_4, 'bo-', label='FIR Coefficients (N=4)')
plt.plot(x_8, coeffs_from_db_8, 'ro-', label='FIR Coefficients (N=8)')
x_fit_4 = np.linspace(x_4[0], x_4[-1], 100)
x_fit_8 = np.linspace(x_8[0], x_8[-1], 100)
plt.plot(x_fit_4, sinc_func(x_fit_4, *popt_4), 'b--', label='Fitted Sinc (N=4)')
plt.plot(x_fit_8, sinc_func(x_fit_8, *popt_8), 'r--', label='Fitted Sinc (N=8)')
plt.legend()
plt.title('FIR Filter Coefficients and Fitted Sinc Functions')
plt.xlabel('N ')
plt.ylabel('Coefficient Value (log scale)')
plt.grid()
plt.show()
## new filter design ############################################################################
num_taps_16 = 259
decimation_factor = 16
cutoff = 1.0 / decimation_factor
coeffs_16 = np.asarray(signal.firwin(num_taps_16, cutoff, window='hamming'), dtype=float)
sum_coeffs_16 = sum(coeffs_16)

# Build a DB-driven 16x filter estimate from mtx4/mtx8/mtx25.
# Hybrid model: stable local interpolation (8x,25x) + small global trend (4x,8x,25x).
def resample_to_length(coeffs, target_len):
    src = np.linspace(-1.0, 1.0, len(coeffs))
    dst = np.linspace(-1.0, 1.0, target_len)
    return np.interp(dst, src, coeffs)

f4 = 1.0 / 4.0
f8 = 1.0 / 8.0
f25 = 1.0 / 25.0
f16 = 1.0 / 16.0

# Quadratic Lagrange weights in reciprocal-decimation domain (f = 1 / decimation_factor).
w4 = ((f16 - f8) * (f16 - f25)) / ((f4 - f8) * (f4 - f25))
w8 = ((f16 - f4) * (f16 - f25)) / ((f8 - f4) * (f8 - f25))
w25 = ((f16 - f4) * (f16 - f8)) / ((f25 - f4) * (f25 - f8))

# Linear local interpolation weights between 8x and 25x only (always positive/stable).
w8_local = (f16 - f25) / (f8 - f25)
w25_local = (f16 - f8) / (f25 - f8)

coeffs_from_db_4_rs = resample_to_length(coeffs_from_db_4, num_taps_16)
coeffs_from_db_8_rs = resample_to_length(coeffs_from_db_8, num_taps_16)
coeffs_from_db_25_rs = resample_to_length(coeffs_from_db_25, num_taps_16)

# Candidate A: global trend from all three filters.
coeffs_16_db_quad = w4 * coeffs_from_db_4_rs + w8 * coeffs_from_db_8_rs + w25 * coeffs_from_db_25_rs

# Candidate B: local stable interpolation near target (8x <-> 25x).
coeffs_16_db_local = w8_local * coeffs_from_db_8_rs + w25_local * coeffs_from_db_25_rs

# Final hybrid: mostly local shape with a modest global correction.
global_trend_mix = 0.20
coeffs_16_db = (1.0 - global_trend_mix) * coeffs_16_db_local + global_trend_mix * coeffs_16_db_quad

# Mild smoothing to reduce sharp center spikes while preserving symmetry.
smooth_kernel = np.hanning(7)
smooth_kernel = smooth_kernel / np.sum(smooth_kernel)
coeffs_16_db = np.convolve(coeffs_16_db, smooth_kernel, mode='same')

coeffs_16_db = 0.5 * (coeffs_16_db + coeffs_16_db[::-1])
coeffs_16_db = coeffs_16_db / np.sum(coeffs_16_db)
sum_coeffs_16_db = np.sum(coeffs_16_db)

print(f"Quadratic weights for 16x from DB: w4={w4:.6f}, w8={w8:.6f}, w25={w25:.6f}")
print(f"Local weights for 16x from DB: w8_local={w8_local:.6f}, w25_local={w25_local:.6f}")
print(f"Hybrid mix: local={1.0 - global_trend_mix:.2f}, global={global_trend_mix:.2f}")
print(f"Sum of FIR coefficients for DB-derived N=16: {sum_coeffs_16_db}")

num_taps_4 = 71
decimation_factor = 4
cutoff = 1.0 / decimation_factor
coeffs_4 = np.asarray(signal.firwin(num_taps_4, cutoff, window='blackman'), dtype=float)
sum_coeffs_4 = sum(coeffs_4)
print(f"Sum of FIR coefficients for N=16: {sum_coeffs_16}")
print(f"Sum of FIR coefficients for N=4: {sum_coeffs_4}")
# plot as above, but with the new coefficients
x_16 = np.arange(-(num_taps_16 // 2), num_taps_16 // 2 + 1)
x_25 = np.arange(-(len(coeffs_from_db_25) // 2), len(coeffs_from_db_25) // 2 + 1)
plt.figure(figsize=(12, 6))
if np.any(coeffs_16 <= 0) or np.any(coeffs_16_db <= 0) or np.any(coeffs_from_db_25 <= 0):
    plt.yscale('symlog')
else:    plt.yscale('log')
plt.plot(x_16, coeffs_16, 'go-', label='FIR Coefficients (N=16, firwin+hamming)')
plt.plot(x_16, coeffs_16_db, 'co-', label='FIR Coefficients (N=16, derived from DB)')
plt.plot(x_25, coeffs_from_db_25, 'mo-', label='FIR Coefficients from DB (mtx25)')
plt.legend()
plt.title('FIR Coefficients: N=16 (firwin vs DB-derived) and DB mtx25')
plt.xlabel('N ')
plt.ylabel('Coefficient Value (log scale)')
plt.grid()
plt.show()
# plot old 4 against new 4
plt.figure(figsize=(12, 6))
if np.any(coeffs_from_db_4 <= 0) or np.any(coeffs_4 <= 0):
    plt.yscale('symlog')
else:    plt.yscale('log')
plt.plot(x_4, coeffs_from_db_4, 'bo-', label='FIR Coefficients from DB (N=4)')
x_4_new = np.arange(-(num_taps_4 // 2), num_taps_4 // 2 + 1)  
plt.plot(x_4_new, coeffs_4, 'go-', label='FIR Coefficients (N=4)')
plt.legend()
plt.title('FIR Filter Coefficients for N=4 (Old vs New)')
plt.xlabel('N ')
plt.ylabel('Coefficient Value (log scale)')
plt.grid()
plt.show()
## a very new filter

