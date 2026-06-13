import math


def skip_samples_from_filter(filter_len, sampling_rate):
    """Compute how many samples to skip to shift the filtered data to a full second.

    Pure translation of atsfile::skip_samples_from_filter(): no file operations.

    Parameters
    ----------
    filter_len : int
        FIR filter length ("filter_length").
    sampling_rate : float
        Sampling rate in Hz ("sample_freq").

    Returns
    -------
    int
        samples_to_skip: number of samples to skip at the start.
    """
    # example for 128 Hz and 32 x filter
    # fracs = 1.8398s delay = 471 * 0.5 = 235 / 128.
    # "-1" gives the correct delay used in later versions of the atss file format
    fracs = ((float(filter_len - 1)) * 0.5) / sampling_rate

    # delay can be more than 1s; split into integer (fulldelay) and fractional (miss) part
    fulldelay = math.floor(fracs)
    miss = fracs - fulldelay

    # 20 = 128 - (128 * 0.8398)
    samples_to_skip = int(sampling_rate - miss * sampling_rate)

    # miss is in the sub sample ?
    if (samples_to_skip + 1) == int(sampling_rate) or (samples_to_skip - 1) == int(sampling_rate):
        samples_to_skip = 0

    return samples_to_skip
