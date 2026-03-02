# load my python code from ./lib/examin_st.py
import os
import sys

from lib.examin_st import (
  parse_adb_boards,
  likely_off,
  extract_standard_messages,
  boards_to_selftest_table,
  create_styled_table,
  summary_html,
)

# Parse the XML file
def measdoc2html(xml_file_path, output_dir, debug=False):
  boards = parse_adb_boards(xml_file_path, debug)
  # print how many boards are found
  # for all boards, print the AdbError element
  if debug:
    for board in boards:
      print(f"Board ID: {board.id}, AdbError: {board.AdbError}")
  # check if the board is likely off
  old_boards = len(boards)
  boards = likely_off(boards, debug)
  # print how many boards are left after filtering

  print(f"found {old_boards} boards, after filtering {len(boards)} boards left")
  # we load the system history
  messages = extract_standard_messages(xml_file_path)
  # print standard messages found
  print(f"Found {len(messages)} standard messages in the XML file.")
  dfx = boards_to_selftest_table(boards)
  styled_table = create_styled_table(dfx, "ADB Boards - Complete Channel Data, Selftest")
  html_data = summary_html(styled_table, boards, "Selftest Result", debug)
  # Save the styled table to an HTML file
  # Ensure output_dir exists
  os.makedirs(output_dir, exist_ok=True)
  # Build the output HTML file path in the output_dir
  base_name = os.path.splitext(os.path.basename(xml_file_path))[0]
  html_file_path = os.path.join(output_dir, base_name + ".html")
  # write 
  with open(html_file_path, "w") as f:
    f.write(html_data)
  print(f"Table saved to {html_file_path}")

if __name__ == "__main__":
  
  xmlfiles = [f for f in os.listdir('./input') if f.endswith('.xml')]
  if not xmlfiles:
    print("No XML files found in the input directory.")
    sys.exit(1)

  for xml_file in xmlfiles:
    xml_file_path = os.path.join('./input', xml_file)
    default_debug = False
    measdoc2html(xml_file_path, './output', default_debug)