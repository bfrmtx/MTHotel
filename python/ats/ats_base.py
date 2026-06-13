import struct
import math
from datetime import datetime, timezone
from pathlib import Path

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

# ##################################################################################################################
# Binary layout of the ats header.
#
# The whole header is exactly 1024 bytes, little endian and packed (no padding).
# If numslices is not zero, that many ats slice headers (32 bytes each) follow
# directly after the 1024 byte header.
#
# Each entry is (name, struct_code) and is given in the SAME order as it appears
# in the binary structure - do NOT reorder.
# String fields use the "<n>s" struct code (fixed number of bytes).
# ##################################################################################################################

# little endian, standard sizes, no alignment padding
_BYTE_ORDER = "<"

ATS_HEADER_FIELDS = [
    ('header_length', 'H'),
    ('header_version', 'h'),
    ('samples', 'I'),
    ('sample_rate', 'f'),
    ('start', 'I'),
    ('lsbval', 'd'),
    ('GMToffset', 'i'),
    ('orig_sample_rate', 'f'),
    ('serial_number', 'H'),
    ('serial_number_ADC_board', 'H'),
    ('channel_number', 'B'),
    ('chopper', 'B'),
    ('channel_type', '2s'),
    ('sensor_type', '6s'),
    ('sensor_serial_number', 'h'),
    ('x1', 'f'),
    ('y1', 'f'),
    ('z1', 'f'),
    ('x2', 'f'),
    ('y2', 'f'),
    ('z2', 'f'),
    ('dipole_length', 'f'),
    ('azimuth', 'f'),
    ('rho_probe_ohm', 'f'),
    ('DC_offset_voltage_mV', 'f'),
    ('gain_stage1', 'f'),
    ('gain_stage2', 'f'),
    ('iLat_ms', 'i'),
    ('iLong_ms', 'i'),
    ('iElev_cm', 'i'),
    ('Lat_Long_TYPE', '1s'),
    ('coordinate_type', '1s'),
    ('ref_meridian', 'h'),
    ('Northing', 'd'),
    ('Easting', 'd'),
    ('gps_clock_status', '1s'),
    ('GPS_accuracy', 'b'),
    ('offset_UTC', 'h'),
    ('SystemType', '12s'),
    ('survey_header_filename', '12s'),
    ('type_of_meas', '4s'),
    ('DCOffsetCorrValue', 'd'),
    ('DCOffsetCorrOn', 'b'),
    ('InputDivOn', 'b'),
    ('bit_indicator', 'h'),
    ('result_selftest', '2s'),
    ('numslices', 'H'),
    ('cal_freqs', 'h'),
    ('cal_entry_length', 'h'),
    ('cal_version', 'h'),
    ('cal_start_address', 'h'),
    ('LF_filters', 'b'),
    ('emptylf', '7s'),
    ('UTMZone', '12s'),
    ('system_cal_datetime', 'I'),
    ('sensor_cal_filename', '12s'),
    ('sensor_cal_datetime', 'I'),
    ('powerline1', 'f'),
    ('powerline2', 'f'),
    ('HF_filters', 'b'),
    ('emptyhf', '7s'),
    ('samples_64bit', 'Q'),
    ('external_gain', 'f'),
    ('ADB_board_type', '4s'),
    ('Client', '16s'),
    ('Contractor', '16s'),
    ('Area', '16s'),
    ('SurveyID', '16s'),
    ('Operator', '16s'),
    # the following entries really make use of UTF-8 characters - where the
    # above are used in EDI and never used full UTF-8
    ('SiteName', '112s'),
    ('XmlHeader', '64s'),
    ('Comments', '288s'),
    ('SiteNameRR', '112s'),
    ('SiteNameEMAP', '112s'),
]

# slice header (32 bytes) - numslices of these follow the 1024 byte header
ATS_SLICE_FIELDS = [
    ('samples', 'I'),
    ('start', 'I'),
    ('DCOffsetCorrValue', 'd'),
    ('gain_stage1', 'f'),
    ('gain_stage2', 'f'),
    ('DCOffsetCorrOn', 'b'),
    # 'emptycea' (7 bytes) seems to be garbage and is read as g0..g6 below
    ('g0', 'b'),
    ('g1', 'b'),
    ('g2', 'b'),
    ('g3', 'b'),
    ('g4', 'b'),
    ('g5', 'b'),
    ('g6', 'b'),
]


