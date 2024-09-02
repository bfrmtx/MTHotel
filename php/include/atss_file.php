<?php
if (!isset($_SESSION)) session_start();

class atss_calibration {
  public $sensor;
  public $serial;
  public $chopper;
  public $units_frequency;
  public $units_amplitude;
  public $units_phase;
  public $datetime;
  public $Operator;
  public $f = array();
  public $a = array();
  public $p = array();

  function __construct($sensor = "", $serial = 0, $chopper = 0, $units_frequency = "Hz", $units_amplitude = "mV/nT", $units_phase = "degrees", $datetime = "1970-01-01T00:00:00", $Operator = "") {
    $this->sensor = $sensor;
    $this->serial = intval($serial);
    $this->chopper = intval($chopper);
    $this->units_frequency = $units_frequency;
    $this->units_amplitude = $units_amplitude;
    $this->units_phase = $units_phase;
    $this->datetime = $datetime;
    $this->Operator = $Operator;
    $this->f = array();
    $this->a = array();
    $this->p = array();
  }
}

class atss_file_tags {

  public $serial;
  public $system;
  public $channel_no;
  public $channel_type;
  public $sample_rate;

  function __construct($serial = 0, $system = '', $channel_no = 0, $channel_type = '', $sample_rate = 0.0) {
    $this->serial = intval($serial);
    $this->system = $system;
    $this->channel_no = intval($channel_no);
    $this->channel_type = $channel_type;
    $this->sample_rate = floatval($sample_rate);
  }
}

class atss_header {
  public $datetime; // ISO 8601 datetime in UTC
  public $latitude; // decimal degree such as 52.2443
  public $longitude; // decimal degree such as 10.5594
  public $elevation; // elevation in meter
  public $azimuth; //  orientation from North to East (90 = East, -90 or 270 = West, 180 South, 0 North)
  public $tilt; //  azimuth positive down - in case it had been measured
  public $resistance; //  resistance of the sensor in Ohm or contact resistance of electrode in Ohm
  public $units; //  for ADUs it will be mV H or other -  or scaled E mV/km (the logger will do this while recording)
  public $filter; //  comma separated list of filters such as "ADB-LF,LF-RF-4" or "ADB-HF,HF-RF-1"
  public $source; //  empty or indicate as, ns, ca, cp, tx or what ever; some users need this

  function __construct($datetime = '1970-01-01T00:00:00.0', $latitude = 0.0, $longitude = 0.0, $elevation = 0.0, $azimuth = 0.0, $tilt = 0.0, $resistance = 0.0, $units = 'mV', $filter = '', $source = '') {
    $this->datetime = $datetime;
    $this->latitude = floatval($latitude);
    $this->longitude = floatval($longitude);
    $this->elevation = floatval($elevation);
    $this->azimuth = floatval($azimuth);
    $this->tilt = floatval($tilt);
    $this->resistance = floatval($resistance);
    $this->units = $units;
    $this->filter = $filter;
    $this->source = $source;
  }

  public function get_date(): string {
    return substr($this->datetime, 0, 10);
  }

  public function get_time(): string {
    return substr($this->datetime, 11, 8);
  }

  public function get_timestamp(): int {
    return strtotime($this->datetime);
  }
}

class atss_channel {
  public $file_tags;
  public $header;
  public $sensor_calibration;

  function __construct($file_tags = null, $header = null, $sensor_calibration = null) {
    $this->file_tags = $file_tags;
    $this->header = $header;
    $this->sensor_calibration = $sensor_calibration;
  }
}
