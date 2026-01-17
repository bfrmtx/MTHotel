# small python module for handling EDI files
# uses ./edi/edi_lib.py 
# parameters:
#   input_file: path to the EDI file to be processed
#   output_file: path to save the processed EDI file
# -remove_zrot
# -remove_trot
# -remove_f f1 f2 f3 ...
# example usage:
#   python ch_edi.py input.edi output.edi -remove_zrot -remove_f 100 200 300
import argparse
from edi import *

def main():
    parser = argparse.ArgumentParser(description="Process EDI files.")
    parser.add_argument("input_file", type=str, help="Path to the input EDI file")
    parser.add_argument("output_file", type=str, help="Path to save the processed EDI file")
    parser.add_argument("-remove_zrot", action="store_true", help="Remove Z rotation block")
    parser.add_argument("-remove_trot", action="store_true", help="Remove T rotation block")
    parser.add_argument("-remove_f", nargs="+", type=float, help="Frequencies to remove from data blocks")

    args = parser.parse_args()
    # we need at least input and output file (that would re-write the file)
    if not args.input_file or not args.output_file:
        parser.print_help()
        return

    # Parse the EDI file (functions expect filename, not content)
    meta_blocks = parse_edi_all_meta_blocks(args.input_file)
    data_blocks = parse_edi_all_data_blocks(args.input_file)

    # Process according to arguments
    if args.remove_zrot:
        remove_zrot_block(data_blocks)

    if args.remove_trot:
        remove_trot_block(data_blocks)

    if args.remove_f:
        remove_frequencies_from_data_blocks(data_blocks, args.remove_f)

    # Get the number of frequencies after processing
    n_frequencies = data_blocks[0].n_frequencies if data_blocks else 0

    # Write the processed EDI file
    write_edi_all_meta_blocks(args.output_file, meta_blocks, n_frequencies)
    write_edi_all_data_blocks(args.output_file, data_blocks)
if __name__ == "__main__":
    main()