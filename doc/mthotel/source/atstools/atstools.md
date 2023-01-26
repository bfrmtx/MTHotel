# atstools


Contains a set of tools for converting old ats format into atss format.

## tojson


From an existing **procmt** directory structure a new [HDF5-Tree](../HDF5-Tree/HDF5-Tree.md#hdf5-tree) is created. <br>
Old ats files appear as [atss](../atss/atss.md#atss) files and the atsheader is converted to a [JSON header](../atss/atss.md#header).

`-tojson -clone -outdir /tmp/Northern_Mining  /survey-master/Northern_Mining`

The calibration is taken from the XML descriptor, converted into the new JSON format and stored inside the JSON descriptor.

The remaining logs, as well as the XML itself are copied into /meta/site_name/run_nnn directory. A complete copy of 
the binary atsheader appears a JSON file. they can be archived.

The philosophy is to store a *minimal information* beside the atss timeseries. <br>
Before you push into MTH5 you verify the data - and off you go! <br>
Hence: if you do not verify the data, a later generation is not in the position to use the data - even if they have the log files
and detailed status information. 



## cat

