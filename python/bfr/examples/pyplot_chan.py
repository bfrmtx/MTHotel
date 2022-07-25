
from ast import arg
import sys
import getopt
import numpy as np
from matplotlib import pyplot as plt
from scipy.fft import rfft, irfft
from scipy import signal
import os
import json
sys.path.append(os.path.realpath('../../include'))
import json_header as jh

def main(argv):
    filename = ''
    try:
        opts, args = getopt.getopt(argv, "hi:", ["ifile="])
    except getopt.GetoptError:
        print('test.py -i <inputfile> -o <outputfile>')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print('test.py -i <inputfile>')
            sys.exit()
        elif opt in ("-i", "--ifile"):
            filename = arg
    print('Input file is ', filename)

    channel = jh.read_atssheader(filename)
    print ("reading")
    print(json.dumps(channel, indent=2, sort_keys=False, ensure_ascii=False))
    file = open(filename + ".atss", 'rb')
    wl = 1024                                   # set a window length for plotting
    # read wl
    data = np.fromfile(file, dtype = np.float64, count = wl)
    t = range(wl) # x axis
    plt.title( channel['channel_type'])       # title
    plt.xlabel("samples @ " + jh.sample_rate_to_string(channel['sample_rate']))
    plt.ylabel(channel['channel_type'] + "[" + channel['units'] +"]")
    plt.plot(t, data, c='b', label='data')
    plt.draw()
    plt.show()
if __name__ == "__main__":
    main(sys.argv[1:])
