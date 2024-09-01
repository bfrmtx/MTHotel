import sys
import os
def get_script_directory(n_levels_up=0, explicit_tmp_dir=None):
    main_dir = ''
    script_dir = ''
    include_dir = ''
    temp_dir = '/tmp'
    try:
        # Check if running in a standard Python script
        script_dir = os.path.dirname(os.path.abspath(__file__))
        # return script_dir
    except NameError:
        # If __file__ is not defined, we are likely in a Jupyter Notebook
        from IPython import get_ipython
        if 'IPKernelApp' in get_ipython().config:
            script_dir = os.getcwd()
        else:
            raise RuntimeError("Unable to determine the script directory")
    # cd up n_levels_up
    main_dir = script_dir
    for i in range(n_levels_up):
        main_dir = os.path.dirname(main_dir)
    include_dir = os.path.join(main_dir, 'include')
    # leave the temporary directory to the user
    if explicit_tmp_dir is not None:
        temp_dir = explicit_tmp_dir
        return main_dir, script_dir, include_dir, temp_dir
    # else we try the simplest way to get the temporary directory
    # determine if we run Linux, MacOS or Windows
    if sys.platform == 'linux':
        temp_dir = '/tmp'
    elif sys.platform == 'darwin':
        temp_dir = '/tmp'
    elif sys.platform == 'win32':
        temp_dir = os.environ['TEMP']
    else:
        raise RuntimeError("Unknown platform")

    # return all directories
    return main_dir, script_dir, include_dir, temp_dir
#
main_dir, script_dir, include_dir, temp_dir = get_script_directory(1)
print(main_dir)
print(script_dir)
print(include_dir)
print(temp_dir)
