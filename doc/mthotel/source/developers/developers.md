# Developers

## ats

The ats file format is outdated.

The atstools conversion tools transform a 100% lossless conversion into the new atss format.

You do not want to write software here.

The ADU series 10e, 11e and 12e provide this format natively.

## atss

Distributed systems can not assemble a MTH5 file.

atss is the format to be pushed into MTH5.

Since HDF5 format is poor in re-writing data, you may decide to create your own timeseries tools here.

This is in general filtering, pre-whitening, spike elimination, splitting. <br>
The new ADU series will provide timeseries splitting (efficient for remote data transfer) - and the new atss format can be easily *concatenated*. 

## HDF 5

**The overall goal** is to push data into MTH5 which is capable to run run *any* processing made by others.

Do not leave questions open before pushing.