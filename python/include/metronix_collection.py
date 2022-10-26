# -*- coding: utf-8 -*-
"""
Metronix file collection

Created on Thu Aug  4 16:48:47 2022

@author: hlarniergeo,
"""

# =============================================================================
# Imports
# =============================================================================

import datetime
import json
import math
import numpy as np
import os
import pandas as pd
import pycstruct
import struct
import sys
from mth5.io import Collection

# =============================================================================
# General functions related to Metronix that can be used for any of the classes
# =============================================================================


def _sample_rate_to_string(sample_rate):
    """
        Converts the sample rate from a float to a string depending on whether it is
        above or below 1 Hz (Hz or seconds) and adjust the suffix accordingly.

    :param sample_rate: float: sample rate in Hz
    :return: sample_rate_string: str:
    """

    try:
        float(sample_rate)
    except TypeError:
        sys.exit(1)

    if sample_rate > 0.99:
        i = int(round(sample_rate, 0))
        sample_rate_string = str(i) + "Hz"
    else:
        d = 1.0 / sample_rate
        i = int(round(d, 0))
        sample_rate_string = str(i) + "s"

    return sample_rate_string


def _aduboard_from_sample_rate(sample_rate: float):
    """
        Converts the sample rate from a float to a Metronix board type depending on sample rate value

    :param sample_rate: float: sample rate in Hz
    :return: board_name: str:
    """

    if sample_rate > 4096:
        board_name = "H"
    else:
        board_name = "L"

    return board_name


def _cal_mfs06e(frequency, spectra, chopper):
    """

    :param i: DEPRECATED: has been removed from BF original functions
    :param frequency: float: frequency value in Hz
    :param spectra: float: spectra value in signal units
    :param chopper: int: 0 (off or unknown), 1 for on
    :return:

    Comment from B.Friedrichs:
    # we stay with V/(nT*Hz) because we have millions of old files and this format is inside the coils
    # this formula doe NOT normalize by f - because the next step is naturally all against the spectra
    # as we do here inside

    """

    if frequency == 0:
        return spectra

    if chopper == 1:
        p1 = complex(0.0, (frequency / 4.))
        p2 = complex(0.0, (frequency / 8192.))
        p4 = complex(0.0, (frequency / 28300.0))
        trf = 0.8 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (1. / (1. + p4)))

        return (spectra / trf) / 1000.  # cal is in V, data in mV
    else:
        p1 = complex(0.0, (frequency / 4.))
        p2 = complex(0.0, (frequency / 8192.))
        p3 = complex(0.0, (frequency / 0.72))
        p4 = complex(0.0, (frequency / 25000.0))
        trf = 0.8 * ((p1 / (1. + p1)) * (1. / (1. + p2)) * (p3 / (1. + p3)) * (1. / (1. + p4)))

        return (spectra / trf) / 1000.  # cal is in V, data in mV


def _pos_to_dip(x1, x2, y1, y2, z1, z2):
    """
        Calculates the dip angle based on the coordinates in the metadata

    :param x1: south negative coordinate
    :param x2: north positive coordinate
    :param y1: west negative coordinate
    :param y2: east negative coordinate
    :param z1: bottom negative
    :param z2: top positive
    :return: length: float: length of the dipole
    :return: angle: horizontal angle of the dipole (compared to NSEW geographic)
    :return: dip: angle of the dipole compared to horizontal plane
    """

    # Calculating distances
    tx = x2 - x1
    ty = y2 - y1
    tz = z2 - z1
    length = np.linalg.norm([tx, ty, tz])  # math.sqrt(tx * tx + ty * ty + tz * tz)

    # Checking if length is non-zero
    if length < 0.001:
        length = 0.0
        angle = 0.0
        dip = 0.0
    else:
        angle = 180. / math.pi * math.atan2(ty, tx)
        dip = 180. / math.pi * (90 - math.acos(tz/length))

    return length, angle, dip


