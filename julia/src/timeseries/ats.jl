#= 

/*!
\brief The ATSComments_80 struct for ats header 80,81 and 90 <br>
Some software will automatically convert to higher version<br>
Some fields are external - the user has to set then; examples:<br>
northing, easting, UTM Zone : these can be used for high density grids<br>
*/
struct ATSComments_80 {
    char          achClient        [16];        //!< 000h  UTF-8 storage, used in EDI
    char          achContractor    [16];        //!< 010h  UTF-8 storage, used in EDI
    char          achArea          [16];        //!< 020h  UTF-8 storage, used in EDI
    char          achSurveyID      [16];        //!< 030h  UTF-8 storage, used in EDI
    char          achOperator      [16];        //!< 040h  UTF-8 storage, used in EDI
    char          achSiteName     [112];        //!< 050h  UTF-8 storage, no BOM at the beginning!; WORST case = 28 Chinese/Japanese chars (multibyte chars)
    char          achXmlHeader     [64];        //!< 0C0h  UTF-8 storage  points to the corresponding XML file, containing addtional information for this header; no PATH=XML file is in the same directory as the ats file
    // deleted September 2018 char          achComments   [512];          //!< 100h  UTF-8 storage  comment block starts (comment field can start with "weather:" but has no meaning
    // new September 2018
    char          achComments     [288];        //!< 100h  UTF-8 storage  comment block starts (comment field can start with "weather:" but has no meaning
    char          achSiteNameRR   [112];        //!< 220h  UTF-8 storage, no BOM at the beginning!; WORST case = 28 Chinese/Japanese chars (multibyte chars)
    char          achSiteNameEMAP [112];        //!< 290h  UTF-8 storage, no BOM at the beginning!; WORST case = 28 Chinese/Japanese chars (multibyte chars); INDICATES a default EMAP center station, where we expect the H (magnetic)


};


/*!
\brief The ATSHeader_80 struct<br>
reference is lat and long; additional entries like northing and easting should be empty // not used<br>
these entries can be overwritten by the user in case of high density grids <br>
The site_name (site name, achSiteName, is in the comment field; the site_no, site_num, site_number, site number is nver stored; this is part of external numeration
*/
struct ATSHeader_80 {
    std::uint16_t uiHeaderLength;               //!< 000h  length of header in bytes (default 1024 and  33760 for CEA)
    std::int16_t  siHeaderVers;                 //!< 002h  80 for ats, 81 for 64bit possible / metronix, 1080 for CEA / sliced header

    // This information can be found in the ChannelTS datastructure
    std::uint32_t uiSamples;                    //!< 004h  amount of samples (each is a 32bit / 64bit int INTEL byte order little endian) in the file total of all slices; if uiSamples == UINT32_MAX, uiSamples64bit at address 0F0h will be used instead; uiSamples64bit replaces uiSamples in that case; do not add!
    //!<
    float         rSampleFreq;                  //!< 008h  sampling frequency in Hz
    std::uint32_t uiStartDateTime;              //!< 00Ch  unix TIMESTAMP (some computers will revert that to negative numbers if this number is grater than 32bit signed); 2106-02-07T06:28:14 is the END of this format
    double        dblLSBMV;                     //!< 010h  least significant bit in mV ()
    std::int32_t  iGMTOffset;                   //!< 018h  not used, default 0; can be used as "UTC to GMT"
    float         rOrigSampleFreq;              //!< 01Ch  sampling frequency in Hz as ORIGINALLY recorded; this value should NOT change (for example after filtering)

    //The required data could probably found in the HardwareConfig
    std::uint16_t uiADUSerNum;                  //!< 020h  Serial number of the system (logger)
    std::uint16_t uiADCSerNum;                  //!< 022h  Serial number of the ADC board (ADB)
    std::uint8_t  uiChanNo;                     //!< 024h  Channel number
    std::uint8_t  uiChopper;                    //!< 025h  Chopper On (1) / Off (0); e.g. chopper is on for 512Hz and lower

