from pathlib import Path
import sys

base_dir = Path(__file__).resolve().parent
if str(base_dir) not in sys.path:
	sys.path.insert(0, str(base_dir))

from atsslib.atsslib import channel, resolve_file_path


def main() -> None:
	base = resolve_file_path("filter/084_ADU-07e_C002_THx_2048Hz")
	chan = channel(base)

	print(f"Path      : {chan.path}")
	print(f"Header    : {chan.header}")
	print(f"Cal sensor: {chan.calibration['sensor']}")
	print(f"Timing    : {chan.timing_info()}")


if __name__ == "__main__":
	main()
