"""
ADU XML and Database Analysis Library

This module provides utilities for parsing and analyzing measurement documentation
XML files from ADU instruments, extracting board self-test data, and generating
styled HTML reports. It also includes database conversion and log message extraction
utilities.

Main Functions:
    - XML Parsing: parse_adb_boards, parse_channel_config, parse_atswriter_configuration
    - Analysis: likely_off, extract_standard_messages, boards_to_selftest_table
    - HTML Generation: create_styled_table, summary_html, write_html_table_logs
    - Database: extract_log_messages, mysql_to_sqlite, AdbError_to_text
"""

__version__ = "1.0.0"
__author__ = "Measurement Team"

from dataclasses import dataclass, field
from typing import Optional
import xml.etree.ElementTree as ET
import sqlite3
import pandas as pd
import re
import os

__all__ = [
    'AdbBoardData',
    'parse_adb_boards',
    'parse_channel_config',
    'parse_atswriter_configuration',
    'likely_off',
    'extract_standard_messages',
    'boards_to_selftest_table',
    'AdbError_to_text',
    'print_adb_error_messages',
    'create_styled_table',
    'summary_html',
    'print_adb_error_messages_as_html',
    'extract_log_messages',
    'write_html_table_logs',
    'mysql_to_sqlite',
]

# ============================================================================
# Data Models
# ============================================================================

@dataclass
class AdbBoardData:
  id: int
  AdbError: Optional[int] = None
  Atten_HF_HP_500Hz: Optional[float] = None
  Atten_LF_LP_4Hz: Optional[float] = None
  DCOffset: Optional[float] = None
  ExtOffsetComp: Optional[float] = None
  GainAmpl: Optional[float] = None
  GainCheckHF16: Optional[int] = None
  GainCheckHF4: Optional[int] = None
  GainCheckHF8: Optional[int] = None
  GainCheckLF16: Optional[int] = None
  GainCheckLF4: Optional[int] = None
  GainCheckLF8: Optional[int] = None
  GainCorr: Optional[float] = None
  HF_LSB: Optional[float] = None
  InputDivCheckOff: Optional[float] = None
  InputDivCheckOn: Optional[float] = None
  IntOffsetCorr_1_HF: Optional[float] = None
  IntOffsetCorr_1_LF: Optional[float] = None
  IntOffsetCorr_2_HF: Optional[float] = None
  IntOffsetCorr_2_LF: Optional[float] = None
  LF_LSB: Optional[float] = None
  MaxAmplitude: Optional[float] = None
  Message: Optional[str] = None
  NoiseCheckHF: Optional[float] = None
  NoiseCheckLF: Optional[float] = None
  RFCheckStrong: Optional[float] = None
  RFCheckWeak: Optional[float] = None
  Resistivity: Optional[float] = None
  Resistivity_connN_or_connE_connGND: Optional[float] = None
  Resistivity_connS_or_connW_connGND: Optional[float] = None
  Severity: Optional[str] = None
  # Add fields from <recording><Hardware><channel_config>
  gain_stage1: Optional[int] = None
  gain_stage2: Optional[int] = None  
  ext_gain: Optional[int] = None    
  filter_type: Optional[str] = None
  hchopper: Optional[int] = None    
  echopper: Optional[int] = None    
  dac_val: Optional[int] = None    
  dac_on: Optional[int] = None      
  input: Optional[int] = None       
  direct_mode: Optional[int] = None 
  input_divider: Optional[int] = None
  # add fields from <ATSWriter><configuration>
  ts_lsb: Optional[float] = None
  channel_type: Optional[str] = None
  sensor_sernum: Optional[int] = None
  sensor_type: Optional[str] = None

# ============================================================================
# XML Parsing Functions
# ============================================================================