    // Data from XML Job-specification
    char          achChanType   [2];            //!< 026h  (Ex, Ey, Ez, Hx, Hy, Hz, Jx, Jy, Jz, Px, Py, Pz, Rx, Ry, Rz and so on)
    char          achSensorType [6];            //!< 028h  (MFS06 MFS06e MFS07 MFS07e MFS10e SHFT02 SHF02e FGS02 FGS03 FGS03e etc. e.g. the "-" in MFS-06e is skipped)
    std::int16_t  siSensorSerNum;               //!< 02Eh  Serial number of connected sensor

    float         rPosX1;                       //!< 030h  e.g. South negative [m]
    float         rPosY1;                       //!< 034h  e.g. West negative [m]
    float         rPosZ1;                       //!< 038h  e.g. top, sky [m]
    float         rPosX2;                       //!< 03Ch  e.g. North positive [m]
    float         rPosY2;                       //!< 040h  e.g. East positive [m]
    float         rPosZ2;                       //!< 044h  e.g. bottom, earth [m]

    // not used any more use pos values!!; GUI interfaces using Length and direction MUST calculate pos x,y,z and set above.
    float         rDipLength;                   //!< 048h  e.g. to be calculated; should not be used - my be over written in FUTURE
    // not used any more use pos values!!
    float         rAngle;                       //!< 04Ch  e.g. to be calculated; should not be used - my be over written in FUTURE

    // Data from Selftest
    float         rProbeRes;                    //!< 050h  [ohm]
    float         rDCOffset;                    //!< 054h  [mV]
    float         rPreGain;                     //!< 058h  e.g. Gain Stage 1
    float         rPostGain;                    //!< 05Ch  e.g. Gain Stage 2

    // Data from status information ?
    std::int32_t  iLatitude;                    //!< 060h  must be used, UNIT = milli seconds
    std::int32_t  iLongitude;                   //!< 064h  must be used, UNIT = milli seconds
    std::int32_t  iElevation;                   //!< 068h  must be used, UNIT = cm
    char          byLatLongType;                //!< 06Ch  'G' default, 'U' user, GPS should be used
    char          byAddCoordType;               //!< 06Dh  'U' = UTM, default empty
    std::int16_t  siRefMeridian;                //!< 06Eh  default empty, should not be used (external)



    //!@todo can we store 64bit time and 64bit samples here ??
    double        dblNorthing;                  //!< 070h  also xcoord should not be used, default 0 (external)
    double        dblEasting;                   //!< 078h  also ycoord should not be used  default 0 (external)
    char          byGPSStat;                    //!< 080h  '-' unknown, 'N' no fix, 'C' full fix
    char          byGPSAccuracy;                //!< 081h  '0' - not used, 1 in case GF4-Fix & Syrlinks was active (system was synced before Syrlinks took over)
    std::int16_t  iUTCOffset;                   //!< 082h  UTC + iUTCOffset = GPS time for example; can be used for other offsets, not used, default 0
    char          achSystemType[12];            //!< 084h  MMS03e GMS05 ADU06 ADU07 ADU08 ADU09 SPAMMKV SPAMMKIII

    // Data from XML-Job specification
    char          achSurveyHeaderName [12];     //!< 090h  UTF-8 storage  free for usage
    char          achMeasType          [4];     //!< 09Ch  free for usage MT CSMT


    double        DCOffsetCorrValue;            //!< 0A0h  DAC offset double
    std::int8_t   DCOffsetCorrOn;               //!< 0A8h  DC offset was switched on (1) or off(0)
    std::int8_t   InputDivOn;                   //!< 0A9h  inputput divider on(1) off(0); e.g when coil was connected = 1
    std::int16_t  bit_indicator;                //!< 0AAh  0 = 32bit int INTEL byte order, 1 = 64bit INTEL byte order, little endian; since atsheader version 81
    char          achSelfTestResult [2];        //!< 0ACh  'NO' or 'OK'
    std::uint16_t numslices;                    //!< 0AEh  number of slices used (1....1024, 1 is the first, that is list.size() )

