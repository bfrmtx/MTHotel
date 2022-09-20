.. _Introduction:

.. |br| raw:: html

   <br />


Introduction
===============

These pages contain the documentation of the formats and algorithms used
by `metronix geophysics <https://manuals.geo-metronix.de/>`_ .

A greater part is about pushing the data into data later in the `USGS <https://www.usgs.gov/>`_ -> `MTH5 <https://mth5.readthedocs.io>`_ format.

The data loggers as "streaming servers" can not directly write into `HDF5 <https://www.hdfgroup.org>`_ 

However the :ref:`ATSS <ATSS>` data stream and his JSON descriptor are designed to be pushed into the MTH5 format.

Some operations (like FIR filtering or rotation) may before pushing.
