
# ##################################################################################################################
# helper function to convert the sample rate to a string ending with Hz or s, you can give the precision
# avoid 265.0001Hz, 256.98 -> 257Hz or 4.0001 -> 4s rounding errors
# precision is the number of digits after the comma, default is 0, you get only full Hertz or seconds
# ATTENTION: the railway frequency is 16.6666Hz, you get 17 Hz then; take precision = 2 to get 16.67Hz
     
# ##################################################################################################################
# returns a filename WITHOUT extension
# prepend_dir is the directory where the file is stored - a convenience function
# precision is the number of digits after the comma, default is 0, you get only full Hertz or seconds
def base_name(channel, precision = 0, prepend_dir = ""):
    array_tags = list(file_tags())                 # these are the file keys
    filename = ""
    count = -1
    fill = ""
    for tag in array_tags:
        count = count + 1
        if count > 0:
            fill = "_"
        for key, value in channel.items():
            if tag == key:
                if tag == "sample_rate":
                    filename = filename + fill + sample_rate_to_string(channel['sample_rate'], precision)
                elif tag == "channel_no":
                    filename = filename + fill + "C" + f"{channel['channel_no']:03}"
                elif tag == "channel_type":
                    filename = filename + fill + "T" + f"{channel['channel_type']}"
                elif tag == "serial":
                    filename = filename + fill + f"{channel['serial']:03}"
                else:
                    filename = filename + fill + channel[tag]
    if (prepend_dir != ""): 
        #use os.path.join to make it platform independent
        filename = os.path.join(prepend_dir, filename)  
    return filename

# ##################################################################################################################
# write the json header file
# the file tags are separated by "_" and will be removed from the dictionary
# they are part of the file name, not the json header
def write_header(channel, file_name):
    file_items = file_tags()
    # temporary channel without the file items
    temp_channel = copy.deepcopy(channel)
    for item in file_items:
        temp_channel.pop(item)                           # remove the header items and write json
    # write the json header file
    with open(atss_json_name(file_name), 'w') as f:
        f.write(json.dumps(temp_channel, indent=2, sort_keys=False, ensure_ascii=False))
        f.close()
# ##################################################################################################################
# the data array is a numpy array of type np.float64 and written as binary file
def write_data(data_array, file_name):
    with open(atss_atss_name(file_name), 'wb') as f:
        # Convert the data array to bytes
        data_bytes = data_array.tobytes()
        # Write the bytes to the binary file
        f.write(data_bytes)
        f.close()

# ##################################################################################################################
# read the json header file
# the tags are separated by "_" and will be mapped to the dictionary
def read_header(file_name):
    # create an empty channel
    chan = channel()
    tagname = atss_basename(file_name)
    # strip the leading path, us os.path.basename instead
    tagname = os.path.basename(tagname)

    tags = tagname.split("_")
    chan["serial"] = int(tags[0]) # no leading tag indicator
    chan["system"] = tags[1]      # no leading tag indicator
    tags.pop(0)
    tags.pop(0)
    for tag in tags:              # the rest of the tags
        if tag.startswith('C'):   # channel number
            tag = tag[1:]
            chan["channel_no"] = int(tag)
        if tag.startswith('T'):   # channel type
            tag = tag[1:]
            chan["channel_type"] = tag
        # sample rate
        if tag.endswith('s') and tag[0].isdigit():
            tag = tag[:-1]
            fl = float(tag)
            chan["sample_rate"] = 1.0/fl

        if tag.endswith('Hz') and tag[0].isdigit():
            tag = tag[:-2]
            chan["sample_rate"] = float(tag)

    with open(atss_json_name(file_name), 'r') as f:
        header = json.load(f)
        f.close()
    # merge the header with the file name
    chan.update(header)
    return chan

# ##################################################################################################################
# read the data file
# run exists_both before to check if the files exist first
def read_data(file_name, start=0, wl=0):
    samples = os.path.getsize(atss_atss_name(file_name)) / 8
    if start + wl > samples:
        raise ValueError(f"start + wl > samples")
    with open(atss_atss_name(file_name), 'rb') as f:
        # Read the binary data
        f.seek(start * 8)
        if wl == 0:
            data_bytes = f.read() # complete file
        else:
            data_bytes = f.read((wl) * 8)
        # Convert the data to a numpy array
        data_array = np.frombuffer(data_bytes, dtype=np.float64)
        f.close()
    return data_array

# ##################################################################################################################

#remove .json or .atss from the file name
def atss_basename(file_name):
    file_name = os.fspath(file_name)
    if file_name.endswith('.atss'):
        return file_name[:-5]
    elif file_name.endswith('.json'):
        return file_name[:-5]
    else:
        return file_name

def atss_json_name(file_name):
    return atss_basename(file_name) + ".json"

def atss_atss_name(file_name):
    return atss_basename(file_name) + ".atss"
    
# ##################################################################################################################

def samples(file_name_or_channel):
    """
    Get the number of samples from either a file name or a channel object.
    
    Args:
        file_name_or_channel: Either a string (file name) or a dict (channel object)
        
    Returns:
        int: Number of samples in the file
    """
    if isinstance(file_name_or_channel, dict):
        # It's a channel object, generate the file name first
        file_name = base_name(file_name_or_channel)
    else:
        # It's a file name
        file_name = file_name_or_channel
    
    samples = os.path.getsize(atss_atss_name(file_name)) / 8
    return int(samples)

# ##################################################################################################################

