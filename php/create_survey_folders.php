#!/usr/bin/php
<?php
$subdirs = array("config", "db", "dump", "edi", "filters", "jle", "jobs", "log", "processings", "shell", "tmp", "stations", "reports");
#
#
if ($argc < 2 ) {
    exit( "Usage: $argv[0] folder_name" . PHP_EOL );
}
else {
 if (!is_dir("$argv[1]") ) mkdir("$argv[1]");
}
#
if (is_dir("$argv[1]")) {
  foreach ($subdirs as $subdir) {
    $cdir = "$argv[1]" . "/" . "$subdir";
    echo "creating " . $cdir . PHP_EOL;
    if (!is_dir("$cdir") ) mkdir("$cdir");
  }
}
#
?>
