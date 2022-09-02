
from ast import arg
from pathlib import Path
import sys
import getopt
import numpy as np
from matplotlib import pyplot as plt

import os
import json
script_file_path = Path(os.path.realpath(__file__))
# print(script_file_path)
inc_path = script_file_path.parent.parent.parent.absolute() / 'include'
# print(inc_path)

sys.path.append(os.path.relpath(inc_path))
import json_header as jh

def main(argv):
    filename = ''
    s = 0
    e = 1024
    try:
        opts, args = getopt.getopt(argv, "h:s:e:i:")
    except getopt.GetoptError:
        print('pyplot_chan.py -s 0 -e 1024 -i <inputfile>')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print('pyplot_chan.py -s 0 -e 1024 -i <inputfile>')
            sys.exit()
        elif opt in ("-i"):
            filename = arg      
        elif opt in ("-s"):
            s = int(arg)
        elif opt in ("-e"):
            e = int(arg)
    if filename.endswith('.'):
        filename = filename[:-1]

    print('Input file is ', filename)

    channel = jh.read_atssheader(filename)
    print ("reading", filename)
    print(json.dumps(channel, indent=2, sort_keys=False, ensure_ascii=False))
    file = open(filename + ".atss", 'rb')
    if e > s :
        wl = e - s
    else :
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
