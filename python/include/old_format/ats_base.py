import struct
## below needs a replacement - can not be installed on some systems
import pycstruct
import math

"""@package ats_base
Contains the interface for the ats binary


# if(is07 || is08) LFFilters["LF-RF-1"] =     0x01;   //! ADU07/8_LF-RF-1 filter on LF board with capacitor 22pF
# if(is07 || is08) LFFilters["LF-RF-2"] =     0x02;   //! ADU07/8_LF-RF-2 filter on LF board with capacitor 122pF
# if (is07)        LFFilters["LF-RF-3"] =     0x04;   //! ADU07_LF-RF-3 filter on LF board with capacitor 242pF
# if (is07)        LFFilters["LF-RF-4"] =     0x08;   //! ADU07_LF-RF-4 filter on LF board with capacitor 342pF
# if(is07 || is08) LFFilters["LF_LP_4HZ"] =   0x10;   //! ADU07/8_LF_LP_4HZ filter on LF board with 4 Hz Lowpass characteristic
# if (is07)        LFFilters["MF-RF-1"] =     0x40;   //! ADU07_MF_RF_1 filter on MF board with capacitor 470nF
# if (is07)        LFFilters["MF-RF-2"] =     0x20;   //! ADU07_MF_RF_2 filter on MF board with capacitor 4.7nF
# // HF Path
# // 1 Hz has been dropped for 08e but is on 07e
# if (is07)        HFFilters["HF-HP-1Hz"] =   0x01;   //! ADU07_HF-HP-1Hz 1Hz filter enable for HF board
# // 500Hz is the HP for 08
# if (is08)        HFFilters["HF-HP-500Hz"] = 0x02;   //! ADU08_HF-HP-500Hz 500Hz filter enable for HF board

"""
C_ATS_CEA_NUM_HEADERS = 1023  # amount of sliced heders for CEA Observatory


def create_ats_slice_header():
    """ creates a slice, 1023 are needed """
    ats_slice = {
        'samples': 0,
        'start': 0,
        'DCOffsetCorrValue': 0.0,
        'gain_stage1': 0.0,
        'gain_stage2': 0.0,
        'DCOffsetCorrOn': 0,
        # 'emptyrs': "",
        'g0': 0,
        'g1': 0,
        'g2': 0,
        'g3': 0,
        'g4': 0,
        'g5': 0,
        'g6': 0,
    }
    return ats_slice


def create_bin_ats_slice_header():
    """ creates a template for reading the binary ats header of CEA """
    b_ats_slice_header = pycstruct.StructDef()
    b_ats_slice_header.add('uint32',  'samples')
    b_ats_slice_header.add('uint32',  'start')
    b_ats_slice_header.add('float64', 'DCOffsetCorrValue')
    b_ats_slice_header.add('float32', 'gain_stage1')
    b_ats_slice_header.add('float32', 'gain_stage2')
    b_ats_slice_header.add('int8',    'DCOffsetCorrOn')
    # b_ats_slice_header.add('utf-8',   'emptycea', length=7)
    # seems to garbarge
    b_ats_slice_header.add('int8',    'g0')
    b_ats_slice_header.add('int8',    'g1')
    b_ats_slice_header.add('int8',    'g2')
    b_ats_slice_header.add('int8',    'g3')
    b_ats_slice_header.add('int8',    'g4')
    b_ats_slice_header.add('int8',    'g5')
    b_ats_slice_header.add('int8',    'g6')

    return b_ats_slice_header


def create_atsheader():
    """ creates a dictionary according to the latest version of the binary C/C++ header """
    header = {
        'header_length': 0,
        'header_version': 0,
        'samples': 0,
        'sample_rate': 0.0,
        'start': 0,
        'lsbval': 0.0,
        'GMToffset': 0,
        'orig_sample_rate': 0.0,
        'serial_number': 0,
        'serial_number_ADC_board': 0,
        'channel_number': 0,
        'chopper': 0,
        'channel_type': "",
        'sensor_type': "",
        'sensor_serial_number': 0,
        'x1': 0.0,
        'y1': 0.0,
        'z1': 0.0,
        'x2': 0.0,
        'y2': 0.0,
        'z2': 0.0,
        'dipole_length': 0.0,
        'azimuth': 0.0,
        'rho_probe_ohm': 0.0,
        'DC_offset_voltage_mV': 0.0,
        'gain_stage1': 0.0,
        'gain_stage2': 0.0,
        'iLat_ms': 0,
        'iLong_ms': 0,
        'iElev_cm': 0,
        'Lat_Long_TYPE': "",
        'coordinate_type': "",
        'ref_meridian': 0,
        'Northing': 0.0,
        'Easting': 0.0,
        'gps_clock_status': "",
        'GPS_accuracy': 0,
        'offset_UTC': 0,
        'SystemType': "",
        'survey_header_filename': "",
        'type_of_meas': "",
        'DCOffsetCorrValue': 0.0,
        'DCOffsetCorrOn': 0,
        'InputDivOn': 0,
        'bit_indicator': 0,

        'result_selftest': "",
        'numslices': 0,

        'cal_freqs': 0,
        'cal_entry_length': 0,
        'cal_version': 0,
        'cal_start_address': 0,
        'LF_filters': 0,
        'emptylf': "",

        'UTMZone': "",
        'system_cal_datetime': 0,
        'sensor_cal_filename': "",
        'sensor_cal_datetime': 0,
        'powerline1': 0.0,
        'powerline2': 0.0,

        'HF_filters': 0,
        'emptyhf': "",

        'samples_64bit': 0,
        'external_gain': 0.0,
        'ADB_board_type': "",
        'Client': "",
        'Contractor': "",
        'Area': "",
        'SurveyID': "",
        'Operator': "",
        'SiteName': "",
        'XmlHeader': "",
        'Comments': "",
        'SiteNameRR': "",
        'SiteNameEMAP': "",
    }
    return header


