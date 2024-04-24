import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import bisplrep, bisplev

data = np.loadtxt('data.txt')

ts1 = data[:, 0]
ts2 = data[:, 1]
ts3 = data[:, 2]
performance = data[:, 3]

# unique_ts1 = np.unique(ts1)
# plot for 2 < ts1 < 128
unique_ts1 = np.unique(ts1[(ts1 > 1) & (ts1 < 128)])

fig, axs = plt.subplots(nrows=len(unique_ts1), ncols=1, subplot_kw={'projection': '3d'}, figsize=(10, 6 * len(unique_ts1)))

if len(unique_ts1) == 1:
    axs = [axs]

for ax, value in zip(axs, unique_ts1):
    mask = ts1 == value
    x = ts2[mask]
    y = ts3[mask]
    z = performance[mask]
    
    xi = np.linspace(min(x), max(x), 100)
    yi = np.linspace(min(y), max(y), 100)
    xi_grid, yi_grid = np.meshgrid(xi, yi)
    
    spline = bisplrep(x, y, z, s=3)
    
    zi_grid = bisplev(xi, yi, spline)
    
    surf = ax.plot_surface(xi_grid, yi_grid, zi_grid, cmap='viridis', edgecolor='none')
    ax.set_title(f'Surface for ts1 = {value}')
    ax.set_xlabel('ts2')
    ax.set_ylabel('ts3')
    ax.set_zlabel('Performance')

plt.tight_layout()
plt.show()

plt.savefig('plot_vary_ts2_ts3_const_ts1.png')
