import pandas as pd
import numpy as np

data = pd.read_csv('clauss.txt', sep=' ', names=['X', 'Y', 'Performance'])

allowed_values = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512]

data = data[data['X'].isin(allowed_values) & data['Y'].isin(allowed_values)]

def find_neighbors(x, y):
    """ Generate coordinates for neighbors that are next powers of two """
    neighbors = []
    for coord in [x, y]:
        higher = [v for v in allowed_values if v > coord]
        lower = [v for v in allowed_values if v < coord]
        neighbors.append((lower[-1] if lower else None, higher[0] if higher else None))

    all_neighbors = []
    for nx in (neighbors[0][0], x, neighbors[0][1]):
        for ny in (neighbors[1][0], y, neighbors[1][1]):
                if nx and ny and (nx != x or ny != y):  # Ensure we don't include the original point
                    all_neighbors.append((nx, ny))
    return all_neighbors

def move_to_minimum(x, y, data):
    """ Move to the neighbor with the minimum performance """
    neighbors = find_neighbors(x, y)
    performances = [(nx, ny, data[(data['X'] == nx) & (data['Y'] == ny)]['Performance'].min())
                    for nx, ny in neighbors if not pd.isnull(data[(data['X'] == nx) & (data['Y'] == ny)]['Performance'].min())]
    
    current_performance = data[(data['X'] == x) & (data['Y'] == y) ]['Performance'].min()
    performances.append((x, y, current_performance))
    
    min_point = min(performances, key=lambda t: t[2])
    return min_point

def coordinate_descent(x, y, data):
    """ Perform the descent algorithm to find the local minimum """
    current_position = (x, y, np.inf)
    while True:
        next_position = move_to_minimum(*current_position[:2], data)
        if next_position[2] >= current_position[2]:
            return current_position
        current_position = next_position
        print("Current position:", current_position)


# x, y, z = 32, 32, 32  
x, y = 1, 1
# x, y = 32, 32

print("Test Seed", x, y)
result = coordinate_descent(x, y, data)
print("Converged to:", result)

