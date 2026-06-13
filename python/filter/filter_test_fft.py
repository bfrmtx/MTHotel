import numpy as np
from scipy import signal
import matplotlib.pyplot as plt
from pathlib import Path
#
# decimation filter. data: array of doubles, coeffs: array of doubles for convolution, decimation_factor: integer, aka shift factor
#returns the decimated data
def decimate(data, coeffs, decimation_factor):
    if decimation_factor <= 0:
        raise ValueError('decimation_factor must be > 0')
    if len(coeffs) == 0:
        raise ValueError('coeffs must not be empty')

    decimated = []
    idx = 0
    window_size = len(coeffs)

    # Process data until EOF: read one window, convolve, store one output sample,
    # then jump ahead by decimation_factor.
    while idx + window_size <= len(data):
        window = data[idx:idx + window_size]
        y = np.convolve(window, coeffs, mode='valid')[0]
        decimated.append(y)
        idx += decimation_factor

    return np.asarray(decimated)
# Resolve all input files from this script's directory.
SCRIPT_DIR = Path(__file__).resolve().parent

# load the filter coefficients from txt files
coeffs_4 = np.loadtxt(SCRIPT_DIR / 'mtx4x.txt')
coeffs_16 = np.loadtxt(SCRIPT_DIR / 'mtx16x_from_4x4x.txt')
sum_4 = np.sum(coeffs_4)
sum_16 = np.sum(coeffs_16)
print(f'Sum of 4x coefficients: {sum_4:.6f}')
print(f'Sum of 16x coefficients: {sum_16:.6f}')
# test the decimation filter with a sample signal. BINARY DOUBLE! no header, just raw doubles, local script dir
data_file = SCRIPT_DIR / '084_ADU-07e_C002_THx_2048Hz.atss'
data = np.fromfile(data_file, dtype=np.float64)
# decimate the data with the loaded coefficients, and plot the results
decimated_data_4 = decimate(data, coeffs_4, 4)
decimated_data_4_4 = decimate(decimated_data_4, coeffs_4, 4)
decimated_data_16 = decimate(data, coeffs_16, 16)
# now we compare the spectra with 4096 points FFT
def plot_spectrum(data, fs, title):
    freqs, psd = signal.welch(data, fs=fs, nperseg=4096)
    plt.figure(figsize=(12, 6))
    plt.semilogy(freqs, psd)
    plt.title(title)
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Power Spectral Density (log scale)')
    plt.grid()
    plt.show()
def plot_spectrum_two(data1, data2, fs, title, lower, upper):
    freqs1, psd1 = signal.welch(data1, fs=fs, nperseg=4096)
    freqs2, psd2 = signal.welch(data2, fs=fs, nperseg=4096)
    plt.figure(figsize=(12, 6))
    plt.semilogy(freqs1, psd1, label='Data 1')
    plt.semilogy(freqs2, psd2, linestyle='None', marker='.', markersize=3, label='Data 2')
    plt.title(title)
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Power Spectral Density (log scale)')
    plt.xlim(lower, upper)
    plt.grid()
    plt.legend()
    plt.show()
fs_original = 2048.0
# plot_spectrum(data, fs_original, 'Original Signal Spectrum')
# plot_spectrum(decimated_data_4, fs_original / 4, 'Decimated Signal Spectrum (4x)')
# plot_spectrum(decimated_data_4_4, fs_original / 16, 'Decimated Signal Spectrum (4x then 4x)')
# plot_spectrum(decimated_data_16, fs_original / 16, 'Decimated Signal Spectrum (16x)')
plot_spectrum_two(decimated_data_4_4, decimated_data_16, fs_original / 16, 'Comparison of 16x Decimation Methods (4x then 4x vs Direct 16x)', 45, 55)