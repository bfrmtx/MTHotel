# load one or more Channel data for plotting
# a) single mode: all Channels in one plot
# b) multi mode: each Channel in a separate plot row
# c) files MUST have same sampling rate like *_2Hz.atss or *_4s.atss
# d) drop down need for changing window length from 64, 128, ... 131072
# e) on the bottom a slider FOR ALL, moving the visible window across the data
# f) a file dialog (or better drag and drop!!) for selecting the data files, e.g. ending with .json
# test data you find in data/sac/run_004 for example
using GLMakie
using Statistics: mean

include(joinpath(@__DIR__, "..", "timeseries", "atss.jl"))

const WL_OPTIONS = [64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072]

detrend(x::AbstractVector{<:Real}) = x .- mean(x)

channel_label(ch::Channel) = "$(ch.channel_type)($(lpad(ch.serial, 3, '0')))"

mutable struct ViewerState
    channels::Vector{Channel}
    axes::Vector{Axis}
    lines_x::Vector{Observable}
    lines_y::Vector{Observable}
end

ViewerState() = ViewerState(Channel[], Axis[], Observable[], Observable[])

function total_samples(state::ViewerState)
    isempty(state.channels) && return 0
    return minimum(get_samples(ch) for ch in state.channels)
end

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
    empty!(state.lines_x)
    empty!(state.lines_y)
end

# (re)creates one Axis per channel (multi mode) or a single shared Axis (single mode)
function rebuild_axes!(plot_area, state::ViewerState, mode::Symbol, start::Integer, wl::Integer)
    clear_axes!(state)
    isempty(state.channels) && return

    xs = collect(start:(start + wl - 1))
    colors = Makie.wong_colors()

    if mode == :single
        ax = Axis(plot_area[1, 1], xlabel="sample", ylabel="amplitude (detrended)",
                  title="ATSS time series")
        push!(state.axes, ax)
        for (i, ch) in enumerate(state.channels)
            ys = detrend(read_data(ch; start=start, wl=wl))
            x_obs = Observable(xs)
            y_obs = Observable(ys)
            lines!(ax, x_obs, y_obs, color=colors[mod1(i, length(colors))], label=channel_label(ch))
            push!(state.lines_x, x_obs)
            push!(state.lines_y, y_obs)
        end
        axislegend(ax)
    else # :multi
        for (i, ch) in enumerate(state.channels)
            ax = Axis(plot_area[i, 1], xlabel="sample", ylabel=channel_label(ch),
                      title=i == 1 ? "ATSS time series" : "")
            push!(state.axes, ax)
            ys = detrend(read_data(ch; start=start, wl=wl))
            x_obs = Observable(xs)
            y_obs = Observable(ys)
            lines!(ax, x_obs, y_obs, color=colors[mod1(i, length(colors))])
            push!(state.lines_x, x_obs)
            push!(state.lines_y, y_obs)
        end
    end
end

# moves the already-built axes/lines to a new window without recreating them (cheap slider updates)
function update_slice!(state::ViewerState, start::Integer, wl::Integer)
    isempty(state.channels) && return
    xs = collect(start:(start + wl - 1))
    for (i, ch) in enumerate(state.channels)
        state.lines_x[i][] = xs
        state.lines_y[i][] = detrend(read_data(ch; start=start, wl=wl))
    end
    foreach(autolimits!, state.axes)
end

function build_viewer()
    state = ViewerState()
    fig = Figure(size=(1200, 800))

    controls = fig[1, 1] = GridLayout()
    Label(controls[1, 1], "mode:")
    mode_menu = Menu(controls[1, 2], options=["single", "multi"], default="single", width=120)
    Label(controls[1, 3], "window length:")
    wl_menu = Menu(controls[1, 4], options=string.(WL_OPTIONS), default="1024", width=120)
    info_label = Label(controls[1, 5], "drag & drop .json header files here to load channels",
                        tellwidth=false)

    plot_area = fig[2, 1] = GridLayout()

    slider_row = fig[3, 1] = GridLayout()
    pos_slider = Slider(slider_row[1, 1], range=0:1:0, startvalue=0)
    range_label = Label(slider_row[1, 2], "no data loaded", tellwidth=false, width=220)

    rowsize!(fig.layout, 2, Relative(0.8))

    current_mode() = mode_menu.selection[] == "single" ? :single : :multi
    current_wl() = parse(Int, wl_menu.selection[])

    # re-derives the slider range from the loaded data and rebuilds the axes for the current mode/wl
    function refresh!(; reset_slider::Bool)
        n = total_samples(state)
        wl = current_wl()
        if n < wl
            clear_axes!(state)
            pos_slider.range[] = 0:1:0
            range_label.text[] = n == 0 ? "no data loaded" : "not enough samples for window length $wl"
            return
        end
        max_start = n - wl
        pos_slider.range[] = 0:wl:max_start
        start = reset_slider ? 0 : pos_slider.value[]
        rebuild_axes!(plot_area, state, current_mode(), start, wl)
        range_label.text[] = "samples $start - $(start + wl - 1) of $n"
    end

    on(_ -> refresh!(reset_slider=false), mode_menu.selection)
    on(_ -> refresh!(reset_slider=true), wl_menu.selection)

    on(pos_slider.value) do start
        isempty(state.channels) && return
        wl = current_wl()
        update_slice!(state, start, wl)
        range_label.text[] = "samples $start - $(start + wl - 1) of $(total_samples(state))"
    end

    on(events(fig.scene).dropped_files) do paths
        chans = load_channels(collect(paths))
        if isempty(chans)
            info_label.text[] = "no valid .json header files dropped"
            return
        end
        state.channels = chans
        info_label.text[] = "loaded $(length(chans)) channel(s): " * join(channel_label.(chans), ", ")
        refresh!(reset_slider=true)
    end

    return fig
end

fig = build_viewer()
screen = display(fig)
wait(screen)
