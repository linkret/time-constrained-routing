import matplotlib.pyplot as plt
import sys
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

def main():
    if len(sys.argv) != 2:
        print("Usage: python plot.py input_file.txt")
        return

    file_path = sys.argv[1]
    customers = parse_input_file(file_path)
    depot = customers[0]

    x_values = [customer['X'] for customer in customers]
    y_values = [customer['Y'] for customer in customers]
    demands = [customer['Demand'] for customer in customers]
    ready_times = [customer['ReadyTime'] for customer in customers]
    due_times = [customer['DueDate'] for customer in customers]
    customer_ids = [customer['ID'] for customer in customers]

    plt.scatter(x_values, y_values, label='Customers')
    plt.scatter(depot['X'], depot['Y'], color='red', label='Depot')
    plt.xlabel('X')
    plt.ylabel('Y')
    plt.title('Customer Locations')
    plt.legend()
    plt.savefig(file_path[:-4] + "_locations.png")

    plt.figure()
    demand_range = range(min(demands), max(demands) + 6, 5)
    plt.hist(demands, bins=demand_range, rwidth=0.8, align='left')
    plt.xlabel('Demand')
    plt.ylabel('Customer count')
    plt.title('Customer Demands')
    plt.savefig(file_path[:-4] + "_demands.png")

    time_hist = [0] * (depot['DueDate'] + 2)
    ready_hist = time_hist[:]
    due_hist = time_hist[:]
    for c in customers:
        time_hist[c['ReadyTime']] += 1
        time_hist[c['DueDate']+1] -= 1
        ready_hist[c['ReadyTime']] += 1
        due_hist[c['DueDate']] += 1
    for i in range(1, len(time_hist)):
        time_hist[i] += time_hist[i - 1]

    plt.figure()
    plt.plot(time_hist, label='Active Windows')
    plt.plot(ready_hist, color='green', label='Starts')
    plt.plot(due_hist, color='red', label='Ends')
    plt.xlabel('Time')
    plt.ylabel('Customer count')
    plt.title('Customer Time Windows')
    plt.legend()
    plt.savefig(file_path[:-4] + "_times.png")

if __name__ == "__main__":
    main()