    //std::int16_t  calentries    // max 128 entries

    // Were the following fields ever used ?
    std::int16_t  siCalFreqs;                   //!< 0B0h  not used  (external)
    std::int16_t  siCalEntryLength;             //!< 0B2h  not used  (external)
    std::int16_t  siCalVersion;                 //!< 0B4h  not used  (external)
    std::int16_t  siCalStartAddress;            //!< 0B6h  not used, never used  (external)


    char          abyLFFilters [8];             //!< 0B8h  is a bitfield

    char          achUTMZone  [12];             //!< 0C0h  not used  (external)  (formerly abyADU06CalFilename) 32U or 01G, 32UMD7403 : alway NNC od NNCCNNNN
    std::uint32_t uiADUCalTimeDate;             //!< 0CCh  not used  (external)
    char          achSensorCalFilename [12];    //!< 0D0h  not used  ("SENSOR.CAL") (external)
    std::uint32_t uiSensorCalTimeDate;          //!< 0DCh  not used  (external)

    float         rPowerlineFreq1;              //!< 0E0h  e.g. empty  (external)
    float         rPowerlineFreq2;              //!< 0E4h  e.g. empty  (external)
    char          abyHFFilters[8];              //!< 0E8h  is a bitfield


    // IF uiSamples == UINT32_MAX you find
    std::uint64_t uiSamples64bit;               //!< 0F0h  amount of samples (each is a 32bit /64bit int) in the file total of all slices; ONLY used in case uiSamples == UINT32_MAX; else 0; REPLACSES the uiSamples; do not add

    //double        OriginalLSBMV;              //!< 0F0h  NOT USED ANYMORE! orig lsb from selftest without gains; used for ADC values
    float         rExtGain;                     //!< 0F8h  for external satellite box
    char          achADBBoardType[4];           //!< 0FCh  LF HF or MF; BB exists only virtually: LF or HF will be used


    ATSComments_80 tscComment;                  //!< 100h

    // size if comments is total 100h + 100h data  + (512byte comments = 200h) = 400h
    // start tsdata = 400h = 1024th byte

};

End of ATS structure definition =#

using JSON3
using Printf
using Dates
using OrderedCollections
using OrderedCollections

 
"""
    pos2azimuth(x1, x2, y1, y2, dip_length, channel_type)
for the 3D vector we calculate azimuth
magnetic sensors (dip_length == 0) have no real dipole geometry; azimuth is fixed by channel type
if 0, return 0.0 defaults for magnetic  sensors
  
"""
function pos2azimuth(x1, x2, y1, y2, dip_length, channel_type)
    if dip_length == 0
        channel_type == "Hx" && return 0.0   # NORTH
        channel_type == "Hy" && return 90.0  # EAST
        channel_type == "Hz" && return 0.0
    end

    dx = x2 - x1
    dy = y2 - y1
    ang = rad2deg(atan(dy, dx))
    ang < 0 && (ang += 360.0)

    # snap azimuth to the nearest cardinal direction
    (89.99 < ang < 90.01) && return 90.0
    (ang > 359.99 || ang < 0.01) && return 0.0
    (179.99 < ang < 180.01) && return 180.0
    (269.99 < ang < 270.01) && return 270.0
    return ang
end

"""
    pos2dip(x1, x2, y1, y2, z1, z2)

    returns the dipole length (distance between points), round for 0
"""
function pos2dip(x1, x2, y1, y2, z1, z2)
    dx = x2 - x1
    dy = y2 - y1
    dz = z2 - z1
    len = sqrt(dx^2 + dy^2 + dz^2)
    if len < 0.01
        return 0.0
    end
    return len
end

function pos2tilt(x1, x2, y1, y2, z1, z2)
    dx = x2 - x1
    dy = y2 - y1
    dz = z2 - z1
    len = sqrt(dx^2 + dy^2 + dz^2)
    if len < 0.01
        return 0.0
    end
    return rad2deg(asin(dz / len))
end