def stop_date_time(file_name):
    nsamples = samples(file_name)
    channel = read_header(file_name)
    sample_rate = channel["sample_rate"]
    # get the start date time ISO 8601 like "1970-01-01T00:00:00.0"
    start_date_time = channel["datetime"]
    # calculate the stop date time
    stop_date_time = np.datetime64(start_date_time) + np.timedelta64(int(nsamples / sample_rate), 's')
    return stop_date_time

def duration(file_name):
    # get the start date time ISO 8601 like "1970-01-01T00:00:00.0"
    start_date_time = read_header(file_name)["datetime"]
    # get the stop date time ISO 8601 like "1970-01-01T00:00:00.0"
    stop_date_time_in = stop_date_time(file_name)
    # calculate the duration
    duration = stop_date_time_in - np.datetime64(start_date_time)
    # return the duration in HH:MM:SS
    return str(duration)
   
# check if atss and json exist, and return the samples    
def exits_both(file_name):
    sfile_name = atss_json_name(file_name)
    # if not exist, terminate with FileNotFoundError
    if not os.path.exists(sfile_name):
        raise FileNotFoundError(f"File {sfile_name} not found")
    #
    sfile_name = atss_atss_name(file_name)
    # if not exist, terminate with FileNotFoundError
    if not os.path.exists(sfile_name):
        raise FileNotFoundError(f"File {sfile_name} not found")
    # if both exist, return the amount samples
    return samples(file_name)

def cal_mfs_06e(spc, file_name, wl):
    # the calibration data for the MFS-06e sensor
    # the spc is the complex spectrum, calculated by the fft "backward" function
    # file_name is the file name of the channel, we take the sample rate from the header
    #
    # get the channel from the file
    file_name = atss_basename(file_name)
    if (not os.path.exists(atss_json_name(file_name))):
        raise FileNotFoundError(f"File {atss_json_name(file_name)} not found")
    channel = read_header(file_name)
    fs = channel['sample_rate']
    chopper = channel['sensor_calibration']['chopper']
    if (chopper == 1):
        # calculate the frequency for each bin
        for i, x in enumerate(spc):
            if (i == 0):
                continue
            f = i * fs / wl
            p1 = complex(0.0, (f / 4.))
            p2 = complex(0.0, (f / 8192.))
            p4 = complex(0.0, (f / 28300.0))
            trf = 800. * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4)))
            spc[i] = spc[i] / trf
    else:
        # calculate the frequency for each bin
        for i in enumerate(spc):
            if (i == 0):
                continue
            f = i * fs / wl
            p1 = complex(0.0, (f / 4.))
            p2 = complex(0.0, (f / 8192.))
            p3 = complex(0.0, (f / 0.720))
            p4 = complex(0.0, (f / 28300.0))
            trf = 800.0 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (p3 / (1. + p3)) * (1. / (1. + p4)))   
            spc[i] = spc[i] / trf

# 002_V01_C04_R000_THz_BH_131072H
# system_serial (3 digits)
# V01 (version 1), always V01 for ever
# C channel number (2 digits) like C04
# R run number (3 digits) - take R000 for now
# T channel type such as THz, TEx, TEy, THx, THy, etc.
# BH indicates board high frequency, take if filter contains "ADB-HF", else BL for board low frequency
# sample rate such as 131072H for 131072Hz, 4S for 0.25Hz
def channel_to_ats_basename(channel):
    filename = f"{channel['serial']:03}_V01_C{channel['channel_no']:02}_R000_T{channel['channel_type']}_"
    # determine BH or BL from filter
    if "ADB-HF" in channel['filter']:
        filename = filename + "BH_"
    else:
        filename = filename + "BL_"
    # sample rate
    if channel['sample_rate'] >= 1.0:
        filename = filename + f"{int(channel['sample_rate'])}H"
    else:
        sr_seconds = int(1.0 / channel['sample_rate'])
        filename = filename + f"{sr_seconds}S"
    return filename

# binary header of .atm marker file
# struct atmheader {
#   std::int16_t siHeaderLength = 8; // length of this header in bytes
#   std::int16_t siHeaderVers = 10;  // version of this header
#   std::uint32_t iSamples;          // number of samples in data block 
# };
#
# boolean data is packed are packed into one char/uint8_t

def write_atm(file_name, samples, indices):
    """
    Write a .atm marker file with the given indices.
    Args:
        file_name (str): The name of the .atm file to write.
        indices np.array with 64bit integers: The list of sample indices to include in the marker file.
    """
    with open(file_name, 'wb') as f:
        # write siHeaderLength
        f.write((0).to_bytes(2, byteorder='little', signed=True))
        # write siHeaderVers
        f.write((0).to_bytes(2, byteorder='little', signed=True))
        # write iSamples
        f.write(samples.to_bytes(4, byteorder='little', signed=False))
        # now we pack
        # in this example we have 12 samples:
        # we write 2 bytes (16 bits)
        # bit 0 = sample 0
        # bit 1 = sample 1 and so on
        # indices = [0, 1, 5, 11] as an example indicating that these bit are set to 0 (false)
        # so the first byte is 00100011 = 0x23
        # the second byte is at the 11th bit set = 00001000 = 0x08
        nbytes = (samples + 7) // 8  # number of bytes needed
        byte_array_0 = bytearray(4)
        for i in range(4):
            byte_array_0[i] = 0x00
        byte_array = bytearray(nbytes)
        # for debugging we simply set first 64 bits to 1
        # for i in range(8):
        #     byte_array[i] = 0xFF
        # # rest bits are 0


        for index in indices:
            byte_index = index // 8
            bit_index = index % 8
            byte_array[byte_index] |= (1 << bit_index)
        
        # write the byte array to the file
        f.write(byte_array_0)
        f.write(byte_array)
        f.close()
