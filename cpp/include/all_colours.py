colors = """
white black dark-grey red web-green web-blue dark-magenta dark-cyan \
dark-orange dark-yellow royalblue goldenrod dark-spring-green \
purple steelblue dark-red dark-chartreuse orchid aquamarine brown \
yellow turquoise grey0 grey10 grey20 grey30 grey40 grey50 grey60 \
grey70 grey grey80 grey90 grey100 light-red light-green light-blue \
light-magenta light-cyan light-goldenrod light-pink light-turquoise \
gold green dark-green spring-green forest-green sea-green blue \
dark-blue midnight-blue navy medium-blue skyblue cyan magenta \
dark-turquoise dark-pink coral light-coral orange-red salmon \
dark-salmon khaki dark-khaki dark-goldenrod beige olive orange \
violet dark-violet plum dark-plum dark-olivegreen orangered4 brown4 \
sienna4 orchid4 mediumpurple3 slateblue1 yellow4 sienna1 tan1 \
sandybrown light-salmon pink khaki1 lemonchiffon bisque honeydew \
slategrey seagreen antiquewhite chartreuse greenyellow gray light-gray \
light-grey dark-gray slategray gray0 gray10 gray20 gray30 gray40 gray50 \
gray60 gray70 gray80 gray90 gray100
""".split()

blue_colors = [color for color in colors if 'blue' in color]
print(blue_colors)