def parse_adb_boards(xml_file, debug=False):
  """Parse the XML file and extract AdbBoardData instances
     the XML structure is as follows:
        
      <channel id="0">  
        <AdbError>0</AdbError>
        <Atten_HF_HP_500Hz>0</Atten_HF_HP_500Hz>
        <Atten_LF_LP_4Hz>0</Atten_LF_LP_4Hz>
        <DCOffset>-6.95064</DCOffset>
        <ExtOffsetComp>0</ExtOffsetComp>
        <GainAmpl>1</GainAmpl>
        <GainCheckHF16>0</GainCheckHF16>
        <GainCheckHF4>0</GainCheckHF4>
        <GainCheckHF8>0</GainCheckHF8>
        <GainCheckLF16>0</GainCheckLF16>
        <GainCheckLF4>0</GainCheckLF4>
        <GainCheckLF8>0</GainCheckLF8>
        <GainCorr>0</GainCorr>
        <HF_LSB>1.18809e-06</HF_LSB>
        <InputDivCheckOff>0</InputDivCheckOff>
        <InputDivCheckOn>0</InputDivCheckOn>
        <IntOffsetCorr_1_HF>-1601735</IntOffsetCorr_1_HF>
        <IntOffsetCorr_1_LF>16230562</IntOffsetCorr_1_LF>
        <IntOffsetCorr_2_HF>0</IntOffsetCorr_2_HF>
        <IntOffsetCorr_2_LF>0</IntOffsetCorr_2_LF>
        <LF_LSB>5.96144e-07</LF_LSB>
        <MaxAmplitude>162.822</MaxAmplitude>
        <Message>---</Message>
        <NoiseCheckHF>0</NoiseCheckHF>
        <NoiseCheckLF>0</NoiseCheckLF>
        <RFCheckStrong>0</RFCheckStrong>
        <RFCheckWeak>0</RFCheckWeak>
        <Resistivity>2277.14</Resistivity>
        <Resistivity_connN_or_connE_connGND>3041.17</Resistivity_connN_or_connE_connGND>
        <Resistivity_connS_or_connW_connGND>3721.54</Resistivity_connS_or_connW_connGND>
        <Severity>OK</Severity>
      </channel>

  """
  tree = ET.parse(xml_file)
  root = tree.getroot()
  boards = []
  if debug:
    print (f"Parsing XML file: {xml_file}")
  # Navigate to <measurement><Selftest><AdbBoards>
  selftest = root.find('.//Selftest')
  if selftest is not None:
    adb_boards = selftest.find('AdbBoards')
    if adb_boards is not None:
      for adb_elem in adb_boards.findall('channel'):
        # find all <channel id="0"> and so on elements
        channel_id = adb_elem.get('id')
        if channel_id is not None:
          # if <AdbError>N/A</AdbError> we skip the entry
          if adb_elem.findtext('AdbError', default='N/A') == 'N/A':
            continue
          # create an instance of AdbBoardData with the values from the XML
          board_data = AdbBoardData(
            AdbError=int(adb_elem.findtext('AdbError', default='0')),
            Atten_HF_HP_500Hz=float(adb_elem.findtext('Atten_HF_HP_500Hz', default='0')),
            Atten_LF_LP_4Hz=float(adb_elem.findtext('Atten_LF_LP_4Hz', default='0')),
            DCOffset=float(adb_elem.findtext('DCOffset', default='0')),
            ExtOffsetComp=float(adb_elem.findtext('ExtOffsetComp', default='0')),
            GainAmpl=float(adb_elem.findtext('GainAmpl', default='0')),
            GainCheckHF16=int(adb_elem.findtext('GainCheckHF16', default='0')),
            GainCheckHF4=int(adb_elem.findtext('GainCheckHF4', default='0')),
            GainCheckHF8=int(adb_elem.findtext('GainCheckHF8', default='0')),
            GainCheckLF16=int(adb_elem.findtext('GainCheckLF16', default='0')),
            GainCheckLF4=int(adb_elem.findtext('GainCheckLF4', default='0')),
            GainCheckLF8=int(adb_elem.findtext('GainCheckLF8', default='0')),
            GainCorr=float(adb_elem.findtext('GainCorr', default='0')),
            HF_LSB=float(adb_elem.findtext('HF_LSB', default='0')),
            InputDivCheckOff=float(adb_elem.findtext('InputDivCheckOff', default='0')),
            InputDivCheckOn=float(adb_elem.findtext('InputDivCheckOn', default='0')),
            IntOffsetCorr_1_HF=float(adb_elem.findtext('IntOffsetCorr_1_HF', default='0')),
            IntOffsetCorr_1_LF=float(adb_elem.findtext('IntOffsetCorr_1_LF', default='0')),
            IntOffsetCorr_2_HF=float(adb_elem.findtext('IntOffsetCorr_2_HF', default='0')),
            IntOffsetCorr_2_LF=float(adb_elem.findtext('IntOffsetCorr_2_LF', default='0')),
            LF_LSB=float(adb_elem.findtext('LF_LSB', default='0')),
            MaxAmplitude=float(adb_elem.findtext('MaxAmplitude', default='0')),
            Message=adb_elem.findtext('Message', default='---'),
            NoiseCheckHF=float(adb_elem.findtext('NoiseCheckHF', default='0')),
            NoiseCheckLF=float(adb_elem.findtext('NoiseCheckLF', default='0')),
            RFCheckStrong=float(adb_elem.findtext('RFCheckStrong', default='0')),
            RFCheckWeak=float(adb_elem.findtext('RFCheckWeak', default='0')),
            Resistivity=float(adb_elem.findtext('Resistivity', default='0')),
            Resistivity_connN_or_connE_connGND=float(adb_elem.findtext('Resistivity_connN_or_connE_connGND', default='0')),
            Resistivity_connS_or_connW_connGND=float(adb_elem.findtext('Resistivity_connS_or_connW_connGND', default='0')),
            Severity=adb_elem.findtext('Severity', default='OK'),
            id=int(channel_id)
          )
          
          boards.append(board_data)
  # we we have boards, we parse the channel configuration
  if boards:
    parse_channel_config(xml_file, boards, debug)
    parse_atswriter_configuration(xml_file, boards, debug)
  return boards

