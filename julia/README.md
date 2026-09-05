# Julia add-ons for MTHotel

Plain Julia scripts (no Pkg package, no Project.toml/Manifest.toml). Packages
(Plots, DataFrames, etc.) are expected to already be installed in your global
environment (`~/.julia/environments/v1.12/`), so scripts just run with plain
`julia scriptfile.jl` — no `--project` / `activate` needed.

## Layout

- `src/` — reusable `.jl` files, grouped by topic (Julia's equivalent of a
  C++ `include/`; the actual keyword `include()` is how you pull them in).
  - `src/timeseries/atss.jl`
  - `src/plotting/plot_ts.jl`
  - `src/includes.jl` — convenience file that `include()`s everything above.
- `scripts/` — the actual add-on scripts you run, e.g. `scripts/example.jl`.

## Usage

```julia
include(joinpath(@__DIR__, "..", "src", "includes.jl"))
atss()
plot_ts()
```

or run a script directly:

```sh
julia scripts/example.jl
```

## Adding a new reusable file

1. Put the `.jl` file under `src/<topic>/` (create the topic folder if new).
2. Add an `include(...)` line for it in `src/includes.jl`.

