import numpy as np
from scipy import signal
import matplotlib.pyplot as plt
import sqlite3
# check if fir_filter.db exists, else exit
import os
script_dir = os.path.dirname(os.path.abspath(__file__))
db_path = os.path.join(script_dir, 'fir_filter.db')

if not os.path.exists(db_path):
    print(f"fir_filter.db not found at: {db_path}")
    print("Please make sure the database file is in the same directory as this script.")
    exit(1)
conn = sqlite3.connect(db_path)
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
cursor.execute("SELECT coeff FROM mtx32")
coeffs_from_db_32 = cursor.fetchall()
# make numbers from the possible string values in coeffs_from_db;
coeffs_from_db_4 = [float(c[0]) for c in coeffs_from_db_4]
coeffs_from_db_4 = np.array(coeffs_from_db_4).flatten()
coeffs_from_db_8 = [float(c[0]) for c in coeffs_from_db_8]
coeffs_from_db_8 = np.array(coeffs_from_db_8).flatten()
coeffs_from_db_25 = [float(c[0]) for c in coeffs_from_db_25]
coeffs_from_db_25 = np.array(coeffs_from_db_25).flatten()
coeffs_from_db_32 = [float(c[0]) for c in coeffs_from_db_32]
coeffs_from_db_32 = np.array(coeffs_from_db_32).flatten()
sum_filter_4 = sum(coeffs_from_db_4)
sum_filter_8 = sum(coeffs_from_db_8)
sum_filter_25 = sum(coeffs_from_db_25)
sum_filter_32 = sum(coeffs_from_db_32)
print(f"Sum of FIR coefficients for N=4: {sum_filter_4}")
print(f"Sum of FIR coefficients for N=8: {sum_filter_8}")
print(f"Sum of FIR coefficients for N=25: {sum_filter_25}")
print(f"Sum of FIR coefficients for N=32: {sum_filter_32}")
#
# needed is a 16x filter.
# we have a odd number of coefficients.
# we have four master functions, 4x, 8x, 25x and 32x decimation.
# they may have created with a remez, hamming, sinc or similar algorithm. The guess is that the same algorithm was used for all.
# they have all odd number of coefficients, and are symmetric.
# there is a small trick: the [0] is not symmetric, but the rest. So the creation was even, and [0] was added later.
# the SUM of the coefficients is 1, so they are normalized.
# procedure: find a fitting function for "all". As a guess we expect around 171 coefficients or nearby


def to_common_length(coeffs, target_len):
    src = np.linspace(-1.0, 1.0, len(coeffs))
    dst = np.linspace(-1.0, 1.0, target_len)
    return np.interp(dst, src, coeffs)


def normalize_sum1(coeffs):
    s = np.sum(coeffs)
    if np.isclose(s, 0.0):
        return coeffs
    return coeffs / s


def enforce_symmetry(coeffs):
    return 0.5 * (coeffs + coeffs[::-1])


def odd_int(x):
    n = int(round(x))
    return n if n % 2 == 1 else n + 1


known_dec = np.array([4.0, 8.0, 25.0, 32.0])
known_f = 1.0 / known_dec
known_len = np.array([
    len(coeffs_from_db_4),
    len(coeffs_from_db_8),
    len(coeffs_from_db_25),
    len(coeffs_from_db_32),
], dtype=float)

# Fit tap-count trend in reciprocal decimation space and keep odd length.
tap_poly = np.polyfit(known_f, known_len, 2)
target_dec = 16.0
target_f = 1.0 / target_dec
target_len = odd_int(np.polyval(tap_poly, target_f))
print(f"Estimated tap count for 16x: {target_len}")

db_4_rs = to_common_length(coeffs_from_db_4, target_len)
db_8_rs = to_common_length(coeffs_from_db_8, target_len)
db_25_rs = to_common_length(coeffs_from_db_25, target_len)
db_32_rs = to_common_length(coeffs_from_db_32, target_len)

f4, f8, f25, f32 = 1.0 / 4.0, 1.0 / 8.0, 1.0 / 25.0, 1.0 / 32.0

