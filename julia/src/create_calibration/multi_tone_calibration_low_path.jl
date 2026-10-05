
#=
1. we loop over a tree, here 132k, which indicates the sampling rate container.
 command line: data/cal_sine/132k

├── 132k
│   ├── 16384
│   │   ├── 003_ADU-11e_C000_TEx_131072Hz.atss
│   │   └── 003_ADU-11e_C000_TEx_131072Hz.json
│   ├── 256
│   │   ├── 003_ADU-11e_C000_TEx_131072Hz.atss
│   │   └── 003_ADU-11e_C000_TEx_131072Hz.json
│   ├── 32786
│   │   ├── 003_ADU-11e_C000_TEx_131072Hz.atss
│   │   └── 003_ADU-11e_C000_TEx_131072Hz.json
│   ├── 512
│   │   ├── 003_ADU-11e_C000_TEx_131072Hz.atss
│   │   └── 003_ADU-11e_C000_TEx_131072Hz.json
│   └── 8192
│       ├── 003_ADU-11e_C000_TEx_131072Hz.atss
│       └── 003_ADU-11e_C000_TEx_131072Hz.json

2. we make a ASD (Amplitude Spectral Density) FFTW calculation with window length == sampling rate (here 131072, which is in the file names), should be  mV/sqrt(Hz) == $$\text{mV}/\sqrt{\text{Hz}}$$

3. from each ASD calculation, we extract the the calibration sine frequency, which is the directory on top of the data files (e.g., 16384, 256, 32786, 512, 8192).

4. we take the pairs, (calibration sine frequency, ASD value). We normalize the ASD to one for the LOWEST frequency (here 256).

4. we plot the extracted calibration sine frequencies in a log-log plot.So here would have plot starting from the lowest frequency (here 256) up to the highest frequency (here 16384), and y-axis representing the normalized ASD values, starting with 1, expected to get smaller values for higher frequencies.
=#

using GLMakie
using CairoMakie
using FFTW
using Statistics: mean
using Printf
using JSON3

include(joinpath(@__DIR__, "..", "timeseries", "atss.jl"))

# CairoMakie is only loaded to save vector/raster files below; make sure interactive display below
# uses GLMakie regardless of which backend a prior `using` last auto-activated
GLMakie.activate!()

detrend(x::AbstractVector{<:Real}) = x .- mean(x)

# formats a single log-axis tick as a plain (non-exponent) number, abbreviating values >= 1000 with a
# "k" suffix (e.g. 20000 -> "20k") instead of Makie's default "10^n" style (same convention as the
# other plotting scripts in this repo)
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

# one calibration-frequency subdirectory -> the sine frequency (Hz) detected in its ASD and the ASD
# peak amplitude there
struct CalPoint
    freq::Float64
    asd::Float64
end

# there is exactly one channel per calibration-frequency subdirectory; finds and reads its .json header
function find_channel_header(dir::AbstractString)
    jsons = filter(f -> endswith(f, ".json"), readdir(dir))
    isempty(jsons) && error("no .json header found in $dir")
    length(jsons) > 1 && @warn "multiple .json headers found, using the first" dir = dir files = jsons
    return read_header(joinpath(dir, jsons[1]))
end

# the single sampling rate shared by every calibration-frequency subdirectory in this container (e.g.
# 131072 Hz for the "132k" tree)
function container_sampling_rate(root::AbstractString)
    first_subdir = first(filter(e -> isdir(joinpath(root, e)), sort(readdir(root))))
    return find_channel_header(joinpath(root, first_subdir)).sampling_rate
end

# windowed ASD (amplitude spectral density, mV/sqrt(Hz)) with window length == sampling rate (req. 2),
# stacked (averaged) over all non-overlapping windows in the file
function asd_spectrum(ch::Channel)
    wl = round(Int, ch.sampling_rate)
    n_windows = get_samples(ch) ÷ wl
    n_windows < 1 && error("file too short for one window of length $wl: $(channel_basename(ch))")
    freqs = rfftfreq(wl, ch.sampling_rate)
    stacked = sum(abs.(rfft(detrend(read_data(ch; start=(k - 1) * wl, wl=wl)))) for k in 1:n_windows)
    stacked ./= wl * n_windows
    return freqs, stacked
