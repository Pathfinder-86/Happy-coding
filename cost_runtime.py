import matplotlib.pyplot as plt

# Initialize empty lists to store cost and runtime values
costs = []
runtimes = []

# Read the file
with open('cost_runtime.txt', 'r') as file:
    # Iterate over each line in the file
    for line in file:
        # Split the line by whitespace
        parts = line.split()
        # Extract the cost and runtime values
        cost = float(parts[1])
        runtime = float(parts[2])
        # Append the values to the respective lists
        costs.append(cost)
        runtimes.append(runtime)

# Plot the figure
plt.plot([runtime / 60 for runtime in runtimes], costs)
plt.xlabel('Runtime (min)')
plt.ylabel('Cost')
plt.title('Cost vs Runtime')
# Save the figure as cost_runtime.png
plt.savefig('cost_runtime.png')