def parse_channel_config(xml_file, boards, debug=False):
  """Parse the channel configuration from the XML file and update the boards"""
  if debug:
    print(f"Parsing channel configuration from XML file: {xml_file}")
  try:
    tree = ET.parse(xml_file)
    root = tree.getroot()
    
    for board in boards:
      channel_config = root.find(f'.//channel_config/channel[@id="{board.id}"]')
      if channel_config is not None:
        if debug:
          print(f"Processing board -> {board.id}")
        board.gain_stage1 = int(channel_config.findtext('gain_stage1', default='0'))
        board.gain_stage2 = int(channel_config.findtext('gain_stage2', default='0'))
        board.ext_gain = int(channel_config.findtext('ext_gain', default='0'))
        board.filter_type = channel_config.findtext('filter_type', default='N/A')
        board.hchopper = int(channel_config.findtext('hchopper', default='0'))
        board.echopper = int(channel_config.findtext('echopper', default='0'))
        board.dac_val = int(channel_config.findtext('dac_val', default='0'))
        board.dac_on = int(channel_config.findtext('dac_on', default='0'))
        board.input = int(channel_config.findtext('input', default='0'))
        board.direct_mode = int(channel_config.findtext('direct_mode', default='0'))
        board.input_divider = int(channel_config.findtext('input_divider', default='0'))        
        if debug:
          print(f"Updated board {board.id} with channel configuration")
  except ET.ParseError as e:
    print(f"XML parsing error: {e}")
  except Exception as e:
    print(f"Error: {e}")
  # No need to return boards, they are modified in place

def parse_atswriter_configuration(xml_file, boards, debug=False):
  """Parse the ATSWriter configuration from the XML file and update the boards"""
  if debug:
    print(f"Parsing ATSWriter configuration from XML file: {xml_file}")
  try:
    tree = ET.parse(xml_file)
    root = tree.getroot()
    
    for board in boards:
      ats_config = root.find(f'.//ATSWriter/configuration/channel[@id="{board.id}"]')
      if ats_config is not None:
        if debug:
          print(f"Processing board -> {board.id}")
        board.ts_lsb = float(ats_config.findtext('ts_lsb', default='0.0'))
        board.channel_type = ats_config.findtext('channel_type', default='N/A')
        board.sensor_sernum = int(ats_config.findtext('sensor_sernum', default='0'))
        board.sensor_type = ats_config.findtext('sensor_type', default='N/A')
        if debug:
          print(f"Updated board {board.id} with ATSWriter configuration")
  except ET.ParseError as e:
    print(f"XML parsing error: {e}")
  except Exception as e:
    print(f"Error: {e}")
  # No need to return boards, they are modified in place

def likely_off(boards, debug=False):
  # if IntOffsetCorr_1_HF, IntOffsetCorr_1_LF, IntOffsetCorr_2_HF, IntOffsetCorr_2_LF, DCOffset = 0.0, we remove the board from the list
  for board in boards:
    if (board.IntOffsetCorr_1_HF == 0.0 and board.IntOffsetCorr_1_LF == 0.0 and
        board.IntOffsetCorr_2_HF == 0.0 and board.IntOffsetCorr_2_LF == 0.0 and
        board.DCOffset == 0.0):
      boards.remove(board)
      if debug:
        # Debug: print the board being removed
        print(f"Board {board.id} likely off, removing from list")
  return boards
#
# now we take care of standard messages, not special messages
# a standard message has exactly this format:
    # <message>
    #   <component>MCP</component>
    #   <date>2025-07-10</date>
    #   <priority>info</priority>
    #   <text>TEXT</text>
    #   <time>14:51:01</time>
    # </message>
  # messages with more entries are special messages, we ignore them

@dataclass
class StandardMessage:
  component: str
  date: str
  priority: str
  text: str
  time: str

