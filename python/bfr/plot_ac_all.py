# we need numpy, matplotlib
import numpy as np
import matplotlib.pyplot as plt
import os
# data directory
d_dir = '/home/bfr/dump_spectra/'

existing_labels = []
existing_colors = []
colors = ['blue', 'green', 'red', 'cyan', 'magenta', 'yellow', 'black']

count = 0;
for filename in os.listdir(d_dir):
    if filename.endswith(".dat"):
        print(filename)
        # read data
        data = np.loadtxt(d_dir + filename)
        # if the filename starts with the label, take the label
        # split the filename at last underscore
        label = filename.rsplit('_', 1)[0]
        # if the label is not in the list of existing labels, add it
        if label not in existing_labels:
            existing_labels.append(label)
            existing_colors.append(colors[count])
            count += 1
            # plot
            plt.plot(data[:,0], data[:,1], label=label, color=existing_colors[existing_labels.index(label)])
        else:
            # plot without label
            # get the position of the label in the list
            label_pos = existing_labels.index(label)
            # get the color of the label
            color = existing_colors[label_pos]
            plt.plot(data[:,0], data[:,1], color=color)

            # plt.plot(data[:,0], data[:,1])
        continue
    else:
        continue
#
plt.xscale('log')
plt.yscale('log')
# 1 remove the underscore from all labels
# 2 replace underscore with space
# 3 concat all labels to a string, sperated by comma
# 4 set the title to the string
plt.title(', '.join([label.replace('_', ' ') for label in existing_labels]))
# plt.title(filename)
plt.xlabel('Frequency (Hz)')
plt.ylabel('Amplitude')
plt.legend()
plt.show()
