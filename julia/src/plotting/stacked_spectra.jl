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

# hence that this is quite familiar with atsplotter which runs fine
using GLMakie
using FFTW
using Statistics: mean

include(joinpath(@__DIR__, "..", "timeseries", "atss.jl"))

const WL_OPTIONS = [64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072]

detrend(x::AbstractVector{<:Real}) = x .- mean(x)

channel_label(ch::Channel) = "$(ch.channel_type)($(lpad(ch.serial, 3, '0')))"

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
end

ViewerState() = ViewerState(Channel[], Axis[])

# reads dropped .json headers, keeping only the channels that share the first one's sampling rate (req. c)
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
    return same_rate
end

function clear_axes!(state::ViewerState)
    foreach(delete!, state.axes)
    empty!(state.axes)
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
# stacked spectrum per channel (req. 4); lo/hi (nothing means unbounded) restrict the plotted x-range
function rebuild_axes!(plot_area, state::ViewerState, mode::Symbol, wl::Integer,
                       lo::Union{Nothing,Real}, hi::Union{Nothing,Real})
    clear_axes!(state)
    isempty(state.channels) && return

    colors = Makie.wong_colors()

    if mode == :single
        ax = Axis(plot_area[1, 1], xlabel="frequency (Hz)", ylabel="amplitude density",
                  title="stacked amplitude spectra", xscale=log10, yscale=log10,
                  xtickformat=real_number_ticks, ytickformat=real_number_ticks)
        push!(state.axes, ax)
        for (i, ch) in enumerate(state.channels)
            freqs, stacked = stacked_spectra(ch, wl)
            lines!(ax, freqs[2:end], stacked[2:end], color=colors[mod1(i, length(colors))], label=channel_label(ch))
        end
        axislegend(ax)
        xlims!(ax, lo, hi)
    else # :multi
        for (i, ch) in enumerate(state.channels)
            ax = Axis(plot_area[i, 1], xlabel="frequency (Hz)", ylabel=channel_label(ch),
                      title=(i == 1 ? "stacked amplitude spectra" : ""), xscale=log10, yscale=log10,
                      xtickformat=real_number_ticks, ytickformat=real_number_ticks)
            push!(state.axes, ax)
            freqs, stacked = stacked_spectra(ch, wl)
            lines!(ax, freqs[2:end], stacked[2:end], color=colors[mod1(i, length(colors))])
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

    plot_area = fig[2, 1] = GridLayout()
    rowsize!(fig.layout, 2, Relative(0.9))

    current_mode() = mode_menu.selection[] == "single" ? :single : :multi
    current_wl() = parse(Int, wl_menu.selection[])
    cut_value(tb::Textbox) = tb.stored_string[] === nothing ? nothing : parse(Float64, tb.stored_string[])

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
        rebuild_axes!(plot_area, state, current_mode(), wl, cut_value(lower_box), cut_value(upper_box))
    end

    on(_ -> refresh!(), mode_menu.selection)
    on(_ -> refresh!(), wl_menu.selection)
    on(_ -> refresh!(), lower_box.stored_string)
    on(_ -> refresh!(), upper_box.stored_string)

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
        refresh!()
    end

    return fig
end

fig = build_viewer()
screen = display(fig)
wait(screen)