def extract_standard_messages(xml_file, debug=False):
  # we look in <SystemHistory> and <message> elements
  try:
    tree = ET.parse(xml_file)
    root = tree.getroot()
    # GOT TO find <SystemHistory>
    system_history = root.find('.//SystemHistory')
    if system_history is None:
      print("No SystemHistory section found")
      return None
    else:
      print("Found SystemHistory section")
    # use the StandardMessage dataclass to store messages
    messages = []
    for message in system_history.findall('message'):
      if (debug):
        # Debug: print the message being processed
        print(f"Processing message: {ET.tostring(message, encoding='unicode')}")
      # check if the message has exactly 5 children: component, date, priority, text, time
      if len(message) == 5:
        component = message.find('component').text if message.find('component') is not None else 'N/A'
        date = message.find('date').text if message.find('date') is not None else 'N/A'
        priority = message.find('priority').text if message.find('priority') is not None else 'N/A'
        text = message.find('text').text if message.find('text') is not None else 'N/A'
        time = message.find('time').text if message.find('time') is not None else 'N/A'
        
        msg = StandardMessage(component=component, date=date, priority=priority, text=text, time=time)
        messages.append(msg)
  except ET.ParseError as e:
    print(f"XML parsing error: {e}")
    return None    
  return messages

# we make a selftest table; rows are the values, columns are the boards
def boards_to_selftest_table(boards, debug=False):
  """Convert the list of AdbBoardData to a selftest table format"""
  if not boards:
    print("No boards found")
    return None
  
  # Create a list of dictionaries for each board
  table_data = []
  for board in boards:
    board_data = {
      'ID': board.id,
      'Type': board.channel_type,
      'Sensor': board.sensor_type,
      'Serial': board.sensor_sernum,
      'AdbError': board.AdbError,
      'Atten_HF_HP_500Hz': board.Atten_HF_HP_500Hz,
      'Atten_LF_LP_4Hz': board.Atten_LF_LP_4Hz,
      'DCOffset': board.DCOffset,
      'ExtOffsetComp': board.ExtOffsetComp,
      'GainAmpl': board.GainAmpl,
      'GainCheckHF16': board.GainCheckHF16,
      'GainCheckHF4': board.GainCheckHF4,
      'GainCheckHF8': board.GainCheckHF8,
      'GainCheckLF16': board.GainCheckLF16,
      'GainCheckLF4': board.GainCheckLF4,
      'GainCheckLF8': board.GainCheckLF8,
      'GainCorr': board.GainCorr,
      'HF_LSB': board.HF_LSB,
      'InputDivCheckOff': board.InputDivCheckOff,
      'InputDivCheckOn': board.InputDivCheckOn,
      'IntOffsetCorr_1_HF': board.IntOffsetCorr_1_HF,
      'IntOffsetCorr_1_LF': board.IntOffsetCorr_1_LF,
      'IntOffsetCorr_2_HF': board.IntOffsetCorr_2_HF,
      'IntOffsetCorr_2_LF': board.IntOffsetCorr_2_LF,
      'LF_LSB': board.LF_LSB,
      'MaxAmplitude': board.MaxAmplitude,
      # 'Message': board.Message,
      'NoiseCheckHF': board.NoiseCheckHF,
      'NoiseCheckLF': board.NoiseCheckLF,
      'RFCheckStrong': board.RFCheckStrong,
      'RFCheckWeak': board.RFCheckWeak,
      'Resistivity': board.Resistivity,
      'Resistivity_connN_or_connE_connGND': board.Resistivity_connN_or_connE_connGND,
      'Resistivity_connS_or_connW_connGND': board.Resistivity_connS_or_connW_connGND,
      'Severity': board.Severity
    }
  # make the table now
    table_data.append(board_data)
  # Convert to DataFrame for better visualization
  import pandas as pd
  df = pd.DataFrame(table_data)
  df = df.transpose()
  if debug:
    print("=== Selftest Table Data ===")
    print(df.to_string(index=False))
  return df

# ============================================================================
# Error Handling and Utility Functions
# ============================================================================

def AdbError_to_text(error_code):
  # load the error codes from adu_error_msg_definitions.sql3
  if error_code is None or error_code == 0:
    return "No error"
  import sqlite3
  error_db_file = './lib/adu_error_msg_definitions.sql3'
  # table error_messages has columns:
  # Component, MainIndex, SubIndex, Priority, Message, DisplayMsg., Description
  # we need mainindex == 9, subindex == error_code, we select Message and Description
  try:
    conn = sqlite3.connect(error_db_file)
    cursor = conn.cursor()
    
    query = """
      SELECT Message, Description
      FROM error_messages
      WHERE MainIndex = 9 AND SubIndex = ?
    """
    cursor.execute(query, (error_code,))
    result = cursor.fetchone()
    
    if result:
      message, description = result
      return f"{message} : {description}"
    else:
      return f"Unknown error code: {error_code}"
  except sqlite3.Error as e:
    print(f"SQLite error: {e}")
    return f"Error retrieving message for code {error_code}"
  finally:
    if conn:
      conn.close()  

