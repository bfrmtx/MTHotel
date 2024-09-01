from ats_base import create_header
import os

header = create_header()

header["header_length"] = 1024
header["header_version"] = 80
header["lsbval"] = 3.606728E-03
header["serial_number"] = 200
header["chopper"] = 1
header["channel_number"] = 2
header["sensor_type"] = "MFS06e"
header["sensor_serial_number"] = 120
header["channel_type"] = "Hx"

# filename = "999_V01_C02_R000_THx_BL_8H.atss"
filesize = os.path.getsize(filename)
header["samples"] = int(filesize / 8)
print("samples ", header['samples'])
