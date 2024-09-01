# Calibration

## ADU-08e

### HF Board 08e

The theoretical transfer function for the HF-channel is given below:

$F_{HF - Channel} = G_{1} \cdot F_{1} \cdot F_{2} \cdot F_{3} \cdot F_{4}$

with

$G_{1} = 1 \ or\  4, 8, 16$  depending on gain setting of first stage


$F_{1} = \frac{1}{1 + P_{1}}$; $P_{1} = i \cdot \frac{f}{338 kHz}$

$F_{2} = \frac{1}{1 + P_{2}}$; $P_{2} = i \cdot \frac{f \cdot G_{1} }{100 MHz}$

$F_{3} = \frac{1}{1 + P_{3}}$; $P_{3} = i \cdot \frac{f}{1.59 MHz}$

$F_{4} = \frac{P_{4}}{1 + P_{4}}$; $P_{4} = i\frac{f}{482 Hz}$
if high-pass is switched on.

$F_{4} = 1$
if high-pass is switched off.  

(gains and input divider are calibrated into the LSB, you don't see them)

### LF Board 08e

$F_{LF - Channel} = G_{1} \cdot G_{2} (\cdot G_{3})  \cdot F_{1} \cdot F_{2} \cdot F_{3} \cdot F_{4}$


$G_{1} = 1 \ or\ 4, 8, 16$ depending on gain stage 1 settings

$G_{2} = 1 \ or\  4, 8, 16, 32, 64$ depending on gain stage 2 settings, inside the ADC; (32, 64 not set by software, only manually)

Gains and input divider do not appear in the ats file, they are calibrated into the **LSB**

$G_{3} = 2$ is a fixed gain and can't be changed; this gain is *invisible* in the time series 

$F_{1} = \frac{1}{1 + P_{1}}$; $P_{1} = i\frac{f}{318 kHz}$

$F_{2} = \frac{1}{1 + P_{2}}$; $P_{2} = i\frac{f \cdot G_1 }{2 MHz}$

$F_{3} = \frac{1}{1 + 1.414 \cdot P_{3} + P_{3}^{2}}$; $P_{3} = i \cdot \frac{f}{4Hz}$
if 4 Hz Low-pass is switched on


and

$F_{4} = \frac{1}{1 + P_{4}}$;
$P_{4} = i \cdot \frac{f}{10.5 kHz}$ if **RF-2 on & DIV-8 = on** (default for **coil**, not used for E)


$F_{4} = \frac{1}{1 + P_{4}}$;
$P_{4} = i \cdot \frac{f}{30 kHz}$ if RF-1 on & DIV-8 = on (not used for E)


$F_{4} = \frac{1}{1 + P_{4}}$;
$P_{4} = i \cdot \frac{f}{\frac{0.159}{(R_{sensor} + 200) \cdot 7.27E^{-9}Hz}}$
if **RF-2 on & DIV-1 = on** (default for electrodes **< 1500 Ohm** contact resistance, default for *buffer* electrodes, not used for H) )


$F_{4} = \frac{1}{1 + P_{4}}$;
$P_{4} = i \cdot \frac{f}{\frac{0.159}{(R_{sensor} + 200) \cdot 470E^{-12}Hz}}$
if **RF-1 on & DIV-1 = on** ( electrodes **> 1500 Ohm** contact resistance, not used for H)

<hr style="height:6px;border-width:0;color:gray;background-color:gray">

## ADU-10e

### LF Board 10e

$F_{LF - Channel} = G_{1}  \cdot F_{1}   \cdot F_{2}$


$G_{1} = 1 \ or\ 4, 8, 16, 32, 64$ depending on gain stage 1 settings, *here tge gain is inside the ADC like gain 2 for old ADUs*


Gains and input divider do not appear in the ats file, they are calibrated into the **LSB**


$F_{1} = \frac{1}{1 + P_{1}}$; $P_{1} = i\frac{f}{318 kHz}$

and

$F_{2} = \frac{1}{1 + P_{2}}$;
$P_{2} = i \cdot \frac{f}{7.8 kHz}$ if **DIV-8 = on** (default for **coil**, not used for E)



$F_{2} = \frac{1}{1 + P_{2}}$;
$P_{2} = i \cdot \frac{f}{\frac{0.159}{(R_{sensor} + 200) \cdot 6.8E^{-9}Hz}}$
if **DIV-1 = on** (default for electrodes **< 1500 Ohm** contact resistance, default for *buffer* electrodes, not used for H) )

