from pathlib import Path
import sys
import os
import atsslib.atsslib as atsslib
import ats

channel = atsslib.channel
resolve_file_path = atsslib.resolve_file_path

# takes atss and ats files as parameters and checks the samples.


def is_ats_file(path: Path) -> bool:
    """An ats file has the .ats extension; everything else is treated as atss."""
    return path.suffix.lower() == ".ats"


def ats_samples(path: Path) -> int:
    """Read the number of samples from the binary ats header."""
    header, _slices = ats.read_atsheader(str(path))
    # samples_64bit holds the count for large files; fall back to samples
    return header["samples_64bit"] or header["samples"]


def atss_samples(path: Path) -> int:
    """Read the number of samples of an atss channel from its data file size.

    The atss header (json) does not store the sample count; it is the .atss
    file size divided by 8 (one float64 per sample).
    """
    chan = channel(path)  # accepts basename, .atss or .json
    if chan.atss_path is None:
        raise FileNotFoundError(f"no .atss data file found for {path}")
    return chan.atss_path.stat().st_size // 8


def samples_from_file(path: Path) -> int:
    """Detect the file type and return its sample count."""
    if is_ats_file(path):
        return ats_samples(path)
    return atss_samples(path)


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        prog = Path(sys.argv[0]).name
        print(f"usage: {prog} <file1> <file2>  (one atss and one ats)")
        return 2

    paths = [resolve_file_path(arg) for arg in argv]

    # detect who is who
    ats_paths = [p for p in paths if is_ats_file(p)]
    atss_paths = [p for p in paths if not is_ats_file(p)]

    if len(ats_paths) != 1 or len(atss_paths) != 1:
        print("error: expected exactly one atss file and one ats file.")
        return 1

    ats_path = ats_paths[0]
    atss_path = atss_paths[0]

    n_ats = ats_samples(ats_path)
    n_ats_bin = ats.binary_ats_samples(ats_path)
    if n_ats != n_ats_bin:
        print(f"WARNING: sample count from header ({n_ats}) does not match count from file size ({n_ats_bin})")
    n_atss = atss_samples(atss_path)

    print(f"ats  : {ats_path}  samples = {n_ats}")
    print(f"atss : {atss_path}  samples = {n_atss}")
    print(f"binary ats samples (from file size) = {n_ats_bin}")

    if n_ats == n_atss:
        print(f"OK: sample counts match ({n_ats})")
        return 0

    print(f"MISMATCH: ats={n_ats} atss={n_atss} (diff {n_ats - n_atss})")
    return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
