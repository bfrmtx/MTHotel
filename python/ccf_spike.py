# cross correlation and spike detections
#
from atss import atss_extended
from atss import atss_file as atss
import numpy as np
from scipy.signal import detrend

def ts_diff(data_1, data_2, slice_size = 4096, threshold=8.0):
    """Calculate the difference between two time series point by point, processing in slices"""
    diff_indices = []
    if len(data_1) != len(data_2):
        raise ValueError("Data arrays must have the same length")
    
    data_length = len(data_1)
    
    # Loop over slices of slice_size
    for slice_start in range(0, data_length, slice_size):
        # Calculate the end of current slice (handle smaller final slice)
        slice_end = min(slice_start + slice_size, data_length)
        
        # Extract slices
        slice_1 = data_1[slice_start:slice_end]
        slice_2 = data_2[slice_start:slice_end]
        
        # Detrend each slice
        data_a = detrend(slice_1, type='linear')
        data_b = detrend(slice_2, type='linear')
        
        # Find differences in this slice
        for i in range(len(data_a)):
            if abs(data_a[i] - data_b[i]) > threshold:
                # Convert relative index to absolute index in original data
                absolute_idx = slice_start + i
                diff_indices.append(np.int64(absolute_idx))
                # print(f"Diff at index {absolute_idx}: {data_a[i]} - {data_b[i]} = {data_a[i] - data_b[i]}")
    
    return np.array(diff_indices, dtype=np.int64)

def median_amplitude_spikes(data, threshold, slice_size=4096):
    """Calculate the median amplitude (absolute value) of the data, processing in slices"""
    data_length = len(data)
    all_outliers = []
    
    # Loop over slices of slice_size
    for slice_start in range(0, data_length, slice_size):
        # Calculate the end of current slice (handle smaller final slice)
        slice_end = min(slice_start + slice_size, data_length)
        
        # Extract slice
        data_slice = data[slice_start:slice_end]
        
        # Detrend this slice
        detrended_data = detrend(data_slice, type='linear')
        median_amplitude = np.median(np.abs(detrended_data))
        
        # Find outliers in this slice
        for i, value in enumerate(detrended_data):
            if np.abs(value) > median_amplitude * threshold:
                # Convert relative index to absolute index in original data
                absolute_idx = slice_start + i
                all_outliers.append(np.int64(absolute_idx))
    
    radius = 4
    # a valid outlier shall consist of at least 4 points (radius), so 248, 250, 251, 252 is an outlier. 12589,12692, 12696, 12697 not
    # so the function look for 4 trailing or 4 preceding indices
    
    outliers_final = []
    if len(all_outliers) < radius:
        return np.array(outliers_final, dtype=np.int64)
    
    # Sort outliers since they might be out of order from slice processing
    all_outliers.sort()
    
    # Group outliers that are close to each other (within radius distance)
    outlier_groups = []
    current_group = [all_outliers[0]]
    
    for i in range(1, len(all_outliers)):
        # If the current outlier is within radius of the last one in the current group
        if all_outliers[i] - current_group[-1] <= radius:
            current_group.append(all_outliers[i])
        else:
            # Start a new group
            if len(current_group) >= radius:
                outlier_groups.append(current_group)
            current_group = [all_outliers[i]]
    
    # Don't forget the last group
    if len(current_group) >= radius:
        outlier_groups.append(current_group)
    
    # Flatten all valid groups into final outliers list
    for group in outlier_groups:
        outliers_final.extend(group)
    
    return np.array(outliers_final, dtype=np.int64)



# 35:556 , jump in data 60 within 6 samples
atss_files = [] # list of ats files to process
atss_files.append('/home/bfr/tmp/spike/meas_2025-11-07_10-58-00/002_ADU-11e_C003_THy_131072Hz.json')
atss_files.append('/home/bfr/tmp/spike/meas_2025-11-07_10-58-00/002_ADU-11e_C004_THz_131072Hz.json')
# read headers
channels = []   
samples = 0
for atss_file in atss_files:
    channels.append(atss.read_header(atss_file))
    #show the samples:
    print(f"File: {atss_file}, Samples: {atss.samples(atss_file)}")
    samples = atss.samples(atss_file)

# read data into numpy arrays
data_ts = []
#start = 3453470 # my test start point
#start = 3343650 # my test start point
start = 3343210 # my test start point
#window_size = 32768
window_size = 128
chunk_size = 1024
for atss_file in atss_files:
    data_ts.append(atss.read_data(atss_file, 0, atss.samples(atss_file)))
#
# atss_extended.plot_time_series_quick(atss_files, start, window_size, title="Spike Detection Example", xlabel="Samples", ylabel="mV")
# Calculate median amplitude and detrend data for each file
outliers = [] # so can be [0] 76 and [1] 123 outliers

for i in range(len(atss_files)):
    # Original data median amplitude
    outliers.append(median_amplitude_spikes(data_ts[i], threshold=4))
#print(outliers)

# if we have two files, calculate the difference between them
if len(data_ts) == 2:
    diff_indices = ts_diff(data_ts[0], data_ts[1], threshold=8)

# print the length of outliers and diff_indices
print(f"Outliers File 1: {len(outliers[0])}, Outliers File 2: {len(outliers[1])}, Diff Indices: {len(diff_indices)}")

atss_extended.plot_time_series_quick(atss_files, 0, samples, title="Spike Detection Example", xlabel="Samples", ylabel="mV", marker=outliers)
#atss_extended.plot_time_series_quick(atss_files, start, window_size, title="Spike Detection Example", xlabel="Samples", ylabel="mV")
atss_extended.plot_time_series_diff(atss_files[0], atss_files[1], start, window_size, title="Spike Detection Example", xlabel="Samples", ylabel="mV", marker=diff_indices)

# prepare for writing old atm files with spikes marked, so we generate the names, using channel_to_ats_basename

atm_files_out = []
for chan in channels:
    atm_basename = atss.channel_to_ats_basename(chan)
    atm_file = atm_basename + ".atm"
    print(f"Generated atm output file name: {atm_file}")
    atm_files_out.append(atm_file)
# write atm files for atm_files_out
for i in range (len(atm_files_out)):
    atss.write_atm(atm_files_out[i], samples, outliers[i])
