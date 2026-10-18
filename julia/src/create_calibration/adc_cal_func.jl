# ADC Calibration Function
# Configuration --------------------
adc = "AD7771"
model = "sinc5" # ADC filter model
mode = "HR" # high resolution
ODR = 131.072 # Output data rate in kHz (65.536, 32.768, 16.384, 8.192, 4.096, 2.048, 1.024) 1.024 is the lowest possible rate, INPUT
R = 0.1 # Input resistance in kOhms, INPUT
C = 1 # Input capacitance in nF, INPUT
# ---------- PLOT SETTINGS ----------
x_axis_min = 1 # minimum value for the x-axis in the calibration plot, INPUT
x_axis_max = 16384 # maximum value for the x-axis in the calibration plot, INPUT
y_axis_min = 0.1 # minimum value for the y-axis in the calibration plot, INPUT
y_axis_max = 1 # maximum value for the y-axis in the calibration plot, INPUT
# ---------- END OF PLOT SETTINGS ----------

# ADC settings select



# Device Parameters
MCLK = 8388.608 # Master clock frequency in kHz