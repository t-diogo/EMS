# -*- coding: utf-8 -*-
"""
Created on Tue Dec 19 00:29:00 2023

@author: Tomas
"""

import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import freqz

# Filter coefficients
b = [0.169994948147430, 0.280415310498794, -1.120574766348363,
     0.131562559965936, 0.974153561246036, -0.282740857326553, -0.152810756202003]
a = [1.00000000000000000, -2.12979364760736134, 0.42996125885751674,
     1.62132698199721426, -0.96669962900852902, 0.00121015844426781, 0.04400300696788968]

# Frequency response
w, h = freqz(b, a, worN=8000)

# Plotting
plt.figure(figsize=(10, 6))
plt.plot(0.5 * 44100 * w / np.pi, 20 * np.log10(np.abs(h)), 'b')  # Convert magnitude to dB
plt.title('Filter Frequency Response')
plt.xlabel('Frequency [Hz]')
plt.ylabel('Gain [dB]')
plt.xscale('log')
plt.xlim([10, 100000])  # Set X axis limits to 20 Hz - 20 kHz
plt.ylim([-60, 5])  # Set Y axis limits to -60 dB to 5 dB
plt.grid(which='both', linestyle='--', linewidth=0.5)
plt.show()
