# read old ats file from command line argument and print its header
include(joinpath(@__DIR__, "ats.jl"))

if length(ARGS) < 1
    println("Usage: julia ats_test.jl <ats_file>")
    exit(1)
end

filename = ARGS[1]
ch = read_header(filename)
JSON3.pretty(ch.header)
data = read_data(ch, 0, 16)
# print the first 16 samples of the data
println(data)
println("new atss values for ", ch.header["filename"])
println(JSON3.pretty(atss_header(ch)))