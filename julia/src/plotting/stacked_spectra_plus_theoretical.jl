# plotting stacked spectra for ATSS time series, amplitude density
# a) single mode: all Channels in one plot
# b) multi mode: each Channel in a separate plot row
# c) files MUST have same sampling rate like *_2Hz.atss or *_4s.atss
# d) drop down need for changing window length (and therewith the FFTW window) from 64, 128, ... 131072
# f) a file dialog (or better drag and drop!!) for selecting the data files, e.g. ending with .json
# test data you find in data/sac/run_004 for example

# 1) read the data with windowed FFT (FFTW), preset the sampling rate as window length (wl = sampling_rate), but not smaller than 512
# 2) stack the spectra for plotting
# 3) divide by the window length to get amplitude density (amplitude per sqrt(Hz))
# 4) plot the stacked amplitude density spectra

# This is a spectral calibration tool: up to two measured (time series) spectra are compared against a
# theoretical one. All three are reduced to their harmonics only, since the reference signal is a
# periodic rect/saw calibration wave with base frequency f0 and base amplitude A0:
#   saw:  f0, 2*f0, 3*f0, 4*f0, ... N*f0
#   rect: f0, 3*f0, 5*f0, ...       N*f0   (odd harmonics only)
# harmonic amplitude falls off as A0/n; measured spectra are sampled at the nearest bin to each harmonic
# and plotted as markers (circle/square per file, star for the theoretical) instead of a continuous line

using GLMakie
using FFTW
using Statistics: mean

include(joinpath(@__DIR__, "..", "timeseries", "atss.jl"))

const WL_OPTIONS = [64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288]
const FORM_OPTIONS = ["saw", "rect"]
const BASE_FREQ_OPTIONS = [64, 128, 200, 256, 400, 512, 800, 1024]
const MAX_INPUT_FILES = 2
const DATA_MARKERS = [:circle, :rect]

detrend(x::AbstractVector{<:Real}) = x .- mean(x)

channel_label(ch::Channel) = "$(ch.channel_type)($(lpad(ch.serial, 3, '0')))"

# formats a single log-axis tick as a plain (non-exponent) number, abbreviating values >= 10000 with a
# "k" suffix (e.g. 20000 -> "20k") instead of Makie's default "10^n" style
function tick_label(v::Real)
    v == 0 && return "0"
    av = abs(v)
    if av >= 10000
        scaled = v / 1000
        s = scaled == round(scaled) ? string(round(Int, scaled)) : string(round(scaled, digits=1))
        return s * "k"
    end
    return v == round(v) ? string(round(Int, v)) : string(round(v, digits=3))
end
real_number_ticks(values) = tick_label.(values)

# preset window length: one second of data (rounded sampling rate), but at least 512, snapped to the
# smallest available dropdown option that covers it (req. 1)
function default_wl(fs::Real)
    target = max(512, round(Int, fs))
    idx = findfirst(>=(target), WL_OPTIONS)
    return idx === nothing ? WL_OPTIONS[end] : WL_OPTIONS[idx]
end

mutable struct ViewerState
    channels::Vector{Channel}
    axes::Vector{Axis}
    legend::Union{Nothing,Legend}
end

ViewerState() = ViewerState(Channel[], Axis[], nothing)

# reads dropped .json headers, keeping only the channels that share the first one's sampling rate (req. c)
# and at most MAX_INPUT_FILES of them (this is a two-source calibration comparison, not a general viewer)
function load_channels(paths::Vector{String})
    hdrs = Channel[]
    for p in paths
        endswith(p, ".json") || continue
        try
            push!(hdrs, read_header(p))
        catch e
            @warn "failed to read header" path = p exception = e
        end
    end
    isempty(hdrs) && return hdrs

    rate = hdrs[1].sampling_rate
    same_rate = filter(ch -> ch.sampling_rate == rate, hdrs)
    if length(same_rate) != length(hdrs)
        @warn "dropped channels with a sampling rate different from the first dropped file" kept_rate = rate
    end
    if length(same_rate) > MAX_INPUT_FILES
        @warn "only the first $MAX_INPUT_FILES channels are kept (max input files reached)"
        same_rate = same_rate[1:MAX_INPUT_FILES]
    end
    return same_rate
