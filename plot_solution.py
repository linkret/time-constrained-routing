import matplotlib.pyplot as plt
import sys
import colorsys
import numpy as np

def parse_input_file(file_path):
    customers = []
    with open(file_path, 'r') as file:
        for line in file:
            fields = line.strip().split()
            if len(fields) == 7:
                customer = {
                    "ID": int(fields[0]),
                    "X": float(fields[1]),
                    "Y": float(fields[2]),
                    "Demand": int(fields[3]),
                    "ReadyTime": int(fields[4]),
                    "DueDate": int(fields[5]),
                    "ServiceTime": int(fields[6])
                }
                customers.append(customer)
    return customers

def parse_solution_file(file_path):
    routes = []
    with open(file_path, 'r') as file:
        lines = file.readlines()
        for line in lines[1:-1]:
            line = line.strip().split('->')
            line[0] = line[0].split(': ')[1]
            line = map(lambda s : int(s[0 : s.index('(')]), line)
            routes.append(list(line))
    return routes

def main():
    if len(sys.argv) != 3:
        print("Usage: python plot_solution.py input_file.txt solution_file.txt")
        return

    file_path = sys.argv[1]
    solution_path = sys.argv[2]
    customers = parse_input_file(file_path)
    solution = parse_solution_file(solution_path)
    depot = customers[0]

    num_colors = len(solution)
    colors = []
    for i in range(num_colors):
        hue = i / num_colors
        saturation = 0.7
        lightness = 0.6
        rgb_color = colorsys.hls_to_rgb(hue, lightness, saturation)
        colors.append(rgb_color)

    x_values = [customer['X'] for customer in customers]
    y_values = [customer['Y'] for customer in customers]
    customer_ids = [customer['ID'] for customer in customers]

    for i, route in enumerate(solution):
        x_route = [x_values[idx] for idx in route]
        y_route = [y_values[idx] for idx in route]
        plt.scatter(x_route, y_route, label=f'Route {i}', color=colors[i])
        
    #plt.scatter(x_values, y_values, label='Customers')
    #plt.scatter(depot['X'], depot['Y'], color='red', label='Depot')
    plt.xlabel('X')
    plt.ylabel('Y')
    plt.title('Customer Locations')
    plt.legend(loc='center right', bbox_to_anchor=(1.125, 0.5), fontsize='xx-small')
    plt.savefig(file_path[:-4] + "_solution.png")

if __name__ == "__main__":
    main()
