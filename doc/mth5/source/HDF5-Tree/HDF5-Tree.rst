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
   
    Northern Mining
    ├── config
    ├── db
    ├── dump
    ├── edi
    ├── filters
    ├── jle
    ├── jobs
    ├── log
    ├── meta
    │   └── Sarıçam
    │       ├── run_001
    │       │   ├── 084_2009-08-20_13-22-00_2009-08-21_07-00-00_R001_128H.xml
    │       │   ├── 084_ADU-07e_C000_TEx_128Hz.json
    │       │   ├── 084_ADU-07e_C001_TEy_128Hz.json
    │       │   ├── 084_ADU-07e_C002_THx_128Hz.json
    │       │   ├── 084_ADU-07e_C003_THy_128Hz.json
    │       │   └── 084_ADU-07e_C004_THz_128Hz.json
    │       └── run_002
    │           ├── 084_2009-08-21_07-01-00_2009-08-21_07-06-00_R001_2048H.xml
    │           ├── 084_ADU-07e_C000_TEx_2048Hz.json
    │           ├── 084_ADU-07e_C001_TEy_2048Hz.json
    │           ├── 084_ADU-07e_C002_THx_2048Hz.json
    │           ├── 084_ADU-07e_C003_THy_2048Hz.json
    │           └── 084_ADU-07e_C004_THz_2048Hz.json
    ├── processings
    ├── reports
    │   └── old_cal
    │       ├── FGS03E_000_master.txt
    │       ├── MFS05141.TXT
    │       ├── MFS06024.TXT
    │       ├── MFS06026.TXT
    │       ├── MFS06032.TXT
    │       ├── MFS06E_000_master.txt
    │       ├── MFS07E_000_master.txt
    │       └── MFS10e0021.TXT
    ├── shell
    │   ├── mkallproc.sh
    │   ├── plot_ascii_table_edi.sh
    │   └── procall.sh
    ├── stations
    │   └── Sarıçam
    │       ├── run_001
    │       │   ├── 084_ADU-07e_C000_TEx_128Hz.atss
    │       │   ├── 084_ADU-07e_C000_TEx_128Hz.json
    │       │   ├── 084_ADU-07e_C001_TEy_128Hz.atss
    │       │   ├── 084_ADU-07e_C001_TEy_128Hz.json
    │       │   ├── 084_ADU-07e_C002_THx_128Hz.atss
    │       │   ├── 084_ADU-07e_C002_THx_128Hz.json
    │       │   ├── 084_ADU-07e_C003_THy_128Hz.atss
    │       │   ├── 084_ADU-07e_C003_THy_128Hz.json
    │       │   ├── 084_ADU-07e_C004_THz_128Hz.atss
    │       │   └── 084_ADU-07e_C004_THz_128Hz.json
    │       └── run_002
    │           ├── 084_ADU-07e_C000_TEx_2048Hz.atss
    │           ├── 084_ADU-07e_C000_TEx_2048Hz.json
    │           ├── 084_ADU-07e_C001_TEy_2048Hz.atss
    │           ├── 084_ADU-07e_C001_TEy_2048Hz.json
    │           ├── 084_ADU-07e_C002_THx_2048Hz.atss
    │           ├── 084_ADU-07e_C002_THx_2048Hz.json
    │           ├── 084_ADU-07e_C003_THy_2048Hz.atss
    │           ├── 084_ADU-07e_C003_THy_2048Hz.json
    │           ├── 084_ADU-07e_C004_THz_2048Hz.atss
    │           └── 084_ADU-07e_C004_THz_2048Hz.json
    └── tmp


.

