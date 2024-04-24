import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from scipy.interpolate import bisplrep, bisplev

data = np.loadtxt("data.txt")

ts1 = data[:, 0]  
ts2 = data[:, 1]  
ts3 = data[:, 2]  
performance = data[:, 3]  

# 332/1000 < 0.183097

# Set up the grid for interpolation
ts3_new = np.linspace(min(ts3), max(ts3), 100)
ts2_new = np.linspace(min(ts2), max(ts2), 100)
ts3_grid, ts2_grid = np.meshgrid(ts3_new, ts2_new)

# Note: ts1, ts2 are nto used in this example for fitting the spline
spline = bisplrep(ts3, ts2, performance, s=3)

performance_grid = bisplev(ts3_new, ts2_new, spline)

fig = plt.figure(figsize=(10, 8))
ax = fig.add_subplot(111, projection='3d')

surf = ax.plot_surface(ts3_grid, ts2_grid, performance_grid, cmap='viridis', edgecolor='none')

ax.set_xlabel('Tile Size (varying)')
ax.set_ylabel('TS2 (constant)')
ax.set_zlabel('Performance')

plt.title('3D Surface Plot of Performance')
plt.colorbar(surf, label='Performance')
plt.show()

