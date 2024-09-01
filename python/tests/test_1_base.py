"""
tiny example to show how to use the ATSS library
generates a random time series and writes it to a file
in the tmp directory

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
# add the include directory to the path
sys.path.append(include_dir)
print(temp_dir)
print(include_dir)
import atss_file as atss
import atss_extended as atss_ext
#
channel_2 = atss.channel()
# set some values, system, which end up in the file name
channel_2['system'] = 'ADU-08e'
channel_2['serial'] = 82
# "again" the channel number, here now as numerical value
channel_2['channel_no'] = 2
channel_2['channel_type'] = 'Hx'
# check the round function -> 256 Hz
channel_2['sample_rate'] = 255.99
# 128.1 should be rounded to 128 s
temp_period = 1./128.4
print(atss.sample_rate_to_string(temp_period))
temp_period = 16.66666
print(atss.sample_rate_to_string(temp_period, 2))
# some values for the json header, beginning with the start time
channel_2['datetime'] = '2020-12-01T14:30:00'
channel_2['latitude'] = 52.2
channel_2['longitude'] = 10.6
channel_2['elevation'] = 200.0
channel_2['azimuth'] = 0.0 # azimuth of the sensor, NORTH is 0.0
channel_2['tilt'] = 0.0 # azimuth of the sensor, HORIZONTAL is 0.0
channel_2['sensor_calibration']['sensor'] = 'MFS-06e'
channel_2['sensor_calibration']['serial'] = 12
channel_2['sensor_calibration']['chopper'] = 1 # you want to know this if you use a theoretical function
#
# create a file name in the tmp directory
file_name = os.path.join(temp_dir, atss.base_name(channel_2))
# and print the json header
# print(json.dumps(channel_2, indent=2))
atss.write_header(channel_2, file_name)
# create a numpy data array with random values, size 4096
data = np.array([random.random() for i in range(4096)])
# write the data
atss.write_data(data, file_name)
#
atss_ext.plot_time_series_quick(file_name, 0, 512)
print("stop date time", atss.stop_date_time(file_name))
