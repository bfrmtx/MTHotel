#if !defined(ATSHEADER_STRUCT_H)
#define ATSHEADER_STRUCT_H

#include <cstdint>
#include <string>
//
//        REMINDER
//        array of chars are not NULL terminated !
//        achChanType   [2] = "Ex" is not NULL terminated
//
//
#define C_ATS_CEA_NUM_HEADERS 1023 /*!< maximum number of 1023 headers of the ATS sliced file.*/

/*!
  \brief The ATSSliceHeader_s struct
   A sliced ats file ALWAYS contains a default header and C_ATS_CEA_NUM_HEADERS (1023) = 1024 total.<br>
   The FIRST and DEFAULT slice contains ALL information (total samples of all slices)<br>
   The 0 ... 1022 slices (=  ATS_CEA_NUM_HEADERS ) contain the individual descriptions<br>
   start tsdata = 400h = 1024th byte<br>
   start of CEA data is 400h + 3FFh * 20h = 83E0h<br>
                        1024 + 1023 * 23  = 33760<br>
   assuming 3 slices of 4096 iSamples will be 12,288 samples in the header,
   followed by 3 slice headers with 4096 iSamples each
   and 1019 empty slice headers<br>
*/

struct ATSSliceHeader_1080 {
  uint32_t uiSamples;         //!< 000h  number of samples for this slice
  uint32_t uiStartDateTime;   //!< 004h  startdate/time as UNIX timestamp (UTC)
  double dbDCOffsetCorrValue; //!< 008h  DC offset correction value in mV
  float rPreGain;             //!< 010h  originally used pre-gain (GainStage1) - LSB is the same for all slices
  float rPostGain;            //!< 014h  originally used post-gain (GainStage2) - LSB is the same for all slices
  std::int8_t DCOffsetCorrOn; //!< 018h  DC offset correction was on/off for this slice
  std::int8_t reserved[7];    //!< 020h  reserved bytes to get to word / dword boundary
};

/*! @todo TX info
  maybe UTM Zone number
  maybe UTM letter
  Tx times six double at least
  Tx base freq

*/

/*!
   \brief The ATSComments_80 struct for ats header 80,81 and 90 <br>
   Some software will automatically convert to higher version<br>
   Some fields are external - the user has to set then; examples:<br>
   northing, easting, UTM Zone : these can be used for high density grids<br>
 */
struct ATSComments_80 {
  char Client[16];     //!< 000h  UTF-8 storage, used in EDI
  char Contractor[16]; //!< 010h  UTF-8 storage, used in EDI
  char Area[16];       //!< 020h  UTF-8 storage, used in EDI
  char SurveyID[16];   //!< 030h  UTF-8 storage, used in EDI
  char Operator[16];   //!< 040h  UTF-8 storage, used in EDI
  char SiteName[112];  //!< 050h  UTF-8 storage, no BOM at the beginning!; WORST case = 28 Chinese/Japanese chars (multibyte chars)
  char XmlHeader[64];  //!< 0C0h  UTF-8 storage  points to the corresponding XML file, containing additional information for this header; no PATH=XML file is in the same directory as the ats file
  // deleted September 2018 char          achComments   [512];          //!< 100h  UTF-8 storage  comment block starts (comment field can start with "weather:" but has no meaning
  // new September 2018
  char Comments[288];     //!< 100h  UTF-8 storage  comment block starts (comment field can start with "weather:" but has no meaning
  char SiteNameRR[112];   //!< 220h  UTF-8 storage, no BOM at the beginning!; WORST case = 28 Chinese/Japanese chars (multibyte chars)
  char SiteNameEMAP[112]; //!< 290h  UTF-8 storage, no BOM at the beginning!; WORST case = 28 Chinese/Japanese chars (multibyte chars); INDICATES a default EMAP center station, where we expect the H (magnetic)
};