def _layout_format(fields):
    """ build the struct format string for an ordered list of fields """
    return _BYTE_ORDER + "".join(code for _, code in fields)


def _default_value(code):
    """ default value for a field: "" for strings, 0.0 for floats, 0 for integers """
    if code.endswith('s'):
        return ""
    if code in ('f', 'd'):
        return 0.0
    return 0


def _decode_string(raw):
    """ decode a fixed length byte field to str, cut at the first NUL byte """
    return raw.split(b"\x00", 1)[0].decode("utf-8", errors="replace")


def _unpack_fields(fields, data):
    """ unpack a binary buffer into a dict according to the ordered fields """
    values = struct.unpack(_layout_format(fields), data)
    header = {}
    for (name, code), value in zip(fields, values):
        if code.endswith('s'):
            value = _decode_string(value)
        header[name] = value
    return header


def _pack_fields(fields, header):
    """ pack a dict into a binary buffer according to the ordered fields

    struct truncates / NUL pads string fields to their fixed length automatically.
    """
    values = []
    for name, code in fields:
        value = header.get(name, _default_value(code))
        if code.endswith('s') and isinstance(value, str):
            value = value.encode("utf-8")
        values.append(value)
    return struct.pack(_layout_format(fields), *values)


# total size of the header in bytes (must be 1024)
ATS_HEADER_SIZE = struct.calcsize(_layout_format(ATS_HEADER_FIELDS))
# size of a single slice header in bytes (32)
ATS_SLICE_SIZE = struct.calcsize(_layout_format(ATS_SLICE_FIELDS))


def create_ats_slice_header():
    """ creates a slice header dictionary; numslices of these follow the header """
    return {name: _default_value(code) for name, code in ATS_SLICE_FIELDS}


def create_atsheader():
    """ creates a dictionary according to the latest version of the binary C/C++ header """
    return {name: _default_value(code) for name, code in ATS_HEADER_FIELDS}


# ##################################################################################################################


def read_atsheader(filename):
    """ read the binary 1024 byte header of an ats file

    Returns a tuple (header, slice_headers).
    If numslices is not zero, that many ats slice headers (32 bytes each)
    follow the 1024 byte header and are returned in the slice_headers list.
    """
    slice_headers = []                      # filled only if numslices is not zero
    try:
        with open(filename, 'rb') as f:
            inbytes = f.read(ATS_HEADER_SIZE)
            if len(inbytes) < ATS_HEADER_SIZE:
                raise Exception("file is shorter than the 1024 byte header")
            header = _unpack_fields(ATS_HEADER_FIELDS, inbytes)

            numslices = header['numslices']
            for _ in range(numslices):
                inslice = f.read(ATS_SLICE_SIZE)
                if len(inslice) < ATS_SLICE_SIZE:
                    break
                slice_headers.append(_unpack_fields(ATS_SLICE_FIELDS, inslice))

            # a single slice carries the real sample count and start time
            if numslices == 1 and slice_headers:
                header['samples'] = slice_headers[0]['samples']
                header['start'] = slice_headers[0]['start']
                # that is not sure for a single header
                # header['DCOffsetCorrValue'] = slice_headers[0]['DCOffsetCorrValue']
                # header['gain_stage1'] = slice_headers[0]['gain_stage1']
                # header['gain_stage2'] = slice_headers[0]['gain_stage2']
                # header['DCOffsetCorrOn'] = slice_headers[0]['DCOffsetCorrOn']
    except Exception:
        raise Exception(f"unable to read header of ats file: {filename}")
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
    """ write the 1024 byte binary ats header """
    buffer = _pack_fields(ATS_HEADER_FIELDS, atsheader)
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
    if abs(length) < 0.0001:
        return pos
    if abs(tilt) < 0.1:
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
    from atss.json_header import channel, pos_to_tilt
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

def binary_ats_samples(path: str | Path ) -> int:
    # we get the file size in bytes
    size = Path(path).stat().st_size
    # the header is 1024 bytes, the rest is data with 4 bytes per sample
    if size < 1024:
        raise Exception(f"file is shorter than the 1024 byte header: {path}")
    data_size = size - 1024
    if data_size % 4 != 0:
        raise Exception(f"data size is not a multiple of 4 bytes: {path}")
    return data_size // 4


