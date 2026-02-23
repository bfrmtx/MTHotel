# MTHotel

the documentation can be found at [MTHotel](https://mth5.geo-metronix.de/introduction/introduction.html)

## Data Formats

Sometimes I need to store data. For sure, ASCII is preferred, but binary formats are also supported. <br>
The **big problem** with ASCII is the precision of floating point numbers, especially for the **frequency** values itself. <br>
If you continue processing data and mix results, 3.999999 and 4.0 and 4.000001 are different values and lead to different results. <br>
Example: the representation of 0.1 in floating point is not exact (0.10000000149011612 in single precision). So when reading a "0.1" I *may* be rounded in a different in your main memory ... and when you now process data (freshly calculated, and loaded from disk) you may have *two different frequencies*. <br>

**HINT**: When you restore .json files with calibration data interpolated, AND load spectra from a binary format, *exchange* the frequency values from the calibration with the frequency values from the spectra file. This way you ensure that both datasets have exactly the same frequency values.

### Binary

- .binfa Binary, 2 columns first frequency, second amplitude
- .binfp Binary, 2 columns first frequency, second phase (very rare)
- .binfap Binary, 3 columns first frequency, second amplitude, third phase
- .binfc Binary, 3 columns frequency, complex (real, imag)
- .binxy Binary, 2 columns first x, second y (generic)
- .binxyz Binary, 3 columns first x, second y, third z (generic)


### ASCII

- .datfa ASCII, 2 columns first frequency, second amplitude
- .datfp ASCII, 2 columns first frequency, second phase (very rare)
- .datfap ASCII, 3 columns first frequency, second amplitude, third phase
- .datfc ASCII, 3 columns frequency, complex (real, imag)
- .datxy ASCII, 2 columns first x, second y (generic)
- .datxyz ASCII, 3 columns first x, second y, third z (generic)