/*!
   \brief The ATSHeader_80 struct<br>
   reference is lat and long; additional entries like northing and easting should be empty // not used<br>
   these entries can be overwritten by the user in case of high density grids <br>
   The site_name (site name, achSiteName, is in the comment field; the site_no, site_num, site_number, site number is nver stored; this is part of external numeration
 */
struct ATSHeader_80 {
  std::uint16_t header_length; //!< 000h  length of header in bytes (default 1024 and  33760 for CEA)
  std::int16_t header_version; //!< 002h  80 for ats, 81 for 64bit possible / metronix, 1080 for CEA / sliced header

  // This information can be found in the ChannelTS datastructure
  uint32_t samples; //!< 004h  amount of samples (each is a 32bit / 64bit int INTEL byte order little endian) in the file total of all slices; if uiSamples == UINT32_MAX, uiSamples64bit at address 0F0h will be used instead; uiSamples64bit replaces uiSamples in that case; do not add!

  float sample_rate;      //!< 008h  sampling frequency in Hz
  uint32_t start;         //!< 00Ch  unix TIMESTAMP (some computers will revert that to negative numbers if this number is grater than 32bit signed); 2106-02-07T06:28:14 is the END of this format
  double lsbval;          //!< 010h  least significant bit in mV ()
  int32_t GMToffset;      //!< 018h  not used, default 0; can be used as "UTC to GMT"
  float orig_sample_rate; //!< 01Ch  sampling frequency in Hz as ORIGINALLY recorded; this value should NOT change (for example after filtering)

  // The required data could probably found in the HardwareConfig
  std::uint16_t serial_number;           //!< 020h  Serial number of the system (logger)
  std::uint16_t serial_number_ADC_board; //!< 022h  Serial number of the ADC board (ADB)
  std::uint8_t channel_number;           //!< 024h  Channel number
  std::uint8_t chopper;                  //!< 025h  Chopper On (1) / Off (0)

  // Data from XML Job-specification
  char channel_type[2];              //!< 026h  (Ex, Ey, Ez, Hx, Hy, Hz, Jx, Jy, Jz, Px, Py, Pz, Rx, Ry, Rz and so on)
  char sensor_type[6];               //!< 028h  (MFS06 MFS06e MFS07 MFS07e MFS10e SHFT02 SHF02e FGS02 FGS03 FGS03e etc. e.g. the "-" in MFS-06e is skipped)
  std::int16_t sensor_serial_number; //!< 02Eh  Serial number of connected sensor

  float x1; //!< 030h  e.g. South negative [m]
  float y1; //!< 034h  e.g. West negative [m]
  float z1; //!< 038h  e.g. top, sky [m]
  float x2; //!< 03Ch  e.g. North positive [m]
  float y2; //!< 040h  e.g. East positive [m]
  float z2; //!< 044h  e.g. bottom, earth [m]

  // not used any more use pos values!!; GUI interfaces using Length and direction MUST calculate pos x,y,z and set above.
  float dipole_length; //!< 048h  e.g. to be calculated; should not be used - my be over written in FUTURE
  // not used any more use pos values!!
  float angle; //!< 04Ch  e.g. to be calculated; should not be used - my be over written in FUTURE

  // Data from Selftest
  float rho_probe_ohm;        //!< 050h  [ohm]
  float DC_offset_voltage_mV; //!< 054h  [mV]
  float gain_stage1;          //!< 058h  e.g. Gain Stage 1
  float gain_stage2;          //!< 05Ch  e.g. Gain Stage 2

  // Data from status information ?
  int32_t iLat_ms;           //!< 060h  must be used, UNIT = milli seconds
  int32_t iLong_ms;          //!< 064h  must be used, UNIT = milli seconds
  int32_t iElev_cm;          //!< 068h  must be used, UNIT = cm
  char Lat_Long_TYPE;        //!< 06Ch  'G' default, 'U' user, GPS should be used
  char coordinate_type;      //!< 06Dh  'U' = UTM, default empty
  std::int16_t ref_meridian; //!< 06Eh  default empty, should not be used (external)

