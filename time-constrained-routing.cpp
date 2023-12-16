#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <set>
#include <cassert>

// Global vars

std::chrono::time_point<std::chrono::steady_clock> start_time;
int max_routes, max_capacity;

// Helpers

typedef std::pair<int, int> pii;
#define x first
#define y second

double distance(pii a, pii b) {
	auto d = sqrt(1.0L * (a.x - b.x) * (a.x - b.x) + 1.0L * (a.y - b.y) * (a.y - b.y));
	// return ceil(d * 100) / 100.0L; // only two decimals? probably not
	return d;
}

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

	route() {
		to_visit.push_back(0); // depot
		to_visit.push_back(0);
	}

	// greedily inserts the new customer at the position which results in the best fitness
	bool add_customer(int c) {

		int best_pos = 1;
		double min_fitness = 1e18;
		
		for (int i = 1; i < to_visit.size(); i++) {
			to_visit.insert(to_visit.begin() + i, c);
			auto f = fitness();
			if (f < min_fitness) {
				min_fitness = f;
				best_pos = i;
			}
			to_visit.erase(to_visit.begin() + i);
		}

		to_visit.insert(to_visit.begin() + best_pos, c);
		return is_valid();
	}

	void remove_customer(int c) {
		to_visit.erase(std::find(to_visit.begin(), to_visit.end(), c));
	}

	struct drive_result {
		double t, pathlen, late_sum;
		int late_cnt, capacity;

		// different parameters have different weights. Most important to never be late?
		double fitness() const {
			double fitness = 0;
			fitness += t * 0.01;
			fitness += pathlen * 0.0001;
			fitness += late_cnt * 100;
			fitness += late_sum * 1;
			if (capacity > max_capacity)
				fitness += (capacity - max_capacity) * 10;
			return fitness;
		}

		bool is_valid() const {
			return late_cnt == 0 && capacity <= max_capacity;
		}
	};

	// we could permute the customer order, like TSP, to minimise fitness
	double fitness() const { return drive().fitness(); }
	double is_valid() const { return drive().is_valid(); }
	double length() const { return drive().pathlen; }

	drive_result drive() const {
		pii pos(depot.x, depot.y);
		double t = 0, pathlen = 0, late_sum = 0;
		int late_cnt = 0, capacity = 0;

		for (int cid : to_visit) {
			const auto& c = customers[cid];

			double dist = distance(pos, { c.x, c.y });
			pos = { c.x, c.y };
			t += ceil(dist);
			pathlen += dist;
			capacity += c.capacity;
			if (t < c.t1)
				t = c.t1;
			if (t > c.t2) {
				late_sum += (t - c.t2);
				late_cnt += 1;
			}
			t += c.service_time;
		}

		//std::cout << t << " " << pathlen << " " << late_sum << " " << late_cnt << " " << capacity << "\n";

		return { t, pathlen, late_sum, late_cnt, capacity };
	}

	std::string to_string() const {
		std::stringstream ss;
		pii pos(depot.x, depot.y);
		double t = 0;

		for (int i = 0; i < to_visit.size(); i++) {
			const auto& c = customers[to_visit[i]];

			double dist = distance(pos, { c.x, c.y });
			t += ceil(dist);
			pos = { c.x, c.y };

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
	std::vector<route> routes;

	// TODO: heuristically shuffling customers back and forth between different routes
	//		 The end goal is to minimise the final number of routes
};

void input() {
	start_time = std::chrono::steady_clock::now();
	// TODO: open file from cmd args? would make it easier to use Debugger
	std::string ignore;

	std::getline(std::cin, ignore); std::cin.ignore();
	std::getline(std::cin, ignore);
	std::cin >> max_routes >> max_capacity;

	std::getline(std::cin, ignore); std::cin.ignore();
	std::getline(std::cin, ignore); std::cin.ignore();
	std::getline(std::cin, ignore);

	int i, x, y, c, t1, t2, st;
	while (std::cin >> i >> x >> y >> c >> t1 >> t2 >> st)
		customers.push_back({ i, x, y, c, t1, t2, st });

	depot = customers[0];
	//customers.erase(customers.begin());

	//std::cout << ignore << " " << max_routes << " " << max_capacity << "\n";
	//std::cout << customers.size();
}

void solve() {
	std::vector<route> routes;
	
	std::set<int> cs;
	for (const auto& c : customers)
		if (c.i > 0) cs.insert(c.i);

	double pathlen = 0;

	while (cs.size() != 0) {
		route r;

		while (true) {
			int best_c = -1;
			double min_fitness = 1e18;

			for (int c : cs) {
				r.add_customer(c);
				auto f = r.fitness();
				if (r.is_valid() && f < min_fitness) {
					min_fitness = f;
					best_c = c;
				}
				r.remove_customer(c);
			}

			if (best_c == -1)
				break;

			r.add_customer(best_c);
			cs.erase(best_c);
		}

		pathlen += r.length(); // kinda slow, drives again
		assert(r.is_valid());
		routes.push_back(std::move(r));
		routes.back().drive();
	}

	std::cout << routes.size() << "\n";
	for (int i = 0; i < routes.size(); i++) {
		std::cout << i+1 << ": " << routes[i].to_string() << "\n";
	}

	// TODO: print 'pathlen' with comma, which is ugly ?
	//double tmp;
	//std::cout << int(pathlen) << ',' << floor(modf(pathlen, &tmp) * 100.0L) << '\n';

	std::cout << pathlen << '\n';
}

int main()
{
	input();
	solve();
}