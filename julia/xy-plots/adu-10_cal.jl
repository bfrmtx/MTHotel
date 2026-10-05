# ADU-10 Calibration Formulas

#=
$F_{LF - Channel} = G_{1}  \cdot F_{1}   \cdot F_{4}$

$G_{1} = 1 \ or\ 4, 8, 16, 32, 64$ <br>
The $G_{1}$ (inside ADC) is a chopper stabilized gain. The SW shall set 1 as default, for E this means $\pm$ 2.5 V input range .

Gains and input divider do not appear in the ats file, they are calibrated into the **LSB**

$F_{1} = \frac{1}{1 + P_{1}}$; $P_{1} = i\frac{f}{318 kHz}$

and H:

$F_{4} = \frac{1}{1 + P_{4}}$;
$P_{4} = i \cdot \frac{f}{7.8 kHz}$ if **DIV-8 = on** (default for **coil**, not used for E)

and E:
$F_{4} = \frac{1}{1 + P_{4}}$;
$P_{4} = i \cdot 2 \pi f \cdot (R_{sensor} + 200) \cdot 6.8E^{-9}Hz$ , if **DIV-1 = on** (default for electrodes, not used for H) )

=#
using GLMakie
using CairoMakie
using Printf
GLMakie.activate!()

# Save PNG only; the 3D SVG output is incompatible with external viewers.
# Save beside this script after closing each window, retaining its final view.
function save_plot(fig, basename)
    CairoMakie.activate!()
    try
        path = joinpath(@__DIR__, "$basename.png")
        save(path, fig)
        println("Saved ", path)
    finally
        GLMakie.activate!()
    end
end

# Explicit meshes avoid Surface's conflicting computations when switching backends.
function response_surface!(ax, x, y, magnitude)
    mesh!(ax, Makie.surface2mesh(x, y, magnitude);
        color=vec(magnitude), colormap=:viridis)
end

frequencies = vcat(1, 10:10:100, 200:100:5000)
resistances = [100.0, 300.0, 800.0, 3200.0, 10000.0]
gains = [1, 4, 8, 16]
iso_levels = [0.85, 0.90, 0.95]

# H-channel response with DIV-8 enabled; normalize magnitude by gain.
h_response = [
    gain / ((1 + im * frequency / 318_000) * (1 + im * frequency / 7_800))
    for frequency in frequencies, gain in gains
]
h_magnitude = abs.(h_response) ./ reshape(gains, 1, :)

# Axis3 has no xscale option, so transform coordinates and retain Hz tick labels.
log_frequencies = log10.(frequencies)
frequency_ticks = [1, 10, 100, 1000, 5000]
frequency_sections = (10, 100, 1000, 2000)

# The last frequency section marks the interpretation limit in every plot.
frequency_section_style(frequency) = frequency == last(frequency_sections) ?
    (color=:red, linestyle=:dot) : (color=:black, linestyle=:solid)

fig = Figure(size=(1100, 750))
ax = Axis3(
    fig[1, 1],
    title="ADU-10 H-channel gain-normalized LF response (DIV-8 on)",
    xlabel="Frequency (Hz, log scale)",
    ylabel="Gain G1",
    zlabel="Normalized amplitude |F_LF-Channel| / G1",
    xticks=(log10.(frequency_ticks), string.(frequency_ticks)),
    yticks=gains,
)
response_surface!(ax, log_frequencies, gains, h_magnitude)
contour3d!(ax, log_frequencies, gains, h_magnitude;
    levels=iso_levels, color=:grey, linewidth=3, labels=true,
    labelformatter=level -> @sprintf("%.2f", level), labelsize=18, overdraw=true)
for (index, gain) in enumerate(gains)
    lines!(ax, log_frequencies, fill(gain, length(frequencies)), h_magnitude[:, index];
        color=:black, linewidth=2)
end
for frequency in frequency_sections
    index = findfirst(==(frequency), frequencies)
    lines!(ax, fill(log_frequencies[index], length(gains)), gains, h_magnitude[index, :];
        frequency_section_style(frequency)..., linewidth=2)
    text!(ax, fill(log_frequencies[index], length(gains)), gains, h_magnitude[index, :];
        text=[@sprintf("%.2e", value) for value in h_magnitude[index, :]],
        align=(:left, :bottom), offset=(4, 4), fontsize=16,
        color=:black, strokecolor=:white, strokewidth=1, overdraw=true)
end
xlims!(ax, log_frequencies[end], log_frequencies[1])
ylims!(ax, first(gains), last(gains))
zlims!(ax, minimum(h_magnitude), 1)
# display(fig)
screen = display(fig)
wait(screen)
save_plot(fig, "adu-10_h_channel_normalized")
# continue with additional plots for each gain value (E-channel response)
# we almost repeat the same steps for the E-channel , but just use the E-channel formula for F4 and include the resistances.

# E-channel response with DIV-1 enabled; normalize magnitude by gain.
e_response = [
    gain / ((1 + im * frequency / 318_000) * (1 + im * 2 * π * frequency * (resistance + 200) * 6.8e-9))
    for frequency in frequencies, resistance in resistances, gain in gains
]
e_magnitude = abs.(e_response) ./ reshape(gains, 1, 1, :)

for (gain_index, gain) in enumerate(gains)
    fig_e = Figure(size=(1100, 750))
    ax_e = Axis3(
        fig_e[1, 1],
        title="ADU-10e E-channel response gain $gain (normalized, DIV-1 on)",
        xlabel="Frequency (Hz, log scale)",
        ylabel="Resistance (Ohm)",
        zlabel="Normalized amplitude |F_LF-Channel| / G1",
        xticks=(log10.(frequency_ticks), string.(frequency_ticks)),
        yticks=resistances,
    )
    response_surface!(ax_e, log_frequencies, resistances, e_magnitude[:, :, gain_index])
    contour3d!(ax_e, log_frequencies, resistances, e_magnitude[:, :, gain_index];
        levels=iso_levels, color=:grey, linewidth=3, labels=true,
        labelformatter=level -> @sprintf("%.2f", level), labelsize=18, overdraw=true)
    for (index, resistance) in enumerate(resistances)
        lines!(ax_e, log_frequencies, fill(resistance, length(frequencies)), e_magnitude[:, index, gain_index];
            color=:black, linewidth=2)
    end
    for frequency in frequency_sections
        index = findfirst(==(frequency), frequencies)
        lines!(ax_e, fill(log_frequencies[index], length(resistances)), resistances, e_magnitude[index, :, gain_index];
            frequency_section_style(frequency)..., linewidth=2)
    end
    xlims!(ax_e, log_frequencies[end], log_frequencies[1])
    ylims!(ax_e, last(resistances), first(resistances))
    zlims!(ax_e, minimum(e_magnitude), 1)
    screen_e = display(fig_e)
    wait(screen_e)
    save_plot(fig_e, "adu-10e_e_channel_gain_$(gain)_normalized")
end