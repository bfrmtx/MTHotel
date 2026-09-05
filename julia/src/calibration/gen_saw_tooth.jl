# generate a artificial saw tooth Channel
# a) load a Channel as template
# b) amount of samples to generate is same as the template Channel (binary atss file / 8 == amount of doubles)
# c) ADU serial number is 999 <serial>_ for the generated saw tooth Channel
# d) the generated saw tooth Channel should have the same sampling rate as the template Channel
# e) parameter controlling the frequency of the saw tooth: 400 (Hz), adjustable by the user, amplitude +/- 800 (mV == natural unit == double number)
# so the trick is to generate a saw tooth waveform with the specified frequency and amplitude, matching the number of samples and sampling rate of the template Channel. As higher the sampling rate appears, looking at 1024 sample window, as less samples are seen within that window.

using Waveforms

include(joinpath(@__DIR__, "..", "timeseries", "atss.jl"))

# builds n_samples of a +/-amplitude saw tooth at the given frequency (Hz), sampled at fs (Hz);
# time_offset (seconds) shifts the time origin only, freq/period are untouched
function saw_tooth_data(n_samples::Integer, fs::Real; freq::Real=400.0, amplitude::Real=800.0, time_offset::Real=0.0)
    t = (0:n_samples-1) ./ fs .+ time_offset
    return amplitude .* sawtoothwave.(2pi .* freq .* t)
end

# time offset (s) at which amplitude*sawtoothwave(2*pi*freq*offset) == value; used to line up the
# synthetic waveform's first sample with the real data's first sample via a time shift (not a DC value
# offset, which would break the +/-amplitude bound)
function time_offset_for_value(value::Real, amplitude::Real, freq::Real)
    v = clamp(value / amplitude, -1.0, 1.0)
    phase = v >= 0 ? v * pi : (v + 2) * pi   # phase in [0, 2*pi) with sawtoothwave(phase) == v
    return phase / (2pi * freq)
end

# writes a synthetic saw tooth Channel (same length/sampling rate as `template`) next to the template files;
# serial is "999" prepended to the template's serial, e.g. template serial 84 -> 999084
# time_offset=nothing (default) auto-aligns the first generated sample with the template's real first sample
function generate_saw_tooth(template::Channel; freq::Real=400.0, amplitude::Real=800.0,
                             time_offset::Union{Nothing,Real}=nothing, dir::AbstractString=template.dir)
    n_samples = get_samples(template)

    ch = deepcopy(template)
    ch.serial = 999
    ch.dir = dir

    t0 = time_offset === nothing ? time_offset_for_value(read_data(template; start=0, wl=1)[1], amplitude, freq) : time_offset
    data = saw_tooth_data(n_samples, ch.sampling_rate; freq=freq, amplitude=amplitude, time_offset=t0)

    write_header(ch; dir=dir)
    atss_path = joinpath(dir, channel_basename(ch) * ".atss")
    open(atss_path, "w") do io
        write(io, data)
    end
    return ch
end

generate_saw_tooth(template_path::AbstractString; kwargs...) = generate_saw_tooth(read_header(template_path); kwargs...)

  # example: generate a saw tooth matching one of the test channels
#  template_path = joinpath(@__DIR__, "..", "..", "data", "cal", "08e_vs_11e", "131072Hz", "003_ADU-11e_C000_TEx_131072Hz")




# read from the command line
template_path = ARGS[1]
freq = parse(Float64, ARGS[2])  # read frequency from the command line
amplitude = parse(Float64, ARGS[3])  # read amplitude from the command line

# print command line usage: julia gen_saw_tooth.jl <template_path> <freq> <amplitude>
if length(ARGS) < 3
    println("Usage: julia gen_saw_tooth.jl <template_path> <freq> <amplitude>")
    println("example  003_ADU-11e_C000_TEx_8192Hz 200 800")
    println("example  003_ADU-11e_C000_TEx_32768Hz 400 800")
    exit(1)
end

ch = generate_saw_tooth(template_path; freq=freq, amplitude=amplitude)
println("wrote: ", joinpath(ch.dir, channel_basename(ch) * ".atss"))