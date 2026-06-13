# from datetime import datetime
# from datetime import timezone
import math
import sqlite3
from os.path import exists
from os.path import getsize
from os.path import basename
import json
import copy
# from numpy import array


# ##################################################################################################################


def calibration():
    # contains the items needed for calibration data
    # the JSON ONLY!! contains the used calibration
    # so you have either the HF (chopper off) or LF (chopper on) data here but not both
    cal = {
        'sensor': "",                # MFS-06e ... SHFT-02e (that also indicates the manufacturer, right?)
        'serial': 0,                 # number 1, 2 ... not negative
        'chopper': 0,                # 0 == off or unkown, 1 == on
        'units_amplitude': "mV/nT",  # default; same unit as time series and no normalization
        'units_frequency': "Hz",     # x-axis of your calibration e.g. Hz (never s )
        'units_phase': "degrees",    # degrees 0... 360
        'datetime': "1970-01-01T00:00:00",   # date of calibration (1970-01-01 indicates no date available)
        'Operator': "",              # the person who has made the calibration (capital letter because operator is a keyword in C++)
        'f': [0.0],                  # frequencies in units as above
        'a': [0.0],                  # amplitudes  in units as above
        'p': [0.0],                  # phases      in units as above
    }
    return cal

# ##################################################################################################################


def get_cal_from_db(dbname, sensor, serial, chopper):
    if serial == 0:
        dbname = 'master_calibration.sql3'
        serial = 1
    conn = None
    f = [0.0] * 0
    a = [0.0] * 0
    p = [0.0] * 0
    try:
        conn = sqlite3.connect(dbname)
        cur = conn.cursor()
        cur.execute("SELECT f, a, p FROM `" + sensor + "` WHERE chopper = " + str(chopper))
        records = cur.fetchall()
        N = len(records)
        f = [0.0] * N
        a = [0.0] * N
        p = [0.0] * N
        i = 0
        for row in records:
            f[i] = float(row[0])
            a[i] = float(row[1])
            p[i] = float(row[2])
            i = i + 1

    except sqlite3.Error as e:
        print(e)
    finally:
        if conn:
            conn.close()

    return f, a, p

# ##################################################################################################################


def atss_file():
    # file name is part of the data - will not be repeated in header
    # order of the dictionary is file name!
    file = {
        'serial': 0,            # such as 1234 (no negative numbers please) for the system
        'system': "",           # such as ADU-08e, XYZ (a manufacturer is not needed because the system indicates it)
        'channel_no': 0,        # channel number - you can integrate EAMP stations as channels if the have the SAME!! timings
        'channel_type': "",     # type such as Ex, Ey, Hx, Hy, Hz or currents like Jx, Jy, Jz or Pitch, Roll, Yaw or x, y, z or T for temperature
        'sample_rate': 0.0,     # contains sample_rate. Unit: Hz (samples per second) - "Hz" or "s" will be appended while writing in real time
        # the FILENAME contains a UNIT for better readability; you MUST have 256Hz (sample rate 256) OR 4s (sample rate 0.25);
        # a "." in the FILENAME is possible on modern systems, 16.6666Hz is possible
    }
    return file

# ##################################################################################################################


def atss_header():
    # contains items needed for a complete channel description together with calibration data
    # does NOT contain values from file name
    # FORBIDDEN strings: are strings like MY_Station ... BECAUSE if it appears in the filename it will be interpreted as seprator
    header = {
        # 2007-12-24T18:21:00.01234Z is NOT supported by C/C++/Python/PHP and others COMPLETELY
        'datetime' : "1970-01-01T00:00:00.0",  # ISO 8601 datetime in UTC
        'latitude': 0.0,        # decimal degree such as 52.2443
        'longitude': 0.0,       # decimal degree such as 10.5594
        'elevation': 0.0,       # elevation in meter
        'azimuth': 0.0,           # orientation from North to East (90 = East, -90 or 270 = West, 180 South, 0 North)
        'tilt': 0.0,             # azimuth positive down - in case it had been measured
        'resistance': 0.0,      # resistance of the sensor in Ohm or contact resistance of electrode in Ohm
        'units': "mV",          # for ADUs it will be mV H or other -  or scaled E mV/km
        'filter': "",           # comma separated list of filters such as "ADB-LF,LF-RF-4" or "ADB-HF,HF-RF-1"
        'source': "",           # empty or indicate as, ns, ca, cp, tx or what ever
    }
    return header

# ##################################################################################################################


def channel():
    # filename and json data are one entity - it is joined here
    f = atss_file()
    h = atss_header()
    chan = f | h                                  # union (merge) dicts
    chan['sensor_calibration'] = calibration()    # needed at least for the sensor name and serial
    h['samples'] = 0                              # the magic volatile samples
    return chan


def json_header(channel):
    # creates the header data from the channel - so the file keys are skipped because they appear in the filename
    header = atss_header()
    for key, value in header.items():
        header[key] = channel[key]

    return header


def atss_filename(channel):
    # returns a filename WITHOUT extension - header keys from above are NOT in there
    array_tags = list(atss_file())                 # these are the file keys
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
                    filename = filename + fill + sample_rate_to_string(channel['sample_rate'])
                elif tag == "channel_no":
                    filename = filename + fill + "C" + f"{channel['channel_no']:03}"
                elif tag == "channel_type":
                    filename = filename + fill + "T" + f"{channel['channel_type']}"
                elif tag == "serial":
                    filename = filename + fill + f"{channel['serial']:03}"
                else:
                    filename = filename + fill + channel[tag]
    return filename


