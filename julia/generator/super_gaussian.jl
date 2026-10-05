using GLMakie

# 1. Setup basic parameters
fs = 65536.0          # Sampling rate: 64 kHz
N = 65536              # Simple buffer size
t = (0:N-1) ./ fs     # Time vector

# 2. Your simple chosen test frequencies
f1 = 0.01 * fs        # 655.36 Hz
f2 = 0.1 * fs         # 6.5536 kHz
f3 = 0.2 * fs         # 13.1072 kHz
f4 = 0.3 * fs         # 19.6608 kHz
f5 = 0.4 * fs         # 26.2144 kHz

# 3. Generate the 5-tone signal (No complex phase math, just simple addition)
x = sin.(2π * f1 .* t) + sin.(2π * f2 .* t) + sin.(2π * f3 .* t) + sin.(2π * f4 .* t) + sin.(2π * f5 .* t)
x = x ./ maximum(abs.(x)) # Normalize to fit ADC range (-1 to 1)

# 4. Define your Super-Gaussian Model for the ADC frequency response
fc = 28000.0          # Assumed cutoff frequency (e.g., 28 kHz)
n = 1.06               # n close to 1
super_gaussian(f) = exp(-(f / fc)^(2 * n))

# --- Plotting the concept ---
f_axis = range(0, fs/2, length=500)
response = super_gaussian.(f_axis)

fig = Figure()
ax = Axis(fig[1, 1], xlabel="Frequency (kHz)", ylabel="Gain",
          title="Your Test Tones vs. ADC Filter Model")
lines!(ax, f_axis ./ 1000, response, label="ADC Filter Profile (Super-Gaussian)", linewidth=2)

tones = [f1, f2, f3, f4, f5]
tone_gains = super_gaussian.(tones)
vlines!(ax, tones ./ 1000, color=:red, linestyle=:dash, linewidth=2, label="Your Test Tones")
for (i, (f, g)) in enumerate(zip(tones, tone_gains))
    text!(ax, f / 1000, g + 0.05, text="f$i: $(round(g, digits=2))", color=:red, fontsize=8)
end
axislegend(ax)

# block the script until the plot window is closed
screen = display(fig)
wait(screen)