end

function clear_axes!(state::ViewerState)
    foreach(delete!, state.axes)
    empty!(state.axes)
    # axislegend() creates a separate Block, not a child of the Axis, so it survives delete!(ax)
    state.legend === nothing || delete!(state.legend)
    state.legend = nothing
end

# programmatically sets a Textbox's displayed AND stored value (stored_string only updates on Enter
# otherwise), so refresh!/cut_value pick it up right away
function set_textbox!(tb::Textbox, value::Real)
    str = string(round(value, sigdigits=4))
    tb.displayed_string[] = str
    tb.stored_string[] = str
end

# harmonic numbers n up to fmax/f0 for a rect (odd harmonics only: 1, 3, 5, ...) or saw (all harmonics:
# 1, 2, 3, ...) calibration waveform of base frequency f0
harmonic_numbers(form::Symbol, f0::Real, fmax::Real) =
    (form == :rect ? (1:2:max(1, floor(Int, fmax / f0))) : (1:max(1, floor(Int, fmax / f0))))

# theoretical peak frequencies (n*f0) and amplitudes (a0/n) at the harmonics of the calibration waveform
function theoretical_spectrum(form::Symbol, f0::Real, a0::Real, fmax::Real)
    ns = harmonic_numbers(form, f0, fmax)
    return f0 .* ns, a0 ./ ns
end

# samples the measured spectrum at the bin nearest to each harmonic frequency (req. calibration: only
# the harmonic bins are compared, not the full spectrum)
function harmonic_values(freqs::AbstractVector, stacked::AbstractVector, form::Symbol, f0::Real, fmax::Real)
    ns = harmonic_numbers(form, f0, fmax)
    hfreqs = f0 .* ns
    hvals = [stacked[argmin(abs.(freqs .- hf))] for hf in hfreqs]
    return hfreqs, hvals
end

# writes one row per harmonic (frequency, amplitude) as csv
function write_harmonics_csv(path::AbstractString, freqs::AbstractVector, vals::AbstractVector)
    open(path, "w") do io
        println(io, "frequency_hz,amplitude")
        for (f, v) in zip(freqs, vals)
            println(io, "$(f),$(v)")
        end
    end
end

# splits the channel's full data into consecutive non-overlapping wl-sized windows, FFTs each one (req. 1/2)
# and returns the frequency axis plus the STACKED (summed then divided by the number of stacks) amplitude
# density spectrum (req. 3/4)
function stacked_spectra(ch::Channel, wl::Integer)
    n_windows = get_samples(ch) ÷ wl
    freqs = rfftfreq(wl, ch.sampling_rate)
    stacked = sum(abs.(rfft(detrend(read_data(ch; start=(k - 1) * wl, wl=wl)))) for k in 1:n_windows)
    stacked ./= wl * n_windows
    return freqs, stacked
end

