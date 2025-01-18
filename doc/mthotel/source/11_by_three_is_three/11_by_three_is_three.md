# 11 by 3 is 3

Quick thoughts on data structures.

## Human vs. Machine

Using C/C++ and integer division, the result is three. <br>
In financial applications, such as your tax declaration, the result is four. <br>

As instrument engineer, you write kernel drivers and use indices starting at zero. <br>
You query your ADC from 0 to N-1, because that is a C or assembler code. <br>

Giant companies with hundreds of developers for your code may add all convenience functions you can think of. <br>
We can't. We have to keep it simple, or [KISS (keep it simple, stupid)](https://en.wikipedia.org/wiki/KISS_principle). <br>

From my experience, even in 100 years, the average user takes a text editor and applies changes to the data.
Even though you supplied a fail safe function to do this with the archive format. <br>

There are two consequences: <br>
1. The metadata is simple such as JSON. <br>
2. The data is simple such as a simple *IEEE 754* double number stream. <br>

Small data, such as calibration data, can be JSON; they are likely to be edited by hand. <br>


## All in, one out

Since we can not write all convince functions, a good idea is to have a dump format maintained by the archive format programmers, where the
user can manipulate (or destroy) the data. <br>
In case he is doing right, he can push the data back into a newly created archive. <br>

```{figure} ../python_graphs/archive_structure_embedded.svg
:name: proposal-io-process
:alt: Proposal IO Process
:align: center
:width: 40%

IO Process
```