# ##################################################################################################################


def create_bin_atsheader():
    """ creates a template for reading the binary ats header """
    b_ats_header = pycstruct.StructDef()
    b_ats_header.add('uint16',  'header_length')
    b_ats_header.add('int16',   'header_version')
    b_ats_header.add('uint32',  'samples')
    b_ats_header.add('float32', 'sample_rate')
    b_ats_header.add('uint32',  'start')
    b_ats_header.add('float64', 'lsbval')
    b_ats_header.add('int32',   'GMToffset')
    b_ats_header.add('float32', 'orig_sample_rate')
    b_ats_header.add('uint16',  'serial_number')
    b_ats_header.add('uint16',  'serial_number_ADC_board')
    b_ats_header.add('uint8',   'channel_number')
    b_ats_header.add('uint8',   'chopper')
    b_ats_header.add('utf-8',   'channel_type', length=2)
    b_ats_header.add('utf-8',   'sensor_type', length=6)
    b_ats_header.add('int16',   'sensor_serial_number')
    b_ats_header.add('float32', 'x1')
    b_ats_header.add('float32', 'y1')
    b_ats_header.add('float32', 'z1')
    b_ats_header.add('float32', 'x2')
    b_ats_header.add('float32', 'y2')
    b_ats_header.add('float32', 'z2')
    b_ats_header.add('float32', 'dipole_length')
    b_ats_header.add('float32', 'azimuth')
    b_ats_header.add('float32', 'rho_probe_ohm')
    b_ats_header.add('float32', 'DC_offset_voltage_mV')
    b_ats_header.add('float32', 'gain_stage1')
    b_ats_header.add('float32', 'gain_stage2')
    b_ats_header.add('int32',   'iLat_ms')
    b_ats_header.add('int32',   'iLong_ms')
    b_ats_header.add('int32',   'iElev_cm')
    b_ats_header.add('utf-8',   'Lat_Long_TYPE', length=1)
    b_ats_header.add('utf-8',   'coordinate_type', length=1)
    b_ats_header.add('int16',   'ref_meridian')
    b_ats_header.add('float64', 'Northing')
    b_ats_header.add('float64', 'Easting')
    b_ats_header.add('utf-8',   'gps_clock_status', length=1)
    b_ats_header.add('int8',    'GPS_accuracy')
    b_ats_header.add('int16',   'offset_UTC')
    b_ats_header.add('utf-8',   'SystemType', length=12)
    b_ats_header.add('utf-8',   'survey_header_filename', length=12)
    b_ats_header.add('utf-8',   'type_of_meas', length=4)
    b_ats_header.add('float64', 'DCOffsetCorrValue')
    b_ats_header.add('int8',    'DCOffsetCorrOn')
    b_ats_header.add('int8',    'InputDivOn')
    b_ats_header.add('int16',   'bit_indicator')
    b_ats_header.add('utf-8',   'result_selftest', length=2)
    b_ats_header.add('uint16',  'numslices')
    b_ats_header.add('int16',   'cal_freqs')
    b_ats_header.add('int16',   'cal_entry_length')
    b_ats_header.add('int16',   'cal_version')
    b_ats_header.add('int16',   'cal_start_address')
    b_ats_header.add('int8',    'LF_filters')
    b_ats_header.add('utf-8',   'emptylf', length=7)
    b_ats_header.add('utf-8',   'UTMZone', length=12)
    b_ats_header.add('uint32',  'system_cal_datetime')
    b_ats_header.add('utf-8',   'sensor_cal_filename', length=12)
    b_ats_header.add('uint32',  'sensor_cal_datetime')
    b_ats_header.add('float32', 'powerline1')
    b_ats_header.add('float32', 'powerline2')
    b_ats_header.add('int8',    'HF_filters')
    b_ats_header.add('utf-8',   'emptyhf', length=7)
    b_ats_header.add('uint64',  'samples_64bit')
    b_ats_header.add('float32', 'external_gain')
    b_ats_header.add('utf-8',   'ADB_board_type', length=4)
    b_ats_header.add('utf-8',   'Client', length=16)
    b_ats_header.add('utf-8',   'Contractor', length=16)
    b_ats_header.add('utf-8',   'Area', length=16)
    b_ats_header.add('utf-8',   'SurveyID', length=16)
    b_ats_header.add('utf-8',   'Operator', length=16)
    # the following entries are really make use of UTF-8 characters - where the above are used in EDI and never used full UTF-8
    b_ats_header.add('utf-8',   'SiteName', length=112)
    b_ats_header.add('utf-8',   'XmlHeader', length=64)
    b_ats_header.add('utf-8',   'Comments', length=288)
    b_ats_header.add('utf-8',   'SiteNameRR', length=112)
    b_ats_header.add('utf-8',   'SiteNameEMAP', length=112)
    return b_ats_header


