# Calibration Setup

## Signal Generator

* Select a function sine waveform 
* Select the start frequency (e.g 1/4 of the sampling rate). If you take too high frequencies, this frequency is poorly estimated, and the filter curve distorted. Take a 2<sup>2</sup> sine frequency.
* Set the amplitude, less than 1.25 V, e.g. **0.8 V** to be off from the limit
* repeat for all desired frequencies 2<sup>2</sup>. I take 5 jobs:<br> 
  1. 1/4
  2. 1/8
  3. 1/16
  4. 1/32
  5. 1/64
<br> until behavior of the system is well captured, so LP ≈ 1, bypass.
* 64 s recording time is ok.

## ADU

* set dipole E<sub>x</sub> to 1000   (old web -500 (x1) and +500 (x2), others 0)
* set dipole E<sub>y</sub> to 1000   (old web -500 (y1) and +500 (y2), others 0)
* Connect the ADU to the signal generator output
* Ensure the ADU is powered on and properly configured
* Ensure the signal generator transmits.
* GPS must be synchronized