  //!@todo can we store 64bit time and 64bit samples here ??
  double Northing;         //!< 070h  also xcoord should not be used, default 0 (external)
  double Easting;          //!< 078h  also ycoord should not be used  default 0 (external)
  char gps_clock_status;   //!< 080h  '-' unknown, 'N' no fix, 'C' full fix
  char GPS_accuracy;       //!< 081h  '0' - not used, 1 in case GF4-Fix & Syrlinks was active (system was synced before Syrlinks took over)
  std::int16_t offset_UTC; //!< 082h  UTC + iUTCOffset = GPS time for example; can be used for other offsets, not used, default 0
  char SystemType[12];     //!< 084h  MMS03e GMS05 ADU06 ADU07 ADU08 ADU09 ADU10 ADU11 SPAMMKV SPAMMKIII

  // Data from XML-Job specification
  char survey_header_filename[12]; //!< 090h  UTF-8 storage  free for usage
  char type_of_meas[4];            //!< 09Ch  free for usage MT CSMT

  double DCOffsetCorrValue;   //!< 0A0h  DAC offset double
  std::int8_t DCOffsetCorrOn; //!< 0A8h  DC offset was switched on (1) or off(0)
  std::int8_t InputDivOn;     //!< 0A9h  inputput divider on(1) off(0); e.g when coil was connected
  std::int16_t bit_indicator; //!< 0AAh  0 = 32bit int INTEL byte order, 1 = 64bit INTEL byte order, little endian; since atsheader version 81
  char result_selftest[2];    //!< 0ACh  'NO' or 'OK'
  std::uint16_t numslices;    //!< 0AEh  number of slices used (1....1024, 1 is the first, that is list.size() )

  // std::int16_t  calentries    // max 128 entries - not existing

  // Were the following fields ever used ?
  std::int16_t cal_freqs;         //!< 0B0h  not used  (external)
  std::int16_t cal_entry_length;  //!< 0B2h  not used  (external)
  std::int16_t cal_version;       //!< 0B4h  not used  (external)
  std::int16_t cal_start_address; //!< 0B6h  not used, never used  (external)

  char LF_filters[8]; //!< 0B8h  is a bitfield

  char UTMZone[12];             //!< 0C0h  not used  (external)  (formerly abyADU06CalFilename) 32U or 01G, 32UMD7403 : alway NNC od NNCCNNNN
  uint32_t system_cal_datetime; //!< 0CCh  not used  (external)
  char sensor_cal_filename[12]; //!< 0D0h  not used  ("SENSOR.CAL") (external)
  uint32_t sensor_cal_datetime; //!< 0DCh  not used  (external)

  float powerline1;   //!< 0E0h  e.g. empty  (external)
  float powerline2;   //!< 0E4h  e.g. empty  (external)
  char HF_filters[8]; //!< 0E8h  is a bitfield

  // IF uiSamples == UINT32_MAX you find
  uint64_t samples_64bit; //!< 0F0h  amount of samples (each is a 32bit /64bit int) in the file total of all slices; ONLY used in case uiSamples == UINT32_MAX; else 0; REPLACSES the uiSamples; do not add

  // double        OriginalLSBMV;              //!< 0F0h  NOT USED ANYMORE! orig lsb from selftest without gains; used for ADC values
  float external_gain;    //!< 0F8h  for external satellite box
  char ADB_board_type[4]; //!< 0FCh  LF HF or MF; BB exists only virtually, LFor HF will be used

  ATSComments_80 comments; //!< 100h

  // size if comments is total 100h + 100h data  + (512byte comments = 200h) = 400h
  // start tsdata = 400h = 1024th byte
  // start of CEA header is 400h + 3FFh * 20h = 83E0h
  //                        1024 + 1023 * 23  = 33760
};

#endif // ATSHEADER_STRUCT_H