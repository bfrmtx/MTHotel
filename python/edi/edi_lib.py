# library for handling EDI files (electrical data interchange)
# we have different entries. First ones are like JSON / dictionary entries, call them meta_blocks
# >HEAD, >INFO, >=DEFINEMEAS, >EMEAS (to be written as a SINGLE LINE), >HMEAS (to be written as a SINGLE LINE) and >=MTSECT 
# then we have the data blocks starting with
# they start with > followed by block name, e.g. FREQ, ZXXR, TXXR, etc. FOLLOWED by optional parameters like ROT=ZROT or ROT=TROT (no spaces) followed by necessary //NN where NN is the number of frequencies, call them data_blocks
# the //NN appears as NFREQ=NN in the >=MTSECT block
#
# we have functions to remove ROT=ZROT or ROT=TROT from the block headers
# and to remove the entire >ZROT or >TROT blocks
# and to remove one or more frequencies from the EDI file, which requires updating all data blocks
#
# hence >END the end of the file, not a block
import os
import sys
# 
class meta_block:
    # prefix '>' or '>='
    #name: str like HEAD, INFO, DEFINEMEAS, EMEAS, HMEAS, MTSECT
    #data: dictionary of key-value pairs; they are strings and created when reading the EDI file
    # we access the later like meta_block.data['ID'] to get the value of ID, so LAT is meta_block.data['LAT'] and value is string like 39:01:34.308
    # when writing EDI file, we write the block as
    # >HEAD (or >=HEAD)
    # and key value pairs as KEY=VALUE line by line, OR if EMEAS or HMEAS, all in a single line
    def __init__(self, prefix, name):
        self.prefix = prefix
        self.name = name
        self.data = {}
#
def write_edi_meta_block(filename, block, n_frequencies=None):
    """
    This function writes a meta block object to an EDI (Electromagnetic Data Interchange) file.
    It handles different block types with specialized formatting:
    - MTSECT blocks: Optionally includes NFREQ parameter
    - EMEAS and HMEAS blocks: Formats key-value pairs on a single line separated by spaces
    - Other blocks: Formats key-value pairs on separate lines
        filename (str): Path to the EDI file to write to.
        block (object): Meta block object containing prefix, name, and data attributes.
            - prefix (str): Prefix string for the block header
            - name (str): Block name (e.g., 'MTSECT', 'EMEAS', 'HMEAS')
            - data (dict): Dictionary of key-value pairs to write
        n_frequencies (int, optional): Number of frequencies. Only used for MTSECT blocks.
            If provided and block name is 'MTSECT', sets NFREQ in the block data.
            Defaults to None.
    Returns:
        None
    Raises:
        IOError: If the file cannot be opened or written to.    
    Args:
        filename: Path to the EDI file
        block: meta_block object to write
    """
    with open(filename, 'w') as f:
        f.write(f"{block.prefix}{block.name}\n")
        if n_frequencies is not None and block.name == 'MTSECT':
            # Special handling for MTSECT to include NFREQ
            block.data['NFREQ'] = str(n_frequencies)
        if block.name in ['EMEAS', 'HMEAS']:
            line = '  '.join([f"{k}={v}" for k, v in block.data.items()])
            f.write(f"{line}\n")
        else:
            for key, value in block.data.items():
                f.write(f"{key}={value}\n")
        
        f.write("\n")