alt: $P_{2} = i \cdot 2 \pi f \cdot (R_{sensor} + 200) \cdot 6.8E^{-9}Hz $

<hr style="height:6px;border-width:0;color:gray;background-color:gray">

## ADU-07e

### HF Board 07e

The theoretical transfer function for the HF-channel is given below:

$F_{HF - Channel} = G_{1} \cdot G_{2} \cdot F_{1} \cdot F_{2} \cdot F_{3}$

with

$G_{1} = 1 \ or\ 8$  depending on gain setting of first stage

$G_{2} = 1 \ or\ 8 \ or\ 64$  depending on gain setting of second stage


$F_{1} = \frac{1}{1 + P_{1}}$; $P_{1} = i \cdot \frac{f}{7.7 MHz}$ if $G_1 \ne 1$

$F_{2} = \frac{1}{1 + P_{2}}$; $P_{2} = i \cdot \frac{f}{7.7 MHz}$ if $G_2 \ne 1$

$F_{3} = \frac{P_{3}}{1 + P_{3}}$; $P_{4} = i\frac{f}{1 Hz}$
if high-pass is switched on.

(gains and input divider are calibrated into the LSB, you don't see them)

### LF Board 07e

$F_{LF - Channel} = G_{1} \cdot G_{2} \cdot F_{1} \cdot F_{2} \cdot F_{3}$


$G_{1} = 1 \ or\ 2, 4, 8, 16, 32, 64$ depending on gain stage 1 settings

$G_{2} = 1 \ or\ 2, 4, 8, 16, 32, 64$ depending on gain stage 2 settings, inside the ADC; (32, 64 not set by software, only manually)

$F_{1} = \frac{1}{1 + P_{1}}$; $P_{1} = i \cdot \frac{f}{4 kHz}$ if $G_1 \ne 1$

$F_{3} = \frac{1}{1 + 1.414 \cdot P_{3} + P_{3}^{2}}$; $P_{3} = i \cdot \frac{f}{4Hz}$
if 4 Hz Low-pass is switched on

### MF Board 07e

tbd.

## MFS Coils

<span style="color:red"><b>OLD Calibration Files are normalized by f!</b></span> <br> 
e.g. the MFSXXX.txt files.<br><br>
The JSON files are using mV (as the time series data) and are not normalized by f. <br>
If you normalize the calibration you end up with a *constant* $\frac{200  mV}{nT \cdot Hz}$ and 90° 
phase below 0.1 Hz for the MFS-06e and $\frac{20  mV}{nT \cdot Hz}$ and 90° fro the MFS-07e. <br>

### MFS-06e
$P_{1} = i \cdot \frac{1}{4 Hz}, \enspace P_{2} = i \cdot \frac{1}{8192 Hz} \enspace$
$P_{3} = i \cdot \frac{1}{0.72 Hz},\enspace P_{4} = i \cdot \frac{1}{28300 Hz}$

#### Chopper on
$F_{on}(f) = \frac{ mV}{nT} = 800 \enspace \frac{mV}{nT} \cdot \frac{P_1}{1+P_1} \cdot \frac{1}{1+P_2} \cdot \frac{1}{1+P_4} $

#### Chopper off
$F_{off}(f) = \frac{ mV}{nT} = 800 \enspace \frac{mV}{nT} \cdot \frac{P_1}{1+P_1} \cdot \frac{1}{1+P_2} \cdot \frac{P_3}{1+P_3}  \cdot \frac{1}{1+P_4} $



### MFS-07e
$P_{1} = i \cdot \frac{1}{32 Hz}, \enspace P_{2} = i \cdot \frac{1}{40000 Hz} \enspace$
$P_{3} = i \cdot \frac{1}{0.72 Hz},\enspace P_{4} = i \cdot \frac{1}{50000 Hz}$

#### Chopper on
$F_{on}(f) = \frac{ mV}{nT} = 640 \enspace \frac{mV}{nT} \cdot \frac{P_1}{1+P_1} \cdot \frac{1}{1+P_2} \cdot \frac{1}{1+P_4} $

#### Chopper off
$F_{off}(f) = \frac{ mV}{nT} = 640 \enspace \frac{mV}{nT} \cdot \frac{P_1}{1+P_1} \cdot \frac{1}{1+P_2} \cdot \frac{P_3}{1+P_3}  \cdot \frac{1}{1+P_4} $