def print_adb_error_messages(boards, debug=False):
  """Print the AdbError messages for each board"""
  if not boards:
    print("No boards found")
    return
  # prepare output for html table and return
  error_messages = []
  for board in boards:
    if board.AdbError is not None:
      error_text = AdbError_to_text(board.AdbError)
      error_messages.append({
        'Board ID': board.id,
        'AdbError': board.AdbError,
        'Message': error_text
      })
      if debug:
        print(f"Board {board.id} - AdbError: {error_text}")
  # convert to DataFrame for better visualization
  import pandas as pd
  error_df = pd.DataFrame(error_messages)
  if debug:
    print("=== AdbError Messages ===")
    print(error_df.to_string(index=False))
  # return the DataFrame
  if error_df.empty:
    print("No AdbError messages found")
    return None
  else:
    # we return the DataFrame, so it can be used in the styled table
    return error_df    

# ============================================================================
# HTML Report Generation Functions
# ============================================================================

def create_styled_table(data_frame, title, debug=False):
    """Create a styled HTML table with color coding for severity and error codes (channels as columns, keywords as rows)"""
    # we make a deep copy of the dataframe to avoid modifying the original
    dataframe = data_frame.copy()
    # ok the display is ugly.
    # if we have a float which is 0.0, we want to display it as 0, not 0.0
    dataframe = dataframe.map(lambda x: 0 if isinstance(x, float) and x == 0.0 else x)
    # if we have a float which is not 0
    # I want float in scientific notation, not in decimal notation, if absolute value is greater than 1E+5 or less than 1E-4
    dataframe = dataframe.map(lambda x: f"{x:.6e}" if isinstance(x, float) and (abs(x) > 1E+5 or abs(x) < 1E-4) else x)
    # remove trailing zeros from floats
    dataframe = dataframe.map(lambda x: f"{x:.6f}".rstrip('0').rstrip('.') if isinstance(x, float) else x)
    # dataframe = dataframe.map(lambda x: f"{x:.6e}" if isinstance(x, float) else x)

    # e_channels = [0, 1, 5, 6]  # Default for 5 channel ADU-08e, ADU-07e in LF mode
    # h_channels = [2, 3, 4, 7, 8, 9]  # Default for 10 channel ADU-08e, ADU-07e in HF mode
    def color_severity(val):
        if val == 'OK':
            return 'background-color: #d4edda; color: #155724'
        elif val == 'Critical':
            return 'background-color: #f8d7da; color: #721c24'
        elif val == 'N/A':
            return 'background-color: #f8f9fa; color: #6c757d'
        else:
            return 'background-color: #fff3cd; color: #856404'

    def color_error(val):
        if val == '0' or val == 0 or val == 0.0:
            return 'background-color: #d4edda; color: #155724'
        elif val == 'N/A' or pd.isna(val):
            return 'background-color: #f8f9fa; color: #6c757d'
        else:
            return 'background-color: #f8d7da; color: #721c24'

    def color_resistivity(val, channel_type):
        """Color resistivity values for E channels based on thresholds"""
        if not (isinstance(channel_type, str) and channel_type.startswith('E')):
            return ''
        if val == 'N/A' or pd.isna(val):
            return 'background-color: #f8f9fa; color: #6c757d'
        try:
            resist_val = float(val)
            if resist_val < 1200:
                return 'background-color: #d4edda; color: #155724'  # Green
            elif resist_val < 3200:
                return 'background-color: #fff3cd; color: #856404'  # Yellow
            else:
                return 'background-color: #fd7e14; color: #ffffff'  # Orange
        except (ValueError, TypeError):
            return ''

    styled_df = dataframe.style.set_caption(title).set_table_styles([
        {'selector': 'caption',
         'props': [('font-size', '14px'),
                   ('font-weight', 'bold'),
                   ('text-align', 'center'),
                   ('margin-bottom', '10px')]},
        {'selector': 'th',
         'props': [('background-color', '#f0f0f0'), ('font-weight', 'bold')]},
        {'selector': 'td',
         'props': [('text-align', 'left'), ('font-size', '12px'), ('border', '1px solid #ddd')]}
    ])

    # Apply styling to the entire row for Severity and AdbError
    if 'Severity' in dataframe.index:
        styled_df = styled_df.map(color_severity, subset=pd.IndexSlice['Severity', :])
    if 'AdbError' in dataframe.index:
        if debug:
            print("Applying color coding for AdbError...")
        styled_df = styled_df.map(color_error, subset=pd.IndexSlice['AdbError', :])
    
        # Apply resistivity color coding for E channels
    if 'Resistivity' in dataframe.index and 'Type' in dataframe.index:
        if debug: 
            print("Applying color coding for Resistivity...")
        for col in dataframe.columns:
            channel_type = dataframe.at['Type', col]
            if debug:
                print(f"Applying resistivity color coding for column: {col} (Type: {channel_type})")
            styled_df = styled_df.map(
                (lambda channel_type: (lambda val: color_resistivity(val, channel_type)))(channel_type),
                subset=pd.IndexSlice['Resistivity', col]
            )

    return styled_df

