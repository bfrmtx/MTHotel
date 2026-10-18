# ADU-10 Calibration Formulas

#=
$F_{LF - Channel} =  F_{1}   \cdot F_{4}$
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
iso_levels = [0.85, 0.90, 0.95]

# H-channel response with DIV-8 enabled.
h_response = [
    1 / ((1 + im * frequency / 318_000) * (1 + im * frequency / 7_800))
    for frequency in frequencies
]
h_magnitude = abs.(h_response)
# phase in degrees, not radians
h_phase = angle.(h_response) .* (180 / π)

# Axis3 has no xscale option, so transform coordinates and retain Hz tick labels.
log_frequencies = log10.(frequencies)
frequency_ticks = [1, 10, 100, 1000, 2000,5000]
plot_red_dot = 2000 # Frequency at which to plot a red dot on the H-channel response curve.
y_line_at = abs.(1 / ((1 + im * plot_red_dot / 318_000) * (1 + im * plot_red_dot / 7_800)))
# same for phase in degrees
y_line_at_phase = angle.(1 / ((1 + im * plot_red_dot / 318_000) * (1 + im * plot_red_dot / 7_800))) * (180 / π)
phase_ticks = sort(unique([collect(-30.0:10.0:0.0); y_line_at_phase]))

fig = Figure(size=(1100, 750))
ax = Axis(
    fig[1, 1],
    title="ADU-10 H-channel LF response (DIV-8 on)",
    xlabel="Frequency (Hz, log scale)",
    ylabel="Amplitude |F_LF-Channel|",
    xticks=(log10.(frequency_ticks), string.(frequency_ticks)),
    yticks=[0.85, 0.90, 0.95, 0.97, 1.00],
)
ax_h_phase = Axis(
    fig[1, 1],
    ylabel="Phase (degrees)",
    yaxisposition=:right,
    yticks=(phase_ticks, [@sprintf("%.1f", tick) for tick in phase_ticks]),
    xgridvisible=false,
    ygridvisible=false,
)
hidexdecorations!(ax_h_phase)
linkxaxes!(ax, ax_h_phase)
amplitude_plot = lines!(ax, log_frequencies, h_magnitude; color=:green, linewidth=2)
phase_plot = lines!(ax_h_phase, log_frequencies, h_phase; color=:dodgerblue, linewidth=2)
axislegend(ax, [amplitude_plot, phase_plot], ["Amplitude", "Phase"]; position=:lb)
scatter!(ax, [log10(plot_red_dot)], [y_line_at]; color=:red, markersize=20)
scatter!(ax_h_phase, [log10(plot_red_dot)], [y_line_at_phase]; color=:red, markersize=20)
hlines!(ax, y_line_at; color=:black, linestyle=:dash, linewidth=2)
hlines!(ax_h_phase, y_line_at_phase; color=:black, linestyle=:dash, linewidth=2)
xlims!(ax, log_frequencies[1], log_frequencies[end])
ylims!(ax, minimum(h_magnitude), 1)
ylims!(ax_h_phase, minimum(h_phase), 0)
# display(fig)
screen = display(fig)
wait(screen)
save_plot(fig, "adu-10_h_channel")
# H phase in degrees, not radians plot


########### NO CHANGES BELOW HERE gains are already removed.
frequency_sections = (10, 100, 1000, 2000) # I take 2000 also as red

# The last frequency section marks the E-channel interpretation limit.
frequency_section_style(frequency) = frequency == last(frequency_sections) ?
    (color=:red, linestyle=:dot) : (color=:black, linestyle=:solid)

# E-channel response with DIV-1 enabled.
e_response = [
    1 / ((1 + im * frequency / 318_000) * (1 + im * 2 * π * frequency * (resistance + 200) * 6.8e-9))
    for frequency in frequencies, resistance in resistances
]
e_magnitude = abs.(e_response)
# phase in degrees not radians
e_phase = angle.(e_response) .* (180 / π)
e_phase_iso_levels = collect(-60.0:10.0:-10.0)

fig_e = Figure(size=(1100, 750))
ax_e = Axis3(
    fig_e[1, 1],
    title="ADU-10e E-channel response (DIV-1 on)",
    xlabel="Frequency (Hz, log scale)",
    ylabel="Resistance (Ohm)",
    zlabel="Amplitude [F_LF-Channel]",
    xticks=(log10.(frequency_ticks), string.(frequency_ticks)),
    yticks=resistances,
)
response_surface!(ax_e, log_frequencies, resistances, e_magnitude)
contour3d!(ax_e, log_frequencies, resistances, e_magnitude;
    levels=iso_levels, color=:grey, linewidth=3, labels=true,
    labelformatter=level -> @sprintf("%.2f", level), labelsize=18, overdraw=true)
for (index, resistance) in enumerate(resistances)
    lines!(ax_e, log_frequencies, fill(resistance, length(frequencies)), e_magnitude[:, index];
        color=:black, linewidth=2)
end
for frequency in frequency_sections
    index = findfirst(==(frequency), frequencies)
    lines!(ax_e, fill(log_frequencies[index], length(resistances)), resistances, e_magnitude[index, :];
        frequency_section_style(frequency)..., linewidth=2)
end
xlims!(ax_e, log_frequencies[end], log_frequencies[1])
ylims!(ax_e, last(resistances), first(resistances))
zlims!(ax_e, minimum(e_magnitude), 1)
screen_e = display(fig_e)
wait(screen_e)
save_plot(fig_e, "adu-10e_e_channel_amplitude")

fig_e_phase = Figure(size=(1100, 750))
ax_e_phase = Axis3(
    fig_e_phase[1, 1],
    title="ADU-10e E-channel phase response (DIV-1 on)",
    xlabel="Frequency (Hz, log scale)",
    ylabel="Resistance (Ohm)",
    zlabel="Phase (degrees)",
    xticks=(log10.(frequency_ticks), string.(frequency_ticks)),
    yticks=resistances,
)
response_surface!(ax_e_phase, log_frequencies, resistances, e_phase)
contour3d!(ax_e_phase, log_frequencies, resistances, e_phase;
    levels=e_phase_iso_levels, color=:grey, linewidth=3, labels=true,
    labelformatter=level -> @sprintf("%.0f", level), labelsize=18, overdraw=true)
for (index, resistance) in enumerate(resistances)
    lines!(ax_e_phase, log_frequencies, fill(resistance, length(frequencies)), e_phase[:, index];
        color=:black, linewidth=2)
end
for frequency in frequency_sections
    index = findfirst(==(frequency), frequencies)
    lines!(ax_e_phase, fill(log_frequencies[index], length(resistances)), resistances, e_phase[index, :];
        frequency_section_style(frequency)..., linewidth=2)
end
xlims!(ax_e_phase, log_frequencies[end], log_frequencies[1])
ylims!(ax_e_phase, last(resistances), first(resistances))
zlims!(ax_e_phase, minimum(e_phase), 0)
screen_e_phase = display(fig_e_phase)
wait(screen_e_phase)
save_plot(fig_e_phase, "adu-10e_e_channel_phase")
