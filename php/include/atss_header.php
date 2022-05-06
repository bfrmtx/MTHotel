<?php

if (!isset($_SESSION)) session_start();

/**
* @class atss_header
* @brief base atssheaer class -identical with the Python and C++ header
*
* @author B. Friedrichs
*/
class atss_header {

  /**
  * @brief init <b>C++ / Python / PHP</b> header for each channel
  * there can be no ADU with zero channels
  * @param $channel_no channel number; channel 0 is the dominator;
  *
  */
  function __construct($channel_no) {

    $this->channel_no = intval($channel_no);

    if(!isset($_SESSION["adu"][$this->channel_no]["atss_header"]["latitude"])) {
      $this->clear(intval($channel_no));
      $this->vars_to_session();
    }
    else $this->session_to_vars();

  }
  /**
  * @brief update the session variables
  * the session conatins all variables for the complete setting of the ADU
  *
  */
  function vars_to_session() {
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_header"]["date"]))      $_SESSION["adu"][$this->channel_no]["atss_header"]["date"] = $this->date;
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_header"]["time"]))      $_SESSION["adu"][$this->channel_no]["atss_header"]["time"] = $this->time;
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_header"]["fracs"]))     $_SESSION["adu"][$this->channel_no]["atss_header"]["fracs"] = floatval($this->fracs);
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_header"]["latitude"]))  $_SESSION["adu"][$this->channel_no]["atss_header"]["latitude"] = floatval($this->latitude);
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_header"]["longitude"])) $_SESSION["adu"][$this->channel_no]["atss_header"]["longitude"] = floatval($this->longitude);
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_header"]["elevation"])) $_SESSION["adu"][$this->channel_no]["atss_header"]["elevation"] = floatval($this->elevation);
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_header"]["dipole_length"]))  $_SESSION["adu"][$this->channel_no]["atss_header"]["dipole_length"] = floatval($this->dipole_length);
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_header"]["angle"]))     $_SESSION["adu"][$this->channel_no]["atss_header"]["angle"] = floatval($this->angle);
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_header"]["dip"]))       $_SESSION["adu"][$this->channel_no]["atss_header"]["dip"] = floatval($this->dip);
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_header"]["units"]))     $_SESSION["adu"][$this->channel_no]["atss_header"]["units"] = $this->units;
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_header"]["source"]))    $_SESSION["adu"][$this->channel_no]["atss_header"]["source"] = $this->source;
    if(!isset($_SESSION["adu"][$this->channel_no]["atss_header"]["site"]))      $_SESSION["adu"][$this->channel_no]["atss_header"]["site"] = $this->site;
  }

  function session_to_vars() {
    if(isset($_SESSION["adu"][$this->channel_no]["atss_header"]["date"]))          $this->date =  $_SESSION["adu"][$this->channel_no]["atss_header"]["date"];
    if(isset($_SESSION["adu"][$this->channel_no]["atss_header"]["time"]))          $this->time =  $_SESSION["adu"][$this->channel_no]["atss_header"]["time"];
    if(isset($_SESSION["adu"][$this->channel_no]["atss_header"]["fracs"]))         $this->fracs =  floatval($_SESSION["adu"][$this->channel_no]["atss_header"]["fracs"]);
    if(isset($_SESSION["adu"][$this->channel_no]["atss_header"]["latitude"]))      $this->latitude =  floatval($_SESSION["adu"][$this->channel_no]["atss_header"]["latitude"]);
    if(isset($_SESSION["adu"][$this->channel_no]["atss_header"]["longitude"]))     $this->longitude =  floatval($_SESSION["adu"][$this->channel_no]["atss_header"]["longitude"]);
    if(isset($_SESSION["adu"][$this->channel_no]["atss_header"]["elevation"]))     $this->elevation =  floatval($_SESSION["adu"][$this->channel_no]["atss_header"]["elevation"]);
    if(isset($_SESSION["adu"][$this->channel_no]["atss_header"]["dipole_length"])) $this->dipole_length =  floatval($_SESSION["adu"][$this->channel_no]["atss_header"]["dipole_length"]);
    if(isset($_SESSION["adu"][$this->channel_no]["atss_header"]["angle"]))         $this->angle =  floatval($_SESSION["adu"][$this->channel_no]["atss_header"]["angle"]);
    if(isset($_SESSION["adu"][$this->channel_no]["atss_header"]["dip"]))           $this->dip =  floatval($_SESSION["adu"][$this->channel_no]["atss_header"]["dip"]);
    if(isset($_SESSION["adu"][$this->channel_no]["atss_header"]["units"]))         $this->units =  $_SESSION["adu"][$this->channel_no]["atss_header"]["units"];
    if(isset($_SESSION["adu"][$this->channel_no]["atss_header"]["source"]))        $this->source =  $_SESSION["adu"][$this->channel_no]["atss_header"]["source"];
    if(isset($_SESSION["adu"][$this->channel_no]["atss_header"]["site"]))          $this->site =  $_SESSION["adu"][$this->channel_no]["atss_header"]["site"];
  }

  function clear($channel_no = -1) {
    $this->channel_no = $channel_no;
    $this->date =  "1970-01-01";   // ISO 8601 date 2021-05-19 UTC
    $this->time =  "00:00:00";     // ISO 8601 time in UTC
    $this->fracs =  0.0;           // factions of seconds; at your own risk. It is always the best to cut the data to full seconds
    $this->latitude =  0.0;        // decimal degree such as 52.2443
    $this->longitude =  0.0;       // decimal degree such as 10.5594
    $this->elevation =  0.0;       // elevation in meter
    $this->dipole_length =  0.0;   // length of dipole in meter
    $this->angle =  0.0;           // orientaion from North to East (90 = East; -90 or 270 = West; 180 South; 0 North)
    $this->dip =  0.0;             // angle positive down North - in case it had been measured
    $this->units =  "mV";          // for ADUs it will be mV (E or H) or scaled E mV/km
    $this->source =  "ns";         // empty or indicate as; ns; ca; cp; tx or what ever
    $this->site =  "site";         // only use when you need it in your file name! leave empty or say site!

  }

  function set_angle($angle) {
    if (!is_numeric($angle)) return;
    $this->angle = floatval($angle);
    $_SESSION["adu"][$this->channel_no]["atss_header"]["angle"] = floatval($this->angle);
  }

  function set_dip($dip) {
    if (!is_numeric($dip)) return;
    $this->dip = floatval($dip);
    $_SESSION["adu"][$this->channel_no]["atss_header"]["dip"] = floatval($this->dip);
  }

} // end class


?>