# Candidate 1: local linear interpolation between neighbors around 16x (8x, 25x).
w8_l = (target_f - f25) / (f8 - f25)
w25_l = (target_f - f8) / (f25 - f8)
cand1 = w8_l * db_8_rs + w25_l * db_25_rs

# Candidate 2: quadratic interpolation from 4x, 8x, 25x.
w4_q = ((target_f - f8) * (target_f - f25)) / ((f4 - f8) * (f4 - f25))
w8_q = ((target_f - f4) * (target_f - f25)) / ((f8 - f4) * (f8 - f25))
w25_q = ((target_f - f4) * (target_f - f8)) / ((f25 - f4) * (f25 - f8))
cand2 = w4_q * db_4_rs + w8_q * db_8_rs + w25_q * db_25_rs

# Candidate 3: quadratic interpolation from 8x, 25x, 32x.
w8_q2 = ((target_f - f25) * (target_f - f32)) / ((f8 - f25) * (f8 - f32))
w25_q2 = ((target_f - f8) * (target_f - f32)) / ((f25 - f8) * (f25 - f32))
w32_q2 = ((target_f - f8) * (target_f - f25)) / ((f32 - f8) * (f32 - f25))
cand3 = w8_q2 * db_8_rs + w25_q2 * db_25_rs + w32_q2 * db_32_rs

# Candidate 4: cubic interpolation from all known filters.
f_nodes = np.array([f4, f8, f25, f32], dtype=float)
coeff_stack = np.vstack([db_4_rs, db_8_rs, db_25_rs, db_32_rs])
cubic_weights = np.ones(4)
for i in range(4):
    for j in range(4):
        if i != j:
            cubic_weights[i] *= (target_f - f_nodes[j]) / (f_nodes[i] - f_nodes[j])
cand4 = np.sum(cubic_weights[:, None] * coeff_stack, axis=0)

candidates = {
    "linear_8_25": cand1,
    "quad_4_8_25": cand2,
    "quad_8_25_32": cand3,
    "cubic_4_8_25_32": cand4,
}


def candidate_score(candidate):
    candidate = normalize_sum1(enforce_symmetry(candidate))
    references = [db_4_rs, db_8_rs, db_25_rs, db_32_rs]
    factors = np.array([4.0, 8.0, 25.0, 32.0])
    # Heavier weight near 16x to emphasize local similarity.
    weights = np.exp(-np.abs(np.log2(factors / 16.0)))
    weights = weights / np.sum(weights)

    corr_terms = []
    rmse_terms = []
    for ref, w in zip(references, weights):
        ref_n = normalize_sum1(enforce_symmetry(ref))
        corr = np.corrcoef(candidate, ref_n)[0, 1]
        rmse = np.sqrt(np.mean((candidate - ref_n) ** 2))
        corr_terms.append(w * corr)
        rmse_terms.append(w * rmse)

    corr_score = float(np.sum(corr_terms))
    rmse_score = float(np.sum(rmse_terms))
    score = corr_score - 5.0 * rmse_score
    return score, corr_score, rmse_score


scores = {}
for name, cand in candidates.items():
    cand_n = normalize_sum1(enforce_symmetry(cand))
    scores[name] = (cand_n,) + candidate_score(cand_n)

print("\n16x candidate similarity scores (higher is better):")
for name, (_, score, corr, rmse) in scores.items():
    print(f"{name:18s} score={score:+.6f} corr={corr:.6f} rmse={rmse:.6e}")

best_name = max(scores, key=lambda k: scores[k][1])
coeffs_16_best = scores[best_name][0]
print(f"\nBest 16x model: {best_name}")
print(f"Sum(best 16x) = {np.sum(coeffs_16_best):.12f}")

