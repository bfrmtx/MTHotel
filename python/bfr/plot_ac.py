# we need numpy, matplotlib
import numpy as np
import matplotlib.pyplot as plt
import sys
# read two files from the command line
filename = sys.argv[1]
filename2 = sys.argv[2]
# if a thrird argument is given
filename3 = sys.argv[3] if len(sys.argv) > 3 else None
# these files are plain x y ascii files (no header, two columns)
data = np.loadtxt(filename)
data2 = np.loadtxt(filename2)

plt.plot(data[:,0], data[:,1], 'o', label=filename)
plt.plot(data2[:,0], data2[:,1], 'o', label=filename2)
if filename3 is not None:
    data3 = np.loadtxt(filename3)
    plt.plot(data3[:,0], data3[:,1], label=filename3)
#
plt.xscale('log')
plt.yscale('log')
plt.title(filename)
plt.xlabel('Frequency (Hz)')
plt.ylabel('Amplitude')
plt.legend()
plt.show()
