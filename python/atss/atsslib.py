# ATSS Library

__version__ = "0.1.0"
__author__ = "Bernhard Friedrichs"
from datetime import datetime, timedelta
from pathlib import Path
import json
import operator
import copy
import math
from typing import cast

import numpy as np

# Python has no native unsigned int type; these aliases indicate intent.
UInt = int
PositiveInt = int

def resolve_file_path(filename: str | Path, start_dir: Path | None = None) -> Path:
    """Resolve a file path by searching the current directory and its parents."""
    file_path = Path(filename)
    if file_path.is_absolute():
        if file_path.exists():
            return file_path
        if file_path.suffix == "" and (file_path.with_suffix(".json").exists() or file_path.with_suffix(".atss").exists()):
            return file_path
        raise FileNotFoundError(f"Could not find {file_path}")

    if start_dir is None:
        try:
            start_dir = Path(__file__).resolve().parent
        except NameError:
            start_dir = Path.cwd()

    for root in (start_dir, *start_dir.parents):
        candidate = root / file_path
        if candidate.exists():
            return candidate
        if candidate.suffix == "" and (candidate.with_suffix(".json").exists() or candidate.with_suffix(".atss").exists()):
            return candidate

    raise FileNotFoundError(f"Could not find {file_path} in {start_dir} or its parents.")