end

# peak ASD value (and its frequency), i.e. the calibration sine tone (req. 3); the DC bin is excluded
# since it carries no tone information and would otherwise dominate after detrending imperfectly
function spectrum_peak(freqs::AbstractVector, asd::AbstractVector)
    mx, mi = findmax(@view asd[2:end])
    return freqs[mi+1], mx
end

# walks the sampling-rate container directory (e.g. data/cal_sine/132k), one calibration-frequency
# subdirectory per sine test (req. 1), detects the sine tone frequency directly from each ASD's peak
# (req. 3 -- the directory name is only an approximate label, e.g. "32786" for an actual 32768 Hz tone,
# so it is not used for the frequency value itself) and returns the (frequency, ASD) pairs sorted by
# frequency, normalized so the lowest calibration frequency has ASD == 1 (req. 4)
function collect_cal_points(root::AbstractString)
    points = CalPoint[]
    for entry in sort(readdir(root))
        subdir = joinpath(root, entry)
        isdir(subdir) || continue

        ch = find_channel_header(subdir)
        freqs, asd = asd_spectrum(ch)
        freq, peak = spectrum_peak(freqs, asd)
        push!(points, CalPoint(freq, peak))
    end

    isempty(points) && error("no calibration points found under $root")
    sort!(points, by=p -> p.freq)
    ref = points[1].asd
    return [(p.freq, p.asd / ref) for p in points]
end

# ---- low-pass filter models fit to the (frequency, normalized ASD) calibration points ----
# each is linearized (log/lin-log/log-log) and solved by ordinary least squares, same approach as
# plotting/calc_calibration_lowpass.jl

# Butterworth: ratio(f) = 1/sqrt(1+(f/fc)^(2n)); linearized via
# log(1/ratio^2 - 1) = 2n*log(f) - 2n*log(fc), a straight line in a log(f)-vs-log(...) plot
function fit_butterworth(freqs::AbstractVector, ratios::AbstractVector)
    mask = (ratios .> 0) .& (ratios .< 1)
    count(mask) < 2 && return (fc=NaN, n=NaN)
    x = log.(freqs[mask])
    y = log.(1 ./ ratios[mask] .^ 2 .- 1)
    slope, intercept = [x ones(length(x))] \ y
    n = slope / 2
    return (fc=exp(-intercept / slope), n=n)
end
butterworth_response(f::Real, fc::Real, n::Real) = 1 / sqrt(1 + (f / fc)^(2n))

# Gaussian: ratio(f) = exp(-(f/fc)^2); linearized via ln(ratio) = -f^2/fc^2, i.e. a straight line
# in a ln(ratio)-vs-f^2 plot (slope = -1/fc^2)
function fit_gaussian(freqs::AbstractVector, ratios::AbstractVector)
    mask = ratios .> 0
    count(mask) < 2 && return (fc=NaN, slope=NaN, intercept=NaN)
    x = freqs[mask] .^ 2
    y = log.(ratios[mask])
    slope, intercept = [x ones(length(x))] \ y
    return (fc=sqrt(-1 / slope), slope=slope, intercept=intercept)
end
gaussian_response(f::Real, fc::Real) = exp(-(f / fc)^2)

# super-Gaussian: ratio(f) = exp(-(f/fc)^(2n)); linearized via log(-log(ratio)) = 2n*log(f) -
# 2n*log(fc); reduces to the Gaussian above when n == 1
function fit_supergaussian(freqs::AbstractVector, ratios::AbstractVector)
    mask = (ratios .> 0) .& (ratios .< 1)
    count(mask) < 2 && return (fc=NaN, n=NaN)
    x = log.(freqs[mask])
    y = log.(.-log.(ratios[mask]))
    slope, intercept = [x ones(length(x))] \ y
    n = slope / 2
    return (fc=exp(-intercept / slope), n=n)
end
supergaussian_response(f::Real, fc::Real, n::Real) = exp(-(f / fc)^(2n))

