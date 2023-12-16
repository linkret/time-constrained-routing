import json
import logging
import math
import os
from os.path import isdir, isfile

import click
import numpy as np


def parse_input(input_file):
    def parse_customer_line(line):
        return [int(x.strip()) for x in line.strip().split()]

    customers = []
    with open(input_file) as f:
        lines = f.readlines()
        vehicle_number, vehicle_capacity = parse_customer_line(lines[2])
        depot = parse_customer_line(lines[7])
        for i in range(8, len(lines)):
            params = parse_customer_line(lines[i])
            customers.append(params)
    return vehicle_number, vehicle_capacity, depot, customers


def parse_output(output_file):
    def parse_route_line(line):
        rid, rt = line.strip().split(":")
        rt_parts = rt.strip().split("->")
        return int(rid), {
            'locations': [int(x.split("(")[0]) for x in rt_parts],
            'start_ts': [int(x.split("(")[1][:-1]) for x in rt_parts]
        }

    routes = {}
    with open(output_file) as f:
        lines = f.readlines()
        num_routes = int(lines[0])
        for line in lines[1:-1]:
            rid, route = parse_route_line(line)
            routes[rid] = route
        distance = float(lines[-1].strip())
    return num_routes, routes, distance


def validate_output(vehicle_number, vehicle_capacity, depot, customers, num_routes, routes, distance,
                    PRECISION_DISTANCE=0.01):
    dist_sum = 0.0
    if num_routes > vehicle_number:
        logging.error(f"Maximum allowed number of vehicles is {num_routes}, but found {vehicle_number}")
        return False
    visited_set = set()
    for rid, r in routes.items():
        for loc, start_t in zip(r['locations'], r['start_ts']):
            c = depot if loc == 0 else customers[customers[:, 0] == loc].ravel()
            if c is None or np.size(c) == 0:
                logging.error(f"Vehicle on the route {rid} is trying to visit non-existing customer with id {loc}")
                return False
            if start_t > c[5]:
                logging.error(f"Vehicle on the route {rid} is trying to visit customer {loc} at {start_t} after due "
                              f"date which is at {c[5]}.")
                return False
        for loc1, st1, loc2, st2 in list(
                zip(r['locations'][:-1], r['start_ts'][:-1], r['locations'][1:], r['start_ts'][1:])):
            c1 = depot if loc1 == 0 else customers[customers[:, 0] == loc1].ravel()
            c2 = depot if loc2 == 0 else customers[customers[:, 0] == loc2].ravel()
            if c1 is None or np.size(c1) == 0 or c2 is None or np.size(c2) == 0:
                continue
            if st1 < c1[4]:
                logging.warning(f"Vehicle on the route {rid} is trying to start delivery service to the customer "
                                f"{loc1} at {st2}, but ready time is at {c1[4]}. Waiting for the ready time...")
                stime = c1[4]
            else:
                stime = st1
            dist_s1_s2 = ((c2[2] - c1[2]) ** 2 + (c2[1] - c1[1]) ** 2) ** 0.5
            dist_sum += dist_s1_s2
            min_start_time = stime + c1[6] + math.ceil(dist_s1_s2)
            if st2 < min_start_time:
                logging.error(f"Vehicle on the route {rid} is trying to start delivery service to the customer {loc2} "
                              f"at {st2}, but cannot start before {min_start_time}")
                return False
        sum_demand = 0
        for loc in r['locations']:
            c = depot if loc == 0 else customers[customers[:, 0] == loc].ravel()
            sum_demand += c[3]
            visited_set.add(c[0])
        if sum_demand > vehicle_capacity:
            logging.error(f"Vehicle on the route {rid} has total demand of {sum_demand} which is greater than the"
                          f" vehicles capacity {vehicle_capacity}")
            return False
        if r['locations'][0] != 0 or r['locations'][-1] != 0:
            logging.error(f"Vehicle on the route {rid} doesn't start/end with a depot")
            return False
    if len(visited_set) < len(customers) + 1:
        logging.error(f"Not all of the customers are visited. Visited {len(visited_set)} out of {len(customers)} "
                      f"customers")
        return False

    if distance > dist_sum + PRECISION_DISTANCE or distance < dist_sum - PRECISION_DISTANCE:
        logging.warning(f"Distance miscalculated. Got {distance}, but should be {dist_sum}")

    return True


logging.basicConfig(format='%(asctime)s - %(levelname)s - %(message)s', level=logging.DEBUG)


def parse_input_params(parameters):
    try:
        if parameters['input'] is None:
            parameters['input'] = 'input.txt'
        elif isfile(parameters['input']) is False:
            return None
        if parameters['output'] is None:
            parameters['output'] = 'output.txt'
        elif isdir(parameters['output']) is True:
            return None
    except Exception:
        return None
    return parameters


@click.command(name="validator")
@click.option(
    '-i',
    '--input',
    default=None,
    type=click.Path(exists=False, dir_okay=False),
    help="File with input data. [default: input.txt]",
)
@click.option(
    '-o',
    '--output',
    default=None,
    type=click.Path(exists=False, dir_okay=False),
    help="File with solution. [default: output.txt]",
)
@click.help_option(help="Show this help message and exit.")  # --help
def validator(**parameters):
    p = parse_input_params(parameters)
    if p is not None:
        try:
            hmo_in = p['input']
            hmo_out = p['output']

            logging.info(f"instances_path = {hmo_in} ({os.path.abspath(hmo_in)})")
            logging.info(f"routes_path = {hmo_out} ({os.path.abspath(hmo_out)})")

            vehicle_number, vehicle_capacity, depot, customers = parse_input(hmo_in)
            depot = np.array(depot)
            customers = np.array(customers)
            locations = np.vstack((depot, customers))

            num_routes, routes, distance = parse_output(hmo_out)

            if np.array(validate_output(vehicle_number, vehicle_capacity, depot, customers, num_routes, routes, distance)):
                logging.info(f"Validation SUCCESSFUL")
            else:
                logging.info(f"Validation FAILED")
        except Exception:
            logging.error("input/output file format not valid!")
    else:
        logging.error("invalid validator script parameters!")
        logging.error("check that input and output file both exist!")


if __name__ == '__main__':
    validator()