#
def write_edi_all_meta_blocks(filename, meta_blocks, n_frequencies=None):
    """
    This function writes multiple meta block objects to an EDI (Electromagnetic Data Interchange) file.
    It handles different block types with specialized formatting:
    - MTSECT blocks: Optionally includes NFREQ parameter
    - EMEAS and HMEAS blocks: Formats key-value pairs on a single line separated by spaces
    - Other blocks: Formats key-value pairs on separate lines
    
    EMEAS and HMEAS blocks are written consecutively without blank lines between them.
    
    Args:
        filename (str): Path to the EDI file to write to.
        meta_blocks (list): List of meta block objects to write.
            Each object should contain:
            - prefix (str): Prefix string for the block header
            - name (str): Block name (e.g., 'MTSECT', 'EMEAS', 'HMEAS')
            - data (dict): Dictionary of key-value pairs to write
        n_frequencies (int, optional): Number of frequencies. Only used for MTSECT blocks.
            If provided and a block name is 'MTSECT', sets NFREQ in the block data.
            Defaults to None.
    Returns:
        None
    Raises:
        IOError: If the file cannot be opened or written to.
    """
    with open(filename, 'w') as f:
        for i, block in enumerate(meta_blocks):
            if n_frequencies is not None and block.name == 'MTSECT':
                block.data['NFREQ'] = str(n_frequencies)
            
            if block.name in ['EMEAS', 'HMEAS']:
                f.write(f"{block.prefix}{block.name} ")
                line = ' '.join([f"{k}={v}" for k, v in block.data.items()])
                f.write(f"{line}\n")
            else:
                f.write(f"{block.prefix}{block.name}\n")
                for key, value in block.data.items():
                    f.write(f"  {key}={value}\n")
            
            # Check if we should write a blank line
            # Don't write blank line after EMEAS/HMEAS if next block is also EMEAS/HMEAS
            should_write_blank = True
            if block.name in ['EMEAS', 'HMEAS']:
                # Check if next block exists and is also EMEAS/HMEAS
                if i + 1 < len(meta_blocks) and meta_blocks[i + 1].name in ['EMEAS', 'HMEAS']:
                    should_write_blank = False
            
            if should_write_blank:
                f.write("\n")
#
#
# a meta block starts with > or >= followed by block name BUT does NOT contain //NN at the end!
# exclude >END
# I want to get ALL meta blocks; call parse_edi_meta_block(block_name) internally
# return and array of classes like HEAD, INFO, MTSECT ...
def parse_edi_all_meta_blocks(filename):
    """
    This function reads an EDI (EDI) file and extracts all meta blocks, which are
    sections that begin with '>' or '>=' and do not contain '//' followed by numbers
    at the end (these indicate data blocks, not meta blocks).
    Each meta block consists of:
    - A block header line (e.g., '>HEAD', '>=DEFINEMEAS')
    - Optional KEY=VALUE pairs on subsequent lines
    Args:
        filename (str): Path to the EDI file to parse.
    Returns:
        list: A list of meta_block objects, where each object contains:
            - prefix (str): Either '>' or '>=' indicating the block type
            - name (str): The name of the meta block (e.g., 'HEAD', 'DEFINEMEAS')
            - data (dict): A dictionary of KEY=VALUE pairs parsed from the block
    Raises:
        FileNotFoundError: If the specified file does not exist.
        IOError: If the file cannot be read.
    Note:
        - Blocks starting with '>END' are excluded from parsing.
        - Empty lines and lines starting with '>' within a block are treated as
          block terminators.
        - The meta_block class must be defined elsewhere in the codebase.
    """

    meta_blocks = []
    with open(filename, 'r') as f:
        current_block = None    
        for line in f:
            line = line.rstrip()
            # Print each line while reading
            # print(line)
            
            # Check if this is a block header (starts with > or >=)
            if (line.startswith('>') or line.startswith('>=')) and not line.startswith('>END'):
                # check if this is not a meta block (i.e., does not contain //NN)
                if '//' not in line:
                    parts = line.split(None, 1)  # Split on first whitespace only
                    prefix = '>=' if parts[0].startswith('>=') else '>'
                    name = parts[0][len(prefix):]  # Remove leading '>' or '>='
                    if current_block:
                        meta_blocks.append(current_block)
                    current_block = meta_block(prefix, name)
                    
                    # For EMEAS and HMEAS, parse KEY=VALUE pairs from the same line
                    if current_block.name in ['EMEAS', 'HMEAS'] and len(parts) > 1:
                        pairs = parts[1].split()
                        for pair in pairs:
                            if '=' in pair:
                                key, value = pair.split('=', 1)
                                current_block.data[key.strip()] = value.strip()
                        continue
            elif current_block and line.strip() and not line.startswith('>'):
                # Parse metadata lines (KEY=VALUE format)
                # Handle EMEAS and HMEAS which are single-line space-separated KEY=VALUE pairs
                if current_block.name in ['EMEAS', 'HMEAS']:
                    pairs = line.split()
                    for pair in pairs:
                        if '=' in pair:
                            key, value = pair.split('=', 1)
                            current_block.data[key.strip()] = value.strip()
                else:
                    # Handle regular meta blocks with KEY=VALUE format
                    if '=' in line:
                        key, value = line.split('=', 1)
                        current_block.data[key.strip()] = value.strip()
        # Don't forget the last block
        if current_block:
            meta_blocks.append(current_block)
    return meta_blocks