def summary_html(styled_table, boards, title, debug=False):
    """Generate HTML summary with the styled table"""
    if debug:
        print("Generating HTML summary...")
    html = f"""
    <html>
    <head>
        <title>{title}</title>
        <style>
            body {{
                font-family: Arial, sans-serif;
                margin: 20px;
                background-color: #fafafa;
            }}
            h1 {{
                text-align: center;
                color: #333;
            }}
            .styled-table-container {{
                margin: 20px 0;
                overflow-x: auto;
                box-shadow: 0 2px 4px rgba(0,0,0,0.1);
                border-radius: 4px;
            }}
            table {{
                border-collapse: collapse;
                width: 100%;
                background-color: white;
            }}
            thead th {{
                background-color: #4a4a4a;
                color: #0066cc;
                padding: 12px;
                text-align: center;
                font-weight: bold;
                border: 1px solid #999;
            }}
            .row_heading {{
                text-align: right;
                background-color: #f5f5f5;
                font-weight: bold;
            }}
            tbody td {{
                padding: 10px 12px;
                border: 1px solid #ddd;
            }}
            tbody td:first-child {{
                text-align: left;
                font-weight: bold;
            }}
            tbody tr:nth-child(odd) {{
                background-color: #f9f9f9;
            }}
            tbody tr:nth-child(even) {{
                background-color: #efefef;
            }}
            tbody tr:hover {{
                background-color: #e8f4f8;
                transition: background-color 0.2s;
            }}
            caption {{
                font-size: 16px;
                font-weight: bold;
                text-align: right;
                margin-bottom: 10px;
                padding: 10px 0;
            }}
            .timestamp {{
                text-align: center;
                color: #666;
                font-size: 12px;
                margin-top: 20px;
            }}
        </style>
    </head>
    <body>
        <h1>Selftest Result</h1>
        <div class="styled-table-container">
            {styled_table.to_html()}
        </div>
        <div class="timestamp">
            <p>Generated on: {pd.Timestamp.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        </div>
        <div style="margin-top: 30px;">
            {print_adb_error_messages_as_html(boards, debug)}
        </div>
    </body>
    </html>
    """
    return html

def print_adb_error_messages_as_html(boards, debug=False):
  """Print the AdbError messages for each board as HTML"""
  error_df = print_adb_error_messages(boards, debug)
  if error_df is not None:
    html = f"""
    <style>
    .adb-error-table {{
      border-collapse: collapse;
      width: 70%;
      font-family: Arial, sans-serif;
    }}
    .adb-error-table th, .adb-error-table td {{
      border: 1px solid #ddd;
      padding: 8px;
      text-align: left;
      font-family: Arial, sans-serif;
    }}
    .adb-error-table th {{
      background-color: #f2f2f2;
      font-weight: bold;
    }}
    .adb-error-table tr:nth-child(even) {{
      background-color: #fafafa;
    }}
    .adb-error-table tr:hover {{
      background-color: #f1f7ff;
    }}
    </style>
    <table class="adb-error-table">
      <thead>
      <tr>
      <th>Board ID</th>
      <th>AdbError</th>
      <th>Message</th>
      </tr>
      </thead>
      <tbody>
    """
    for _, row in error_df.iterrows():
      html += f"""
      <tr>
      <td>{row['Board ID']}</td>
      <td>{row['AdbError']}</td>
      <td>{row['Message']}</td>
      </tr>
      """
    html += """
      </tbody>
    </table>
    """
    return html
  else:
    return "<p>No AdbError messages found</p>"

# ============================================================================
# Database Operations
# ============================================================================

