"""
check the channel_2_file function
and the frequency round function

"""
import sys
import os
import json
import numpy as np
import random
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
main_dir, script_dir, include_dir, temp_dir = get_script_directory(1)
data_dir = os.path.join(main_dir, 'data')
# add the include directory to the path
sys.path.append(include_dir)
#
import atss_file as atss
import atss_extended as atss_ext
# DONE EDITING #############################################
#

# read the data from the data Sarıçam folder, run 006
# file is 084_ADU-07e_C002_THx_8s, Hx magnetic North, wl 1024
wl = 1024
channel_2_file = os.path.join(data_dir, 'Sarıçam', 'run_006', '084_ADU-07e_C002_THx_8s')
#wl = 2048
#channel_2_file = "/tmp/aa/noise/stations/1/run_013/999_ADU-08e_C002_THx_32s"
channel_2 = atss.read_header(channel_2_file)
#plot the time series
atss_ext.plot_time_series_quick(channel_2_file, 0, wl)
# plot the fft
atss_ext.plot_fft_quick(channel_2_file, 0, wl)

# we have a set of channel_2 and channel_2_file
print("start time", channel_2['datetime'], "stop time", atss.stop_date_time(channel_2_file), "sample rate", channel_2['sample_rate'], "Hz")
# plot the calibration function
atss_ext.plot_calibration_quick(channel_2_file)
#
atss_ext.plot_fft_inverse_quick(channel_2_file, 0, wl, False)
# finally print the keys of the channel_2 dictionary
print(channel_2.keys())
