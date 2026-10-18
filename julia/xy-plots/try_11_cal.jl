using Plots

"""
We will use typical AD7771 parameters: a Modulator Frequency f_{MOD} of 2.048 MHz and a Decimation Factor (N) of 64, which yields an Output Data Rate (ODR) of 32 kSPS (Nyquist = 16 kHz).
"""
function plot_sinc5_response(; odr=32000.0, decimation_N=64)
    # Define frequency range from DC to the Nyquist frequency
    nyquist = odr / 2
    frequencies = range(0.01, stop=nyquist, length=10000)
    
    # Pre-allocate arrays for magnitude and correction
    h_mag = zeros(length(frequencies))
    h_inv = zeros(length(frequencies))
    
    for (i, f) in enumerate(frequencies)
        if f == 0.0
            h_mag[i] = 1.0  # DC gain is 1 (0 dB)
        else
            numerator = sin(pi * f / odr)
            denominator = decimation_N * sin(pi * f / (decimation_N * odr))
            h_mag[i] = (numerator / denominator)^5
        end
        h_inv[i] = 1.0 / h_mag[i]
    end
    
    # Convert magnitude to Decibels (dB)
    #h_mag_db = 20 .* log10.(h_mag)
    h_mag_db = h_mag
    
    # --- Plotting ---
    # Plot 1: Filter Magnitude Attenuation (dB)
    p1 = plot(frequencies ./ 1000, h_mag_db, 
              xlabel="Frequency (kHz)", 
              ylabel="Gain (dB)", 
              label="sinc5 Response", 
              linewidth=2, 
              color=:blue, 
              title="AD7771 sinc5 Filter Passband Droop",
              grid=true)
    
    # Plot 2: Linear Inverse Correction Factors
    p2 = plot(frequencies ./ 1000, h_inv, 
              xlabel="Frequency (kHz)", 
              ylabel="Multiplier Factor", 
              label="Inverse Filter (1/|H|)", 
              linewidth=2, 
              color=:crimson, 
              title="Required Amplitude Correction",
              grid=true)
    
    # Combine plots vertically
    return plot(p1, p2, layout=(2, 1), size=(800, 600))
end

# Render in Julia front ends and save a file when launched as a script.
fig = plot_sinc5_response(odr=32000.0, decimation_N=128)
display(fig)
# wait for user input to close the plot
readline()
output_path = joinpath(@__DIR__, "try_11_cal.png")
savefig(fig, output_path)
println("Saved plot to ", output_path)