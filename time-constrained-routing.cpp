#include <iostream>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <set>
#include <cassert>
#include <thread>

#include <fstream>
#include <filesystem>

struct solution;

// Global vars

std::chrono::time_point<std::chrono::steady_clock> start_time;
int max_routes, max_capacity;

std::filesystem::path input_path, output_path;

// Helpers

typedef std::pair<int, int> pii;
#define x first
#define y second

double distance(pii a, pii b) {
	auto d = sqrt(1.0L * (a.x - b.x) * (a.x - b.x) + 1.0L * (a.y - b.y) * (a.y - b.y));
	return d;
}
 
const int N = 5000; // Important, code will crash if customers.size() > N
double distances[N][N]; // precompute, to avoid slow sqrt() calls in runtime

std::chrono::milliseconds get_time() {
	using namespace std::chrono;
	auto dt = steady_clock::now() - start_time;
	return duration_cast<milliseconds>(dt);
}

// Classes

struct customer {
	int i, x, y, capacity, t1, t2, service_time;

	friend bool operator==(const customer& c1, const customer& c2) {
		return c1.i != c2.i;
	}
};

customer depot;
std::vector<customer> customers;

struct result {
	int num_routes;
	double length;
};

struct route {
	std::vector<int> to_visit; // customers
	int capacity = 0;

	route() {
		to_visit.push_back(0); // depot
		to_visit.push_back(0);
	}

	route(std::vector<int> to_visit) : to_visit(to_visit) {
		assert(to_visit.size() >= 2);
		assert(to_visit.front() == 0);
		assert(to_visit.back() == 0);
	}

	// greedily inserts the new customer at the position which results in the best fitness
	bool add_customer(int c) {
		int best_pos = 1;
		double min_fitness = 1e18;
		
		to_visit.insert(to_visit.begin() + 1, c);

		for (int i = 1; i < to_visit.size() - 1; i++) {
			auto f = fitness();
			if (f < min_fitness) {
				min_fitness = f;
				best_pos = i;
			}
			std::swap(to_visit[i], to_visit[i + 1]);
		}
		
		to_visit.pop_back();
		to_visit.insert(to_visit.begin() + best_pos, c);
		capacity += customers[c].capacity;
		return is_valid();
	}

	void remove_customer(int c) {
		auto it = std::find(to_visit.begin(), to_visit.end(), c);
		if (it != to_visit.end()) {
			capacity -= customers[c].capacity;
			to_visit.erase(it);
		}
	}

	struct drive_result {
		double t = 0, pathlen = 0, late_sum = 0;
		int late_cnt = 0, capacity = 0;

		// different parameters have different weights
		double fitness() const {
			double fitness = 0;
			fitness += t * 0.01;
			fitness += pathlen * 0.5;
			//fitness += late_cnt * 10;
			fitness += late_sum * 1;
			if (capacity > max_capacity)
				fitness += (capacity - max_capacity) * 10;
			else
				fitness += (max_capacity - capacity) * 0.005;
			return fitness;
		}

		bool is_valid() const {
			return late_cnt == 0 && capacity <= max_capacity;
		}
	} last_run; // optimization, might cause trouble

	// we could permute the customer order, like TSP, to minimise fitness
	double fitness() { return drive().fitness(); }
	double is_valid() { return drive().is_valid(); }
	double length() { return drive().pathlen; }

	const drive_result& drive() {
		int pos = 0; // depot
		double t = 0, pathlen = 0, late_sum = 0;
		int late_cnt = 0;

		for (int cid : to_visit) {
			const auto& c = customers[cid];

			double dist = distances[pos][c.i];
			pos = c.i;
			t += ceil(dist);
			pathlen += dist;

			if (t < c.t1)
				t = c.t1;
			if (t > c.t2) {
				late_sum += (t - c.t2);
				late_cnt += 1;
			}
			t += c.service_time;
		}

		//std::cout << t << " " << pathlen << " " << late_sum << " " << late_cnt << " " << capacity << "\n";

		last_run = { t, pathlen, late_sum, late_cnt, capacity };
		return last_run;
	}

	std::string to_string() const {
		std::stringstream ss;
		int pos = 0;
		double t = 0;

		for (int i = 0; i < to_visit.size(); i++) {
			const auto& c = customers[to_visit[i]];

			double dist = distances[pos][c.i];
			t += ceil(dist);
			pos = c.i;

			if (t < c.t1)
				t = c.t1;

			if (i != 0)
				ss << "->";
			ss << to_visit[i] << "(" << int(t) << ")";

			t += c.service_time;
		}
		return ss.str();
	}
};

struct solution {
	double distance;
	std::vector<route> routes;

	solution() : distance(1e18) {}
	solution(std::vector<route> routes, double distance) : routes(routes), distance(distance) {}

