#!/usr/bin/env python3

import sys
import os
from lib.examin_st import extract_log_messages, write_html_table_logs, mysql_to_sqlite

# 1) in database adu_error_msg.sql3, table error_messages, columns:
# Component, MainIndex, SubIndex, Priority, Message, DisplayMsg., Description
# 2) in the newly created sqlite database, table log has columns:
# id, timestamp, priority, component, mainindex, subindex, message
# 3) from the newly created database select timestamp, priority, component, mainindex, subindex
# from the error_messages table we find select where component == component and mainindex == mainindex and subindex == subindex, and take Message and Description
# finally we make a html table with the following columns:
# timestamp, priority, component, mainindex, subindex, message, description

if __name__ == "__main__":
  # scan all files *.sql in ./input  
  input_dir = './input'
  error_msg_db_file = "./lib/adu_error_msg_definitions.sql3"  # error messages SQLite database file  

  mysql_dump_files = [os.path.join(input_dir, f) for f in os.listdir(input_dir) if f.endswith('.sql')]
  for mysql_dump_file in mysql_dump_files:
    # Ensure output directory exists
    os.makedirs('./output', exist_ok=True)
    # Create sqlite file in output directory
    sqlite_db_file = os.path.join('./output', os.path.basename(os.path.splitext(mysql_dump_file)[0]) + ".sql3")
    mysql_to_sqlite(mysql_dump_file, sqlite_db_file)
    # Extract log messages and write to HTML
    log_messages = extract_log_messages(sqlite_db_file, error_msg_db_file)
    if log_messages:
      output_file = os.path.join('./output/', os.path.basename(sqlite_db_file).replace('.sql3', '.html'))
      # Write the log messages to an HTML file
      write_html_table_logs(log_messages, output_file)
      print(f"Log messages written to {output_file}")
    else:
      print("No log messages found or an error occurred.")
  print("All operations completed successfully.")
  sys.exit(0)
