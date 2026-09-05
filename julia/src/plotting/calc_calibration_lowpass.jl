# try to find a mathematical representation of the lowpass behavior in the calibration data
# we have to loop over the measured spectra xxxx_smplingrate_cal.csv and the corresponding theoretical spectra yyy_theo_samplingrate_cal.csv
# example: 003_ADU-11e_C000_TEx_8192Hz_cal.csv and 003_theo_C000_TEx_8192Hz_cal.csv, so only ADU-11e and theo differ
# this repeats for 8192Hz up to 131072Hz for example, same system
# expected are 5 different sampling rates for the SAME system
# the hope is that all lowpass behaviors can be captured consistently across different sampling rates (SAME system)
# we assume that the lowest frequency in spectra == 1, and fc (cut off frequency) is to be determined
# the overall goal is to do this five times, so that I can determine the lowpass behavior consistently across all sampling rates for the same system

# MODEL: the measured spectrum is the theoretical (reference) spectrum attenuated by a lowpass,
# ratio(f) = measured(f)/theoretical(f) ~ H(f) (both spectra are already normalized to 1 at their
# lowest frequency, so ratio(lowest_freq) ~ 1 and any constant gain cancels out, leaving only the
# roll-off shape to fit). Two candidate shapes for H are given below (see fit_lowpass/lowpass_response);
# both are fit by LINEAR regression rather than a nonlinear solver, by linearizing in log-log space.

using CSV
using DataFrames
using GLMakie
using Printf

const CAL_FILENAME_RE = r"^(\d+)_([^_]+)_C(\d+)_T([A-Za-z]+)_(\d+(?:Hz|s))_cal\.csv$"

struct CalRecord
    serial::Int
    system::String        # device system tag, or "theo" for the theoretical reference
    channel_no::Int
    channel_type::String
    rate::String           # e.g. "131072Hz" or "2s"
    df::DataFrame
end

# splits "<serial>_<system>_C<channel_no>_T<channel_type>_<rate>_cal.csv" into its tags
function parse_cal_filename(path::AbstractString)
    m = match(CAL_FILENAME_RE, basename(path))
    return m === nothing ? nothing :
        (serial=parse(Int, m[1]), system=String(m[2]), channel_no=parse(Int, m[3]),
         channel_type=String(m[4]), rate=String(m[5]))
end

# loads all dropped/given *_cal.csv files into CalRecords, skipping anything that doesn't match
function load_cal_records(paths::Vector{String})
    records = CalRecord[]
    for p in paths
        endswith(p, ".csv") || continue
        tags = parse_cal_filename(p)
        if tags === nothing
            @warn "filename does not match calibration csv pattern, skipping" path = p
            continue
        end
        try
            df = CSV.read(p, DataFrame)
            push!(records, CalRecord(tags.serial, tags.system, tags.channel_no, tags.channel_type, tags.rate, df))
        catch e
            @warn "failed to read csv" path = p exception = e
        end
    end
    return records
end

# "same system" key: everything the filename encodes except the system tag and the sampling rate, so
# a measured record and its theoretical counterpart (system == "theo") land in the same group -- they
# are expected to share the same serial (req.: "only ADU-11e and theo differ")
channel_key(r::CalRecord) = (r.serial, r.channel_no, r.channel_type)

# rate string like "131072Hz" or "2s" -> numeric Hz-equivalent, for sorting
function rate_sort_key(rate::AbstractString)
    n = parse(Float64, match(r"^(\d+)", rate).captures[1])
    return endswith(rate, "s") ? 1 / n : n
end

# formats a single log-axis tick as a plain (non-exponent) number, abbreviating values >= 1000 with a
# "k" suffix (e.g. 20000 -> "20k") instead of Makie's default "10^n" style
function tick_label(v::Real)
    v == 0 && return "0"
    av = abs(v)
    if av >= 1000
        scaled = v / 1000
        s = scaled == round(scaled) ? string(round(Int, scaled)) : string(round(scaled, digits=1))
        return s * "k"
    end
    return v == round(v) ? string(round(Int, v)) : string(round(v, digits=3))
