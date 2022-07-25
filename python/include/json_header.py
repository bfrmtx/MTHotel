from datetime import datetime
from datetime import timezone
import math
import sqlite3
from os.path import exists
from os.path import getsize
from os.path import basename
import json
import copy
from numpy import array


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
        'date': "1970-01-01",        # date of calibration (1970-01-01 indicates no date available)
        'time': "00:00:00",          # time of calibration (leave empty if you want)
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
    # file name is partof the data - will not be repeated in header
    # order of the dictionary is file name!
    file = {
        'serial': 0,            # such as 1234 (no negative numbers please) for the system
        'system': "",           # such as ADU-08e, XYZ (a manufacturer is not needed because the system indicates it)
        'channel_no': 0,        # channel number - you can integrate EAMP stations as channels if the have the SAME!! timings
        'run': 0,               # counts for same frequency at the same place - or a sclice
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
        'date': "1970-01-01",   # ISO 8601 date 2021-05-19 UTC
        'time': "00:00:00",     # ISO 8601 time in UTC
        'fracs': 0.0,           # factions of seconds, at your own risk. It is always the best to cut the data to full seconds
        'latitude': 0.0,        # decimal degree such as 52.2443
        'longitude': 0.0,       # decimal degree such as 10.5594
        'elevation': 0.0,       # elevation in meter
        'angle': 0.0,           # orientaion from North to East (90 = East, -90 or 270 = West, 180 South, 0 North)
        'dip': 0.0,             # angle positive down - in case it had been measured
        'units': "",            # for ADUs it will be mV H or oher -  or scaled E mV/km
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
                elif tag == "run":
                    filename = filename + fill + "R" + f"{channel['run']:03}"
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
        if tag.startswith('R'):
            tag = tag[1:]
            chan["run"] = int(tag)
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

    # try the binnary atss file with doubles
    if exists(headername + ".atss"):
        samples = getsize(headername + ".atss")
        chan['samples'] = int(samples / 8)

    # read the json file from disk
    if exists(headername + ".json"):
        with open(headername + ".json") as json_file:
            data = json.load(json_file)
            if 'date' in data:                      # date should always be there
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


# ##################################################################################################################


def channel_form_oldheader(oldheader):
    # copy the mimimum neccessary parts to the new header
    # most of the old header is not needed for processing
    chan = channel()
    dt = datetime.fromtimestamp(oldheader['start'], timezone.utc)  # utc is important
    chan['date'] = dt.strftime("%Y-%m-%d")
    chan['time'] = dt.strftime("%H:%M:%S")
    chan['sample_rate'] = oldheader['sample_rate']
    chan['channel_no'] = oldheader['channel_number']
    chan['channel_type'] = oldheader['channel_type']
    chan['run'] = 1       # get from filename
    chan['latitude'] = (oldheader['iLat_ms'] / 1000.) / 3600.
    chan['longitude'] = (oldheader['iLong_ms'] / 1000.) / 3600.
    str = oldheader['SystemType']
    if str.startswith("ADU") and not(str.startswith("ADU-")):
        str = str.replace("ADU", "ADU-")
        chan['system'] = str
    else:
        chan['system'] = oldheader['SystemType']  # change not
    chan['serial'] = oldheader['serial_number']
    chan['site'] = oldheader['SiteName']
    chan['elevation'] = oldheader['iElev_cm'] / 100.
    # since 15 years we do use pos
    p = pos_to_dip(oldheader['x1'], oldheader['x2'], oldheader['y1'], oldheader['y2'], oldheader['z1'], oldheader['z2'])
    chan['dipole_length'] = p[0]
    chan['angle'] = p[1]
    chan['dip'] = p[2]
    # ADU uses mV without mentioning it
    chan['units'] = "mV"          # H, E -> change that if you scale E to mV/km
    # remove ancient MS DOS shortened names
    str = oldheader['sensor_type']
    if str.startswith("MFS") and not(str.startswith("MFS-")):
        str = str.replace("MFS", "MFS-")
        chan['sensor_calibration']['sensor'] = str

    elif str.startswith("FGS") and not(str.startswith("FGS-")):
        str = str.replace("FGS", "FGS-")
        chan['sensor_calibration']['units_amplitude'] = "mV"                # maybe mV or temperature
        chan['sensor_calibration']['sensor'] = str

    elif str.startswith("SHFT") and not(str.startswith("SHFT-")):
        str = str.replace("SHFT", "SHFT-")
        chan['sensor_calibration']['sensor'] = str

    elif str.startswith("EFP") and not(str.startswith("EFP-")):
        str = str.replace("EFP", "EFP-")
        chan['sensor_calibration']['sensor'] = str
        chan['sensor_calibration']['units_amplitude'] = "mV"                # maybe mV or temperature

    else:
        chan['sensor_calibration']['sensor'] = oldheader['sensor_type']

    chan['sensor_calibration']['serial'] = oldheader['sensor_serial_number']
    chan['sensor_calibration']['chopper'] = oldheader['chopper']
    chan['lsb'] = oldheader['lsbval']            # temporary
    chan['samples'] = oldheader['samples']       # temporary

    return chan


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


def aduboard_from_sample_rate(sample_rate):
    # the data processing does not make use of the board
    # the LF, MF and HF filters are only informal, so a fake board is ok
    sname = "failed"
    if sample_rate > 4096:
        sname = "H"
    else:
        sname = "L"
    return sname


def cal_mfs06e(i, f, spc, chopper):
    # we stay with V/(nT*Hz) because we have millions of old files and this format is inside the coils
    # this formula doe NOT normalize by f - because the next step is naturally all against the spectra
    # as we do here inside
    if f == 0:
        return spc
    if chopper == 1:
        p1 = complex(0.0, (f / 4.))
        p2 = complex(0.0, (f / 8192.))
        p4 = complex(0.0, (f / 28300.0))
        trf = 0.8 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4)))
        return (spc / trf) / 1000.  # cal is in V, data in mV
    else:
        p1 = complex(0.0, (f / 4.))
        p2 = complex(0.0, (f / 8192.))
        p3 = complex(0.0, (f / 0.72))
        p4 = complex(0.0, (f / 25000.0))
        trf = 0.8 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (p3 / (1. + p3)) * (1. / (1. + p4)))
        return (spc / trf) / 1000.  # cal is in V, data in mV


