import numpy as np
import plotly.graph_objects as go
from scipy.interpolate import griddata

# Load data from 'clauss.txt'
data = np.loadtxt('clauss_m1.txt')

# Extract columns
x, y, z = data[:, 0], data[:, 1], data[:, 2]

# Create grid values for plotting
xi = np.linspace(x.min(), x.max(), 100)
yi = np.linspace(y.min(), y.max(), 100)
xi, yi = np.meshgrid(xi, yi)

# Interpolate unstructured D-dimensional data
zi = griddata((x, y), z, (xi, yi), method='cubic')

# Create the plot with Plotly
fig = go.Figure(data=[go.Surface(x=xi, y=yi, z=zi, colorscale='Viridis')])

# Update the layout
fig.update_layout(
    title='3D Surface Plot from Data',
    scene=dict(
        xaxis_title='X Axis Label',
        yaxis_title='Y Axis Label',
        zaxis_title='Z Axis Value'
    )
)

# Save the figure as an interactive HTML file
fig.write_html('clauss_3d_plot_m1.html')
