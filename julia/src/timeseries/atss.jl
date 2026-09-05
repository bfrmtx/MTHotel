# ATSS channel object. Format: https://mth5.geo-metronix.de/atss/atss.html
# Metadata is split between the filename tags and the JSON header; both end
# up as fields on the same Channel object.
using JSON3
using Printf

# filename: <serial>_<system>_C<channel_no>_T<channel_type>_<sampling_rate>(Hz|s)
mutable struct Channel
    # --- tags, come from the filename, not the JSON ---
    serial::Int
    system::String
    channel_no::Int
    channel_type::String
    sampling_rate::Float64          # Hz

    # --- JSON header ---
    datetime::String                # ISO 8601 UTC
    latitude::Float64
    longitude::Float64
    elevation::Float64
    azimuth::Float64
    tilt::Float64
    resistance::Float64
    units::String
    filter::String
    id::String
    source::String
    sensor_calibration::Dict{String,Any}

    dir::String                     # directory the .json/.atss files live in
end

Channel() = Channel(0, "", 0, "", 0.0,
                     "1970-01-01T00:00:00.0", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                     "mV", "", "", "", Dict{String,Any}(),
                     "")

get_latitude(ch::Channel) = ch.latitude
set_latitude!(ch::Channel, l::Real) = (ch.latitude = Float64(l); ch)

get_sampling_rate(ch::Channel) = ch.sampling_rate
function set_sampling_rate!(ch::Channel, rate::Real)
    rate <= 0 && error("sampling_rate must be positive")
    ch.sampling_rate = Float64(rate)
    return ch
end

# sample rate <-> filename tag, e.g. 128.0 -> "128Hz", 0.25 -> "4s"
function sample_rate_to_tag(sampling_rate::Real)
    sampling_rate <= 0 && error("sampling_rate must be positive")
    if sampling_rate >= 1.0
        return string(round(Int, sampling_rate)) * "Hz"
    else
        return string(round(Int, 1.0 / sampling_rate)) * "s"
    end
end

function tag_to_sample_rate(tag::AbstractString)
    if endswith(tag, "Hz")
        return parse(Float64, tag[1:end-2])
    elseif endswith(tag, "s")
        return 1.0 / parse(Float64, tag[1:end-1])
    else
        error("invalid sample rate tag in filename: $tag")
    end
end

# filename (without extension and without directory), computed from the tags
function channel_basename(ch::Channel)
    return @sprintf("%03d_%s_C%03d_T%s_%s", ch.serial, ch.system, ch.channel_no,
                     ch.channel_type, sample_rate_to_tag(ch.sampling_rate))
end

function strip_atss_ext(path::AbstractString)
    endswith(path, ".atss") && return path[1:end-5]
    endswith(path, ".json") && return path[1:end-5]
    endswith(path, ".ATSS") && return path[1:end-5]
    endswith(path, ".JSON") && return path[1:end-5]
    # just a trailing "dot" as a command may leave
    endswith(path, ".") && return path[1:end-1]
    return path
end

# reads <path>.json and maps both the filename tags and the JSON header into a Channel
function read_header(path::AbstractString)
    base = strip_atss_ext(path)
    dir, base_name = dirname(base), Base.basename(base)

    parts = split(base_name, '_')
    length(parts) < 5 && error("invalid ATSS basename: $base_name")

    ch = Channel()
    ch.dir = dir
    ch.serial = parse(Int, parts[1])
    ch.system = parts[2]

    chan_no_tag = parts[3]
    startswith(chan_no_tag, "C") || error("expected channel number tag starting with 'C': $chan_no_tag")
    ch.channel_no = parse(Int, chan_no_tag[2:end])

    chan_type_tag = parts[4]
    startswith(chan_type_tag, "T") || error("expected channel type tag starting with 'T': $chan_type_tag")
    ch.channel_type = chan_type_tag[2:end]

    ch.sampling_rate = tag_to_sample_rate(parts[5])

    json_path = base * ".json"
    isfile(json_path) || error("JSON header not found: $json_path")
    jdata = JSON3.read(read(json_path, String), Dict{String,Any})

    ch.datetime = get(jdata, "datetime", ch.datetime)
    ch.latitude = get(jdata, "latitude", ch.latitude)
    ch.longitude = get(jdata, "longitude", ch.longitude)
    ch.elevation = get(jdata, "elevation", ch.elevation)
    ch.azimuth = get(jdata, "azimuth", ch.azimuth)
    ch.tilt = get(jdata, "tilt", ch.tilt)
    ch.resistance = get(jdata, "resistance", ch.resistance)
    ch.units = get(jdata, "units", ch.units)
    ch.filter = get(jdata, "filter", ch.filter)
    ch.id = get(jdata, "id", ch.id)
    ch.source = get(jdata, "source", ch.source)
    ch.sensor_calibration = get(jdata, "sensor_calibration", ch.sensor_calibration)

    return ch
end

# writes <dir>/<basename-from-current-tags>.json; basename changes if tags (e.g. sampling_rate) changed since reading
function write_header(ch::Channel; dir::AbstractString=ch.dir)
    isdir(dir) || mkpath(dir)
    base = joinpath(dir, channel_basename(ch))

    jdata = Dict{String,Any}(
        "datetime" => ch.datetime,
        "latitude" => ch.latitude,
        "longitude" => ch.longitude,
        "elevation" => ch.elevation,
        "azimuth" => ch.azimuth,
        "tilt" => ch.tilt,
        "resistance" => ch.resistance,
        "units" => ch.units,
        "filter" => ch.filter,
        "id" => ch.id,
        "source" => ch.source,
        "sensor_calibration" => ch.sensor_calibration,
    )

    json_path = base * ".json"
    open(json_path, "w") do io
        JSON3.write(io, jdata)
    end
    return json_path
end

# number of samples in the .atss file, derived from its size (8 bytes per double)
function get_samples(ch::Channel)
    path = joinpath(ch.dir, channel_basename(ch) * ".atss")
    isfile(path) || error("ATSS data file not found: $path")
    return filesize(path) ÷ 8
end

# reads wl samples starting at sample index `start` (0-based); wl=0 means "until end of file"
function read_data(ch::Channel; start::Integer=0, wl::Integer=0)
    start < 0 && error("start must be non-negative")
    path = joinpath(ch.dir, channel_basename(ch) * ".atss")
    isfile(path) || error("ATSS data file not found: $path")

    total_samples = filesize(path) ÷ 8
    wl = wl == 0 ? total_samples - start : wl
    (start + wl) > total_samples && error("start + wl ($(start + wl)) exceeds available samples ($total_samples)")

    data = Vector{Float64}(undef, wl)
    open(path, "r") do io
        seek(io, start * 8)
        read!(io, data)
    end
    return data
end