# (re)creates one Axis per channel (multi mode) or a single shared Axis (single mode) and plots the
# measured spectrum per channel restricted to the calibration waveform's harmonics (req. 4), as circle
# (1st file) / square (2nd file) markers; lo/hi (nothing means unbounded) restrict the plotted x-range;
# theo_a0 (nothing means skip) adds the theoretical harmonic peaks as star markers for comparison
function rebuild_axes!(plot_area, state::ViewerState, mode::Symbol, wl::Integer,
                       lo::Union{Nothing,Real}, hi::Union{Nothing,Real},
                       theo_form::Symbol, theo_f0::Real, theo_a0::Union{Nothing,Real})
    clear_axes!(state)
    isempty(state.channels) && return

    colors = Makie.wong_colors()

    # samples one channel's measured spectrum at the calibration harmonics and plots it as markers
    function plot_measured!(ax, ch, i)
        freqs, stacked = stacked_spectra(ch, wl)
        fmax = hi === nothing ? freqs[end] : hi
        hfreqs, hvals = harmonic_values(freqs, stacked, theo_form, theo_f0, fmax)
        scatter!(ax, hfreqs, hvals, color=colors[mod1(i, length(colors))],
                 marker=DATA_MARKERS[mod1(i, length(DATA_MARKERS))], markersize=14, label=channel_label(ch))
        return freqs
    end

    # overlays the theoretical harmonic peaks (as stars) on an axis, up to the axis' plotted frequencies
    function plot_theoretical!(ax, freqs)
        (theo_a0 === nothing || theo_a0 <= 0) && return
        fmax = hi === nothing ? freqs[end] : hi
        tfreqs, tamps = theoretical_spectrum(theo_form, theo_f0, theo_a0, fmax)
        scatter!(ax, tfreqs, tamps, color=:black, marker=:star5, markersize=16, label="theoretical")
    end

    if mode == :single
        ax = Axis(plot_area[1, 1], xlabel="frequency (Hz)", ylabel="amplitude density",
                  title="stacked amplitude spectra", xscale=log10, yscale=log10,
                  xtickformat=real_number_ticks, ytickformat=real_number_ticks)
        push!(state.axes, ax)
        local freqs
        for (i, ch) in enumerate(state.channels)
            freqs = plot_measured!(ax, ch, i)
        end
        plot_theoretical!(ax, freqs)
        state.legend = axislegend(ax)
        xlims!(ax, lo, hi)
    else # :multi
        for (i, ch) in enumerate(state.channels)
            ax = Axis(plot_area[i, 1], xlabel="frequency (Hz)", ylabel=channel_label(ch),
                      title=(i == 1 ? "stacked amplitude spectra" : ""), xscale=log10, yscale=log10,
                      xtickformat=real_number_ticks, ytickformat=real_number_ticks)
            push!(state.axes, ax)
            freqs = plot_measured!(ax, ch, i)
            plot_theoretical!(ax, freqs)
            xlims!(ax, lo, hi)
        end
    end
end

