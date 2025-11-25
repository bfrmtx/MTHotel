from atss import atss_extended
from atss import atss_file as atss
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import detrend
# file /home/bfr/tmp/delft/stations/s1EW/run_002/211_ADU-08e_C003_THy_524288Hz.atss
file = '/home/bfr/tmp/delft/stations/s1EW/run_002/211_ADU-08e_C003_THy_524288Hz'
channel = atss.read_header(file)
#print(channel)
window_size = 524288 # 1 second window at 524288 Hz, 1 Hz bandwidth
# loop over data in chunks, detrend and make a fft, stack the ffts
count = 0
stacked_fft = None
for start in range(0, atss.samples(file), window_size):
    data = atss.read_data(file, start, window_size)
    data_detrended = detrend(data)
    fft_data = np.fft.rfft(data_detrended * np.hanning(len(data_detrended)))
    fft_magnitude = np.abs(fft_data)
    # Here you can store or process the fft_magnitude as needed
    print(f"Processed FFT for samples {start} to {start + len(data_detrended)}")
    # stack the ffts as needed
    if start == 0:
        stacked_fft = fft_magnitude
    else:
        stacked_fft += fft_magnitude
    count += 1
# take the average fft by dividing by count
stacked_fft = stacked_fft / count
print(f"Total FFT windows processed: {count}")
# Calculate the average of the stacked FFTs
# now a log log plot of the stacked fft, amplitude is y axis, frequency is x axis
# limit the plot range from 10 kHz to 100 kHz
fmin = 15e3
fmax = 80e3
#from 
frequencies = np.fft.rfftfreq(window_size, d=1/channel['sample_rate'])
plt.figure(figsize=(10, 6))
plt.loglog(frequencies[1:]/1000, stacked_fft[1:])  # skip the zero frequency, convert to kHz
plt.xlim(fmin/1000, fmax/1000)  # limit frequency range from 10 kHz to 100 kHz
plt.title('Average FFT of Detrended Data')
plt.xlabel('Frequency (kHz)')
plt.ylabel('Amplitude')
plt.grid(True, which="both", ls="--")
plt.gca().xaxis.set_major_formatter(plt.FuncFormatter(lambda x, _: f'{int(x)}'))
plt.gca().xaxis.set_minor_formatter(plt.FuncFormatter(lambda x, _: f'{int(x)}'))
# now from vlf_transmitters.csv we load Name and Frequency_kHz columns
import csv
transmitter_frequencies = {}
#with open('./vlf/vlf_transmitters.csv', 'r') as csvfile:
with open('./vlf/vlf_recovered.csv', 'r') as csvfile:
  reader = csv.DictReader(csvfile)
  for row in reader:
    name = row['Name']
    frequency_khz = float(row['Frequency_kHz'])
    transmitter_frequencies[name] = frequency_khz
# plot Name at Frequency_kHz
for name, freq_khz in transmitter_frequencies.items():
    if fmin/1000 <= freq_khz <= fmax/1000:
        plt.axvline(x=freq_khz, color='r', linestyle='--', alpha=0.7)
        plt.text(freq_khz, plt.ylim()[1]*0.8, name, rotation=90, verticalalignment='top', fontsize=14) 

plt.show()
