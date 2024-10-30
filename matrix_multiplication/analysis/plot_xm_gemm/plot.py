import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import LinearNDInterpolator

# Reading the data from the file
data = np.loadtxt('data_gemm.txt')

# Extracting the columns
x = data[:, 0]
y = data[:, 1]
z = data[:, 2]
t = data[:, 3]

# Creating the interpolator
interpolator = LinearNDInterpolator((x, y, z), t)

# Creating a grid for plotting
x_unique = np.unique(x)
y_unique = np.unique(y)
z_unique = np.unique(z)
x_grid, y_grid, z_grid = np.meshgrid(x_unique, y_unique, z_unique, indexing='ij')

# Interpolating the values (this time for color mapping)
t_grid = interpolator(x, y, z)

# Plotting the scatter plot
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
sc = ax.scatter(x, y, z, c=t, cmap='viridis', marker='o')

ax.set_xlabel('Tile Size X')
ax.set_ylabel('Tile Size Y')
ax.set_zlabel('Tile Size Z')
fig.colorbar(sc, label='Execution Time')

plt.show()
