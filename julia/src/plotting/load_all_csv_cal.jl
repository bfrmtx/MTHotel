# get many .csv files by drag & drop
# filename: <serial>_<system>_C<channel_no>_T<channel_type>_<sampling_rate>(Hz|s)
# pattern: 
# 003_ADU-11e_C000_TEx_131072Hz_cal.csv ... 003_ADU-11e_C000_TEx_8192Hz_cal.csv
# 004_ADU-08e_C000_TEx_131072Hz_cal.csv ... 004_ADU-08e_C000_TEx_8192Hz_cal.csv
# group by 003_ADU-11e_C000_TEx_ and ... 003_ADU-11e_C000_TEx_  so same system, but different "_<sampling_rate>"
# 
# a) load all CSV files into DataFrames
# b) group DataFrames by common prefix (same system, different sampling rate) above we have two groups
# c) plot the grouped DataFrames using GLMakie (log, log)
# e) example data is below data/cal
using CSV
using DataFrames
using GLMakie

struct CalFile
    prefix::String   # e.g. "003_ADU-11e_C000_TEx" (system/channel, sampling rate stripped)
    rate::String     # e.g. "131072Hz" or "2s"
    df::DataFrame
end

# splits "<prefix>_<rate>_cal.csv" into (prefix, rate); returns nothing if the filename doesn't match
function parse_cal_filename(path::AbstractString)
    m = match(r"^(.+)_(\d+(?:Hz|s))_cal\.csv$", basename(path))
    return m === nothing ? nothing : (prefix=String(m.captures[1]), rate=String(m.captures[2]))
end

# a) loads all dropped *_cal.csv files into DataFrames, skipping anything that doesn't match the pattern
function load_cal_files(paths::Vector{String})
    files = CalFile[]
    for p in paths
        endswith(p, ".csv") || continue
        parsed = parse_cal_filename(p)
        if parsed === nothing
            @warn "filename does not match calibration csv pattern, skipping" path = p
            continue
        end
        try
            push!(files, CalFile(parsed.prefix, parsed.rate, CSV.read(p, DataFrame)))
        catch e
            @warn "failed to read csv" path = p exception = e
        end
    end
    return files
end

# b) groups loaded files by common prefix (same system/channel, different sampling rate)
function group_by_prefix(files::Vector{CalFile})
    groups = Dict{String,Vector{CalFile}}()
    for f in files
        push!(get!(groups, f.prefix, CalFile[]), f)
    end
    return groups
end

# rate string like "131072Hz" or "2s" -> numeric value, so groups plot with increasing sampling rate
function rate_sort_key(rate::AbstractString)
    n = parse(Float64, match(r"^(\d+)", rate).captures[1])
    return endswith(rate, "s") ? 1 / n : n
end

const LINESTYLES = [:solid, :dash, :dot, :dashdot, :dashdotdot]

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

mutable struct ViewerState
    files::Vector{CalFile}
    axes::Vector{Axis}
    legend::Union{Nothing,Legend}
end

ViewerState() = ViewerState(CalFile[], Axis[], nothing)

function clear_axes!(state::ViewerState)
    foreach(delete!, state.axes)
    empty!(state.axes)
    # axislegend()/Legend() creates a separate Block, not a child of the Axis, so it survives delete!(ax)
    state.legend === nothing || delete!(state.legend)
    state.legend = nothing
end

# c) a single shared Axis: one color per group (system), one linestyle per sampling rate within that
# group, so all measurements for all systems are visible together on the same log-log plot
function rebuild_axes!(plot_area, legend_area, state::ViewerState)
    clear_axes!(state)
    isempty(state.files) && return

    groups = group_by_prefix(state.files)
    colors = Makie.wong_colors()

    ax = Axis(plot_area[1, 1], xlabel="frequency (Hz)", ylabel="amplitude (normalized)",
              title="calibration spectra (grouped by system)", xscale=log10, yscale=log10,
              xtickformat=real_number_ticks, ytickformat=real_number_ticks)
    push!(state.axes, ax)

    for (i, prefix) in enumerate(sort(collect(keys(groups))))
        group = sort(groups[prefix], by=f -> rate_sort_key(f.rate))
        color = colors[mod1(i, length(colors))]
        for (j, f) in enumerate(group)
            order = sortperm(f.df.frequency_hz)
            freqs = f.df.frequency_hz[order]
            amps = f.df.amplitude[order]
            lines!(ax, freqs, amps ./ amps[1], color=color,
                   linestyle=LINESTYLES[mod1(j, length(LINESTYLES))], label="$(prefix) @$(f.rate)")
        end
    end
    state.legend = Legend(legend_area, ax, framevisible=false)
end

function build_viewer()
    state = ViewerState()
    fig = Figure(size=(1200, 800))

    info_label = Label(fig[1, 1:2], "drag & drop *_cal.csv files here to load", tellwidth=false)
    plot_area = fig[2, 1] = GridLayout()
    legend_area = fig[2, 2]
    rowsize!(fig.layout, 2, Relative(0.95))
    colsize!(fig.layout, 1, Relative(0.8))

    on(events(fig.scene).dropped_files) do paths
        files = load_cal_files(collect(paths))
        if isempty(files)
            info_label.text[] = "no valid *_cal.csv files dropped"
            return
        end
        state.files = files
        n_groups = length(group_by_prefix(files))
        info_label.text[] = "loaded $(length(files)) file(s) in $n_groups group(s)"
        rebuild_axes!(plot_area, legend_area, state)
    end

    return fig
end

fig = build_viewer()
screen = display(fig)
wait(screen)
