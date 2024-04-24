import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt


data_path = 'data.txt'  
data = pd.read_csv(data_path, sep=' ', names=['X', 'Y', 'Z', 'Performance'])

filtered_data = data[data['Z'] == 32]  

pivot_table = filtered_data.pivot(index='Y', columns='X', values='Performance')

plt.figure(figsize=(10, 8))
sns.heatmap(pivot_table, annot=True, fmt=".2f", cmap='coolwarm')
plt.title('Heat Map of Performance for Z=1')
plt.show()