# root-mean-square error between measured and model-predicted normalized ASD values, i.e. the fit
# quality of a given model
rmse(measured::AbstractVector, predicted::AbstractVector) = sqrt(mean((measured .- predicted) .^ 2))

# fits all three candidate low-pass models once and reports each one's parameters plus its RMSE
# against the measured (freqs, values) pairs
function fit_all_models(freqs::AbstractVector, values::AbstractVector)
    bw = fit_butterworth(freqs, values)
    gs = fit_gaussian(freqs, values)
    sg = fit_supergaussian(freqs, values)
    return (
        butterworth=(fc=bw.fc, n=bw.n, rmse=rmse(values, butterworth_response.(freqs, bw.fc, bw.n))),
        gaussian=(fc=gs.fc, slope=gs.slope, intercept=gs.intercept, rmse=rmse(values, gaussian_response.(freqs, gs.fc))),
        super_gaussian=(fc=sg.fc, n=sg.n, rmse=rmse(values, supergaussian_response.(freqs, sg.fc, sg.n))),
    )
end

# writes the calibration points and fit summary (fc/n/rmse per model) as JSON to <root>/fit_results.json,
# i.e. directly below the sampling-rate container directory (req.: table "below the top directory");
# also reports the Gaussian fit rounded to the container-scaled precision (see fc_round_precision),
# so the JSON records both the raw regression result and the value actually worth quoting
function write_fit_results(root::AbstractString, points, fits, container_fs::Real)
    freqs = first.(points)
    values = last.(points)

    precision = fc_round_precision(container_fs)
    fc_rounded = round_down(fits.gaussian.fc, precision)

    out = Dict(
        "points" => [Dict("freq_hz" => f, "normalized_asd" => v) for (f, v) in points],
        "fits" => Dict(
            "butterworth" => Dict("fc_hz" => fits.butterworth.fc, "n" => fits.butterworth.n, "rmse" => fits.butterworth.rmse),
            "gaussian" => Dict("fc_hz" => fits.gaussian.fc, "rmse" => fits.gaussian.rmse),
            "gaussian_rounded" => Dict("fc_hz" => fc_rounded, "precision_hz" => precision,
                                        "rmse" => rmse(values, gaussian_response.(freqs, fc_rounded))),
            "super_gaussian" => Dict("fc_hz" => fits.super_gaussian.fc, "n" => fits.super_gaussian.n, "rmse" => fits.super_gaussian.rmse),
        ),
    )
    path = joinpath(root, "fit_results.json")
    open(path, "w") do io
        JSON3.pretty(io, out)
    end
    return path
end

# log-log plot of normalized ASD vs. calibration sine frequency (req. 4/5), overlaid with the fitted
# Butterworth/Gaussian/super-Gaussian low-pass models
function plot_cal_points(points, fits)
    fig = Figure(size=(900, 600))
    ax = Axis(fig[1, 1], xlabel="calibration frequency (Hz)", ylabel="normalized ASD",
              title="multi-tone low-pass calibration", xscale=log10, yscale=log10,
              xtickformat=real_number_ticks, ytickformat=real_number_ticks)

    freqs = first.(points)
    values = last.(points)
    scatter!(ax, freqs, values, color=:dodgerblue, markersize=12, marker=:circle, label="measured")

    fplot = exp.(range(log(freqs[1]), log(freqs[end]), length=200))

    bw = fits.butterworth
    if !isnan(bw.fc)
        lines!(ax, fplot, butterworth_response.(fplot, bw.fc, bw.n), color=:orange, linestyle=:dash,
               label=@sprintf("Butterworth: fc=%.0fHz n=%.2f rmse=%.4g", bw.fc, bw.n, bw.rmse))
    end

    gs = fits.gaussian
    if !isnan(gs.fc)
        lines!(ax, fplot, gaussian_response.(fplot, gs.fc), color=:seagreen, linestyle=:dot,
               label=@sprintf("Gaussian: fc=%.0fHz rmse=%.4g", gs.fc, gs.rmse))
    end

    sg = fits.super_gaussian
    if !isnan(sg.fc)
        lines!(ax, fplot, supergaussian_response.(fplot, sg.fc, sg.n), color=:crimson, linestyle=:dashdot,
               label=@sprintf("super-Gaussian: fc=%.0fHz n=%.2f rmse=%.4g", sg.fc, sg.n, sg.rmse))
    end

    axislegend(ax, position=:lb)
    return fig
