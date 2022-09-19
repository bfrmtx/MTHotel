.. _HDF5-Tree:

.. |br| raw:: html

   <br />


HDF5 Tree
===========

The directory structure follows the `MTH5 <https://mth5.readthedocs.io/en/latest/>`_ 
project of the `USGS <https://www.usgs.gov/>`_.

The `HDF5 <https://https://www.hdfgroup.org/>`_ format is a *Hierarchical Data Format* version 5.

It is organized in a virtual directory structure with data description. Especially for storage and reading
this format is well organized.

For distributed data recording and frequent data changes it is *not* made. The **atss** and **JSON** files
are that way organized that the can be finally pushed into the MTH5 HDF5 format.


::
   
   └── Northern Mining
    ├── cal
    ├── db
    ├── doc
    ├── dump
    ├── edi
    ├── filters
    ├── jobs
    ├── log
    ├── processings
    ├── shell
    ├── stations
    │   ├── Line 56 Station 896
    │   ├── Sarıçam
    │   │   ├── Run 001
    │   │   └── Run 999
    │   └── Station 123
    └── tmp

.

