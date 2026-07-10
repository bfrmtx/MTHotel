import numpy as np
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent


def normalize_sum1(coeffs: np.ndarray) -> np.ndarray:
    s = np.sum(coeffs)
    if np.isclose(s, 0.0):
        return coeffs
    return coeffs / s


def resample_to_length(coeffs: np.ndarray, target_len: int) -> np.ndarray:
    src = np.linspace(-1.0, 1.0, len(coeffs))
    dst = np.linspace(-1.0, 1.0, target_len)
    return np.interp(dst, src, coeffs)


def sinc_comparison(target_len: int, decimation: int) -> np.ndarray:
    # Build a symmetric sinc template with the same tap count as target_len.
    pp = (target_len - 1) // 2
    n = np.arange(-pp, pp + 1, dtype=np.float64)
    cutoff = 1.0 / (2.0 * decimation)
    sinc_taps = 2.0 * cutoff * np.sinc(2.0 * cutoff * n)
    return normalize_sum1(sinc_taps)

# Load original 4x FIR coefficients.
coeffs_4 = np.loadtxt(SCRIPT_DIR / 'mtx4x.txt')

# Upsample the second-stage filter by 4: insert 3 zeros between taps.
coeffs_4_upsampled = np.zeros((len(coeffs_4) - 1) * 4 + 1)
coeffs_4_upsampled[::4] = coeffs_4

# Convolve original 4x filter with upsampled second filter.
# This is the exact equivalent single-stage FIR for 4x followed by 4x (overall 16x).
coeffs_16_equivalent = np.convolve(coeffs_4, coeffs_4_upsampled)

# Write generated coefficients to file for reuse.
out_path = SCRIPT_DIR / 'mtx16x_from_4x4x.txt'
np.savetxt(out_path, coeffs_16_equivalent, fmt='%.18e')

print(f'Loaded 4x taps: {len(coeffs_4)}')
print(f'Upsampled second-stage taps: {len(coeffs_4_upsampled)}')
print(f'Generated equivalent 16x taps: {len(coeffs_16_equivalent)}')
print(f'Sum of original 4x coefficients: {np.sum(coeffs_4):.12f}')
print(f'Sum of equivalent 16x coefficients: {np.sum(coeffs_16_equivalent):.12f}')
print(f'Wrote: {out_path}')
print("plot? ENTER == continue, anything else quit")
if input().strip() == '':
    import matplotlib.pyplot as plt

    x_4 = np.arange(-(len(coeffs_4) // 2), len(coeffs_4) // 2 + 1)
    x_16 = np.arange(-(len(coeffs_16_equivalent) // 2), len(coeffs_16_equivalent) // 2 + 1)
    sinc_4 = sinc_comparison(len(coeffs_4), decimation=4)
    sinc_16 = sinc_comparison(len(coeffs_16_equivalent), decimation=16)

    plt.figure(figsize=(12, 6))
    plt.plot(x_4, coeffs_4, 'bo-', label='Original 4x Coefficients')
    plt.plot(x_16, coeffs_16_equivalent, 'ro-', label='Equivalent 16x Coefficients (from 4x-4x)')
    plt.plot(x_4, sinc_4, 'c--', linewidth=1.4, label='sinc comp. 4x (71 taps)')
    plt.plot(x_16, sinc_16, 'm:', linewidth=1.8, label='sinc comp. 16x (351 taps)')
    plt.title('FIR Filter Coefficients: Original 4x vs Equivalent 16x')
    plt.xlabel('N')
    plt.ylabel('Coefficient Value')
    plt.grid()
    plt.legend()
    plt.show()

    # Scaled comparison: resample 4x to 16x length and normalize both sums to 1.
    coeffs_4_rs = resample_to_length(coeffs_4, len(coeffs_16_equivalent))
    coeffs_4_scaled = normalize_sum1(coeffs_4_rs)
    coeffs_16_scaled = normalize_sum1(coeffs_16_equivalent)
    sinc_4_scaled = normalize_sum1(resample_to_length(sinc_4, len(coeffs_16_equivalent)))
    sinc_16_scaled = normalize_sum1(sinc_16)

    plt.figure(figsize=(12, 6))
    if (
        np.any(coeffs_4_scaled <= 0)
        or np.any(coeffs_16_scaled <= 0)
        or np.any(sinc_4_scaled <= 0)
        or np.any(sinc_16_scaled <= 0)
    ):
        plt.yscale('symlog')
    else:
        plt.yscale('log')
    plt.plot(x_16, coeffs_4_scaled, 'g--', label='Original 4x (resampled + sum-normalized)')
    plt.plot(x_16, coeffs_16_scaled, 'k-', label='Equivalent 16x (sum-normalized)')
    plt.plot(x_16, sinc_4_scaled, 'c--', linewidth=1.4, label='sinc comp. 4x (resampled)')
    plt.plot(x_16, sinc_16_scaled, 'm:', linewidth=1.8, label='sinc comp. 16x')
    plt.title('Scaled Comparison: 4x vs Equivalent 16x + sinc comps.')
    plt.xlabel('N')
    plt.ylabel('Coefficient Value (scaled)')
    plt.grid()
    plt.legend()
    plt.show()