end

# x-axis ticks sit at f^2 (the plotted quantity) but are labeled with f (Hz), the physical quantity
sqrtf_ticks(vs) = tick_label.(sqrt.(vs))

# how finely fc is meaningfully reported, tied to the container's own sampling rate: at the top native
# rate (131072 Hz) round to the nearest 1000 Hz; each halving of the sampling rate shifts the rounding
# down by one decimal digit (100, 10, 1 Hz), bottoming out at 1 Hz -- the ASD's own frequency
# resolution here, since window length == sampling rate, so nothing finer is physically meaningful
function fc_round_precision(container_fs::Real)
    exponent = round(Int, log2(container_fs))
    return 10.0^clamp(exponent - 14, 0, 3)
end

round_down(fc::Real, precision::Real) = floor(fc / precision) * precision

# diagnostic plot for the standard Gaussian model only: ln(ratio) vs. f^2 is a straight line (slope
# = -1/fc^2), so this visualizes how well the linearization used by fit_gaussian actually holds; also
# overlays the same line using fc rounded down to the container-scaled precision (see fc_round_precision)
# to show that this rounding is indistinguishable from the full-precision fit
# the x label shows the physical frequency f (Hz), even though the x-axis is actually f^2 for the linearization, so 4096 Hz label is placed at 4096^2 on the axis
function plot_gaussian_linearization(freqs::AbstractVector, values::AbstractVector, gs, container_fs::Real)
    fig = Figure(size=(900, 600))
    ax = Axis(fig[1, 1], xlabel="calibration frequency, f (Hz)", ylabel="ln(normalized ASD)",
              title="Gaussian fit linearization: ln(ASD) vs. f²", xtickformat=sqrtf_ticks)

    x = freqs .^ 2
    y = log.(values)
    scatter!(ax, x, y, color=:dodgerblue, markersize=12, marker=:circle, label="measured")

    if !isnan(gs.fc)
        xline = range(0, maximum(x), length=200)
        lines!(ax, xline, gs.slope .* xline .+ gs.intercept, color=:seagreen, linestyle=:dash,
               label=@sprintf("full precision: fc=%.4fHz slope=%.4g", gs.fc, gs.slope))

        precision = fc_round_precision(container_fs)
        fc_rounded = round_down(gs.fc, precision)
        slope_rounded = -1 / fc_rounded^2
        lines!(ax, xline, slope_rounded .* xline .+ gs.intercept, color=:crimson, linestyle=:dot,
               label=@sprintf("rounded (%.0fHz steps): fc= %.0f Hz slope=%.4g", precision, fc_rounded, slope_rounded))
        println(@sprintf("rounded (%.0fHz steps): fc= %.0f Hz slope=%.4g", precision, fc_rounded, slope_rounded))
    end

    axislegend(ax, position=:rt)
    return fig
end

root = length(ARGS) >= 1 ? ARGS[1] : joinpath(@__DIR__, "..", "..", "data", "cal_sine", "132k")
points = collect_cal_points(root)
for (f, v) in points
    @printf("f = %8.1f Hz   normalized ASD = %.6g\n", f, v)
end

fits = fit_all_models(first.(points), last.(points))
container_fs = container_sampling_rate(root)
result_path = write_fit_results(root, points, fits, container_fs)
println("wrote fit results to $result_path")

fig = plot_cal_points(points, fits)
screen = display(fig)
wait(screen)

fig2 = plot_gaussian_linearization(first.(points), last.(points), fits.gaussian, container_fs)
screen2 = display(fig2)
wait(screen2)

# CairoMakie renders the vector/raster output; switch back to GLMakie afterward so a later
# interactive `display` in this session doesn't return an incompatible CairoMakie screen
CairoMakie.activate!()
save(joinpath(root, "gaussian_linearization.svg"), fig2)
save(joinpath(root, "gaussian_linearization.png"), fig2)
GLMakie.activate!()