# ##################################################################################################################


def read_atsheader(filename):
    """ read the binary header of an ats file """
    b_header = create_bin_atsheader()
    slice_headers = {}                      # almost never used, but defined
    try:
        with open(filename, 'rb') as f:
            b_length = f.read(2)
            b_version = f.read(2)
            (length,) = struct.unpack('<H', b_length)
            (version,) = struct.unpack('<h', b_version)
            f.seek(0)
            if version >= 1080:
                inbytes = f.read(1024)      # sliced used first full header with 1024
            else:
                inbytes = f.read(length)    # may end up somewhere if header is 80,81 and size differs from 1024
            header = b_header.deserialize(inbytes)
            if header['numslices'] == 1:
                b_slice = create_bin_ats_slice_header()
                inslice = f.read(32)
                slice_header = b_slice.deserialize(inslice)
                header['samples'] = slice_header['samples']
                header['start'] = slice_header['start']
                # that is not sure for a single header
                # header['DCOffsetCorrValue'] = slice_header['DCOffsetCorrValue']
                # header['gain_stage1'] = slice_header['gain_stage1']
                # header['gain_stage2'] = slice_header['gain_stage2']
                # header['DCOffsetCorrOn'] = slice_header['DCOffsetCorrOn']

            elif header['numslices'] > 1:

                for i in range(header['numslices']):
                    b_slice = create_bin_ats_slice_header()
                    inslice = f.read(32)
                    slice_header = b_slice.deserialize(inslice)
                    slice_headers.append(slice_header)

    except Exception:
        raise Exception(f"unable to read header of ats file: {filename}")
    f.close()
    # some corrections
    # remove not printable chars
    header.pop('emptyhf', None)
    header.pop('emptylf', None)

    # evaluate filters
    lffilters = {
        'LF-RF-1': 1,
        'LF-RF-2': 2,
        'LF-RF-3': 4,
        'LF-RF-4': 8,
        'LF_LP_4HZ': 16,
        'MF-RF-1': 64,
        'MF-RF-2': 32,
    }

    if ((header['ADB_board_type'] == "LF") or (header['ADB_board_type'] == "MF")):
        tmp = header['LF_filters']
        if (tmp > 16):
            header['LF_LP_4HZ'] = "on"
            tmp -= 16
        for key, value in lffilters.items():
            # print(key, "  ", value)
            if (value == tmp):
                # print("found")
                header[key] = "on"
        header.pop('LF_filters', None)
        header.pop('HF_filters', None)

    hffilters = {
        'HF-HP-1Hz': 1,         # is default on ADU-07e HF board
        'HF-HP-500Hz': 2,       # is default on ADU-08 BB board in HF mode
    }

    if ((header['ADB_board_type'] == "HF") or (header['ADB_board_type'] == "BB")):
        for key, value in hffilters.items():
            # print(key, "  ", value)
            if (value == header["HF_filters"]):
                # print("found")
                header[key] = "on"
        header.pop('HF_filters', None)
        header.pop('LF_filters', None)

    # make UTF-8 for access and json
    # hence some Geosystem programmers used " " instead of "\x0" for filling
    # try it least to strip
    hffilters.clear()
    lffilters.clear()
    header['channel_type'] = header['channel_type'].strip()
    header['sensor_type'] = header['sensor_type'].strip()

    header['Lat_Long_TYPE'] = header['Lat_Long_TYPE'].strip()
    header['coordinate_type'] = header['coordinate_type'].strip()

    header['gps_clock_status'] = header['gps_clock_status'].strip()
    header['SystemType'] = header['SystemType'].strip()
    header['survey_header_filename'] = header['survey_header_filename'].strip()
    header['type_of_meas'] = header['type_of_meas'].strip()

    header['UTMZone'] = header['UTMZone'].strip()
    header['result_selftest'] = header['result_selftest'].strip()

    header['sensor_cal_filename'] = header['sensor_cal_filename'].strip()

    header['ADB_board_type'] = header['ADB_board_type'].strip()
    header['Client'] = header['Client'].strip()
    header['Contractor'] = header['Contractor'].strip()
    header['Area'] = header['Area'].strip()
    header['SurveyID'] = header['SurveyID'].strip()
    header['Operator'] = header['Operator'].strip()
    header['SiteName'] = header['SiteName'].strip()
    header['XmlHeader'] = header['XmlHeader'].strip()
    header['Comments'] = header['Comments'].strip()
    # never used - clients put anything
    header['Comments'] = header['Comments'].replace("weather:", "", 1).lstrip()
    header['SiteNameRR'] = header['SiteNameRR'].strip()
    header['SiteNameEMAP'] = header['SiteNameEMAP'].strip()

    # old headers - maybe ADU-06
    if header['header_version'] < 80:
        header['DCOffsetCorrOn'] = 0
        header['DCOffsetCorrValue'] = 0.0
        header['InputDivOn'] = 0
        header['orig_sample_rate'] = 0.0

    return header, slice_headers