def cea_atss_filename(channel):
    sname = sample_rate_to_string(channel['sample_rate'])
    date_cea = channel['date'].replace('-', '')
    filename = "cea_site" + "_" + date_cea + "_" + channel['source'] + "_" + sname + "_" + channel['channel_type']
    # slices are not supported - create a run/slice instead
    filename = filename + "_R" + f"{channel['run']:03}"

    return filename


# ##################################################################################################################


def read_atssheader(filename):
    chan = channel()               # create a dictionary with keys from filename and header
    headername = filename
    tagname = basename(filename)
    tags = tagname.split("_")


    chan["serial"] = int(tags[0])
    chan["system"] = tags[1]
    tags.pop(0)
    tags.pop(0)

    for tag in tags:
        if tag.startswith('C'):
            tag = tag[1:]
            chan["channel_no"] = int(tag)
        if tag.startswith('T'):
            tag = tag[1:]
            chan["channel_type"] = tag
        if tag.endswith('s') and tag[0].isdigit():
            tag = tag[:-1]
            fl = float(tag)
            chan["sample_rate"] = 1.0/fl

        if tag.endswith('Hz') and tag[0].isdigit():
            tag = tag[:-2]
            chan["sample_rate"] = float(tag)

    # try the binary atss file with doubles
    if exists(headername + ".atss"):
        samples = getsize(headername + ".atss")
        chan['samples'] = int(samples / 8)

    # read the json file from disk
    if exists(headername + ".json"):
        with open(headername + ".json") as json_file:
            data = json.load(json_file)
            if 'datetime' in data:                      # datetime should always be there
                chan = chan | data

    return chan


# ##################################################################################################################

def write_atssheader(channel):
    filename = atss_filename(channel)
    fileitems = atss_file()

    tchannel = copy.deepcopy(channel)
    for item in fileitems:
        tchannel.pop(item)                           # remove the header items and write json

    with open(filename + ".json", 'w') as f:
        f.write(json.dumps(tchannel, indent=2, sort_keys=False, ensure_ascii=False))
        f.close()

def sample_rate_to_string(sample_rate):
    sname = "failed"
    if sample_rate > 0.99:
        i = int(round(sample_rate, 0))
        sname = str(i) + "Hz"
    else:
        d = 1.0 / sample_rate
        i = int(round(d, 0))
        sname = str(i) + "s"
    return sname


def to_json(channel):
    filename = atss_filename(channel) + ".json"
    header = json_header(channel)
    header['sensor_calibration'] = channel['sensor_calibration']
    print("write: ", filename)
    with open(filename, 'w') as f:
        json_string = json.dumps(header, indent=2, sort_keys=False, ensure_ascii=False)
        f.write(json_string)
        f.close()


def cal_mfs06e(f, spc, chopper):
    # we use mV/nT as unit, this also the unit of the time series
    # this formula does NOT normalize by f - because the next step is naturally all against the spectra
    # as we do here inside
    if f == 0:
        return spc
    if chopper == 1:
        p1 = complex(0.0, (f / 4.))
        p2 = complex(0.0, (f / 8192.))
        p4 = complex(0.0, (f / 28300.0))
        trf = 800. * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4)))
        return (spc / trf)   # cal is in mV, data in mV
    else:
        p1 = complex(0.0, (f / 4.))
        p2 = complex(0.0, (f / 8192.))
        p3 = complex(0.0, (f / 0.720))
        p4 = complex(0.0, (f / 28300.0))
        trf = 800.0 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (p3 / (1. + p3)) * (1. / (1. + p4)))
        return (spc / trf)   # cal is in mV, data in mV

def cal_mfs07e(f, spc, chopper):
    # we use mV/nT as unit, this also the unit of the time series
    # this formula does NOT normalize by f - because the next step is naturally all against the spectra
    # as we do here inside
    if f == 0:
        return spc
    if chopper == 1:
        p1 = complex(0.0, (f / 32.0))
        p2 = complex(0.0, (f / 40000.0))
        p4 = complex(0.0, (f / 50000.0))
        trf = 640. * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4)))
        return (spc / trf)   # cal is in mV, data in mV
    else:
        p1 = complex(0.0, (f / 32.0))
        p2 = complex(0.0, (f / 40000.0))
        p3 = complex(0.0, (f / 0.720))
        p4 = complex(0.0, (f / 50000.0))
        trf = 640.0 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (p3 / (1. + p3)) * (1. / (1. + p4)))
        return (spc / trf)   # cal is in mV, data in mV
    
def pos_to_tilt(x1, x2, y1, y2, z1, z2):
    tx = x2 - x1
    ty = y2 - y1
    tz = z2 - z1
    length = 0.0
    azimuth = 0.0
    tilt = 0.0
    length = math.sqrt(tx * tx + ty * ty + tz * tz)
    if length < 0.001:
        length = 0.0
    else:
        azimuth = 180. / math.pi * math.atan2(ty, tx)
        tilt = 180. / math.pi * (90 - math.acos(tz/length))

    tilt = [length, azimuth, tilt]
    return tilt



# ##################################################################################################################