end
real_number_ticks(values) = tick_label.(values)

# groups by channel_key (same physical channel/system), then by sampling rate within that; each rate
# bucket is expected to hold one theoretical record (system == "theo") plus one or more measured ones
function group_by_channel(records::Vector{CalRecord})
    groups = Dict{Tuple{Int,Int,String},Dict{String,Vector{CalRecord}}}()
    for r in records
        by_rate = get!(groups, channel_key(r), Dict{String,Vector{CalRecord}}())
        push!(get!(by_rate, r.rate, CalRecord[]), r)
    end
    return groups
end

# measured/theoretical amplitude ratio at each measured frequency, matched to the nearest theoretical
# frequency (they should coincide exactly, both being the same harmonic series); points with a
# non-positive amplitude or without a close theoretical match are skipped
function ratio_spectrum(measured::DataFrame, theo::DataFrame)
    freqs = Float64[]
    ratios = Float64[]
    for (f, a) in zip(measured.frequency_hz, measured.amplitude)
        idx = argmin(abs.(theo.frequency_hz .- f))
        (abs(theo.frequency_hz[idx] - f) > 1e-6 * max(f, 1.0) || a <= 0 || theo.amplitude[idx] <= 0) && continue
        push!(freqs, f)
        push!(ratios, a / theo.amplitude[idx])
    end
    return freqs, ratios
end

# fits ratio(f) ~ 1/sqrt(1+(f/fc)^(2n)) via the log-log linearization described above; only points
# with 0 < ratio < 1 contribute (ratio >= 1 would need a negative argument to the log)
# -- commented out in favor of the super-Gaussian model below, which starts its roll-off later but
# falls off more steeply once it kicks in, matching the measured data better
# function fit_lowpass(freqs::AbstractVector, ratios::AbstractVector)
#     mask = (ratios .> 0) .& (ratios .< 1)
#     count(mask) < 2 && return (fc=NaN, n=NaN)
#     x = log.(freqs[mask])
#     y = log.(1 ./ ratios[mask] .^ 2 .- 1)
#     slope, intercept = [x ones(length(x))] \ y
#     n = slope / 2
#     return (fc=exp(-intercept / slope), n=n)
# end
# lowpass_response(f::Real, fc::Real, n::Real) = 1 / sqrt(1 + (f / fc)^(2n))

# ALTERNATIVE MODEL: super-Gaussian roll-off, ratio(f) ~ exp(-(f/fc)^(2n)); unlike the Butterworth-
# style model above, this stays close to 1 for longer (decay starts nearer fc) but then drops off much
# faster than any power law once (f/fc)^(2n) exceeds ~1 -- i.e. later onset, steeper drop.
# Same linearization trick applies: -log(ratio) = (f/fc)^(2n), so
#     log(-log(ratio)) = 2n*log(f) - 2n*log(fc)
# is again a straight line in log-log space, fit by the same OLS as before.
function fit_lowpass(freqs::AbstractVector, ratios::AbstractVector)
    mask = (ratios .> 0) .& (ratios .< 1)
    count(mask) < 2 && return (fc=NaN, n=NaN)
    x = log.(freqs[mask])
    y = log.(.-log.(ratios[mask]))
    slope, intercept = [x ones(length(x))] \ y
    n = slope / 2
    return (fc=exp(-intercept / slope), n=n)
end

# the fitted lowpass magnitude response, for overlaying on the ratio plot
lowpass_response(f::Real, fc::Real, n::Real) = exp(-(f / fc)^(2n))


# undoes the fitted roll-off: divide each measured amplitude by the model's predicted attenuation at
# that frequency to recover an estimate of the true (theoretical) amplitude
correct_spectrum(freqs::AbstractVector, amps::AbstractVector, fc::Real, n::Real) =
    amps ./ lowpass_response.(freqs, fc, n)

mutable struct ViewerState
    records::Vector{CalRecord}
    axes::Vector{Axis}
end

ViewerState() = ViewerState(CalRecord[], Axis[])

function clear_axes!(state::ViewerState)
    foreach(delete!, state.axes)
    empty!(state.axes)
end

