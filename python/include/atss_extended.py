import os
import json
import copy
import numpy as np
from matplotlib import pyplot as plt
import atss_file as atss

# a simple time series plot from an array of atss data files
def plot_time_series_quick(atss_files_in, start, wl, title=None, xlabel=None, ylabel=None, legend=None, save=None):
    atss_files = []
    if type(atss_files_in) is list:
        atss_files = atss_files_in
    else:
        atss_files.append(atss_files_in)
    
    #check if the files exist and samples is >= start + wl
    all_samples = []
    for atss_file in atss_files:
        print("printing,", atss_file)
        all_samples.append(atss.exits_both(atss_file))

    # check if all files have the same number of samples
    # if not, raise an error; iterate over all samples and check if they are equal compared to first value
    if not all(x == all_samples[0] for x in all_samples):
        raise ValueError(f"Files have different number of samples")
   
    # create channels
    channels = []   
    for atss_file in atss_files:
        channels.append(atss.read_header(atss_file))

    # create a figure and axis
    fig, ax = plt.subplots()

    # set the title
    if title is not None:
        ax.set_title(title)
    else:
        # comma separated list of channel types
        channel_types = [channel['channel_type'] for channel in channels]
        ax.set_title(', '.join(channel_types))


    # set the x-axis label
    if xlabel is not None:
        ax.set_xlabel(xlabel)
    else:
        ax.set_xlabel('samples [n]')
        

    # set the y-axis label
    if ylabel is not None:
        ax.set_ylabel(ylabel)
    else:
        ax.set_ylabel('mV')

    # plot the data
    for atss_file in atss_files:
        atss_data = atss.read_data(atss_file, start, wl)
        # add the data to the plot
        ax.plot(atss_data)

    # add a legend
    if legend is not None:
        ax.legend(legend)

    # save the plot
    if save is not None:
        plt.savefig(save)

    fig = plt.gcf()
    if (len(atss_files) == 1):
        fig.canvas.manager.set_window_title(atss_files[0])
    else:
        fig.canvas.manager.set_window_title(atss_files[0] + ' etc.')
    # show the plot
    plt.show()

def plot_calibration_quick(file_name):
    # read the header
    channel = atss.read_header(file_name)
    # create a figure and axis
    fig, ax = plt.subplots()


    # set the title
    if (channel['sensor_calibration']['chopper'] == 1):
        ax.set_title('Calibration ' + channel['sensor_calibration']['sensor'] + ' ' + str(channel['sensor_calibration']['serial']) + ' Chopper on')
    else:
        ax.set_title('Calibration ' + channel['sensor_calibration']['sensor'] + ' ' + str(channel['sensor_calibration']['serial']) + ' Chopper off')

    # set the x-axis label
    ax.set_xlabel('f [Hz]')

    # set the y-axis label
    ax.set_ylabel('mV/nT')

    # plot the data
    ax.loglog(channel['sensor_calibration']['f'], channel['sensor_calibration']['a'], marker='.')
    # plt.loglog(channel['sensor_calibration']['f'],channel['sensor_calibration']['a'], marker='.') # these are the values from the json header


    fig = plt.gcf()
    fig.canvas.manager.set_window_title(file_name)
    # show the plot
    plt.show()

def plot_fft_quick(file_name, start, wl, cdown = 0.006, cup = 0.75, window = "hanning"):
    # check if the file exists
    atss.exits_both(file_name)
    # create a channel as dictionary
    channel = atss.read_header(file_name)  
    # read the data, which returns an np.array
    data = atss.read_data(file_name, start, wl)

    # do a fft
    data = np.array(data)
    data = np.array(data, dtype=float)
    data = data - np.mean(data)
    # use no window function incase you want to inverse fft
    # while plotting the fft, the visible difference is small
    if window is not None:
        # Hanning window
        if window == 'hanning':
            data = data * np.hanning(len(data))
        
    spec = np.fft.rfft(data, norm="backward")
    wincal = np.sqrt(1.0 / (0.5 * wl * channel['sample_rate'])) * 2.0
    spec *= wincal

    ampl_spec = np.abs(spec)
    # floor division //, so that we get an integer
    sz = wl//2 + 1
    ampl_freq = np.linspace(0, 1, sz, endpoint=True)
    ampl_freq = ampl_freq * channel['sample_rate']
    # shorten the spectrum to the cup factor
    ampl_spec = ampl_spec[:int(cup * len(ampl_spec))]
    ampl_freq = ampl_freq[:int(cup * len(ampl_freq))]
    # shorten the spectrum to the cdown factor from the lower end (beginning)
    ampl_spec = ampl_spec[int(cdown * len(ampl_spec)):]
    ampl_freq = ampl_freq[int(cdown * len(ampl_freq)):]
    # remove the 0 frequency
    #ampl_spec = ampl_spec[1:]
    #ampl_freq = ampl_freq[1:]
    fig, ax = plt.subplots()
    ax.plot(ampl_freq, ampl_spec)
    ax.set_title('Amplitude Density Raw Spectrum')
    ax.set_xlabel('f [Hz]')
    ax.set_ylabel('mV/sqrt(Hz)')
    # log log
    ax.set_xscale('log')
    ax.set_yscale('log')
    # set the file name plot title of the window (not the plot title)
    # set window title
    fig = plt.gcf()
    fig.canvas.manager.set_window_title(file_name)
    # show the plot
    plt.show()

def plot_fft_inverse_quick(file_name, start, wl, theo = True):
    # check if the file exists
    atss.exits_both(file_name)
    # create a channel as dictionary
    channel = atss.read_header(file_name)  
    # read the data, which returns an np.array
    data = atss.read_data(file_name, start, wl)
    # do a fft
    # data = np.array(data)
    # data = np.array(data, dtype=float)
    data = data - np.mean(data)
    # use no window function incase you want to inverse fft
    # while plotting the fft, the visible difference is small
    spec = np.fft.rfft(data, norm="backward")
    # prepare the plot
    fig, ax = plt.subplots()
    ax.set_title(channel['channel_type'] + ' calibrated')
    ax.set_xlabel('samples [n]')
    ax.set_ylabel('nT')
    fig = plt.gcf()
    fig.canvas.manager.set_window_title(file_name)
    #
    if (theo):
        # generate a theoretical calibration
        atss.cal_mfs_06e(spec, file_name, wl)
        data = np.fft.irfft(spec, norm="backward")
        
        ax.plot(data)
        # show the plot
        plt.show()
    else:        
        spec *=  2.0                  # disagree with normalization 
        # calculate the fft frequencies, sz must be an integer
        sz = wl//2 + 1
        ampl_freq = np.linspace(0, 1, sz, endpoint=True)
        ampl_freq = ampl_freq * channel['sample_rate']
        # get f, a, p from the calibration
        f = channel['sensor_calibration']['f']
        a = channel['sensor_calibration']['a']
        p = channel['sensor_calibration']['p']
        # interpolate the calibration so that f,a and f,p fit the fft frequencies
        a = np.interp(ampl_freq, f, a)
        p = np.interp(ampl_freq, f, p)
        # make a complex array, so that we can multiply with spec
        # a is the amplitude, p is the phase in degrees
        # we need to convert the phase to radians
        p = np.radians(p) 
        cplx = a * np.exp(1j * p)
        # multiply the spec with the complex array
        spec = spec / cplx    
        # do an inverse fft
        data = np.fft.irfft(spec, norm="backward")
        ax.plot(data)
        # show the plot
        plt.show()