# ##################################################################################################################


def write_atsheader(atsheader, filename, keep_open):
    new_header = create_bin_atsheader()
    buffer = new_header.serialize(atsheader)
    f = open(filename, 'wb')
    f.write(buffer)
    if keep_open is False:
        f.close()


def atss_file_from_atsfile(ats_filename, atss_channel, atss_filename):
    lsb = atss_channel['lsb']
    units = atss_channel['units']
    # avoid rounding errors
    if (atss_channel['dipole_length']) > 0.001 and (atss_channel['units'] == "mV") and (atss_channel['channel_type'][0] == "E"):
        lsb *= (1000.0/atss_channel['dipole_length'])
        units = "mV/km"

    # create a file with doubles for direct access
    try:
        fo = open(atss_filename, 'wb')
    except IOError:
        raise Exception(f'unable to open file for writing: {atss_filename}')
    try:
        with open(ats_filename, 'rb') as f:
            b_length = f.read(2)
            (length,) = struct.unpack('<H', b_length)
            f.seek(0)
            f.seek(length)
            # b_header = f.read(length)
            # print("header length is", length)
            while (byte := f.read(4)):                      # ats file has 32 bits
                (ints,) = struct.unpack('<i', byte)
                # dat = int.from_bytes(byte, byteorder='little')
                data = lsb * ints
                # s = struct.pack('f'*len(data), data)
                fo.write(struct.pack('d', data))

            fo.close()
            f.close()

    except Exception:
        raise Exception(f'unable to read header of ats file: {ats_filename}')

    return units

def aduboard_from_sample_rate(sample_rate):
    # the data processing does not make use of the board
    # the LF, MF and HF filters are only informal, so a fake board is ok
    sname = "failed"
    if sample_rate > 4096:
        sname = "H"
    else:
        sname = "L"
    return sname


def dip_to_pos(length, azimuth, tilt):
    pos = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
    dp = tilt
    if math.abs(length) < 0.0001:
        return pos
    if math.abs(tilt) < 0.1:
        dp = 0.0
    x = length * math.cos(math.pi / 180. * azimuth) * math.cos(math.pi / 180. * dp)
    y = length * math.sin(math.pi / 180. * azimuth) * math.cos(math.pi / 180. * dp)
    z = length * math.sin(math.pi / 180. * dp)

    pos[0] = -0.5 * x
    pos[1] = 0.5 * x
    pos[2] = -0.5 * y
    pos[3] = 0.5 * y
    pos[4] = 0.0
    pos[5] = z

    return pos

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
    chan['elevation'] = oldheader['iElev_cm'] / 100.
    # since 15 years we do use pos
    p = pos_to_tilt(oldheader['x1'], oldheader['x2'], oldheader['y1'], oldheader['y2'], oldheader['z1'], oldheader['z2'])
    chan['dipole_length'] = p[0]
    chan['azimuth'] = p[1]
    chan['tilt'] = p[2]
    chan['resistance'] = oldheader['rho_probe_ohm']
    # ADU uses mV without mentioning it
    chan['units'] = "mV"          # H, E -> change that if you scale E to mV/km
    if 'source' in oldheader:
        chan['source'] = oldheader['source']
    else:
        chan['source'] = ""
    if 'filter' in oldheader:
        chan['filter'] = oldheader['filter']
    else:
        chan['filter'] = ""
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

