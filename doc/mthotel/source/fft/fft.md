# FFT / DFT

Symbols

```{include} ../sqltables/fft_abbrev.md
```

N corresponds to time measured/recorded or used; for a sampling rate of 1024 Hz and different
window length you get a corresponding time:

2048 pts @1024Hz = 2s = N/fs <br>
1024 pts @1024Hz = 1s <br>
1024 pts @0.0625 (16s) = 16384s <br>

Definitions

```{eval-rst}
.. csv-table::
 :header: "time Domain", "symbol", "frequency domain", "symbol"
 :delim: ;

 sampling rate; f\ :sub:`s` = 1/Δt; (sampling) bandwidth; bw = f\ :sub:`s` /2
 samples; N; spectral lines; N/2 + 1 (+1 == DC part)
 window length;wl = T = N * Δt; frequency resolution [Hz] (bandwith); :math:`Δf = \frac{bw}{N/2} = \frac{ f_s }{N}`
```


```{warning}
Almost everywhere the DC part (or N+1) is not included. <br>
The **FFTW** returns the DC part as 1<sup>st</sup> element. <br>
You need the DC part only for the *inverse* transform. <br>
For MT and noise analysis you *detrend* the data (DC part is removed). And here only
you have the symmetry of (for example) 1024 real input and 512 (511) complex output <br>
The **FFTW** returns the Nyquist frequency (which is a real number) as last element. You
can not use the Nyquist frequency in your analysis.
```

```{note}
In the real world by selecting N **samples** = *wl* you already have a window length of **T**
```

The *Nyquist* frequency is $f_s /2$ where a sine wave would be described with two points. <br>
Additionally some digitizers or loggers use an anti alias filter at 80% of the Nyquist frequency.
So at 1024 Hz sample rate the cut off could be 0.8 * 512 = 410 Hz. <br>
As a good choice for data interpretation a limit of  $f_s / 4$ can be used

weiter

## Power Spectral Density (PSD)

When you change the bandwidth the amplitude also changes.

For a sample rate of 1024 we use 1024 samples and 4096 samples, according to 1 s and 4 s data. <br>
The bin size in the frequency domain has changed now $Δf = \frac{bw}{N/2}$ == 1 Hz and 0.25 Hz
frequency resolution, respectively bin sizes. So *each bin or bucket* of the 4096 window contain 4 times less data.

On <https://community.sw.siemens.com/s/article/what-is-a-power-spectral-density-psd> you find a genius picture for that:

```{warning}
Almost everywhere people are playing with sine waves. <br>
In MT we have broad band noise and **must** scale the FFT. You want to process data with different window length but get the same amplitudes independently from the window length.
```

