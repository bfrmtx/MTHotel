```{csv-table} fft_definitions
:name: fft_definitions
:align: center
:header-rows: 1
"time domain","symbol (t)","frequency domain","symbol (f)"
"sampling rate","f<sub>s</sub> = 1/Δt",bandwidth,"f<sub>s</sub> = 1/(2*Δt)"
samples,N,"spectral lines","N/2 + 1 (+1 == DC part)"
"window length","wl = T = N *Δt","frequency resolution [Hz] <br> (bandwith)",":math:`Δf = \frac{bw}{N/2} = \frac{ f_s }{N}`"
```
