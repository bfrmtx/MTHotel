# Example add-on script: pulls in the reusable helpers and calls them.
include(joinpath(@__DIR__, "..", "src", "includes.jl"))

data_dir = joinpath(@__DIR__, "..", "data", "run_004")
channels = [
    read_header(joinpath(data_dir, "084_ADU-07e_C000_TEx_2Hz")),
    read_header(joinpath(data_dir, "084_ADU-07e_C002_THx_2Hz")),
]

plt = plot_ts_html(channels; start=0, wl=1024)
out_file = joinpath(@__DIR__, "ts_plot.html")
PlotlyJS.savefig(plt, out_file)
println("wrote: ", out_file)
