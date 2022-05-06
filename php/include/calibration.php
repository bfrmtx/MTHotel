<?php

if (!isset($_SESSION)) session_start();

class calibration {
  function __construct($channel_no, $sensor, $serial, $chopper) {

    $this->channel_no = $channel_no;

    if(!isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["sensor"])) {

      $this->clear($channel_no);
      $this->sensor = $sensor;
      $this->serial = $serial;
      $this->chopper = $chopper;
      $this->vars_to_session();

    }
    else $this->session_to_vars();
  }
  
  /**
  * @ref atss_header::vars_to_session() "-> atss_header::vars_to_session"
  *
  */
  function vars_to_session() {
    if(!isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["sensor"]))          $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["sensor"] = $this->sensor;
    if(!isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["serial"]))          $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["serial"] = $this->serial;
    if(!isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["chopper"]))         $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["chopper"] = $this->chopper;
    if(!isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["units_amplitude"])) $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["units_amplitude"] = $this->units_amplitude;
    if(!isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["units_frequency"])) $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["units_frequency"] = $this->units_frequency;
    if(!isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["units_phase"]))     $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["units_phase"] = $this->units_phase;
    if(!isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["date"]))            $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["date"] = $this->date;
    if(!isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["time"]))            $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["time"] = $this->time;
    if(!isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["operator"]))        $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["operator"] = $this->operator;
    if(!isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["f"]))               $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["f"] = $this->f;
    if(!isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["a"]))               $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["a"] = $this->a;
    if(!isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["p"]))               $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["p"] = $this->p;
  }

  function session_to_vars() {
    if(isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["sensor"]))            $this->sensor = $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["sensor"];
    if(isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["serial"]))            $this->serial = $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["serial"];
    if(isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["chopper"]))           $this->chopper = $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["chopper"];
    if(isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["units_amplitude"]))   $this->units_amplitude = $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["units_amplitude"];
    if(isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["units_frequency"]))   $this->units_frequency = $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["units_frequency"];
    if(isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["units_phase"]))       $this->units_phase = $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["units_phase"];
    if(isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["date"]))              $this->date = $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["date"];
    if(isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["time"]))              $this->time = $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["time"];
    if(isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["operator"]))          $this->operator = $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["operator"];
    if(isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["f"]))                 $this->f = $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["f"];
    if(isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["a"]))                 $this->a = $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["a"];
    if(isset($_SESSION["adu"][$this->channel_no]["sensor_calibration"]["p"]))                 $this->p = $_SESSION["adu"][$this->channel_no]["sensor_calibration"]["p"];
  }

  function clear($channel_no = -1) {
    $this->sensor = "";
    $this->serial = "";
    $this->chopper = "";
    $this->channel_no = $channel_no;
    $this->units_amplitude = 'mV/nT';
    $this->units_frequency = 'Hz';
    $this->units_phase = 'degrees';
    $this->date = '1970-01-01';
    $this->time = '00:00:00';
    $this->operator = 'mtx';

    $this->f = array();
    $this->a = array();
    $this->p = array();

  }

}

?>