def _dip_to_pos(length, angle, dip):
    """
        Calculates the location of the sensor based on the length, angle and dip of the dipole

    :param: length: float: length of the dipole
    :param: angle: horizontal angle of the dipole (compared to NSEW geographic)
    :param: dip: angle of the dipole compared to horizontal plane

    :return pos: list of horizontal positions
    """

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


class AtssMetronix:
    """


    """

    def __init__(self, dict_fn=None, dict_header=None, dict_calibration=None, **kwargs):

        if dict_fn is not None:
            if type(dict_fn) is dict:
                self.dict_fn = dict_fn
                self.validate_atss_file_dictionary(self.dict_fn)
            else:
                sys.exit(1)
        else:
            self.dict_fn, _ = self.atss_file()

        if dict_header is not None:
            if type(dict_header) is dict:
                self.dict_header = dict_header
                self.validate_atss_header(self.dict_header)
            else:
                sys.exit(1)
        else:
            self.dict_header, _ = self.atss_header()

        if dict_calibration is not None:
            if type(dict_calibration) is dict:
                self.dict_calibration = dict_calibration
                self.validate_atss_calibration(self.dict_calibration)
            else:
                sys.exit(1)
        else:
            self.dict_calibration = self.atss_calibration()

        self.channel = None

    @staticmethod
    def atss_file():
        """
            Creates a dictionary template corresponding to the atss file name. The order for the dictionary is the
            file name.

            Rules for dictionary keys:
                serial: integer (such as 1234) for the system, no negative number
                system: str, such as ADU-08e
                channel_no: integer
                channel_type: str, such as Ex, Ey, Hx, Hy, Hz
                                   or Jx, Jy, Jz
                                   or Pitch, Roll, Yaw
                                   or x, y, z or T for temperature
                sample_rate: float (units Hz). "Hz" or "s" will be appended while writing in real time
                             The FILENAME contains a UNIT for better readability; you MUST have 256Hz (sample rate 256)
                             or 4s (sample rate 0.25). A "." in the FILENAME is possible on modern systems, 16.6666Hz
                            is possible.

            :return: file_dictionary: dictionary for filename construction
            :return: types_dictionary: dictionary for types for each key

        """

        file_dictionary = {
            'serial': 0,
            'system': "",
            'channel_no': 0,
            'channel_type': "",
            'sample_rate': 0.0,
        }

        types_dictionary = {'serial': int,
                            'system': str,
                            'channel_no': int,
                            'channel_type': str,
                            'sample_rate': float}

        return file_dictionary, types_dictionary

    def validate_atss_file_dictionary(self, file_dictionary):
        """
            Validates the dictionary for the file name based on rules described above.
            Returns an error if a key is missing or with the wrong type to avoid downstream issues

        :return: Exception if issue
        """

        dict_template, dict_types = self.atss_file()            # using the template for future updates
        keys_for_validation = list(dict_template)

        if not type(file_dictionary) is dict:
            return TypeError
        else:
            keys_dictionary = list(file_dictionary.keys())
            for key in keys_for_validation:
                if key not in keys_dictionary:
                    print('Missing key', key)
                    return TypeError
                else:
                    if not type(file_dictionary[key]) is dict_types[key]:
                        return TypeError

    @staticmethod
    def atss_header():
        """
            Creates a dictionary template corresponding to the atss header. It contains items needed for a complete
            description together with calibration data. It does NOT contains any values from file name.

            # FORBIDDEN STRINGS: String like 'MY_Station' with underscores, because it might be interpreted as
            separator

            Rules for dictionary keys:
                datetime: datetime.datetime object in UTC
                latitude: float, decimal degree such as 52.2443
                longitude: float, decimal degree such as 10.5594
                elevation: float, elevation in meter
                angle: float, orientation from North to East (90 = East, -90 or 270 = West, 180 South, 0 North)
                dip: float: angle positive down - in case it had been measured
                units: string: "mV", for ADUs it will be mV, H or other - or scaled E mV/km
                source: string: empty or indicate as, ns, ca, cp, tx or what ever ?????


            :return: header_dictionary: dictionary for filename header metadata
            :return: types_dictionary: dictionary for types for each key

        """

        header_dictionary = {
            'datetime': "1970-01-01T00:00:00.0",
            'latitude': 0.0,
            'longitude': 0.0,
            'elevation': 0.0,
            'angle': 0.0,
            'dip': 0.0,
            'units': "mV",
            'source': "",
        }

        types_dictionary = {
                            'datetime': datetime.datetime,
                            'latitude': float,
                            'longitude': float,
                            'elevation': float,
                            'angle': float,
                            'dip': float,
                            'units': str,
                            'source': str,
        }

        return header_dictionary, types_dictionary

    def validate_atss_header(self, header_dictionary):
        """
            Validates the dictionary for the file header based on rules described above.
            Returns an error if a key is missing or wrong to avoid downstream issues

        :return: Exception if issue
        """

        dict_template, dict_types = self.atss_header()     # using the template for future updates
        keys_for_validation = list(dict_template)

        if not type(header_dictionary) is dict:
            return TypeError
        else:
            keys_dictionary = list(header_dictionary.keys())
            for key in keys_for_validation:
                if key not in keys_dictionary:
                    print('Missing key', key)
                    return TypeError
                else:
                    if not type(header_dictionary[key]) is dict_types[key]:
                        return TypeError

    @staticmethod
    def atss_calibration():
        """
            Creates a dictionary template corresponding to the atss header. It contains items needed for a complete
            description together with calibration data. It does NOT contains any values from file name.

            # FORBIDDEN STRINGS: String like 'MY_Station' with underscores, because it might be interpretated as
            separator

            Rules for dictionary keys:
                sensor: string to describe the sensor type: e.g. MFS-06e... SHFT-02e
                serial: integer: 1, 2, no negative number
                chopper: integer: 0 for off or unknown, 1 for on
                units_amplitude: string: units for time series and no normalization
                units_frequency: string: x-axis of your calibration e.g. Hz (never seconds)
                units_phase: string: degrees from 0 to 360
                datetime: string: indicating time
                operator: string: The person who made the calibration
                f: numpy.array of frequencies in units as above
                a: numpy.array of amplitude in units as above
                p: numpy.array of phases in units as above

                @HL 2022-10-25:
                    - Should the datetime be in datetime.datetime format for quick transformations?
                    - "operator" key has been switched to all lowercase for PEP8 consistency. Can be adjusted
                        on the fly for C operations (Comment from BF that "operator" is a keyword in C++).
                    -

            :return: calibration_dict: dictionary for calibration metadata

        """

        calibration_dict = {
            'sensor': "",
            'serial': 0,
            'chopper': 0,
            'units_amplitude': "mV/nT",
            'units_frequency': "Hz",
            'units_phase': "degrees",
            'datetime': "1970-01-01T00:00:00",
            'operator': "",
            'f': [0.0],
            'a': [0.0],
            'p': [0.0],
        }

        return calibration_dict

    def validate_atss_calibration(self, header_dictionary):
        """
            Validates the dictionary for the file header based on rules described above.
            Returns an error if a key is missing or wrong to avoid downstream issues

        :return: Exception if issue
        """

        dict_template, dict_types = self.atss_calibration()     # using the template for future updates
        keys_for_validation = list(dict_template)

        if not type(header_dictionary) is dict:
            return TypeError
        else:
            keys_dictionary = list(header_dictionary.keys())
            for key in keys_for_validation:
                if key not in keys_dictionary:
                    print('Missing key', key)
                    return TypeError
                else:
                    if not type(header_dictionary[key]) is dict_types[key]:
                        return TypeError

    def create_channel(self):
        """
            Fills channel dictionary from the combination of the two dictionary for header and filename

            @HL 2022-10-25:
                - Not sure why the number of samples in the dictionary for the header gets back to 0?

        """

        # self.channel = self.dict_fn | self.dict_header expression not valid in 3.7
        self.channel = {**self.dict_fn, **self.dict_header, 'sensor_calibration': self.dict_calibration}
        self.dict_header['samples'] = 0  #????

    def create_atss_filename(self):
        """
            Returns a filename without the extension by using the dictionary keys in
            The header dictionary keys are NOT in the filename

        :return: filename: str: filename for the .atss file
        """

        file_name = ""

        if self.channel is not None:
            dict_template, _ = self.atss_file()  # using the template for future updates
            array_tags = list(dict_template.keys())  # these are the file keys

            count = -1
            fill = ""
            for tag in array_tags:
                count = count + 1
                if count > 0:
                    fill = "_"
                for key, value in self.channel.items():
                    if tag == key:
                        if tag == "sample_rate":
                            file_name = file_name + fill + _sample_rate_to_string(self.channel['sample_rate'])
                        elif tag == "channel_no":
                            file_name = file_name + fill + "C" + f"{self.channel['channel_no']:03}"
                        elif tag == "channel_type":
                            file_name = file_name + fill + "T" + f"{self.channel['channel_type']}"
                        elif tag == "serial":
                            file_name = file_name + fill + f"{self.channel['serial']:03}"
                        else:
                            file_name = file_name + fill + self.channel[tag]

        return file_name

    def read_atss_filename(self, file_path):
        """
            Fills in the header dictionary from an atss filename (absolute or relative path).
            Will do some basic verifications on the file path to make sure it exists and is atss
            Will also try to pull the 'atss' file and the 'json' file to complete the dictionaries if available

            @HL 2022-10-25:
                - Could derive a validation method for json files to confirm that the json file is the proper one
                  to load its content in the dictionary

        """

        # Making sure the path exists
        if not os.path.exists(file_path):
            raise OSError

        # Making sure the path is a file
        if not os.path.isfile(file_path):
            raise OSError

        file_name, extension = os.path.splitext(file_path)
        if not extension == '.atss':
            raise OSError

        file_name = file_name[:]

        tag_name = os.path.basename(file_name)
        tags = tag_name.split("_")

        try:
            self.dict_header["serial"] = int(tags[0])
        except ValueError:
            print('Check if filename is actually for an atss file')
            sys.exit(1)

        self.dict_header["system"] = tags[1]
        tags.pop(0)
        tags.pop(0)

        for tag in tags:
            if tag.startswith('C'):
                tag = tag[1:]
                self.dict_header["channel_no"] = int(tag)
            if tag.startswith('T'):
                tag = tag[1:]
                self.dict_header["channel_type"] = tag
            if tag.endswith('s') and tag[0].isdigit():
                tag = tag[:-1]
                fl = float(tag)
                self.dict_header["sample_rate"] = 1.0 / fl

            if tag.endswith('Hz') and tag[0].isdigit():
                tag = tag[:-2]
                self.dict_header["sample_rate"] = float(tag)

        samples = os.path.getsize(file_path)
        self.dict_header['samples'] = int(samples / 8)

        # read the json file from disk
        if os.path.exists(file_name + ".json"):
            with open(file_name + ".json") as json_file:
                data = json.load(json_file)
                if 'datetime' in data:  # datetime should always be there
                    self.dict_header = self.dict_header | data

    def to_json(self):
        """
            Writing json file from header information
        :return: nothing
        """
        filename = self.create_atss_filename() + ".json"

        print("write: ", filename)
        with open(filename, 'w') as f:
            json_string = json.dumps(self.dict_header, indent=2, sort_keys=False, ensure_ascii=False)
            f.write(json_string)
            f.close()