	bool empty() { return routes.empty(); }

	static solution from_file(const std::filesystem::path& filename) {
		using std::string;

		std::ifstream file(filename);
		int n_routes = 0;
		file >> n_routes;

		std::vector<route> routes;

		for (int i = 0; i < n_routes; i++) {
			std::string line;
			file.ignore();
			std::getline(file, line);

			std::vector<int> r;
			for (int pos = line.find(":", 0); pos != string::npos; pos = line.find("->", pos+1)) {
				int v = atoi(line.data() + pos + 2); // +2 to skip over "->"
				r.push_back(v);
			}

			routes.push_back(r);
		}

		double distance_sum = 0;
		file >> distance_sum;

		return solution(routes, distance_sum);
	}

	void to_file(const std::filesystem::path& filename) {
		std::ofstream file(filename);

		file << routes.size() << "\n";
		for (int i = 0; i < routes.size(); i++) {
			file << i + 1 << ": " << routes[i].to_string() << "\n";
		}

		file << std::fixed << std::setprecision(2) << distance << '\n';
	}

	bool operator<(const solution& s2) {
		if (routes.size() != s2.routes.size())
			return routes.size() < s2.routes.size();
		return distance < s2.distance && abs(distance - s2.distance) > 0.1; // big epsilon
	}

	// TODO: heuristically shuffling customers back and forth between different routes
	//		 The end goal is to minimise the final number of routes
};

solution best_solution;

void input_customers() {
	start_time = std::chrono::steady_clock::now();
	
	std::ifstream file(input_path);
	std::string ignore;

	std::getline(file, ignore); file.ignore();
	std::getline(file, ignore);
	file >> max_routes >> max_capacity;

	std::getline(file, ignore); file.ignore();
	std::getline(file, ignore); file.ignore();
	std::getline(file, ignore);

	int i, x, y, c, t1, t2, st;
	while (file >> i >> x >> y >> c >> t1 >> t2 >> st)
		customers.push_back({ i, x, y, c, t1, t2, st });

	depot = customers[0];

	for (int i = 0; i < customers.size(); i++) {
		for (int j = 0; j < customers.size(); j++) {
			distances[i][j] = distance({ customers[i].x, customers[i].y }, { customers[j].x, customers[j].y });
		}
	}
}

solution solve_greedy() {
	std::vector<route> routes;
	
	std::set<int> remaining_cs;
	for (const auto& c : customers)
		if (c.i > 0) remaining_cs.insert(c.i);

	double pathlen = 0;

	while (remaining_cs.size() != 0) {
		route r;
		std::set<int> cs = remaining_cs;

		while (true) {
			int best_c = -1;
			double min_fitness = 1e18;

			for (auto it = cs.begin(); it != cs.end();) {
				const auto& c = *it;

				if (r.capacity + customers[c].capacity > max_capacity) {
					it++;
					continue;
				}

				r.add_customer(c);
				auto dr = r.drive();
				if (dr.is_valid() && dr.fitness() < min_fitness) {
					min_fitness = dr.fitness();
					best_c = c;
				}

				// if we already can't fit customer C into our route, we will never be able to in the future
				if (!dr.is_valid()) {
					it = cs.erase(it);
				}
				else {
					it++;
				}

				r.remove_customer(c);
			}

			if (best_c == -1)
				break;

			r.add_customer(best_c);
			remaining_cs.erase(best_c);
			cs.erase(best_c);
		}

		pathlen += r.length();
		assert(r.is_valid());
		routes.push_back(std::move(r));
	}

	solution sol(routes, pathlen);

	std::cout
		<< "New solution: num_routes = " << sol.routes.size()
		<< ", pathlen = " << sol.distance << std::endl;

	if (best_solution.empty() || sol < best_solution) {
		std::cout << "Writing new solution to disk..." << std::endl;
		sol.to_file(output_path);
	}

	//for (const auto& r : routes) {
	//	std::cout << r.drive().capacity << " ";
	//}
	//std::cout << std::endl;

	return sol;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <input_path> <optional output_path>" << std::endl;
		return 1;
	}

	input_path = std::filesystem::path(argv[1]);

	if (argc == 3)
		output_path = std::filesystem::path(argv[2]);
	else {
		auto fname = input_path.filename().string();
		// transform instances/inst1.txt into res/inst1.txt, if the output_path was not given
		output_path = input_path.parent_path().parent_path().append("res").append(fname);
	}
	
	if (std::filesystem::exists(output_path)) {
		best_solution = solution::from_file(output_path);
		std::cerr 
			<< "Old solution: num_routes = " << best_solution.routes.size() 
			<< ", pathlen = " << best_solution.distance << std::endl;
	}

	input_customers();
	solve_greedy();

	return 0;
}