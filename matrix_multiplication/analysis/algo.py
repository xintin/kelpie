import pandas as pd
import numpy as np

data = pd.read_csv('data.txt', sep=' ', names=['X', 'Y', 'Z', 'Performance'])

allowed_values = [2, 4, 8, 16, 32, 64, 128, 256, 512]

data = data[data['X'].isin(allowed_values) & data['Y'].isin(allowed_values) & data['Z'].isin(allowed_values)]

def find_neighbors(x, y, z):
    """ Generate coordinates for neighbors that are next powers of two """
    neighbors = []
    for coord in [x, y, z]:
        higher = [v for v in allowed_values if v > coord]
        lower = [v for v in allowed_values if v < coord]
        neighbors.append((lower[-1] if lower else None, higher[0] if higher else None))

    all_neighbors = []
    for nx in (neighbors[0][0], x, neighbors[0][1]):
        for ny in (neighbors[1][0], y, neighbors[1][1]):
            for nz in (neighbors[2][0], z, neighbors[2][1]):
                if nx and ny and nz and (nx != x or ny != y or nz != z):  # Ensure we don't include the original point
                    all_neighbors.append((nx, ny, nz))
    return all_neighbors

def move_to_minimum(x, y, z, data):
    """ Move to the neighbor with the minimum performance """
    neighbors = find_neighbors(x, y, z)
    performances = [(nx, ny, nz, data[(data['X'] == nx) & (data['Y'] == ny) & (data['Z'] == nz)]['Performance'].min())
                    for nx, ny, nz in neighbors if not pd.isnull(data[(data['X'] == nx) & (data['Y'] == ny) & (data['Z'] == nz)]['Performance'].min())]
    
    current_performance = data[(data['X'] == x) & (data['Y'] == y) & (data['Z'] == z)]['Performance'].min()
    performances.append((x, y, z, current_performance))
    
    min_point = min(performances, key=lambda t: t[3])
    return min_point

def coordinate_descent(x, y, z, data):
    """ Perform the descent algorithm to find the local minimum """
    current_position = (x, y, z, np.inf)
    while True:
        next_position = move_to_minimum(*current_position[:3], data)
        if next_position[3] >= current_position[3]:
            return current_position
        current_position = next_position
        print("Current position:", current_position)


x, y, z = 32, 32, 32  
result = coordinate_descent(x, y, z, data)
print("Converged to:", result)

