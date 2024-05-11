import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import numpy as np
import sys

def parse_data(file_path):
    die_coords = []
    row_height = 0
    bins = []
    with open(file_path, 'r') as file:
        for line in file:
            parts = line.split()
            if parts[0] == 'Die':
                die_coords = list(map(int, parts[1:]))
            elif parts[0] == 'RowHeight':
                row_height = int(parts[1])
            elif parts[0] == 'Bin':
                bins.append(list(map(int, parts[1:])))
    return die_coords, row_height, bins

def plot_layout(die_coords, row_height, bins):
    fig, ax = plt.subplots()
    ax.set_xlim([die_coords[0], die_coords[2]])
    ax.set_ylim([die_coords[1], die_coords[3]])
    ax.set_aspect('equal')

    # Draw die boundary
    ax.plot([die_coords[0], die_coords[2], die_coords[2], die_coords[0], die_coords[0]],
            [die_coords[1], die_coords[1], die_coords[3], die_coords[3], die_coords[1]], 'k-')
    # Draw bins
    cmap = plt.get_cmap('coolwarm')
    norm = mcolors.Normalize(vmin=0, vmax=100)  # Assuming utilization is in percentage
    for bin in bins:
        ax.add_patch(plt.Rectangle((bin[0], bin[1]), bin[2]-bin[0], bin[3]-bin[1], 
                           facecolor=cmap(norm(bin[4])), edgecolor='black', label=str(bin[4])))
        ax.text((bin[0]+bin[2])/2, (bin[1]+bin[3])/2, str(bin[4]), 
                horizontalalignment='center', verticalalignment='center')

    plt.savefig('output.png', dpi=300)
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py <file_path>")
        sys.exit(1)

    file_path = sys.argv[1]
    die_coords, row_height, bins = parse_data(file_path)
    plot_layout(die_coords, row_height, bins)