# class data_block
# The block starts with >FREQ, >ZXXR, >ZYYI, etc., always followed by //NN at the end
# CRITICAL: All data blocks store paired data as (frequency, value):
#   - >FREQ block: stores (f, f) pairs where frequency is both key and value
#   - All other blocks: store (f, data) pairs where f comes from >FREQ
# This is why >FREQ MUST be read first before parsing any other data blocks!
# All blocks must have the same number of data points, otherwise EDI file is CORRUPTED
class data_block:
    # prefix: Always '>' for data blocks
    # name: Block name like FREQ, ZXXR, ZXXI, TXXR, TXXI, etc.
    # parameters: Optional dict of parameters like ROT=ZROT, MEAS1=xxx, MEAS2=xxx
    # n_frequencies: Number of frequency points (from //NN)
    # paired_data: List of (frequency, value) tuples
    #              For FREQ block: [(f1,f1), (f2,f2), ...]
    #              For other blocks: [(f1,val1), (f2,val2), ...]
    
    def __init__(self, prefix, name, parameters=None, n_frequencies=None):
        self.prefix = prefix
        self.name = name
        self.parameters = parameters if parameters else {}
        self.n_frequencies = n_frequencies
        self.paired_data = []  # List of (frequency, value) tuples
    
    def set_paired_data(self, frequencies, values):
        """
        Set paired data from frequency and value lists.
        
        Args:
            frequencies: List of frequency values (from FREQ block)
            values: List of data values (from this block)
            
        Raises:
            ValueError: If lengths don't match
        """
        if len(frequencies) != len(values):
            raise ValueError(f"Data corruption: {self.name} has {len(values)} values but expected {len(frequencies)} frequencies")
        
        self.paired_data = list(zip(frequencies, values))
        self.n_frequencies = len(self.paired_data)
    
    def get_frequencies(self):
        """Get list of all frequencies."""
        return [pair[0] for pair in self.paired_data]
    
    def get_values(self):
        """Get list of all values."""
        return [pair[1] for pair in self.paired_data]
    
    def get_f(self, freq_index):
        """
        Get the (frequency, value) pair at a specific index.
        
        Args:
            freq_index: Index of the frequency
            
        Returns:
            Tuple (frequency, value) or None if index out of range
        """
        if 0 <= freq_index < len(self.paired_data):
            return self.paired_data[freq_index]
        return None
    
    def get_value_at_freq(self, freq_index):
        """
        Get just the value at a specific frequency index.
        
        Args:
            freq_index: Index of the frequency
            
        Returns:
            The value at that frequency index, or None if out of range
        """
        if 0 <= freq_index < len(self.paired_data):
            return self.paired_data[freq_index][1]
        return None
    
    def remove_f(self, frequencies_to_remove):
        """
        Remove frequency-value pairs at specified indices.

        Args:
            frequencies_to_remove: List of frequency values to remove (or single value)
        """
        if isinstance(frequencies_to_remove, (int, float)):
            frequencies_to_remove = [frequencies_to_remove]

        # Compare rounded floats to avoid tiny representation differences; match within 4 decimals.
        targets = {round(float(f), 4) for f in frequencies_to_remove}
        self.paired_data = [pair for pair in self.paired_data if round(float(pair[0]), 4) not in targets]

        self.n_frequencies = len(self.paired_data)
    def remove_zrot(self):
        """
        Remove ROT=ZROT parameter from the block header if present.
        """
        if 'ROT' in self.parameters and self.parameters['ROT'] == 'ZROT':
            del self.parameters['ROT']
    def remove_trot(self):
        """
        Remove ROT=TROT parameter from the block header if present.
        """
        if 'ROT' in self.parameters and self.parameters['ROT'] == 'TROT':
            del self.parameters['ROT']
