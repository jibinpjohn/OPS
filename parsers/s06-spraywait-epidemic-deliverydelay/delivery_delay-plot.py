#!/usr/bin/python
# Script to plot the stats extracted related to sent
# packets.
#
# Author: Asanga Udugama (adu@comnets.uni-bremen.de)
# Date: 22-aug-2017

import numpy as np
import matplotlib.pyplot as plt

n_groups = 5

total_dd =   ( 15578.458331,14334.4020092,  13341.4416946,12830.1386885,12685.419471)
#data =            ( 15578.458331,14334.4020092,  13341.4416946,12830.1386885,12685.419471)
#feedback =        (     0,        0,    72404)
#summary_vector =  (409146,        0,        0)
#data_request =    (409130,        0,        0)

fig, ax = plt.subplots()

index = np.arange(n_groups)
bar_width = 0.1

opacity = 0.4

rects1 = plt.bar(index, total_dd, 2*bar_width,
                 alpha=opacity,
                 color='green', label='Delivery Delay')

#rects2 = plt.bar(index + bar_width, data, bar_width,
                 #alpha=opacity,
                 #color='darkblue', label='Data')

#rects3 = plt.bar(index + (bar_width * 2), feedback, bar_width,
                 #alpha=opacity,
                 #color='darkred', label='Feedback')

#rects4 = plt.bar(index + (bar_width * 3), summary_vector, bar_width,
                 #alpha=opacity,
                 #color='green', label='Summary Vector')

#rects5 = plt.bar(index + (bar_width * 4), data_request, bar_width,
                 #alpha=opacity,
                 #color='darkgreen', label='Data Request')

plt.xlabel('The no.of copies')
plt.ylabel('Delivery Delay')
plt.title('Delivery Delay Vs L')
plt.xticks(index + (bar_width * 2), ('L=2', 'L=5', 'L=10','L=15','L=20'))
plt.ylim(12000, 16000)
plt.legend()

plt.tight_layout()
plt.show()

