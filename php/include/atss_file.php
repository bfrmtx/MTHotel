<?php

if (!isset($_SESSION)) session_start();
class atss_file {
  function __construct($channel_no, $serial = 0, $system = 0, $run = 0, $channel_type = 0, $sample_rate = 0) {

    $this->channel_no = intval($channel_no);

    if(!isset($_SESSION["adu"][$this->channel_no]["atss_file"]["channel_type"])) {
      $this->clear();
      $this->serial = $serial;
      $this->system = $system;
      $this->channel_no = intval($channel_no);
      $this->run = $run;
      $this->channel_type = $channel_type;
      $this->sample_rate = floatval($sample_rate);

      $this->vars_to_session();
    }
    else $this->session_to_vars();

  }
  function vars_to_session() {
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_file"]["serial"]))       $_SESSION["adu"][$this->channel_no]["atss_file"]["serial"] = intval($this->serial);
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_file"]["system"]))       $_SESSION["adu"][$this->channel_no]["atss_file"]["system"] = $this->system;
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_file"]["channel_no"]))   $_SESSION["adu"][$this->channel_no]["atss_file"]["channel_no"] = intval($this->channel_no);
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_file"]["run"]))          $_SESSION["adu"][$this->channel_no]["atss_file"]["run"] = intval($this->run);
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_file"]["channel_type"])) $_SESSION["adu"][$this->channel_no]["atss_file"]["channel_type"] = $this->channel_type;
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_file"]["sample_rate"]))  $_SESSION["adu"][$this->channel_no]["atss_file"]["sample_rate"] = floatval($this->sample_rate);
  }

  function session_to_vars() {
    if(isset($_SESSION["adu"][$this->channel_no]["atss_file"]["serial"]))            $this->serial =       intval($_SESSION["adu"][$this->channel_no]["atss_file"]["serial"]);
    if(isset($_SESSION["adu"][$this->channel_no]["atss_file"]["system"]))            $this->system =       $_SESSION["adu"][$this->channel_no]["atss_file"]["system"];
    if(isset($_SESSION["adu"][$this->channel_no]["atss_file"]["channel_no"]))        $this->channel_no =   intval($_SESSION["adu"][$this->channel_no]["atss_file"]["channel_no"]);
    if(isset($_SESSION["adu"][$this->channel_no]["atss_file"]["run"]))               $this->run =          intval($_SESSION["adu"][$this->channel_no]["atss_file"]["run"]);
    if(isset($_SESSION["adu"][$this->channel_no]["atss_file"]["channel_type"]))      $this->channel_type = $_SESSION["adu"][$this->channel_no]["atss_file"]["channel_type"];
    if(isset($_SESSION["adu"][$this->channel_no]["atss_file"]["sample_rate"]))       $this->sample_rate =  floatval($_SESSION["adu"][$this->channel_no]["atss_file"]["sample_rate"]);
  }

  function clear() {
    $this->serial = 0;
    $this->system = "";
    $this->channel_no = -1;
    $this->run = 0;
    $this->channel_type = 0;
    $this->sample_rate = 0.0;
  }

  function get_channel_type() {
    return $this->channel_type;
  }

  function get_sample_rate() {
    return floatval($this->sample_rate);
  }
}

?>
