
# This script is used to plot the time series of the channels in the ATSS file.
# Just a quick example of how to use the atss_file and atss_extended modules.
import sys
import getopt
#import numpy as np
#from matplotlib import pyplot as plt

import os
import json
###############################################
def get_script_directory(n_levels_up=0, explicit_tmp_dir=None):
    main_dir = ''
    script_dir = ''
    include_dir = ''
    temp_dir = '/tmp'
    try:
        # Check if running in a standard Python script
        script_dir = os.path.dirname(os.path.abspath(__file__))
        # return script_dir
    except NameError:
        # If __file__ is not defined, we are likely in a Jupyter Notebook
        from IPython import get_ipython
        if 'IPKernelApp' in get_ipython().config:
            script_dir = os.getcwd()
        else:
            raise RuntimeError("Unable to determine the script directory")
    # cd up n_levels_up
    main_dir = script_dir
    for i in range(n_levels_up):
        main_dir = os.path.dirname(main_dir)
    include_dir = os.path.join(main_dir, 'include')
    # leave the temporary directory to the user
    if explicit_tmp_dir is not None:
        temp_dir = explicit_tmp_dir
        return main_dir, script_dir, include_dir, temp_dir
    # else we try the simplest way to get the temporary directory
    # determine if we run Linux, MacOS or Windows
    if sys.platform == 'linux':
        temp_dir = '/tmp'
    elif sys.platform == 'darwin':
        temp_dir = '/tmp'
    elif sys.platform == 'win32':
        temp_dir = os.environ['TEMP']
    else:
        raise RuntimeError("Unknown platform")

    # return all directories
    return main_dir, script_dir, include_dir, temp_dir
####################################

def main(argv):
    main_dir, script_dir, include_dir, temp_dir = get_script_directory(0)
    sys.path.append(include_dir)
    import atss_file as atss
    import atss_extended as atss_ext
    filenames = []
    s = 0
    e = 1024
    try:
        opts, args = getopt.getopt(argv, "h:s:e:i:")
    except getopt.GetoptError:
        print('pyplot_chan.py -s 0 -e 1024 -i <inputfile> <inputfile> ...')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print('pyplot_chan.py -s 0 -e 1024 -i <inputfile> <inputfile> ...')
            sys.exit()
        elif opt in ("-s"):
            s = int(arg)
        elif opt in ("-e"):
            e = int(arg)
        elif opt in ("-i"):
            # take the rest of the arguments as filenames
            while arg != '':
                filenames.append(arg)
                arg = args.pop(0) if args else ''
        

    print ("reading", filenames)
    if e > s :
        wl = e - s
    else :
        wl = 1024                                   # set a window length for plotting
    # plot wl
    atss_ext.plot_time_series_quick(filenames, s, wl)


if __name__ == "__main__":
    main(sys.argv[1:])