def pos_to_dip(x1, x2, y1, y2, z1, z2):
    tx = x2 - x1
    ty = y2 - y1
    tz = z2 - z1
    length = 0.0
    angle = 0.0
    dip = 0.0
    length = math.sqrt(tx * tx + ty * ty + tz * tz)
    if length < 0.001:
        length = 0.0
    else:
        angle = 180. / math.pi * math.atan2(ty, tx)
        dip = 180. / math.pi * (90 - math.acos(tz/length))

    dip = [length, angle, dip]
    return dip


def dip_to_pos(length, angle, dip):
    pos = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
    dp = dip
    if math.abs(length) < 0.0001:
        return pos
    if math.abs(dip) < 0.1:
        dp = 0.0
    x = length * math.cos(math.pi / 180. * angle) * math.cos(math.pi / 180. * dp)
    y = length * math.sin(math.pi / 180. * angle) * math.cos(math.pi / 180. * dp)
    z = length * math.sin(math.pi / 180. * dp)

    pos[0] = -0.5 * x
    pos[1] = 0.5 * x
    pos[2] = -0.5 * y
    pos[3] = 0.5 * y
    pos[4] = 0.0
    pos[5] = z

    return pos

# ##################################################################################################################


def system_channel():
    # these values are not used for processing
    # they are logged from the ADU system
    sys = {
        'lsbval': 0.0,                          # the lsb value is ONLY evaluated when converting to double
        'ADB_board_name': "",                   # MF, LF, HF, BB
        'ADB_board_serial': 0,
        'ADB_board_RevMain': "",
        'ADB_board_RevSub': "",
        'ADB_board_FW': "",
        'rho_probe_ohm': 0.0,
        'DC_offset_voltage_mV': 0.0,
        'gain_stage1': 0.0,
        'gain_stage2': 0.0,
        'gps_clock_status': "",
        'GPS_accuracy': 0,
        'survey_header_filename': "",
        'type_of_meas': "",
        'DCOffsetCorrValue': 0.0,
        'DCOffsetCorrOn': 0,                    # controlls if the DCOffsetCorrValue was on and used
        'InputDivOn': 0,
        'Input': 0,
        'calon': 0,
        'atton': 0,
        'calref': 0,
        'calint': 0,
        'calfreq': 0.0,
        'short_circuit': 0,
        'decimation': 0,
        'ADB_board_type': "",
        'external_gain': 0.0,
        'LF_LP_4Hz': "off",             # php needs at a lower case letter
        'HF_HP_500Hz': "off",
        'HF_HP_1Hz': "off",
        'LF_RF_1': "off",              # radio filters php needs "_"
        'LF_RF_2': "off",
        'LF_RF_3': "off",
        'LF_RF_4': "off",              # switch on for buffer !!
        'MF_RF_1': "off",
        'MF_RF_2': "off",
        'Client': "",                   # values below can be taken if wanted
        'Contractor': "",
        'Area': "",
        'SurveyID': "",
        'Operator': "",
        'SiteName': "",
        'Line': "",
        'XmlHeader': "",
        'Comments': "",
        'SiteNameRR': "",
        'SiteNameEMAP': "",
    }

    return sys