def extract_log_messages(log_db_file, error_msg_db_file):
  """Extract log messages and their corresponding error descriptions"""
  try:
    # Connect to the SQLite databases
    log_conn = sqlite3.connect(log_db_file)
    error_msg_conn = sqlite3.connect(error_msg_db_file)

    log_cursor = log_conn.cursor()
    error_msg_cursor = error_msg_conn.cursor()

    # Query to get log messages
    log_query = """
      SELECT timestamp, priority, component, mainindex, subindex, message
      FROM log
    """
    log_cursor.execute(log_query)
    logs = log_cursor.fetchall()

    # Prepare a list to hold the final results
    results = []

    for log in logs:
      timestamp, priority, component, mainindex, subindex, message = log

      # Query to find the corresponding error message
      error_query = """
        SELECT Message, Description
        FROM error_messages
        WHERE Component = ? AND MainIndex = ? AND SubIndex = ?
      """
      error_msg_cursor.execute(error_query, (component, mainindex, subindex))
      error_msg = error_msg_cursor.fetchone()

      if error_msg:
        msg_text, description = error_msg
      else:
        msg_text, description = 'N/A', 'N/A'

      results.append({
        'timestamp': timestamp,
        'priority': priority,
        'component': component,
        'mainindex': mainindex,
        'subindex': subindex,
        'message': msg_text,
        'description': description
      })

    return results

  except sqlite3.Error as e:
    print(f"SQLite error: {e}")
    return None
  finally:
    if 'log_conn' in locals():
      log_conn.close()
    if 'error_msg_conn' in locals():
      error_msg_conn.close()

def write_html_table_logs(results, output_file):
  """Write the results to an HTML table file."""
  with open(output_file, 'w', encoding='utf-8') as f:
    f.write('<!DOCTYPE html>\n<html>\n<head>\n<meta charset="UTF-8">\n')
    f.write('<title>Log Messages</title>\n')
    f.write('''<style>
      body {
        font-family: Arial, sans-serif;
        margin: 20px;
        background-color: #fafafa;
      }
      h1 {
        text-align: center;
        color: #333;
      }
      .table-container {
        margin: 20px 0;
        overflow-x: auto;
        box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        border-radius: 4px;
      }
      table {
        border-collapse: collapse;
        width: 100%;
        background-color: white;
      }
      thead th {
        background-color: #d0d0d0;
        color: #0066cc;
        padding: 12px;
        text-align: center;
        font-weight: bold;
        border: 1px solid #999;
      }
      tbody td {
        padding: 10px 12px;
        border: 1px solid #ddd;
      }
      tbody td:first-child {
        text-align: left;
        font-weight: bold;
      }
      tbody tr:nth-child(odd) {
        background-color: #f9f9f9;
      }
      tbody tr:nth-child(even) {
        background-color: #efefef;
      }
      tbody tr:hover {
        background-color: #e8f4f8;
        transition: background-color 0.2s;
      }
      .priority-exception {
        background-color: #f8d7da;
        color: #721c24;
        font-weight: bold;
      }
      .priority-warning {
        background-color: #fff3cd;
        color: #856404;
        font-weight: bold;
      }
      .timestamp {
        text-align: center;
        color: #666;
        font-size: 12px;
        margin-top: 20px;
      }
    </style>\n''')
    f.write('</head>\n<body>\n')
    f.write('<h1>Log Messages</h1>\n')
    f.write('<div class="table-container">\n')
    f.write('<table>\n')
    f.write('<thead><tr><th>Timestamp</th><th>Priority</th><th>Component</th><th>MainIndex</th><th>SubIndex</th><th>Message</th><th>Description</th></tr></thead>\n')
    f.write('<tbody>\n')
    for row in results:
      f.write('<tr>')
      f.write(f'<td>{row["timestamp"]}</td>')
      # Color code priority: exception=red, warning=orange, else default
      priority = row["priority"].lower()
      if priority == "exception":
        f.write(f'<td class="priority-exception">{row["priority"]}</td>')
      elif priority == "warning":
        f.write(f'<td class="priority-warning">{row["priority"]}</td>')
      else:
        f.write(f'<td>{row["priority"]}</td>')
      f.write(f'<td>{row["component"]}</td>')
      f.write(f'<td>{row["mainindex"]}</td>')
      f.write(f'<td>{row["subindex"]}</td>')
      f.write(f'<td>{row["message"]}</td>')
      f.write(f'<td>{row["description"]}</td>')
      f.write('</tr>\n')
    f.write('</tbody>\n')
    f.write('</table>\n</div>\n')
    f.write('<div class="timestamp">\n')
    f.write(f'<p>Generated on: {pd.Timestamp.now().strftime("%Y-%m-%d %H:%M:%S")}</p>\n')
    f.write('</div>\n')
    f.write('</body>\n</html>\n')

