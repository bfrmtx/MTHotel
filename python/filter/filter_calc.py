import numpy as np
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent

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
print(f'Wrote: {out_path}')
