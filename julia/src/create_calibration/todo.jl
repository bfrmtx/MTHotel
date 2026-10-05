# needs atss.jl functions
include(joinpath(@__DIR__, "..", "timeseries", "atss.jl"))

native_rates = get_native_sampling_rates()

println("use 500 mV peak to peak: ")
println("use 1000m dipole length")

for rate in native_rates
    println("sampling rate: $rate")
    println("frequency generator f for rate $rate:")
    println(Int.(frequency_fracs(rate)))
    println()
end