def remove_zrot_block(data_blocks):
    """
    Remove the >ZROT data block from the list of data blocks if it exists.
    
    Args:
        data_blocks: List of data_block objects
    """
    data_blocks[:] = [block for block in data_blocks if block.name != 'ZROT']
    # call remove_zrot on all remaining blocks
    for block in data_blocks:
        block.remove_zrot()
def remove_trot_block(data_blocks):
    """
    Remove the >TROT data block from the list of data blocks if it exists.
    
    Args:
        data_blocks: List of data_block objects
    """
    data_blocks[:] = [block for block in data_blocks if block.name != 'TROT']
    # call remove_trot on all remaining blocks
    for block in data_blocks:
        block.remove_trot()
def remove_frequencies_from_data_blocks(data_blocks, frequencies_to_remove):
    """
    Remove specified frequencies from all data blocks.
    
    Args:
        data_blocks: List of data_block objects
        frequencies_to_remove: List of frequency values to remove
    """
    for block in data_blocks:
        block.remove_f(frequencies_to_remove)
#        
# CRITICAL: We MUST parse the >FREQ block FIRST to get frequency values
# All other data blocks depend on these frequencies to create (f, data) pairs

def parse_edi_freq_block(filename):
    """
    Parse the FREQ block from an EDI file to get frequency values.
    This MUST be called before parsing other data blocks!
    
    Args:
        filename: Path to the EDI file
        
    Returns:
        data_block object for FREQ block with paired_data as [(f1,f1), (f2,f2), ...]
        
    Raises:
        ValueError: If FREQ block is missing or malformed
    """
    with open(filename, 'r') as f:
        current_block = None
        raw_values = []
        reading_freq = False
        
        for line in f:
            line = line.rstrip()
            
            # Stop at >END
            if line.startswith('>END'):
                break
            
            # Check if this is the FREQ block header
            if line.startswith('>FREQ') and '//' in line:
                parts = line.split('//')
                n_frequencies = int(parts[1].strip())
                current_block = data_block('>', 'FREQ', n_frequencies=n_frequencies)
                reading_freq = True
                
            elif reading_freq and line.startswith('>'):
                # Hit next block, stop reading FREQ
                break
                
            elif reading_freq and line.strip():
                # Parse data lines - these contain only numerical values (space-separated)
                values = line.split()
                raw_values.extend(values)
        
        if not current_block:
            raise ValueError("FREQ block not found in EDI file!")
        
        # For FREQ block, paired_data is (f, f) - frequency as both key and value
        current_block.set_paired_data(raw_values, raw_values)
        
        return current_block        

