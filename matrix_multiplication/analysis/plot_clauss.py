import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from scipy.interpolate import griddata
import mpld3

# Load the data from a text file
data = np.loadtxt('clauss_a64fx.txt')

# Extract columns
x = data[:, 0]
y = data[:, 1]
z = data[:, 2]

# Create grid values for plotting
xi = np.linspace(x.min(), x.max(), 100)
yi = np.linspace(y.min(), y.max(), 100)
xi, yi = np.meshgrid(xi, yi)

# Interpolate unstructured D-dimensional data
zi = griddata((x, y), z, (xi, yi), method='cubic')

# Create the plot
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')

# Plot the surface
surf = ax.plot_surface(xi, yi, zi, cmap='viridis', edgecolor='none')

# Add labels and title
ax.set_xlabel('X Axis Label')
ax.set_ylabel('Y Axis Label')
ax.set_zlabel('Z Axis Value')
ax.set_title('3D Surface Plot')

# Add a color bar which maps values to colors.
fig.colorbar(surf, shrink=0.5, aspect=5)

# Step 5: Convert to HTML and save
html_str = mpld3.fig_to_html(fig)
with open("plots/clauss_surface_a64fx.html", "w") as f:
    f.write(html_str)

print("Plot saved as HTML.")
