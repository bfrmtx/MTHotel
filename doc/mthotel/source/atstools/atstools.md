# atstools

```{index} Software; atstools
```

Contains a set of tools for converting **old ats format into new atss** format.

For changing data from ats or *procmt* [^myref] to the new atss format only two steps are needed.

## create old survey

Skip this step if you already have *procmt* data. <br>
atstools possibly makes use of all information inside the *procmt* survey directory.

If you haven't use *procmt* you create a survey tree and put your timeseries folder from the ADU system inside the ts/site folder 

`atstools -create_old_tree /survey/old/iron_mountain L1_S3 L2_S24 "ref station"` <br>
creates a survey tree in /surveys named iron_mountain with stations L1_S3 and L2_S24 and a station "ref station" : **note the quotes!!**

Copy your meas directories from the ADU into desired station, so that is looks like

```text
/survey/old/iron_mountain/ts 
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
Due to the (possible) complexity of conversion, a "single file" conversion is not yet supported. However, creating and copying 
old data is a question of a minute.


## tojson clone


From an existing **procmt** directory structure a new [HDF5-Tree](../HDF5-Tree/HDF5-Tree.md#hdf5-tree) is created. <br>
Old ats files appear as [atss](../atss/atss.md#atss) files and the atsheader is converted to a [JSON header](../atss/atss.md#header).

`atstools -tojson -clone -outdir /survey/  /survey/old/iron_mountain`

A new MTH5 compatible tree with name *iron_mountain* will be created in /survey.

The calibration is taken from the XML descriptor, converted into the new JSON format and stored inside the JSON descriptor.

The remaining logs, as well as the XML itself are copied into **/meta/site_name/run_nnn directory**. A complete copy of 
the binary atsheader appears a JSON file. They can be archived. <br>
The electric filed is scaled to **mV/km** but no rotated. 

The philosophy is to store a *minimal information* beside the atss timeseries. <br>
Before you push into MTH5 you verify the data - and off you go! <br>
Hence: if you do not verify the data, a later generation is not in the position to use the data - even if they have the log files
and detailed status information. 

## extend_cal

By default the coils are calibrated down to 0.1 Hz. They are trimmed that way that they meet the theoretical calibration.
This is the case for frequencies below 512 Hz.
For 512 Hz and lower the theoretical function meets perfectly.<br>
For all those who can / will not calculate the lower end, they can use the extend option; here the calibration is extended
down to 10E-5 Hz and saved in the corresponding json file.

`atstools -tojson -extend_cal -clone -outdir /survey/  /survey/old/iron_mountain`

## chats

chats is a switch to convert the old **ADU06** ats data into **XML & ats** in ADU-07e/08e format. <br>
Refer also to [chcal](../chcal/chcal.md#chcal) for the calibration files. The (possible) input of 
calibration files will be JSON only; this avoids conflicts with older mtx calibration files.

## cat

Footnotes

[^myref]: that tree contains sites, timeseries and in some cases calibration files. Most systems however write their calibration data into  corresponding xml file.