def parse_edi_all_data_blocks(filename):
    """
    Parse all data blocks from an EDI file with STRICT validation.
    
    CRITICAL WORKFLOW:
    1. Parse >FREQ block FIRST to get frequency values
    2. Parse all other data blocks and pair their values with frequencies
    3. Validate that ALL blocks have the same number of data points
    4. Return list of data_block objects with paired (frequency, value) data
    
    Data blocks structure:
    - Start with '>' followed by block name (e.g., >FREQ, >ZXXR, >ZXYI)
    - Contain '//' followed by number of frequencies at the end of header
    - May have optional parameters in header (e.g., ROT=ZROT, MEAS1=xxx MEAS2=yyy)
    - Contain only numerical values on subsequent lines (NOT key=value pairs)
    
    Example header formats:
        >FREQ //48
        >ZXXR ROT=ZROT //48
        >COH MEAS1=1000.0001 MEAS2=1003.0001 ROT=ZROT //48
    
    Args:
        filename: Path to the EDI file
        
    Returns:
        List of data_block objects, each containing:
            - prefix: Always '>' for data blocks
            - name: Block name (FREQ, ZXXR, etc.)
            - parameters: Dict of optional parameters (ROT, MEAS1, MEAS2, etc.)
            - n_frequencies: Number of frequency points
            - paired_data: List of (frequency, value) tuples
            
    Raises:
        ValueError: If FREQ block is missing or if data counts are inconsistent
    """
    # STEP 1: Parse FREQ block FIRST - this is MANDATORY!
    freq_block = parse_edi_freq_block(filename)
    frequencies = freq_block.get_frequencies()
    expected_count = len(frequencies)
    
    print(f"✓ Parsed FREQ block: {expected_count} frequencies")
    
    # STEP 2: Parse all data blocks (including FREQ again for consistency)
    data_blocks = []
    raw_blocks = []  # Temporary storage: list of (block_obj, [raw_values]) tuples
    
    with open(filename, 'r') as f:
        current_block = None
        current_values = []
        
        for line in f:
            line = line.rstrip()
            
            # Stop at >END
            if line.startswith('>END'):
                # Save last block
                if current_block:
                    raw_blocks.append((current_block, current_values))
                break
            
            # Check if this is a data block header (starts with > and contains //NN)
            if line.startswith('>') and '//' in line:
                # Save the previous block if exists
                if current_block:
                    raw_blocks.append((current_block, current_values))
                
                # Parse header: >BLOCKNAME [PARAMS] //NN
                parts = line.split('//')
                header = parts[0].strip()
                n_frequencies = int(parts[1].strip())
                
                # Extract block name and optional parameters
                header_parts = header.split(None, 1)
                name = header_parts[0][1:]  # Remove leading '>'
                
                # Parse optional parameters (e.g., ROT=ZROT, MEAS1=xxx)
                parameters = {}
                if len(header_parts) > 1:
                    param_parts = header_parts[1].split()
                    for param in param_parts:
                        if '=' in param:
                            key, value = param.split('=', 1)
                            parameters[key.strip()] = value.strip()
                
                # Create new data block and reset values
                current_block = data_block('>', name, parameters, n_frequencies)
                current_values = []
                
            elif current_block and line.strip() and not line.startswith('>'):
                # Parse data lines - these contain only numerical values (space-separated)
                values = line.split()
                current_values.extend(values)
    
    # STEP 3: Create paired data and validate consistency
    # Add FREQ block first
    data_blocks.append(freq_block)
    
    # Process other blocks
    for block_obj, raw_values in raw_blocks:
        if block_obj.name == 'FREQ':
            continue  # Already added
        
        # Validate data count
        if len(raw_values) != expected_count:
            # Build parameter description for error message
            param_desc = f" with params {block_obj.parameters}" if block_obj.parameters else ""
            raise ValueError(
                f"EDI FILE CORRUPTED! Block {block_obj.name}{param_desc} has {len(raw_values)} values "
                f"but FREQ has {expected_count}. All blocks must have the same count!"
            )
        
        # Set paired data: (frequency from FREQ, value from this block)
        block_obj.set_paired_data(frequencies, raw_values)
        data_blocks.append(block_obj)
        
        # Build parameter description for logging
        param_desc = f" ({', '.join(f'{k}={v}' for k, v in block_obj.parameters.items())})" if block_obj.parameters else ""
        print(f"✓ Parsed {block_obj.name}{param_desc} block: {len(raw_values)} values paired with frequencies")
    
    print(f"\n✓ Successfully parsed {len(data_blocks)} data blocks (all validated)")
    return data_blocks
    