# convert milliseconds to degrees assuming 360 degrees per second
function msecs2deg(ms)
    return ms / 1000.0 * 360.0
end

# insert a - separator for clarity after MFS or FGS or SHFT
# so MFS06e becomes MFS-06e
function insert_separator_sensor(str::String)
    if startswith(str, "MFS") && length(str) > 3
        return "MFS-" * str[4:end]
    elseif startswith(str, "FGS") && length(str) > 3
        return "FGS-" * str[4:end]
    elseif startswith(str, "SHFT") && length(str) > 4
        return "SHFT-" * str[5:end]
    elseif startswith(str, "EFP") && length(str) > 3
        return "EFP-" * str[4:end]
    else
        return str
    end
end

function insert_separator_system(str::String)
    if startswith(str, "ADU") && length(str) > 3
        sys = "ADU-" * str[4:end]
        if sys == "ADU-07"    # old bug
            return "ADU-07e"
        end
        return sys
    else
        return str
    end
end

# ADU LF/HF filter bit values (see mt_base.hpp enum class ADU); MF_RF_1/MF_RF_2 comments in the
# C++ header are swapped, the values below are the ones actually assigned/used in the enum
const ADU_LF_RF_1    = 0x01
const ADU_LF_RF_2    = 0x02
const ADU_LF_RF_3    = 0x04
const ADU_LF_RF_4    = 0x08
const ADU_LF_LP_4HZ  = 0x10
const ADU_MF_RF_1    = 0x20
const ADU_MF_RF_2    = 0x40

const ADU_HF_HP_1HZ    = 0x01
const ADU_HF_HP_500HZ  = 0x02

# available LF filter bits per system, mirrors set_filter_bank() in atsfile.hpp
function _lf_filter_map(system::AbstractString)
    m = Dict{UInt8,String}()
    if system == "ADU-07e" || system == "ADU-08e"
        m[ADU_LF_RF_1]   = "LF-RF-1"
        m[ADU_LF_RF_2]   = "LF-RF-2"
        m[ADU_LF_LP_4HZ] = "LF-LP-4Hz"
    end
    if system == "ADU-07e"
        m[ADU_LF_RF_3] = "LF-RF-3"
        m[ADU_LF_RF_4] = "LF-RF-4"
        m[ADU_MF_RF_1] = "MF-RF-1"
        m[ADU_MF_RF_2] = "MF-RF-2"
    end
    return m
end

# available HF filter bits per system, mirrors set_filter_bank() in atsfile.hpp
function _hf_filter_map(system::AbstractString)
    m = Dict{UInt8,String}()
    system == "ADU-07e" && (m[ADU_HF_HP_1HZ] = "HF-HP-1Hz")
    system == "ADU-08e" && (m[ADU_HF_HP_500HZ] = "HF-HP-500Hz")
    return m
end

# greedy bit decomposition (largest bit value first) of a filter byte into its comma separated names,
# mirrors get_lf_filter_strings()/get_hf_filter_strings() in atsfile.hpp
function _filter_byte_to_string(byte::UInt8, filter_map::Dict{UInt8,String})
    names = String[]
    remaining = byte
    for (value, name) in sort(collect(filter_map); by=first, rev=true)
        if remaining >= value
            push!(names, name)
            remaining -= value
        end
    end
    return join(names, ",")
end

# lf_filters to comma separated string; only the first byte of the bitfield is used, like in atsfile.hpp
function lf_filter_to_strings(lf_filters::Int64, system::AbstractString)
    return _filter_byte_to_string(UInt8(lf_filters & 0xFF), _lf_filter_map(system))
end

# hf_filters to comma separated string; only the first byte of the bitfield is used, like in atsfile.hpp
function hf_filter_to_strings(hf_filters::Int64, system::AbstractString)
    return _filter_byte_to_string(UInt8(hf_filters & 0xFF), _hf_filter_map(system))
end

# ##############################################################################################
# ATS channel: binary (ATSHeader_80/81) header parsed into a dictionary.
# All numeric values are stored as Int64 or Float64 (mirrors the JSON dict used by atss.jl); read-only.
mutable struct ATSChannel
    header::Dict{String,Any}   # parsed header fields, keys per field name below
    path::String                # path to the .ats file this header was read from
