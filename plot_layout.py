import matplotlib.pyplot as plt
import matplotlib.patches as patches

def parse_data(file_path):
    die_coords = []
    row_height = 0
    instances = []
    with open(file_path, 'r') as file:
        for line in file:
            parts = line.split()
            if parts[0] == 'Die':
                die_coords = list(map(int, parts[1:]))
            elif parts[0] == 'RowHeight':
                row_height = int(parts[1])
            elif parts[0] == 'Inst':
                instances.append((parts[1], parts[2],list(map(int, parts[3:]))))
    return die_coords, row_height, instances

def plot_layout(die_coords, row_height, instances):
    fig, ax = plt.subplots()
    large_side = max(die_coords[2]-die_coords[0], die_coords[3]-die_coords[1])
    shrink_ratio = large_side / 1000    
    ax.set_xlim([die_coords[0] / shrink_ratio, die_coords[2] / shrink_ratio])
    ax.set_ylim([die_coords[1] / shrink_ratio, die_coords[3] / shrink_ratio])
    #ax.set_aspect('equal')

    # Draw die boundary in red
    ax.plot([die_coords[0] / shrink_ratio, die_coords[2] / shrink_ratio, die_coords[2] / shrink_ratio, die_coords[0] / shrink_ratio, die_coords[0] / shrink_ratio],
            [die_coords[1] / shrink_ratio, die_coords[1] / shrink_ratio, die_coords[3] / shrink_ratio, die_coords[3] / shrink_ratio, die_coords[1] / shrink_ratio], color='red', linewidth=2)

    # Draw rows in green
    #for y in range(die_coords[1] + row_height, die_coords[3], row_height):
    #    ax.plot([die_coords[0] / shrink_ratio, die_coords[2] / shrink_ratio], [y / shrink_ratio, y / shrink_ratio], color='green')

    # Draw instances in blue
    for instance in instances:
        name, inst_type,coords = instance
        #ax.add_patch(patches.Rectangle((coords[0] / shrink_ratio, coords[1] / shrink_ratio), (coords[2]-coords[0]) / shrink_ratio, (coords[3]-coords[1]) / shrink_ratio, 
        #                               fill=False, edgecolor='blue', linewidth=1))
        if inst_type == 'FF':
            ax.add_patch(patches.Rectangle((coords[0] / shrink_ratio, coords[1] / shrink_ratio), (coords[2]-coords[0]) / shrink_ratio, (coords[3]-coords[1]) / shrink_ratio,color='blue'))
        else :
            ax.add_patch(patches.Rectangle((coords[0] / shrink_ratio, coords[1] / shrink_ratio), (coords[2]-coords[0]) / shrink_ratio, (coords[3]-coords[1]) / shrink_ratio,color='green',alpha=0.5))
        #ax.text((coords[0]+coords[2])/2, (coords[1]+coords[3])/2, name, 
        #        horizontalalignment='center', verticalalignment='center')    
    plt.savefig('layout.png')

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print("Usage: python script.py <file_path>")
        sys.exit(1)

    file_path = sys.argv[1]
    die_coords, row_height, instances = parse_data(file_path)
    plot_layout(die_coords, row_height, instances)