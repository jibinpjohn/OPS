#!/usr/bin/python
# Script to plot the stats extracted related to sent
# packets.
#
# Author: Asanga Udugama (adu@comnets.uni-bremen.de)
# Date: 22-aug-2017

import numpy as np
import matplotlib.pyplot as plt

n_groups = 5
  

total_dd =   ( 1.9151,5.0531, 11.836,19.285,24.777)
data =            ( 1.0,4.0602,  9.871,15.348,20.64)
feedback =        ( 1.51,4.780,    11.405,18.485,23.96)
#summary_vector =  (409146,        0,        0)
#data_request =    (409130,        0,        0)

fig, ax = plt.subplots()

index = np.arange(n_groups)
bar_width = 0.2

opacity = 0.4

rects1 = plt.bar(index, total_dd, bar_width,
                 alpha=opacity,
                 color='green', label='Average Tx of RCVD')

rects2 = plt.bar(index + bar_width, data, bar_width,
                 alpha=opacity,
                 color='darkblue', label='Average Tx of NOTRCVD')

rects3 = plt.bar(index + (bar_width * 2), feedback, bar_width,
                 alpha=opacity,
                 color='darkred', label='Average Tx')

#rects4 = plt.bar(index + (bar_width * 3), summary_vector, bar_width,
                 #alpha=opacity,
                 #color='green', label='Summary Vector')

#rects5 = plt.bar(index + (bar_width * 4), data_request, bar_width,
                 #alpha=opacity,
                 #color='darkgreen', label='Data Request')

plt.xlabel('The no.of copies')
plt.ylabel('Average No. of Tx')
plt.title('Average No. of Tx Vs L')
plt.xticks(index + (bar_width * 2), ('L=2', 'L=5', 'L=10','L=15','L=20'))
plt.ylim(0, 26)
plt.legend()

plt.tight_layout()
plt.show()