class AtsMetronix:

    """
        All methods to parse and read ATS file from Metronix

        :param fn: absolute path to ATS file
        :type fn: string

        :param station_name: name of the station
        :type station_name: string

    """

    def __init__(self, fn=None, station_name=None, **kwargs):

        self.fn = fn
        self.instrument_id = 'Metronix_'
        self.header = None
        self.slice_headers = None

        # Testing path existence
        if not os.path.exists(self.fn):
            # Exception to return non-existent path
            sys.stdout.write("Path does not exists.")
            return

        # Testing if path is file and an .ats
        if not os.path.isfile(self.fn):
            # Exception to return path being a file
            sys.stdout.write("Path is not a file.")
            return

        file_name, extension = os.path.splitext(self.fn)
        if not extension == '.ats':
            # Exception for file not being a Metronix one
            sys.stdout.write("Path is not an ATS file.")
            return

    @staticmethod
    def create_bin_ats_header():
        """
            creates a template for reading the binary ats header


        """
        b_ats_header = pycstruct.StructDef()
        b_ats_header.add('uint16',  'header_length')
        b_ats_header.add('int16',   'header_version')
        b_ats_header.add('uint32',  'samples')
        b_ats_header.add('float32', 'sample_rate')
        b_ats_header.add('uint32',  'start')
        b_ats_header.add('float64', 'lsbval')
        b_ats_header.add('int32',   'GMT_offset')
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
        b_ats_header.add('float32', 'angle')
        b_ats_header.add('float32', 'rho_probe_ohm')
        b_ats_header.add('float32', 'DC_offset_voltage_mV')
        b_ats_header.add('float32', 'gain_stage1')
        b_ats_header.add('float32', 'gain_stage2')
        b_ats_header.add('int32',   'i_lat_ms')
        b_ats_header.add('int32',   'i_long_ms')
        b_ats_header.add('int32',   'i_elev_cm')
        b_ats_header.add('utf-8',   'lat_long_type', length=1)
        b_ats_header.add('utf-8',   'coordinate_type', length=1)
        b_ats_header.add('int16',   'ref_meridian')
        b_ats_header.add('float64', 'northing')
        b_ats_header.add('float64', 'easting')
        b_ats_header.add('utf-8',   'GPS_clock_status', length=1)
        b_ats_header.add('int8',    'GPS_accuracy')
        b_ats_header.add('int16',   'offset_UTC')
        b_ats_header.add('utf-8',   'system_type', length=12)
        b_ats_header.add('utf-8',   'survey_header_filename', length=12)
        b_ats_header.add('utf-8',   'type_of_meas', length=4)
        b_ats_header.add('float64', 'DCOffsetCorrValue')
        b_ats_header.add('int8',    'DCOffsetCorrOn')
        b_ats_header.add('int8',    'InputDivOn')
        b_ats_header.add('int16',   'bit_indicator')
        b_ats_header.add('utf-8',   'result_selftest', length=2)
        b_ats_header.add('uint16',  'num_slices')
        b_ats_header.add('int16',   'cal_freqs')
        b_ats_header.add('int16',   'cal_entry_length')
        b_ats_header.add('int16',   'cal_version')
        b_ats_header.add('int16',   'cal_start_address')
        b_ats_header.add('int8',    'LF_filters')
        b_ats_header.add('utf-8',   'empty_lf', length=7)
        b_ats_header.add('utf-8',   'UTM_zone', length=12)
        b_ats_header.add('uint32',  'system_cal_datetime')
        b_ats_header.add('utf-8',   'sensor_cal_filename', length=12)
        b_ats_header.add('uint32',  'sensor_cal_datetime')
        b_ats_header.add('float32', 'powerline1')
        b_ats_header.add('float32', 'powerline2')
        b_ats_header.add('int8',    'HF_filters')
        b_ats_header.add('utf-8',   'empty_hf', length=7)
        b_ats_header.add('uint64',  'samples_64bit')
        b_ats_header.add('float32', 'external_gain')
        b_ats_header.add('utf-8',   'ADB_board_type', length=4)
        b_ats_header.add('utf-8',   'client', length=16)
        b_ats_header.add('utf-8',   'contractor', length=16)
        b_ats_header.add('utf-8',   'area', length=16)
        b_ats_header.add('utf-8',   'survey_ID', length=16)
        b_ats_header.add('utf-8',   'operator', length=16)
        # the following entries are really make use of UTF-8 characters - where the above are used in EDI and never used full UTF-8
        b_ats_header.add('utf-8',   'site_name', length=112)
        b_ats_header.add('utf-8',   'XML_header', length=64)
        b_ats_header.add('utf-8',   'comments', length=288)
        b_ats_header.add('utf-8',   'site_name_RR', length=112)
        b_ats_header.add('utf-8',   'site_name_EMAP', length=112)

        return b_ats_header

    @staticmethod
    def create_bin_ats_slice_header():
        """ creates a template for reading the binary ats header of CEA """
        b_ats_slice_header = pycstruct.StructDef()
        b_ats_slice_header.add('uint32', 'samples')
        b_ats_slice_header.add('uint32', 'start')
        b_ats_slice_header.add('float64', 'DCOffsetCorrValue')
        b_ats_slice_header.add('float32', 'gain_stage1')
        b_ats_slice_header.add('float32', 'gain_stage2')
        b_ats_slice_header.add('int8', 'DCOffsetCorrOn')
        # b_ats_slice_header.add('utf-8',   'emptycea', length=7)
        # seems to garbarge
        b_ats_slice_header.add('int8', 'g0')
        b_ats_slice_header.add('int8', 'g1')
        b_ats_slice_header.add('int8', 'g2')
        b_ats_slice_header.add('int8', 'g3')
        b_ats_slice_header.add('int8', 'g4')
        b_ats_slice_header.add('int8', 'g5')
        b_ats_slice_header.add('int8', 'g6')

        return b_ats_slice_header

    def read_metadata(self):
        """
            read the binary header of an ats file
            Will branch out depending on the version of the ats file.

            From B. Friedrichs, modified by H. Larnier
        """
        b_header = self.create_bin_ats_header()

        try:
            with open(self.fn, 'rb') as f:
                b_length = f.read(2)
                b_version = f.read(2)
                (length,) = struct.unpack('<H', b_length)
                (version,) = struct.unpack('<h', b_version)
                f.seek(0)
                if version >= 1080:
                    inbytes = f.read(1024)      # sliced used first full header with 1024
                else:
                    inbytes = f.read(length)    # may end up somewhere if header is 80,81 and size differs from 1024
                self.header = b_header.deserialize(inbytes)
                if self.header['num_slices'] == 1:
                    b_slice = self.create_bin_ats_slice_header()
                    inslice = f.read(32)
                    self.slice_headers = b_slice.deserialize(inslice)
                    self.header['samples'] = self.slice_headers['samples']
                    self.header['start'] = self.slice_headers['start']
                    # that is not sure for a single header
                    # header['DCOffsetCorrValue'] = slice_header['DCOffsetCorrValue']
                    # header['gain_stage1'] = slice_header['gain_stage1']
                    # header['gain_stage2'] = slice_header['gain_stage2']
                    # header['DCOffsetCorrOn'] = slice_header['DCOffsetCorrOn']

                elif self.header['num_slices'] > 1:
                    for i in range(self.header['num_slices']):
                        b_slice = self.create_bin_ats_slice_header()
                        inslice = f.read(32)
                        slice_header = b_slice.deserialize(inslice)
                        self.slice_headers.append(slice_header)

        except Exception:
            raise Exception(f"unable to read header of ats file: {self.fn}")
        f.close()
        # some corrections
        # remove not printable chars
        self.header.pop('empty_hf', None)
        self.header.pop('empty_lf', None)

        # evaluate filters
        lf_filters = {
            'LF-RF-1': 1,
            'LF-RF-2': 2,
            'LF-RF-3': 4,
            'LF-RF-4': 8,
            'LF_LP_4HZ': 16,
            'MF-RF-1': 64,
            'MF-RF-2': 32,
        }

        if (self.header['ADB_board_type'] == "LF") or (self.header['ADB_board_type'] == "MF"):
            tmp = self.header['LF_filters']
            if (tmp > 16):
                self.header['LF_LP_4HZ'] = "on"
                tmp -= 16
            for key, value in lf_filters.items():
                # print(key, "  ", value)
                if (value == tmp):
                    # print("found")
                    self.header[key] = "on"
            self.header.pop('LF_filters', None)
            self.header.pop('HF_filters', None)

        hf_filters = {
            'HF-HP-1Hz': 1,         # is default on ADU-07e HF board
            'HF-HP-500Hz': 2,       # is default on ADU-08 BB board in HF mode
        }

        if ((self.header['ADB_board_type'] == "HF") or (self.header['ADB_board_type'] == "BB")):
            for key, value in hf_filters.items():
                # print(key, "  ", value)
                if (value == self.header["HF_filters"]):
                    # print("found")
                    self.header[key] = "on"
            self.header.pop('HF_filters', None)
            self.header.pop('LF_filters', None)

        # make UTF-8 for access and json
        # hence some Geosystem programmers used " " instead of "\x0" for filling
        # try it least to strip
        hf_filters.clear()
        lf_filters.clear()
        self.header['channel_type'] = self.header['channel_type'].strip()
        self.header['sensor_type'] = self.header['sensor_type'].strip()

        self.header['Lat_Long_TYPE'] = self.header['Lat_Long_TYPE'].strip()
        self.header['coordinate_type'] = self.header['coordinate_type'].strip()

        self.header['gps_clock_status'] = self.header['gps_clock_status'].strip()
        self.header['SystemType'] = self.header['SystemType'].strip()
        self.header['survey_header_filename'] = self.header['survey_header_filename'].strip()
        self.header['type_of_meas'] = self.header['type_of_meas'].strip()

        self.header['UTMZone'] = self.header['UTMZone'].strip()
        self.header['result_selftest'] = self.header['result_selftest'].strip()

        self.header['sensor_cal_filename'] = self.header['sensor_cal_filename'].strip()

        self.header['ADB_board_type'] = self.header['ADB_board_type'].strip()
        self.header['Client'] = self.header['Client'].strip()
        self.header['Contractor'] = self.header['Contractor'].strip()
        self.header['Area'] = self.header['Area'].strip()
        self.header['SurveyID'] = self.header['SurveyID'].strip()
        self.header['Operator'] = self.header['Operator'].strip()
        self.header['SiteName'] = self.header['SiteName'].strip()
        self.header['XmlHeader'] = self.header['XmlHeader'].strip()
        self.header['Comments'] = self.header['Comments'].strip()
        # never used - clients put anything
        self.header['Comments'] = self.header['Comments'].replace("weather:", "", 1).lstrip()
        self.header['SiteNameRR'] = self.header['SiteNameRR'].strip()
        self.header['SiteNameEMAP'] = self.header['SiteNameEMAP'].strip()

        # old headers - maybe ADU-06
        if self.header['header_version'] < 80:
            self.header['DCOffsetCorrOn'] = 0
            self.header['DCOffsetCorrValue'] = 0.0
            self.header['InputDivOn'] = 0
            self.header['orig_sample_rate'] = 0.0

    def _read_metadata(self, folder):
        """
            Reading the metadata from a specific folder (aka. run),
            and return the corresponding dictionary
        """


        pass

    @property
    def file_size(self):
        """size of file in bytes"""
        if self.fn is not None:
            return self.fn.stat().st_size



class MetronixCollection(Collection):

    """
        Collection of Metronix ATS files

        Metronix files are organized by folder, each run being in a specific folders

    """


    def __init__(self, files_path=None, **kwargs):
        super().__init__(file_path=files_path, **kwargs)

        self.extension = '.ats'
        self.file_path = files_path


    def get_folders(self):
        pass

    def read_folder(self, folder):
        pass


    def to_dataframe(self, folder):
        """
            Reading folders and adding runs to dataframe
        """
        pass