def write_edi_data_block(filename, block):
    """
    This function writes a data block object to an EDI (Electromagnetic Data Interchange) file.
    The data block consists of:
    - A block header line (e.g., '>FREQ //48', '>ZXXR ROT=ZROT //48')
    - Numerical values on subsequent lines, space-separated.
    
    Args:
        filename (str): Path to the EDI file to write to.
        block (object): Data block object containing:
            - prefix (str): Always '>' for data blocks
            - name (str): Block name (e.g., 'FREQ', 'ZXXR')
            - parameters (dict): Optional parameters (e.g., ROT=ZROT)
            - n_frequencies (int): Number of frequency points
            - paired_data (list): List of (frequency, value) tuples
    Returns:
        None
    Raises:
        IOError: If the file cannot be opened or written to.
    """
    with open(filename, 'w') as f:
        # Construct header line
        param_str = ' '.join([f"{k}={v}" for k, v in block.parameters.items()])
        if param_str:
            header_line = f"{block.prefix}{block.name} {param_str} //{block.n_frequencies}\n"
        else:
            header_line = f"{block.prefix}{block.name} //{block.n_frequencies}\n"
        
        f.write(header_line)
        
        # Write data values (only the second element of each pair)
        values = [str(pair[1]) for pair in block.paired_data]
        
        # Write values in lines of up to 5 values each for readability
        for i in range(0, len(values), 5):
            line_values = values[i:i+5]
            f.write(' '.join(line_values) + '\n')
        
        f.write("\n")
#
def write_edi_all_data_blocks(filename, data_blocks):
    """
    This function writes multiple data block objects to an EDI (Electromagnetic Data Interchange) file.
    Each data block consists of:
    - A block header line (e.g., '>FREQ //48', '>ZXXR ROT=ZROT //48')
    - Numerical values on subsequent lines, space-separated.
    
    Args:
        filename (str): Path to the EDI file to write to.
        data_blocks (list): List of data block objects to write.
            Each object should contain:
            - prefix (str): Always '>' for data blocks
            - name (str): Block name (e.g., 'FREQ', 'ZXXR')
            - parameters (dict): Optional parameters (e.g., ROT=ZROT)
            - n_frequencies (int): Number of frequency points
            - paired_data (list): List of (frequency, value) tuples
    Returns:
        None
    Raises:
        IOError: If the file cannot be opened or written to.
    """
    with open(filename, 'a') as f:
        for block in data_blocks:
            # Construct header line
            param_str = ' '.join([f"{k}={v}" for k, v in block.parameters.items()])
            if param_str:
                header_line = f"{block.prefix}{block.name} {param_str} //{block.n_frequencies}\n"
            else:
                header_line = f"{block.prefix}{block.name} //{block.n_frequencies}\n"
            
            f.write(header_line)
            
            # Write data values (only the second element of each pair)
            values = [str(pair[1]) for pair in block.paired_data]
            
            # Write values in lines of up to 5 values each for readability
            for i in range(0, len(values), 5):
                line_values = values[i:i+5]
                aligned_values = [' ' + val if float(val) < 0 else '  ' + val for val in line_values]
                # Align negative and positive values for better readability
                # the line shall start with " 1.2" or "-1.2" so no space if negative, one space if positive
                if aligned_values and aligned_values[0].startswith('  '):
                    aligned_values[0] = aligned_values[0][1:]  # Remove leading space for first positive value
                # if negative, no space!
                if aligned_values and aligned_values[0].startswith(' -'):
                    aligned_values[0] = aligned_values[0][1:]  # Remove leading space for first negative value
                
                
                
                f.write(''.join(aligned_values) + '\n')
            
            f.write("\n")
        f.write(">END\n")
#