# one Axis per channel_key (physical channel/system): the measured/theoretical ratio at each sampling
# rate as markers, plus the fitted lowpass curve (dashed) with fc/n reported in the legend label, so
# fc/n can be compared by eye across all sampling rates of the SAME system
function rebuild_axes!(plot_area, state::ViewerState)
    clear_axes!(state)
    isempty(state.records) && return

    groups = group_by_channel(state.records)
    colors = Makie.wong_colors()

    keys_sorted = sort(collect(keys(groups)))
    for (i, key) in enumerate(keys_sorted)
        serial, channel_no, channel_type = key
        by_rate = groups[key]
        ax = Axis(plot_area[i, 1], xlabel="frequency (Hz)", ylabel="ratio (measured/theoretical)",
                  title=@sprintf("%03d_C%03d_T%s", serial, channel_no, channel_type),
                  xscale=log10, yscale=log10,
                  xtickformat=real_number_ticks, ytickformat=real_number_ticks)
        push!(state.axes, ax)

        println(@sprintf("%03d_C%03d_T%s", serial, channel_no, channel_type))
        plotted = false
        for (j, rate) in enumerate(sort(collect(keys(by_rate)), by=rate_sort_key))
            recs = by_rate[rate]
            theo_idx = findfirst(r -> r.system == "theo", recs)
            if theo_idx === nothing
                @warn "no theoretical reference for this rate, skipping fit" key = key rate = rate
                continue
            end
            theo = recs[theo_idx]
            color = colors[mod1(j, length(colors))]
            for meas in filter(r -> r.system != "theo", recs)
                freqs, ratios = ratio_spectrum(meas.df, theo.df)
                isempty(freqs) && continue
                fit = fit_lowpass(freqs, ratios)
                lbl = isnan(fit.fc) ? "$(rate) (fit failed)" : @sprintf("%s: fc=%.0fHz n=%.2f", rate, fit.fc, fit.n)
                println("  ", lbl)
                scatter!(ax, freqs, ratios, color=color, markersize=8, label=lbl)
                plotted = true
                isnan(fit.fc) && continue
                fplot = exp.(range(log(freqs[1]), log(freqs[end]), length=200))
                lines!(ax, fplot, lowpass_response.(fplot, fit.fc, fit.n), color=color, linestyle=:dash)
            end
        end
        ylims!(ax, nothing, 1.2)
        plotted && axislegend(ax, position=:lb)
    end
end

function build_viewer()
    state = ViewerState()
    fig = Figure(size=(1200, 900))

    info_label = Label(fig[1, 1], "drag & drop *_cal.csv files (measured + theoretical) here", tellwidth=false)
    plot_area = fig[2, 1] = GridLayout()
    rowsize!(fig.layout, 2, Relative(0.95))

    on(events(fig.scene).dropped_files) do paths
        records = load_cal_records(collect(paths))
        if isempty(records)
            info_label.text[] = "no valid *_cal.csv files dropped"
            return
        end
        state.records = records
        n_groups = length(group_by_channel(records))
        info_label.text[] = "loaded $(length(records)) file(s) in $n_groups channel group(s)"
        rebuild_axes!(plot_area, state)
    end

    return fig
end

fig = build_viewer()
screen = display(fig)
wait(screen)

# SUMMARY: to correct a measured spectrum for this system's lowpass roll-off, you only need fc and n
# (fitted once per system/sampling rate above, see fit_lowpass) to generate the predicted attenuation
# lowpass_response(f, fc, n) at every frequency f, then divide it out of the measured amplitude
# (see correct_spectrum) to recover an estimate of the true, unattenuated spectrum.
# So with fc and n in hand, you do:
# vector of frequencies `freqs` and measured amplitudes `amps`:
# corrected_amps = correct_spectrum(freqs, amps, fc, n)
#
# C++ version:
# std::vector<double> freqs = ...; // your frequency vector
# std::vector<complex<double>> spectrum = ...;  // your measured amplitude vector
# double fc = ...; // fitted cutoff frequency
# double n = ...;  // fitted order

# vector<double> calibration_amplitude(const std::vector<double>& freqs, double fc, double n,
#                      
