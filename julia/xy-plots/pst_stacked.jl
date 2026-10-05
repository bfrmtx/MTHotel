# plot stacked PST (parallel sensor test)
# data will be two columns: frequency and the corresponding measurement
# files to b grouped: 033_ADU-11e_C002_THx_131072Hz__033_ADU-11e_C002_THx_131072Hz and 033_ADU-11e_C002_THx_32768Hz__033_ADU-11e_C002_THx_32768Hz only differ in the frequency part (last segment separated by "_" underscore)
# x-axis is always frequency, logarithmic scale
# if nothing is specified, default to logarithmic scale for y-axis
# -aml (amplitude), y-axis will be logarithmic, default, can be omitted
# -phs (phase), y-axis will be linear
# -coh (coherence), y-axis will be linear
# -noise, y-axis will be linear

using Plots