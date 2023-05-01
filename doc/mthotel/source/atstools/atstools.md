# atstools


Contains a set of tools for converting old ats format into atss format.

For changing data from ats or *procmt*[^myref] to the new atss format only two steps are needed.

## create old survey

Skip this step if you already have *procmt* data. <br>
atstools possibly makes use of  all information inside the *procmt* survey directory.

If you haven't use *procmt* you create a survey tree and put your timeseries folder from the ADU system inside the ts/site folder 

`atstools -create_old_tree /survey/old/iron_mountain L1_S3 L2_S24 "ref station"` <br>
creates a survey tree in /surveys named iron_mountain with stations L1_S3 and L2_S24 and a station "ref station" : **note the quotes!!**

Cpoy your meas directories from the ADU into desired station, so that is looks like
```text
 /survey/iron_mountain/ts 
L1_S3
├── meas_2009-08-20_13-22-00
│   ├── 084_2009-08-20_13-22-00_2009-08-21_07-00-00_R001_128H.xml
│   ├── 084_V01_C00_R001_TEx_BL_128H.ats
│   ├── 084_V01_C01_R001_TEy_BL_128H.ats
│   ├── 084_V01_C02_R001_THx_BL_128H.ats
│   ├── 084_V01_C03_R001_THy_BL_128H.ats
│   └── 084_V01_C04_R001_THz_BL_128H.ats
├── meas_2009-08-20_13-22-01
│   ├── 084_2009-08-20_13-22-01_2009-08-21_06-59-59_R001_32H.xml
│   ├── 084_V01_C00_R001_TEx_BL_32H.ats
│   ├── 084_V01_C01_R001_TEy_BL_32H.ats
│   ├── 084_V01_C02_R001_THx_BL_32H.ats
│   ├── 084_V01_C03_R001_THy_BL_32H.ats
│   └── 084_V01_C04_R001_THz_BL_32H.ats

... and so on

```


## tojson


From an existing **procmt** directory structure a new [HDF5-Tree](../HDF5-Tree/HDF5-Tree.md#hdf5-tree) is created. <br>
Old ats files appear as [atss](../atss/atss.md#atss) files and the atsheader is converted to a [JSON header](../atss/atss.md#header).

`atstools -tojson -clone -outdir /survey/  /survey/old/iron_mountain`

A new MTH5 compatible tree with name *iron_mountain* will be created in /survey.

The calibration is taken from the XML descriptor, converted into the new JSON format and stored inside the JSON descriptor.

The remaining logs, as well as the XML itself are copied into /meta/site_name/run_nnn directory. A complete copy of 
the binary atsheader appears a JSON file. they can be archived.

The philosophy is to store a *minimal information* beside the atss timeseries. <br>
Before you push into MTH5 you verify the data - and off you go! <br>
Hence: if you do not verify the data, a later generation is not in the position to use the data - even if they have the log files
and detailed status information. 



## cat

[^myref]: that tree contains sites, timeseries and in some cases calibration files. Most systems however write their calibration data into  corresponding xml file.