def duration_to_hms(seconds: float) -> tuple[int, int, float]:
    """Convert seconds to (hours, minutes, seconds)."""
    total = float(seconds)
    if total < 0:
        raise ValueError("Duration must be non-negative.")

    hours = int(total // 3600)
    rem = total - hours * 3600
    minutes = int(rem // 60)
    sec = rem - minutes * 60
    return hours, minutes, sec


def duration_to_hms_string(seconds: float, precision: int = 2) -> str:
    """Convert seconds to HH:MM:SS.ss style text."""
    h, m, s = duration_to_hms(seconds)
    width = precision + 3  # at least SS.
    return f"{h:02}:{m:02}:{s:0{width}.{precision}f}"

def calibration():
    """
    It is a simple JSON structure.
    Returns a dictionary with the calibration data.
    """
    # contains the items needed for calibration data
    # the JSON ONLY!! contains the used calibration
    # so you have either the HF (chopper off) or LF (chopper on) data here but not both
    cal = {
                "sensor": "",  # MFS-06e ... SHFT-02e 
                "serial": 0,  # number 1, 2 ... not negative
                "chopper": 0,  # 0 == off or unkown, 1 == on (aka switch)
                "units_frequency": "Hz",  # x-axis of your calibration in Hz (never seconds )
                "units_amplitude": "mV/nT",  # default; same unit as time series and no normalization (old mtx format)
                "units_phase": "degrees",  # degrees 0... 360
                "datetime": "1970-01-01T00:00:00",  # date of calibration (1970-01-01 indicates no date available)
                "Operator": "",  # the person who has made the calibration (capital letter because operator is a keyword in C++)
                "f": [0.0],  # frequencies in units as above, array of doubles
                "a": [0.0],  # amplitudes  in units as above, array of doubles
                "p": [0.0],  # phases      in units as above, array of doubles
    }
    return cal


def file_tags():
    """
    The file name is PART of the metadata. There is no redundancy in header + filename. examples:
    099_ADU-07e_C004_THz_32Hz .atss or .json for sample rates >= 1Hz
    099_ADU-07e_C004_THz_8s   .atss or .json for sample rates < 1 Hz (aka periods)
    tags are separated by "_"
    a tag can NOT contain a "_" because that is the separator, so ADU_12e is not allowed
    USER MUST ENSURE TO APPEND Hz or s !!
    """
    tags = {
        "serial": 0,  # such as 1234 (no negative numbers please) for the system
        "system": "",  # such as ADU-08e, XYZ (a manufacturer is not needed because the system indicates it)
        "channel_no": 0,  # channel number - you can integrate EAMP stations as channels if the have the SAME!! timings
        "channel_type": "",  # type such as Ex, Ey, Hx, Hy, Hz or currents like Jx, Jy, Jz or Pitch, Roll, Yaw or x, y, z or T for temperature
        "sampling_rate": 0.0,  # contains sample_rate. Unit: Hz (never seconds)
    }
    return tags


def header():
    """
    The header contains the metadata of the file. It is a dictionary with the following keys:
    The header is stored in the .json file as JSON string
    No values from file_tags are repeated here.
    stop_date_time and duration are not stored in the header because they can be calculated from the data (byte size / 8) and the sampling rate
    """
    header = {
                # the Z suffix is mostly not supported by C/C++/Python/PHP and others
                "datetime": "1970-01-01T00:00:00.0",  # ISO 8601 datetime in UTC
                "latitude": 0.0,  # decimal degree such as 52.2443
                "longitude": 0.0,  # decimal degree such as 10.5594
                "elevation": 0.0,  # elevation in meter
                "azimuth": 0.0,  # orientation from North to East (90 = East, -90 or 270 = West, 180 South, 0 North)
                "tilt": 0.0,  # azimuth positive down - in case it had been measured
                "resistance": 0.0,  # resistance of the sensor in Ohm or contact resistance of electrode in Ohm
                "units": "mV",  # for ADUs it will be mV H or other -  or scaled E mV/km (the logger will do this while recording)
                "id": "",  # string id for the station
                "filter": "",  # comma separated list of filters such as "ADB-LF,LF-RF-4" or "ADB-HF,HF-RF-1"
                "source": "",  # empty or indicate as, ns, ca, cp, tx or what ever; some users need this
    }
    return header

# ##################################################################################################################

class channel:
    """
    The channel class contains the data and the metadata of a channel. It is a container for the data and the metadata.
    """

    def __init__(self, path: str | Path | None = None):
        self.path = None
        self.atss_path = None
        self.json_path = None
        self.tags = file_tags()
        self.header = header()
        self.calibration = calibration()

        if path is None:
            return

        input_path = Path(path)
        suffix = input_path.suffix.lower()

        # Accept only basename, *.atss or *.json and normalize to basename.
        if suffix in (".atss", ".json"):
            base_path = input_path.with_suffix("")
        elif suffix == "":
            base_path = input_path
        else:
            raise ValueError(
                f"Unsupported file extension '{input_path.suffix}'. Use .json, .atss, or no extension."
            )

        atss_path = base_path.with_suffix(".atss")
        json_path = base_path.with_suffix(".json")

        # Both channel files must exist.
        if not atss_path.exists() or not json_path.exists():
            missing = []
            if not atss_path.exists():
                missing.append(str(atss_path))
            if not json_path.exists():
                missing.append(str(json_path))
            raise FileNotFoundError(
                f"Missing required channel file(s): {', '.join(missing)}"
            )

        self.path = base_path
        self.atss_path = atss_path
        self.json_path = json_path
        self.read_header()

    def set_new_base_path(self, new_path: str | Path) -> None:
        """Set a new output base path.

        `new_path` can be either:
        - a directory path (no suffix), which will be created if needed and the
          current basename is kept
        - an explicit channel base path (no suffix)
        """
        new_base_path = Path(new_path)

        if new_base_path.suffix in (".atss", ".json"):
            raise ValueError("new_path must not include .atss or .json suffix.")

        use_as_directory = new_base_path.exists() and new_base_path.is_dir()
        if not use_as_directory and new_base_path.suffix == "":
            try:
                self._read_tags_from_basename(new_base_path)
            except (ValueError, TypeError):
                use_as_directory = True

        if use_as_directory:
            if self.path is None:
                raise ValueError("Current path is not set. Cannot derive basename from directory path.")
            new_base_path.mkdir(parents=True, exist_ok=True)
            new_base_path = new_base_path / self.path.name
        else:
            new_base_path.parent.mkdir(parents=True, exist_ok=True)

        self.path = new_base_path
        self.atss_path = new_base_path.with_suffix(".atss")
        self.json_path = new_base_path.with_suffix(".json")

    def set_parent_path(self, parent_dir: str | Path) -> None:
        """Set a new parent directory for the channel files, creating it if needed."""
        if self.path is None:
            raise ValueError("Current path is not set. Cannot change parent directory.")

        self.set_new_base_path(parent_dir)

    def read_header(self):
        """Read the channel metadata from the .json file."""
        if self.path is None:
            raise ValueError("No path provided for reading channel metadata.")

        # Parse filename tags from the normalized basename.
        self.tags = self._read_tags_from_basename(self.path)

        # Read json metadata and merge into header/calibration dictionaries.
        if self.json_path is None:
            raise ValueError("JSON path is not set.")

        with open(self.json_path, "r", encoding="utf-8") as f:
            jdata = json.load(f)

        if not isinstance(jdata, dict):
            raise ValueError(f"JSON content must be an object: {self.json_path}")

        for key in self.header:
            if key in jdata:
                self.header[key] = jdata[key]

        # Legacy json files may use "angle" instead of "azimuth".
        if "azimuth" not in jdata and "angle" in jdata:
            self.header["azimuth"] = jdata["angle"]

        cal = jdata.get("sensor_calibration")
        if isinstance(cal, dict):
            for key in self.calibration:
                if key in cal:
                    self.calibration[key] = cal[key]

    @staticmethod
    def _read_tags_from_basename(base_path: Path) -> dict:
        """Extract file tags from basename like 084_ADU-07e_C002_THx_128Hz."""
        tags = file_tags()
        parts = base_path.name.split("_")
        if len(parts) < 5:
            raise ValueError(f"Invalid ATSS basename format: {base_path.name}")

        tags["serial"] = int(parts[0])
        tags["system"] = parts[1]

        channel_no = parts[2]
        if channel_no.startswith("C"):
            channel_no = channel_no[1:]
        tags["channel_no"] = int(channel_no)

        channel_type = parts[3]
        if channel_type.startswith("T"):
            channel_type = channel_type[1:]
        tags["channel_type"] = channel_type

        rate = parts[4]
        if rate.endswith("Hz"):
            tags["sampling_rate"] = float(rate[:-2])
        elif rate.endswith("s"):
            tags["sampling_rate"] = 1.0 / float(rate[:-1])
        else:
            raise ValueError(f"Invalid sampling rate tag in basename: {base_path.name}")

        return tags
    
    def write_header(self):
        """Write the channel metadata to the .json file."""
        if self.path is None:
            raise ValueError("No path provided for writing channel metadata.")
        if self.json_path is None:
            raise ValueError("JSON path is not set.")

        # Construct JSON content from header and calibration dictionaries.
        jdata = dict(self.header)
        jdata["sensor_calibration"] = dict(self.calibration)

        with open(self.json_path, "w", encoding="utf-8") as f:
            json.dump(jdata, f, indent=4)
        
    def get_samples(self) -> int:
        """Calculate the number of samples in the .atss file."""
        if self.atss_path is None:
            raise ValueError("ATSS path is not set.")
        file_size_bytes = self.atss_path.stat().st_size
        num_samples = file_size_bytes // 8  # 8 bytes per double
        return num_samples

    def duration(self) -> float:
        """Calculate duration in seconds from .atss file size and sampling rate."""
        if self.atss_path is None:
            raise ValueError("ATSS path is not set.")
        if self.tags["sampling_rate"] <= 0:
            raise ValueError("Invalid sampling rate in tags.")

        file_size_bytes = self.atss_path.stat().st_size
        num_samples = file_size_bytes / 8  # 8 bytes per double
        duration_seconds = num_samples / self.tags["sampling_rate"]
        return duration_seconds
    
    def stop_date_time(self) -> str:
        """Calculate stop datetime by adding duration to start datetime."""
        from datetime import datetime, timedelta

        if self.header["datetime"] == "1970-01-01T00:00:00.0":
            return "unknown"

        try:
            start_dt = datetime.fromisoformat(self.header["datetime"])
        except ValueError:
            return "invalid_start_datetime"

        try:
            duration_sec = self.duration()
        except ValueError:
            return "invalid_duration"

        stop_dt = start_dt + timedelta(seconds=duration_sec)
        return stop_dt.isoformat(sep="T", timespec="seconds")
    
    def timing_info(self) -> str:
        """Return a human-readable string of the timing information."""
        start = self.header["datetime"]
        stop = self.stop_date_time()
        duration = self.duration() if self.atss_path is not None else "unknown"
        if isinstance(duration, (int, float)):
            duration_hms = duration_to_hms_string(duration)
            return f"Start: {start}, Stop: {stop}, Duration: {duration_hms} ({duration:.2f} seconds)"
        return f"Start: {start}, Stop: {stop}, Duration: unknown"
    
    def data(self, start: UInt = 0, wl: PositiveInt = 1024) -> np.ndarray[tuple[int], np.dtype[np.float64]]:
        """Read a window of data from the .atss file starting at sample index 'start' with window length 'wl'."""

        if self.atss_path is None:
            raise ValueError("ATSS path is not set.")

        # Accept integer-like values (including numpy integers) and validate ranges.
        start_i = operator.index(start)
        wl_i = operator.index(wl)

        if start_i < 0 or wl_i <= 0:
            raise ValueError("Start index must be non-negative and window length must be positive.")
        
        # check if we can seek and can read the requested window without going beyond the file size
        file_size_bytes = self.atss_path.stat().st_size
        if (start_i * 8) >= file_size_bytes:
            return np.array([])  # return empty array if start is beyond the file size
        if ((start_i + wl_i) * 8) > file_size_bytes:
            return np.array([])  # return empty array if the window extends beyond the file size

        with open(self.atss_path, "rb") as f:
            f.seek(start_i * 8)  # Seek to the start sample (8 bytes per double)
            data = f.read(wl_i * 8)  # Read wl samples
            return np.frombuffer(data, dtype=np.float64)
        
    def read_all_data(self) -> np.ndarray[tuple[int], np.dtype[np.float64]]:
        """Read the entire data from the .atss file."""
        if self.atss_path is None:
            raise ValueError("ATSS path is not set.")
        return np.fromfile(self.atss_path, dtype=np.float64)
    
    def change_sampling_rate(self, new_rate: float) -> "channel":
        """Return a copied channel with updated sampling rate and basename/path tags.

        This does not write files. The returned channel is ready for subsequent
        write operations (json/data) using its updated basename.
        """
        if new_rate <= 0:
            raise ValueError("New sampling rate must be positive.")

        # Create a detached copy so the original channel remains unchanged.
        new_chan = channel()
        new_chan.tags = copy.deepcopy(self.tags)
        new_chan.header = copy.deepcopy(self.header)
        new_chan.calibration = copy.deepcopy(self.calibration)
        new_chan.path = self.path
        new_chan.atss_path = self.atss_path
        new_chan.json_path = self.json_path

        # Update tag value with the new numeric sampling rate.
        new_chan.tags["sampling_rate"] = float(new_rate)

        # Rebuild basename tag using sample_rate_to_string (e.g. 0.25 -> "4s").
        if self.path is not None:
            serial = int(new_chan.tags["serial"])
            system = str(new_chan.tags["system"])
            channel_no = int(new_chan.tags["channel_no"])
            channel_type = str(new_chan.tags["channel_type"])
            sr_tag = sample_rate_to_string(new_chan.tags["sampling_rate"])

            new_basename = f"{serial:03}_{system}_C{channel_no:03}_T{channel_type}_{sr_tag}"
            new_base_path = self.path.parent / new_basename
            new_chan.set_new_base_path(new_base_path)

        return new_chan
    
    def add_second_to_start_time(self, seconds: int) -> None:
        """Add a number of seconds to the start time in the header."""
        if self.header["datetime"] == "1970-01-01T00:00:00.0":
            return  # Do not modify unknown start time

        try:
            start_dt = datetime.fromisoformat(self.header["datetime"])
        except ValueError:
            return  # Do not modify invalid start time

        new_start_dt = start_dt + timedelta(seconds=seconds)
        self.header["datetime"] = new_start_dt.isoformat(sep="T", timespec="seconds")

    def decimate(self, filter_coefficients: np.ndarray, factor: int, out_path: str | Path) -> "channel":
        """Decimate the data by applying a FIR filter with downsampling.

        `out_path` may be either a target directory or a full channel base path.
        Directory targets keep the channel basename; explicit base paths replace it.
        """

        if self.atss_path is None:
            raise ValueError("ATSS path is not set.")
        if factor <= 0:
            raise ValueError("Decimation factor must be positive.")

        coeffs = np.asarray(filter_coefficients, dtype=np.float64)
        if coeffs.ndim != 1:
            raise ValueError("filter_coefficients must be a 1D vector of doubles.")

        # this returns a new channel with the updated sampling rate and so on ...
        new_chan = self.change_sampling_rate(self.tags["sampling_rate"] / factor)
        sample_shift, add_secs = self.skip_samples_from_filter(len(coeffs))
        new_chan.add_second_to_start_time(add_secs)
        new_chan.set_new_base_path(out_path)

        new_chan.write_header()  # Update JSON metadata for the new sampling rate.
        # delete existing .atss file if it exists, because we will write new data to it
        if new_chan.atss_path is not None and new_chan.atss_path.exists():
            new_chan.atss_path.unlink()

        print(f"Decimating with factor {factor}, filter length {len(coeffs)}, shift {sample_shift} samples.")

        idx = sample_shift  # Start index for reading input data, accounting for filter delay.
        window_size = len(coeffs)
        buffer_size = 4096  # number of output samples per write chunk
        buffer = np.empty(buffer_size, dtype=np.float64)
        count = 0

        with open(self.atss_path, "rb") as f:
            while True:
                f.seek(idx * 8)  # Seek to the current index
                raw = f.read(window_size * 8)  # Read a window of float64 values
                if len(raw) < window_size * 8:
                    break  # End of file reached

                data_window = np.frombuffer(raw, dtype=np.float64)
                y = np.convolve(data_window, coeffs, mode="full")[0]
                buffer[count] = np.float64(y)
                count += 1

                # Write chunked binary data to avoid storing all samples in RAM.
                if count == buffer_size:
                    new_chan.write_data(buffer, append=True)
                    count = 0

                idx += factor  # Move to the next window based on the decimation factor

        # Flush remaining buffered samples.
        if count > 0:
            new_chan.write_data(buffer[:count], append=True)
        return new_chan

    def write_data(self, data: np.ndarray | list[float], append: bool = True) -> None:
        """Write a 1D vector as binary float64 values to the .atss file."""
        if self.atss_path is None:
            raise ValueError("ATSS path is not set.")

        vec = np.asarray(data, dtype=np.float64)
        if vec.ndim != 1:
            raise ValueError("data must be a 1D vector of doubles (float64).")

        mode = "ab" if append else "wb"
        with open(self.atss_path, mode) as f:
            vec.tofile(f)

    def skip_samples_from_filter(self, filter_len) -> tuple[int, int]:
        """Compute how many samples to skip to shift the filtered data to a full second.
        Parameters
        this is called inside the old channel, so the SOURCE channel
        and returns for usage in the NEW (filtered) TARGET channel
        ----------
        filter_len : int
            FIR filter length ("filter_length"), wich is ALWAYS ODD

        Returns
        -------
        tuple[int, int]
            samples_to_skip: number of samples to skip at the start.
            add_secs_to_start_time: number of seconds to add to the start time.
        """
        # example for 128 Hz and 32 x filter, (471 -1) // 2 = 235 samples delay
        # fracs = 1.8359375 = 235 / 128 .. that also can be 4.34 ... more than 1s
        sampling_rate = self.tags["sampling_rate"]
        fracs = ((float(filter_len - 1)) * 0.5) / sampling_rate

        # delay can be more than 1s; split into integer (fulldelay) and fractional (miss) part
        fulldelay = math.floor(fracs)  # in the example 1
        miss = fracs - fulldelay       # in the example 0.8359375
        add_secs_to_start_time = int(0)
        # 21 = 128 - (128 * 0.8359375) = 128 - 107 = 21 samples to skip
        samples_to_skip = int(sampling_rate - miss * sampling_rate)

        # miss is in the sub sample ?
        # if miss is very small (almost zero), we assume a rounding error
        if abs(samples_to_skip - int(sampling_rate)) <= 1:
            samples_to_skip = 0

        # allow rounding error
        if (sampling_rate <= 1.001):
            # for very low sampling rates, we can not skip samples, but we can add seconds to the start time
            add_secs_to_start_time = int(math.ceil(fracs))  # 2 seconds to add to the start time, if it would be < 1 Hz
        else:
            add_secs_to_start_time = int(fulldelay)  # in the example 1 second to add to the start time
            if samples_to_skip > 0:
                add_secs_to_start_time += 1  # samples_to_skip is valide, we add another second
        # in the example we skip 21, read 235 samples and therewith reach the 256th sample, which is at 2 seconds. ... and sure, we also read the full filter length       
        return samples_to_skip, add_secs_to_start_time
        
    def rm_seconds_from_start(self, seconds: int) -> None:
        """Remove a number of seconds from the start time in the header.
        and cut the data accordingly, that means remove the corresponding number of samples from the start of the data and update the .atss file. 
        """
        if seconds < 0:
            raise ValueError("seconds must be non-negative.")

        if self.header["datetime"] == "1970-01-01T00:00:00.0":
            return  # Do not modify unknown start time

        try:
            start_dt = datetime.fromisoformat(self.header["datetime"])
        except ValueError:
            return  # Do not modify invalid start time

        # Removing data from the beginning shifts the effective start time later.
        new_start_dt = start_dt + timedelta(seconds=seconds)
        self.header["datetime"] = new_start_dt.isoformat(sep="T", timespec="seconds")
        self.write_header()  # Update JSON metadata with the new start time.
        # Now we need to cut the data accordingly
        if self.atss_path is None:
            raise ValueError("ATSS path is not set.")
        if self.tags["sampling_rate"] <= 0:
            raise ValueError("Invalid sampling rate in tags.")
        sampling_rate = float(self.tags["sampling_rate"])
        if sampling_rate < 1.0:
            # For low-rate channels (period-based), remove only full period-sized segments.
            period_seconds_f = 1.0 / sampling_rate
            period_seconds = int(period_seconds_f)
            if (period_seconds_f - period_seconds) > 0.5:
                period_seconds += 1
            period_seconds = max(1, period_seconds)

            if seconds % period_seconds != 0:
                raise ValueError(
                    f"For sampling_rate={sampling_rate}, seconds must be a multiple of {period_seconds}."
                )
            samples_to_remove = seconds // period_seconds
        else:
            samples_to_remove = int(seconds * sampling_rate)
        if samples_to_remove <= 0:
            return  # No samples to remove
        # Read the existing data, remove the samples, and write back the remaining data.
        data = self.read_all_data()
        if samples_to_remove >= len(data):
            raise ValueError("Cannot remove more samples than exist in the data.")
        remaining_data = data[samples_to_remove:]
        self.write_data(remaining_data, append=False)  # Overwrite with remaining data
         
# #################################END CHANNEL #########################################################

## plotting with bokeh

def plot_ts(channels: list[channel], start: UInt = 0, wl: PositiveInt = 1024):
    """Plot time series data from the given channels using one overlaid Bokeh figure."""
    from bokeh.io import output_notebook
    from bokeh.palettes import Category10
    from bokeh.plotting import figure, show

    # Ensure Bokeh renders inline in notebook environments (VS Code/Jupyter).
    output_notebook(hide_banner=True)

    p = figure(
        title="Overlay Time Series",
        x_axis_label="Sample Index",
        y_axis_label="Signal",
    )

    lines = 0
    for i, chan in enumerate(channels):
        data = chan.data(start, wl)
        if data.size == 0:
            continue

        x = np.arange(start, start + len(data)).tolist()
        y = data.tolist()
        color = Category10[10][i % 10]
        label = f"{chan.tags['channel_type']} @ {chan.tags['sampling_rate']} Hz"
        p.line(x, y, line_width=2, color=color, legend_label=label)
        lines += 1

    if lines > 0:
        p.legend.location = "top_left"
        p.legend.click_policy = "hide"
        show(p)
        return p

    return None

def fft_quick(chan: channel, start: UInt = 0, wl: PositiveInt = 1024, cdown = 0.006, cup = 0.75, stack_all: bool = False, window = "hanning" ) -> list[tuple[float, float]]:
    """Compute a quick FFT of the data from the given channel.
    returns the amplitude spectrum as (frequency, amplitude) pairs.
    in main you can fetch the result like this:
    result = fft_quick(chan, start, wl, cdown, cup, stack_all, window)
    and then you can access the frequencies and amplitudes like this:
    frequencies = [f for f, a in result]
    amplitudes = [a for f, a in result]
    """
    ampl_spec_sum = None
    n_stacked = 0
    current_start = operator.index(start)

    while True:
        data = chan.data(current_start, wl)
        if data.size == 0:
            break

        data = data - np.mean(data)
        if window is not None:
            # Hanning window
            if window == "hanning":
                data = data * np.hanning(len(data))

        spec = np.fft.rfft(data, norm="backward")
        wincal = np.sqrt(1.0 / (0.5 * wl * chan.tags["sampling_rate"])) * 2.0
        spec *= wincal
        ampl_spec = np.abs(spec)

        if ampl_spec_sum is None:
            ampl_spec_sum = np.zeros_like(ampl_spec)
        ampl_spec_sum += ampl_spec
        n_stacked += 1

        if not stack_all:
            break

        # Move start to the next window in stack_all mode.
        current_start += wl

    if n_stacked == 0 or ampl_spec_sum is None:
        return []

    ampl_spec = ampl_spec_sum / n_stacked

    # floor division //, so that we get an integer
    sz = wl//2 + 1
    ampl_freq = np.linspace(0, 1, sz, endpoint=True)
    ampl_freq = ampl_freq * chan.tags["sampling_rate"]
    # shorten the spectrum to the cup factor
    ampl_spec = ampl_spec[:int(cup * len(ampl_spec))]
    ampl_freq = ampl_freq[:int(cup * len(ampl_freq))]
    # shorten the spectrum to the cdown factor from the lower end (beginning)
    ampl_spec = ampl_spec[int(cdown * len(ampl_spec)):]
    ampl_freq = ampl_freq[int(cdown * len(ampl_freq)):]


    # in main you fetch the result like this:
    # result = fft_quick(chan, start, wl, cdown, cup, stack_all, window)
    # and then you can access the frequencies and amplitudes like this:
    # frequencies = [f for f, a in result]
    # amplitudes = [a for f, a in result]

    return list(zip(ampl_freq, ampl_spec))

def plot_fft(
    input: list[tuple[float, float]] | list[list[tuple[float, float]]],
    title: str = "FFT Spectrum",
    x_label: str = "Frequency (Hz)",
    y_label: str = "Amplitude",
    f_min: float | None = None,
    f_max: float | None = None,
    labels: list[str] | None = None,
) -> None:
    """Plot one or multiple FFT spectra using Bokeh.

    input can be either:
    - one spectrum: [(f, a), ...]
    - multiple spectra: [[(f, a), ...], [(f, a), ...], ...]
    """
    from bokeh.io import output_notebook
    from bokeh.palettes import Category10
    from bokeh.plotting import figure, show

    output_notebook(hide_banner=True)

    if not input:
        return

    # Normalize to a list of spectra.
    spectra: list[list[tuple[float, float]]] = []
    if all(isinstance(item, tuple) and len(item) == 2 for item in input):
        single_input = cast(list[tuple[float, float]], input)
        single = [(float(f), float(a)) for f, a in single_input]
        spectra = [single]
    else:
        multi_input = cast(list[list[tuple[float, float]]], input)
        for spec in multi_input:
            if not isinstance(spec, list):
                raise ValueError("input must be one spectrum or a list of spectra")
            if not all(isinstance(item, tuple) and len(item) == 2 for item in spec):
                raise ValueError("each spectrum must be a list of (frequency, amplitude) tuples")
            spectra.append([(float(f), float(a)) for f, a in spec])

    if labels is not None and len(labels) != len(spectra):
        raise ValueError("labels length must match number of spectra")

    p = figure(title=title, x_axis_label=x_label, y_axis_label=y_label)

    lines = 0
    for i, spec in enumerate(spectra):
        if not spec:
            continue

        if f_min is None and f_max is None:
            filtered = spec
        else:
            filtered = [
                (f, a)
                for f, a in spec
                if (f_min is None or f >= f_min) and (f_max is None or f <= f_max)
            ]

        if not filtered:
            continue

        frequencies = [f for f, a in filtered]
        amplitudes = [a for f, a in filtered]
        color = Category10[10][i % 10]
        legend = labels[i] if labels is not None else f"fft_{i + 1}"
        p.line(frequencies, amplitudes, line_width=2, color=color, legend_label=legend)
        lines += 1

    if lines > 0:
        p.legend.location = "top_right"
        p.legend.click_policy = "hide"
        show(p)

def sample_rate_to_string(sample_rate, precision=0):
    if sample_rate == 0.0:
        return "failed__zero_sample_rate"

    if precision == 0:
        if sample_rate > 0.5:  # assumed to be an rounding error
            fd = sample_rate - int(sample_rate)  # get the decimal part
            fi = int(sample_rate)  # get the integer part
            if fd > 0.9:
                fi = fi + 1
            return str(fi) + "Hz"
        else:
            fd = 1.0 / sample_rate - int(1.0 / sample_rate)  # get the decimal part
            fi = int(1.0 / sample_rate)  # get the integer part
            if fd > 0.5:
                fi = fi + 1
            return str(fi) + "s"
    else:
        if sample_rate > 0.999:  # you may want to correct the rounding error manually, I set to Hz
            return "{:.{prec}f}Hz".format(sample_rate, prec=precision)
        else:
            return "{:.{prec}f}s".format(1.0 / sample_rate, prec=precision)

def expected_decimation_samples(samples_in: int, samples_shift: int, sampling_rate: int, filter_length: int, factor: int) -> int:
    """Calculate the expected number of output samples after decimation, given the input sample count, filter delay, and decimation factor."""
    if samples_in <= 0:
        return 0
    if factor <= 0:
        raise ValueError("Decimation factor must be positive.")
    if filter_length <= 0:
        raise ValueError("Filter length must be positive.")
    if samples_shift < 0:
        raise ValueError("Samples shift must be non-negative.")

    # Calculate the number of valid samples after accounting for filter delay.
    valid_samples = max(0, samples_in - samples_shift - (filter_length - 1) // 2)

    # Calculate the number of output samples after decimation.
    output_samples = valid_samples // factor

    return output_samples