def mysql_to_sqlite(mysql_dump_file, sqlite_db_file):
  """Convert MySQL dump file to SQLite database"""
  # Read MySQL dump
  with open(mysql_dump_file, 'r', encoding='utf-8') as f:
    lines = f.readlines()

  # Prepare SQLite connection
  if os.path.exists(sqlite_db_file):
    os.remove(sqlite_db_file)
  conn = sqlite3.connect(sqlite_db_file)
  cur = conn.cursor()

  # Regex patterns for conversion
  create_table_re = re.compile(r'^CREATE TABLE `(.*?)` \((.*)', re.S)
  insert_re = re.compile(r'^INSERT INTO `(.*?)` VALUES (.*);', re.S)

  buffer = ""
  in_create = False
  do_nothing = False
  for line in lines:
    line = line.strip()
    # debug, print the first 20 characters of each line
    # print(line[:20])
    if not line or line.startswith('--') or line.startswith('/*'):
      continue

    # we jump over DROP TABLE IF EXISTS `waveform`; and DROP TABLE IF EXISTS `sequence`;
    if line.startswith('DROP TABLE IF EXISTS `waveform`;') or line.startswith('DROP TABLE IF EXISTS `sequence`;'):
      # and skip until UNLOCK TABLES; is reached
      do_nothing = True
      continue
    if do_nothing:
      if line.startswith('UNLOCK TABLES;'):
        do_nothing = False
      continue

    # Handle CREATE TABLE
    if line.startswith('CREATE TABLE'):
      buffer = line
      in_create = True
      continue
    if in_create:
      buffer += " " + line
      # Detect end of CREATE TABLE by ') ENGINE=' or ');'
      if line.endswith(');') or re.search(r'\)\s*ENGINE=', line):
        # Convert MySQL types to SQLite types
        stmt = buffer
        stmt = stmt.replace('`', '"')
        stmt = re.sub(r' int\([0-9]+\)', ' INTEGER', stmt)
        stmt = re.sub(r' AUTO_INCREMENT', '', stmt)
        stmt = re.sub(r' ENGINE=.*', '', stmt)
        stmt = re.sub(r' unsigned', '', stmt)
        stmt = re.sub(r' CHARACTER SET [^ ]+', '', stmt)
        stmt = re.sub(r' COLLATE [^ ]+', '', stmt)
        stmt = re.sub(r' DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP', '', stmt)
        stmt = re.sub(r' COMMENT \'.*?\'', '', stmt)
        stmt = re.sub(r'\\n', '', stmt)
        stmt = re.sub(r'(?i)tinyint\(1\)', 'BOOLEAN', stmt)
        stmt = re.sub(r'(?i)double', 'REAL', stmt)
        stmt = re.sub(r'(?i)datetime', 'TEXT', stmt)
        stmt = re.sub(r'(?i)text', 'TEXT', stmt)
        stmt = re.sub(r'(?i)blob', 'BLOB', stmt)
        stmt = re.sub(r'(?i)enum\([^)]+\)', 'TEXT', stmt)
        stmt = re.sub(r'(?i)SET\([^)]+\)', 'TEXT', stmt)
        stmt = re.sub(r'(?i)int\([0-9]+\)', 'INTEGER', stmt)
        stmt = re.sub(r'(?i)bigint\([0-9]+\)', 'INTEGER', stmt)
        stmt = re.sub(r'(?i)float\([0-9,]+\)', 'REAL', stmt)
        stmt = re.sub(r'(?i)double\([0-9,]+\)', 'REAL', stmt)
        stmt = re.sub(r'(?i)varchar\([0-9]+\)', 'TEXT', stmt)
        stmt = re.sub(r'(?i)char\([0-9]+\)', 'TEXT', stmt)
        stmt = re.sub(r'(?i)DEFAULT NULL', '', stmt)
        stmt = re.sub(r'(?i)unsigned', '', stmt)
        stmt = re.sub(r'(?i)ZEROFILL', '', stmt)
        stmt = re.sub(r'(?i)ON UPDATE [^,]+', '', stmt)
        stmt = re.sub(r',\s*\)', ')', stmt)
        try:
          cur.execute(stmt)
        except Exception as e:
          print(f"Error executing: {stmt}\n{e}")
        buffer = ""
        in_create = False
      continue

    # Handle INSERT INTO
    if line.startswith('INSERT INTO'):
      stmt = line.replace('`', '"')
      try:
        cur.executescript(stmt)
      except Exception as e:
        print(f"Error executing: {stmt}\n{e}")
      continue

  conn.commit()
  conn.close()
  print(f"Converted {mysql_dump_file} to {sqlite_db_file}")