function build_viewer()
    state = ViewerState()
    fig = Figure(size=(1200, 800))

    controls = fig[1, 1] = GridLayout()
    Label(controls[1, 1], "mode:")
    mode_menu = Menu(controls[1, 2], options=["single", "multi"], default="single", width=120)
    Label(controls[1, 3], "window length:")
    wl_menu = Menu(controls[1, 4], options=string.(WL_OPTIONS), default="1024", width=120)
    Label(controls[1, 5], "cut lower:")
    lower_box = Textbox(controls[1, 6], placeholder="Hz", validator=Float64, width=90)
    Label(controls[1, 7], "cut upper:")
    upper_box = Textbox(controls[1, 8], placeholder="Hz", validator=Float64, width=90)
    info_label = Label(controls[1, 9], "drag & drop .json header files here to load channels",
                        tellwidth=false)

    Label(controls[2, 1], "form:")
    form_menu = Menu(controls[2, 2], options=FORM_OPTIONS, default="saw", width=120)
    Label(controls[2, 3], "base freq.:")
    freq_menu = Menu(controls[2, 4], options=string.(BASE_FREQ_OPTIONS), default=string(BASE_FREQ_OPTIONS[1]), width=120)
    Label(controls[2, 5], "base ampl.:")
    ampl_box = Textbox(controls[2, 6], placeholder="A0", validator=Float64, width=90)
    export_button = Button(controls[2, 7], label="export")

    plot_area = fig[2, 1] = GridLayout()
    rowsize!(fig.layout, 2, Relative(0.9))

    current_mode() = mode_menu.selection[] == "single" ? :single : :multi
    current_wl() = parse(Int, wl_menu.selection[])
    cut_value(tb::Textbox) = tb.stored_string[] === nothing ? nothing : parse(Float64, tb.stored_string[])
    current_form() = form_menu.selection[] == "rect" ? :rect : :saw
    current_f0() = parse(Float64, freq_menu.selection[])

    function refresh!()
        if isempty(state.channels)
            clear_axes!(state)
            return
        end
        wl = current_wl()
        n = minimum(get_samples(ch) for ch in state.channels)
        if n < wl
            clear_axes!(state)
            info_label.text[] = "not enough samples ($n) for window length $wl"
            return
        end
        rebuild_axes!(plot_area, state, current_mode(), wl, cut_value(lower_box), cut_value(upper_box),
                      current_form(), current_f0(), cut_value(ampl_box))
    end

    # (re)seeds the base amplitude from the first loaded channel's spectrum at the current f0 (req.
    # step 1); a no-op while no channels are loaded
    function seed_base_amplitude!()
        isempty(state.channels) && return
        wl = current_wl()
        get_samples(state.channels[1]) < wl && return
        freqs, stacked = stacked_spectra(state.channels[1], wl)
        set_textbox!(ampl_box, stacked[argmin(abs.(freqs .- current_f0()))])
    end

    on(_ -> refresh!(), mode_menu.selection)
    on(_ -> refresh!(), wl_menu.selection)
    on(_ -> refresh!(), lower_box.stored_string)
    on(_ -> refresh!(), upper_box.stored_string)
    on(_ -> refresh!(), form_menu.selection)
    on(freq_menu.selection) do _
        seed_base_amplitude!()
        refresh!()
    end
    on(_ -> refresh!(), ampl_box.stored_string)

    # exports each loaded channel's harmonic values as <basename>_cal.csv, plus the theoretical
    # spectrum (derived from the first file's tags, with system replaced by "theo") if a base
    # amplitude is set
    on(export_button.clicks) do _
        if isempty(state.channels)
            info_label.text[] = "no channels loaded, nothing to export"
            return
        end
        wl = current_wl()
        form = current_form()
        f0 = current_f0()
        hi = cut_value(upper_box)
        a0 = cut_value(ampl_box)

        paths = String[]
        local freqs
        for ch in state.channels
            freqs, stacked = stacked_spectra(ch, wl)
            fmax = hi === nothing ? freqs[end] : hi
            hfreqs, hvals = harmonic_values(freqs, stacked, form, f0, fmax)
            path = joinpath(ch.dir, channel_basename(ch) * "_cal.csv")
            write_harmonics_csv(path, hfreqs, hvals)
            push!(paths, path)
        end

        if a0 !== nothing && a0 > 0
            fmax = hi === nothing ? freqs[end] : hi
            tfreqs, tamps = theoretical_spectrum(form, f0, a0, fmax)
            theo_ch = deepcopy(state.channels[1])
            theo_ch.system = "theo"
            path = joinpath(theo_ch.dir, channel_basename(theo_ch) * "_cal.csv")
            write_harmonics_csv(path, tfreqs, tamps)
            push!(paths, path)
        end

        info_label.text[] = "exported: " * join(basename.(paths), ", ")
    end

    on(events(fig.scene).dropped_files) do paths
        chans = load_channels(collect(paths))
        if isempty(chans)
            info_label.text[] = "no valid .json header files dropped"
            return
        end
        state.channels = chans
        info_label.text[] = "loaded $(length(chans)) channel(s): " * join(channel_label.(chans), ", ")

        wl = default_wl(chans[1].sampling_rate)
        idx = findfirst(==(wl), WL_OPTIONS)
        idx === nothing || (wl_menu.i_selected[] = idx)

        seed_base_amplitude!()
        refresh!()
    end

    return fig
end

fig = build_viewer()
screen = display(fig)
wait(screen)