end

ATSChannel() = ATSChannel(Dict{String,Any}(), "")

# reads a fixed-size char array and trims at the first NUL byte (or full length if none)
function _read_str(io::IO, n::Integer)
    bytes = read(io, n)
    zero_idx = findfirst(==(0x00), bytes)
    stop = zero_idx === nothing ? length(bytes) : zero_idx - 1
    return String(bytes[1:stop])
end

# reads n raw bytes (little endian) into an Int64, used for the LF/HF filter bitfields
function _read_bits(io::IO, n::Integer)
    bytes = read(io, n)
    v = UInt64(0)
    for i in n:-1:1
        v = (v << 8) | UInt64(bytes[i])
    end
    return Int64(v)
end

_i64(x) = Int64(x)
_f64(x) = Float64(x)

# reads the 1024-byte binary ATS header (versions 80/81/1080) into an ATSChannel
function read_header(path::AbstractString)
    ch = ATSChannel()
    ch.path = String(path)
    h = ch.header

    open(path, "r") do io
        h["header_length"]      = _i64(read(io, UInt16))
        h["header_version"]     = _i64(read(io, Int16))
        h["samples"]            = _i64(read(io, UInt32))
        h["sampling_rate"]        = _f64(read(io, Float32))
        h["start_date_time"]    = _i64(read(io, UInt32))
        h["lsb_mv"]             = _f64(read(io, Float64))
        h["gmt_offset"]         = _i64(read(io, Int32))
        h["original_sampling_rate"]   = _f64(read(io, Float32))
        h["serial"]             = _i64(read(io, UInt16))
        h["adc_ser_num"]        = _i64(read(io, UInt16))
        h["channel_no"]         = _i64(read(io, UInt8))
        h["chopper"]            = _i64(read(io, UInt8))
        h["channel_type"]       = _read_str(io, 2)
        h["sensor"]             = _read_str(io, 6)
        h["sensor_serial"]      = _i64(read(io, Int16))
        h["pos_x1"]             = _f64(read(io, Float32))
        h["pos_y1"]             = _f64(read(io, Float32))
        h["pos_z1"]             = _f64(read(io, Float32))
        h["pos_x2"]             = _f64(read(io, Float32))
        h["pos_y2"]             = _f64(read(io, Float32))
        h["pos_z2"]             = _f64(read(io, Float32))
        h["dip_length"]         = _f64(read(io, Float32))
        h["angle"]              = _f64(read(io, Float32))
        h["probe_res"]          = _f64(read(io, Float32))
        h["dc_offset"]          = _f64(read(io, Float32))
        h["pre_gain"]           = _f64(read(io, Float32))
        h["post_gain"]          = _f64(read(io, Float32))
        h["latitude"]           = _i64(read(io, Int32))
        h["longitude"]          = _i64(read(io, Int32))
        h["elevation"]          = _i64(read(io, Int32))
        h["lat_long_type"]      = _read_str(io, 1)
        h["add_coord_type"]     = _read_str(io, 1)
        h["ref_meridian"]       = _i64(read(io, Int16))
        h["northing"]           = _f64(read(io, Float64))
        h["easting"]            = _f64(read(io, Float64))
        h["gps_stat"]           = _read_str(io, 1)
        h["gps_accuracy"]       = _read_str(io, 1)
        h["utc_offset"]         = _i64(read(io, Int16))
        h["system"]             = _read_str(io, 12)
        h["survey_header_name"] = _read_str(io, 12)
        h["meas_type"]          = _read_str(io, 4)
        h["dc_offset_corr_value"] = _f64(read(io, Float64))
        h["dc_offset_corr_on"]  = _i64(read(io, Int8))
        h["input_div_on"]       = _i64(read(io, Int8))
        h["bit_indicator"]      = _i64(read(io, Int16))
        h["self_test_result"]   = _read_str(io, 2)
        h["numslices"]          = _i64(read(io, UInt16))
        h["cal_freqs"]          = _i64(read(io, Int16))
        h["cal_entry_length"]   = _i64(read(io, Int16))
        h["cal_version"]        = _i64(read(io, Int16))
        h["cal_start_address"]  = _i64(read(io, Int16))
        h["lf_filters"]         = _read_bits(io, 8)
        h["utm_zone"]           = _read_str(io, 12)
        h["adu_cal_time_date"]  = _i64(read(io, UInt32))
        h["sensor_cal_filename"] = _read_str(io, 12)
        h["sensor_cal_time_date"] = _i64(read(io, UInt32))
        h["powerline_freq1"]    = _f64(read(io, Float32))
        h["powerline_freq2"]    = _f64(read(io, Float32))
        h["hf_filters"]         = _read_bits(io, 8)
        h["samples_64bit"]      = _i64(read(io, UInt64))
        h["ext_gain"]           = _f64(read(io, Float32))
        h["adb_board_type"]     = _read_str(io, 4)

        # ATSComments_80
        h["client"]             = _read_str(io, 16)
        h["contractor"]         = _read_str(io, 16)
        h["area"]               = _read_str(io, 16)
        h["survey_id"]          = _read_str(io, 16)
        h["operator"]           = _read_str(io, 16)
        h["site_name"]          = _read_str(io, 112)
        h["xml_header"]         = _read_str(io, 64)
        h["comments"]           = _read_str(io, 288)
        h["site_name_rr"]       = _read_str(io, 112)
        h["site_name_emap"]     = _read_str(io, 112)
    end

    # uiSamples == UINT32_MAX means the real (64bit) sample count is stored in samples_64bit
    if h["samples"] == Int64(typemax(UInt32))
        h["samples"] = h["samples_64bit"]
    end


    h["system"] = insert_separator_system(h["system"])
    h["sensor"] = insert_separator_sensor(h["sensor"])
    h["latitude"] = msecs2deg(h["latitude"])
    h["longitude"] = msecs2deg(h["longitude"])

    # make it atss friendly
    h["elevation"] = h["elevation"] / 100.0 # convert from centimeters to meters
    h["dipole_length"] = pos2dip(h["pos_x1"], h["pos_x2"], h["pos_y1"], h["pos_y2"], h["pos_z1"], h["pos_z2"])
    h["azimuth"] = pos2azimuth(h["pos_x1"], h["pos_x2"], h["pos_y1"], h["pos_y2"], h["dipole_length"], h["channel_type"])
    # we want mV/km for the E field, so convert from mV and dipole length in meters to mV / km
    if h["dipole_length"] < 0.01
        h["factor"] = h["lsb_mv"]   # e.g. for magnetic point sensors with no real dipole length
        h["units"] = "mV"           # for magnetic point sensors with no real dipole length
    else
        h["factor"] = h["lsb_mv"] / (h["dipole_length"] / 1000.0)
        h["units"] = "mV/km"        # for E field with real dipole length
    end
    h["tilt"] = pos2tilt(h["pos_x1"], h["pos_x2"], h["pos_y1"], h["pos_y2"], h["pos_z1"], h["pos_z2"])
    h["id"] = ""
    h["resistance"] = h["probe_res"]
    #filename: <serial>_<system>_C<channel_no>_T<channel_type>_<sampling_rate>(Hz|s)
    sunit ="";
    srate = ""          # need int without decimal point for filename
    if h["sampling_rate"] < 1.0
        sunit = "s"
        srate = Int(1/h["sampling_rate"])
    else
        sunit = "Hz"
        srate = Int(h["sampling_rate"])
    end
    # 084_ADU-07e_C001_TEy_128Hz
    h["filename"] = @sprintf("%03d_%s_C%03d_T%s_%d%s", h["serial"], h["system"], h["channel_no"], h["channel_type"], srate, sunit)
    h["source"] = ""
    # datetime": "2009-08-20T13:22:00" (UTC, no trailing Z)
    h["datetime"] = string(unix2datetime(h["start_date_time"]))
    h["lf_filters"] = lf_filter_to_strings(h["lf_filters"], h["system"])
    h["hf_filters"] = hf_filter_to_strings(h["hf_filters"], h["system"])
    # "adb_board_type": "LF" -> ADB-LF, "HF" -> ADB-HF
    # "input_div_on": 1 -> div-8, 0 -> div-1
    if h["adb_board_type"] == "LF"
        h["adb_board_type"] = "ADB-LF"
    elseif h["adb_board_type"] == "HF"
        h["adb_board_type"] = "ADB-HF"
    end
    if h["input_div_on"] == 1
        h["input_div_on"] = "div-8"
    else
        h["input_div_on"] = "div-1"
    end
    # join board type, filters, input divider and gains into one comma separated string, mirrors get_ats_filter() in atsfile.hpp
    gains = @sprintf("gain_1_%d,gain_2_%d", Int(h["pre_gain"]), Int(h["post_gain"]))
    # remove strings ending with 0, so that gains like "gain_1_0,gain_2_0" are omitted
    gains = join(filter(x -> !endswith(x, "_0"), split(gains, ",")), ",")
    h["filter"] = join(filter(!isempty, [h["adb_board_type"], h["lf_filters"], h["hf_filters"], h["input_div_on"], gains]), ",")

    # sensor_calibration is a nested atss-style stub; f/a/p are filled in separately when a calibration is loaded
    h["sensor_calibration"] = Dict{String,Any}(
        "sensor"          => h["sensor"],
        "serial"          => h["sensor_serial"],
        "chopper"         => h["chopper"],
        "units_frequency" => "Hz",
        "units_amplitude" => "mV",
        "units_phase"     => "degrees",
        "datetime"        => "1970-01-01T00:00:00",
        "Operator"        => "",
        "f"               => Float64[],
        "a"               => Float64[],
        "p"               => Float64[],
    )

    # return the channel with the updated header
    return ch