x_16 = np.arange(-(target_len // 2), target_len // 2 + 1)
plt.figure(figsize=(12, 7))
if np.any(coeffs_16_best <= 0):
    plt.yscale('symlog')
else:
    plt.yscale('log')

for name, (cand_n, _, _, _) in scores.items():
    dashed_candidates = {'quad_8_25_32', 'cubic_4_8_25_32'}
    line_style = '--' if name in dashed_candidates else '-'
    plt.plot(x_16, cand_n, line_style, linewidth=1.2, alpha=0.85, label=f'16x candidate: {name}')

plt.title('16x Candidates from DB Filters (4x, 8x, 25x, 32x)')
plt.xlabel('N')
plt.ylabel('Coefficient Value (log scale)')
plt.grid()
plt.legend()
plt.show()

plt.figure(figsize=(12, 7))
if np.any(coeffs_16_best <= 0) or np.any(db_25_rs <= 0) or np.any(db_8_rs <= 0):
    plt.yscale('symlog')
else:
    plt.yscale('log')

plt.plot(x_16, coeffs_16_best, 'k-', linewidth=2.0, label=f'Best 16x estimate ({best_name})')
plt.plot(x_16, normalize_sum1(db_8_rs), 'r--', linewidth=1.5, label='DB 8x (resampled)')
plt.plot(x_16, normalize_sum1(db_25_rs), 'm--', linewidth=1.5, label='DB 25x (resampled)')
plt.plot(x_16, normalize_sum1(db_32_rs), 'c--', linewidth=1.5, label='DB 32x (resampled)')
plt.title('Best 16x Estimate vs Nearby Known DB Filters')
plt.xlabel('N')
plt.ylabel('Coefficient Value (log scale)')
plt.grid()
plt.legend()
plt.show()

# focus on quad_4_8_25 linear_8_25
#  
# quad_4_8_25
sum_cand2 = np.sum(cand2)
print(f"Sum of quad_4_8_25 candidate: {sum_cand2:.12f}")
# linear_8_25
sum_cand1 = np.sum(cand1)
print(f"Sum of linear_8_25 candidate: {sum_cand1:.12f}")
# we do not get 1 as integral, so we need to normalize.
cand2 = normalize_sum1(cand2)
cand1 = normalize_sum1(cand1)
print(f"Sum of normalized quad_4_8_25 candidate: {np.sum(cand2):.12f}")
print(f"Sum of normalized linear_8_25 candidate: {np.sum(cand1):.12f}")
# we print now the +/- 80 coefficients around the center, to see the shape.
# we do this for cand1 and cand2 and 25x, 8x and 32x for comparison.
x_16 = np.arange(-(target_len // 2), target_len // 2 + 1)
x_25 = np.arange(-(len(coeffs_from_db_25) // 2), len(coeffs_from_db_25) // 2 + 1)
plt.figure(figsize=(12, 6))
if np.any(cand1 <= 0) or np.any(cand2 <= 0) or np.any(db_25_rs <= 0) or np.any(db_8_rs <= 0) or np.any(db_32_rs <= 0):
    plt.yscale('symlog')
else:    plt.yscale('log')
plt.plot(x_16, cand1, 'go-', label='Linear 8-25x Candidate (cand1)')
plt.plot(x_16, cand2, 'co-', label='Quadratic 4-8-25x Candidate (cand2)')
plt.plot(x_16, normalize_sum1(cand3), 'b--', linewidth=1.5, label='Quadratic 8-25-32x Candidate (cand3)')
plt.plot(x_16, normalize_sum1(db_8_rs), 'r--', label='DB 8x (resampled)')
plt.plot(x_16, normalize_sum1(db_25_rs), 'm--', label='DB 25x (resampled)')
plt.legend()
plt.title('16x Candidates vs Nearby DB Filters')
plt.xlabel('N ')
plt.ylabel('Coefficient Value (log scale)')
plt.grid()
plt.show()
# save linear_8_25 candidate to a text file
output_path = os.path.join(script_dir, 'mtx16x.txt')
with open(output_path, 'w') as f:
    for coeff in cand3:
        f.write(f"{coeff:.12e}\n")
print(f"Estimated 16x coefficients (linear 8-25x) saved to: {output_path}")
# save also the loaded mtx4 original coefficients for reference
output_path_4 = os.path.join(script_dir, 'mtx4x.txt')
with open(output_path_4, 'w') as f:
    for coeff in coeffs_from_db_4:
        f.write(f"{coeff:.12e}\n")
print(f"Original 4x coefficients saved to: {output_path_4}")



