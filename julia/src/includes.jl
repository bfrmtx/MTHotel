# Plain include list (no package/module) - pulls all reusable helpers into scope.
# Use from a script with: include(joinpath(@__DIR__, "..", "src", "includes.jl"))

include(joinpath(@__DIR__, "timeseries", "atss.jl"))
include(joinpath(@__DIR__, "plotting", "plot_ts.jl"))