end

# reads wl samples starting at sample index `start` (0-based); wl=0 means "until end of file"
# returned as double mV (raw int32 * lsb_mv)
# data starts at the end of the header, so at 1024 bytes 
# data is stored as 32-bit integers, little-endian
function read_data(ch::ATSChannel, start::Int64=0, wl::Int64=0)
    wl = wl == 0 ? ch.header["samples"] - start : wl
    raw = Vector{Int32}(undef, wl)
    open(ch.path, "r") do io
        seek(io, 1024 + start * 4)  # 1024 bytes header + 4 bytes per sample
        read!(io, raw)
    end
    lsb_mv = ch.header["factor"]::Float64
    return raw .* lsb_mv
end

# returns the ATSS JSON header for the given channel
function atss_header(ch::ATSChannel)
    # return the ATSS JSON header for the given channel
    # we create a new json object based on the channel header for the new ATSS format
    nj = OrderedDict{String,Any}()
    nj["datetime"] = ch.header["datetime"]
    nj["latitude"] = ch.header["latitude"]
    nj["longitude"] = ch.header["longitude"]
    nj["elevation"] = ch.header["elevation"]
    nj["azimuth"] = ch.header["azimuth"]
    nj["tilt"] = ch.header["tilt"]
    nj["resistance"] = ch.header["resistance"]
    nj["id"] = ch.header["id"]
    nj["filter"] = ch.header["filter"]
    nj["source"] = ch.header["source"]
    # now add the sensor calibration nested dictionary
    nj["sensor_calibration"]  = OrderedDict{String,Any}(
        "sensor"          => ch.header["sensor"],
        "serial"          => ch.header["sensor_serial"],
        "chopper"         => ch.header["chopper"],
        "units_frequency" => "Hz",
        "units_amplitude" => "mV",
        "units_phase"     => "degrees",
        "datetime"        => "1970-01-01T00:00:00",
        "Operator"        => "",
        "f"               => Float64[],
        "a"               => Float64[],
        "p"               => Float64[],
    )

    return nj
end
