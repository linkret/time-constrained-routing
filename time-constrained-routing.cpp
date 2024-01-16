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
#include <random>
#include <mutex>
#include <fstream>
#include <filesystem>

// Settings
struct {
	double allowed_gap;
	int anneal_max_iter;
	double starting_T;
	double swap_chance;
	double move_chance;
	double shorten_chance;
	bool just_greedy;
} typedef settings;


struct solution;

// Global vars

std::chrono::time_point<std::chrono::steady_clock> start_time;
int max_routes, max_capacity;

long long global_iteration_count = 0;
std::filesystem::path input_path, output_path_1m, output_path_5m, output_path_un;

// Helpers

typedef std::pair<int, int> pii;
#define x first
#define y second

double distance(pii a, pii b) {
	auto d = sqrt(1.0L * (a.x - b.x) * (a.x - b.x) + 1.0L * (a.y - b.y) * (a.y - b.y));
	return d;
}
 
const int N = 2000; // Important, code will crash if customers.size() > N
double distances[N][N]; // precompute, to avoid slow sqrt() calls in runtime
bool can_go_after[N][N];
bool can_swap[N][N];

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
		to_visit.resize(2); // depot
		//visit_time.resize(2);
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
		
		int i = 1, l = 1, r = to_visit.size() - 1;

		// optimization to drastically cut down the range
		for (int j = 1; j < to_visit.size() - 1; j++) {
			if (!can_go_after[to_visit[j]][c]) r = std::min(r, j);
			if (!can_go_after[c][to_visit[j]]) l = std::max(l, j);
		}

		i = l;
		to_visit.insert(to_visit.begin() + i, c);

		for (; i <= r; i++) {
			auto f = fitness(); // slow, visits entire route
			if (f < min_fitness) {
				min_fitness = f;
				best_pos = i;
			}
			std::swap(to_visit[i], to_visit[i + 1]);
		}
		
		to_visit.erase(to_visit.begin() + i);
		to_visit.insert(to_visit.begin() + best_pos, c);
		capacity += customers[c].capacity;
		return is_valid();
	}

	bool shorten() {
		// intra-route localsearch, greedily takes any improvement it can find
		// pairwise swaps order of customers to decrease distance
		// TODO: call drive() at the start?

		double original_dist = last_run.pathlen;
		double best_dist = last_run.pathlen; // can also try best fitness?

		bool moved = true;
		while (moved) {
			moved = false;
			// simple node-swap
			for (int i = 1; i < to_visit.size()-1; i++) {
				for (int j = i + 1; j < to_visit.size() - 1; j++) {
					if (!can_swap[to_visit[i]][to_visit[j]])
						continue;

					// can skip in advance if distances are worse, can be added in 2opt too, < 1% optimization as is

					std::swap(to_visit[i], to_visit[j]);
					auto dr = drive();
					if (dr.is_valid() && dr.pathlen < best_dist) {
						best_dist = dr.pathlen;
						moved = true;
					}
					else {
						std::swap(to_visit[i], to_visit[j]);
					}
				}
			}

			// 2-opt
			for (int i = 1; i < to_visit.size() - 1; i++) {
				for (int j = i + 1; j < to_visit.size() - 1; j++) {
					if (!can_swap[to_visit[i+1]][to_visit[j]])
						continue;

					std::reverse(to_visit.begin() + i + 1, to_visit.begin() + j + 1);
					
					auto dr = drive();
					if (dr.is_valid() && dr.pathlen < best_dist) {
						best_dist = dr.pathlen;
						moved = true;
					}
					else {
						std::reverse(to_visit.begin() + i + 1, to_visit.begin() + j + 1);
					}
				}
			}
		}

		return best_dist != original_dist;
	}

	bool remove_customer(int c) {
		auto it = std::find(to_visit.begin(), to_visit.end(), c);
		if (it != to_visit.end()) {
			capacity -= customers[c].capacity;
			to_visit.erase(it);
		}
		return is_valid();
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

		bool is_valid(bool verbose = false) const {
			if (verbose) {
				std::cout << "late_cnt = " << late_cnt << ", capacity = " << capacity << ", max_capacity = " << max_capacity << "\n";
			}
			return late_cnt == 0 && capacity <= max_capacity;
		}
	} last_run; // optimization, might cause trouble

	// we could permute the customer order, like TSP, to minimise fitness
	double fitness() { return drive().fitness(); }
	double is_valid(bool verbose = false) { return drive().is_valid(verbose); }
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

	double update_distance() {
		this->distance = 0;
		for (const auto& r : routes)
			this->distance += r.last_run.pathlen;
		return this->distance;
	}

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

	void to_file(const std::filesystem::path& filename) const {
		std::ofstream file(filename);

		file << routes.size() << "\n";
		for (int i = 0; i < routes.size(); i++) {
			file << i + 1 << ": " << routes[i].to_string() << "\n";
		}

		file << std::fixed << std::setprecision(2) << distance << '\n';

		std::ofstream file_cnt(filename.string() + "_itercnt.txt");
		file_cnt << global_iteration_count << '\n';
	}

	bool operator<(const solution& s2) const {
		if (routes.size() != s2.routes.size())
			return routes.size() < s2.routes.size();
		return distance < s2.distance && abs(distance - s2.distance) > 0.1; // big epsilon
	}

	// TODO: heuristically shuffling customers back and forth between different routes
	//		 The end goal is to minimise the final number of routes
};

solution best_solution_un, best_solution_1m, best_solution_5m;

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

	assert (customers.size() < N);

	double max_dist = 0;
	for (int i = 0; i < customers.size(); i++) {
		for (int j = 0; j < customers.size(); j++) {
			const auto& c1 = customers[i];
			const auto& c2 = customers[j];
			distances[i][j] = distance({ c1.x, c1.y }, { c2.x, c2.y });
			max_dist = std::max(max_dist, distances[i][j]);
		}
	}

	// we can additionally restrict times of customers too far away from the depot
	for (auto& c : customers) {
		c.t1 = std::max(c.t1, int(distances[0][c.i]));
		c.t2 = std::min(c.t2, int(depot.t2 - distances[c.i][0] - c.service_time));
	}

	for (int i = 0; i < customers.size(); i++) {
		for (int j = 0; j < customers.size(); j++) {
			const auto& c1 = customers[i];
			const auto& c2 = customers[j];

			if (c1.t1 + c1.service_time + distances[i][j] <= c2.t2) {
				can_go_after[i][j] = true;
			}

			//if (i && j && distances[i][j] > max_dist * 0.8) // ban too-far customers from being together
			//	can_go_after[i][j] = false;
		}
	}

	for (int i = 0; i < customers.size(); i++) {
		for (int j = 0; j < customers.size(); j++) {
			if (can_go_after[i][j] && can_go_after[j][i]) {
				can_swap[i][j] = true; // perhaps useless - rethink? only applies to customers in the same route!
			}
		}
	}
}

double random01() {
	auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
	static thread_local std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count() + thread_id);
	return std::uniform_real_distribution<double>(0, 1)(rng);
}

solution solve_greedy(const settings& sett) {
	global_iteration_count++;

	std::vector<route> routes;
	
	std::vector<int> remaining_cs;
	for (const auto& c : customers)
		if (c.i > 0) remaining_cs.push_back(c.i);

	double pathlen = 0;

	while (remaining_cs.size() != 0) {
		route r;

		int start_idx = rand() % remaining_cs.size();
		r.add_customer(remaining_cs[start_idx]);
		remaining_cs.erase(remaining_cs.begin() + start_idx);

		for (int it = 0; true; it++) { // TODO: can reduce to 1 iteration
			std::vector<int> cs = remaining_cs;

			while (true) {
				int best_c = -1;
				double min_fitness = 1e18;

				for (auto it = cs.begin(); it != cs.end();) {
					auto c = *it;

					if (r.capacity + customers[c].capacity > max_capacity) {
						it++;
						continue;
					}

					r.add_customer(c);
					auto dr = r.last_run;
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
				
				auto it = std::find(remaining_cs.begin(), remaining_cs.end(), best_c);
				if (it != remaining_cs.end()) remaining_cs.erase(it);

				it = std::find(cs.begin(), cs.end(), best_c);
				if (it != cs.end()) cs.erase(it);
			}

			double old_length = r.length();
			if (!r.shorten()) break;
			double new_length = r.length();
			//std::cout << old_length << " -> " << new_length << ", d = " << old_length - new_length << ", " << it << std::endl;
		}

		pathlen += r.length();
		assert(r.is_valid());
		routes.push_back(std::move(r));
	}

	return solution(routes, pathlen);
}

void save_solution(const solution& sol) {

	// static mutex
	static std::mutex mtx;

	std::lock_guard<std::mutex> lock(mtx);

	std::cout << "TID " << std::this_thread::get_id() << ": ";
	std::cout
		<< "New solution: num_routes = " << sol.routes.size()
		<< ", pathlen = " << sol.distance << std::endl;

	if (get_time() <= std::chrono::minutes(1) && (best_solution_1m.empty() || sol < best_solution_1m)) {
		std::cout << "!!! Writing new 1m solution to disk: " << output_path_1m.string() << std::endl;
		sol.to_file(output_path_1m);
		best_solution_1m = sol;
	}

	if (get_time() <= std::chrono::minutes(5) && (best_solution_5m.empty() || sol < best_solution_5m)) {
		std::cout << "!!! Writing new solution to disk: " << output_path_5m.string() << std::endl;
		sol.to_file(output_path_5m);
		best_solution_5m = sol;
	}

	if (best_solution_un.empty() || sol < best_solution_un) {
		std::cout << "!!! Writing new solution to disk: " << output_path_un.string() << std::endl;
		sol.to_file(output_path_un);
		best_solution_un = sol;
	}

	//for (const auto& r : routes) {
	//	std::cout << r.drive().capacity << " ";
	//}
	//std::cout << std::endl;
}

bool two_swap(solution& s0, double T) {
	int r0idx;
	int r1idx;

	do {
		r0idx = rand() % s0.routes.size();
		r1idx = rand() % s0.routes.size();
	} while (r0idx == r1idx);

	auto& r0 = s0.routes[r0idx];
	auto& r1 = s0.routes[r1idx];

	int c0idx = rand() % (r0.to_visit.size() - 2) + 1;
	int c1idx = rand() % (r1.to_visit.size() - 2) + 1;

	int c0 = r0.to_visit[c0idx];
	int c1 = r1.to_visit[c1idx];

		int new_r0c = r0.capacity + customers[c1].capacity - customers[c0].capacity;
		int new_r1c = r1.capacity + customers[c0].capacity - customers[c1].capacity;
		
		if (new_r0c > max_capacity || new_r1c > max_capacity) {
			return false;
		}
		
		double old_fitness = r0.last_run.fitness() + r1.last_run.fitness();
		
		r0.remove_customer(c0);
		r0.add_customer(c1);
		r1.remove_customer(c1);
		r1.add_customer(c0);
		s0.update_distance();

		if (!r0.last_run.is_valid() || !r1.last_run.is_valid()) {
			r0.remove_customer(c1);
			r0.add_customer(c0);
			r1.remove_customer(c0);
			r1.add_customer(c1);
			s0.update_distance();

			return false;
		}

		double new_fitness = r0.last_run.fitness() + r1.last_run.fitness();
		
		double d = new_fitness - old_fitness;
		
		if (d > 0 && exp(-d / T) < random01()) {
			r0.remove_customer(c1);
			r0.add_customer(c0);
			r1.remove_customer(c0);
			r1.add_customer(c1);
			s0.update_distance();

			return true;
		}

	return true;
}

bool move_one(solution& s0, double T) {
	int r0idx;
	int r1idx;

	do {
		r0idx = rand() % s0.routes.size();
		r1idx = rand() % s0.routes.size();
	} while (r0idx == r1idx);

	auto& r0 = s0.routes[r0idx];
	auto& r1 = s0.routes[r1idx];

		int c0idx = rand() % (r0.to_visit.size() - 2) + 1;

		double old_fitness = r0.last_run.fitness() + r1.last_run.fitness();

	int c = r0.to_visit[c0idx];

		if (r1.capacity + customers[c].capacity > max_capacity) {
			return false;
		}
		
		r0.remove_customer(c);
		r1.add_customer(c);
		s0.update_distance();
		
		if (!r0.last_run.is_valid() || !r1.last_run.is_valid()) {
			r1.remove_customer(c);
			r0.add_customer(c);
			s0.update_distance();

		return false;
	}

		double new_fitness = r0.fitness() + r1.fitness() - (r0.to_visit.size() == 2) * -1e9;

		double d = new_fitness - old_fitness;
		
		if (d > 0 && exp(-d / T) < random01()) {
			r1.remove_customer(c);
			r0.add_customer(c);
			s0.update_distance();

			return true;
		}

	// if removed all in route 0, delete route 0
	if (r0.to_visit.size() == 2) {
		s0.routes.erase(s0.routes.begin() + r0idx);
	}

	return true;
}

solution anneal(solution s0, const settings& sett) {
	const int max_iter = sett.anneal_max_iter;

	solution best = s0;

	for (int iter = 1; iter <= max_iter; ++iter) {
		global_iteration_count++;
		double T = 1. / (1. + exp(-3. + (10. * iter / max_iter)));
		T = sett.starting_T * T;

		// std::cout << "Iteration " << iter << " / " << max_iter << ", T = " << T << std::endl;

		// std::cout << "Iteration " << iter << " / " << max_iter << ", T = " << T << std::endl;

		double rand01 = random01() * (sett.swap_chance + sett.move_chance + sett.shorten_chance);

		if (rand01 < sett.swap_chance) {
			// 5 retries
			for (int i = 0; i < 5; i++)
				if (two_swap(s0, T))
					break;
			
		} else if (rand01 < sett.move_chance + sett.swap_chance) {
			// 5 retries
			for (int i = 0; i < 5; i++)
				if (move_one(s0, T))
					break;
		} else {
			for (auto& r : s0.routes) 
				r.shorten();
		}

		if (s0 < best) {
			if (s0.routes.size() != best.routes.size()) std::cout << "!!! ANNEAL ELIMINATED A ROUTE !!!" << std::endl;;
			best = s0;
		}
	}

	double pathlen = 0;
	for (auto& r : s0.routes) {
		r.shorten();
		pathlen += r.length();
	}

	s0.distance = pathlen;

	// assert all routes are valid
	for (auto& r : s0.routes) {
		assert(r.is_valid());
	}

	return s0;
}

void runner(const settings& sett) {	
	auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
	srand(time(0) + thread_id);
	
	double time_lim = 60. * 60.;
	int best_num_routes = 1e9;

	while (true) {
		if (get_time().count() / 1000.0 >= time_lim) {
			std::cout << "Time limit reached, exiting..." << std::endl;
			break;
		}

		auto sol = solve_greedy(sett);

		save_solution(sol);

		// std::cout << "Greedy solution: num_routes = " << sol.routes.size() << ", pathlen = " << sol.distance << std::endl;

		if (sett.just_greedy) {
			continue;
		}

		if (sol.routes.size() >= (1. + sett.allowed_gap) * best_num_routes) {
			continue;
		}

		sol = anneal(sol, sett);

		// std::cout << "Annealed solution: num_routes = " << sol.routes.size() << ", pathlen = " << sol.distance << std::endl;

		save_solution(sol);
		// std::cout << "Currently at " << get_time().count() / 1000.0 << "s\n";
		//break;
	}
}

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <input_path>" << std::endl;
		return 1;
	}

	input_path = std::filesystem::path(argv[1]);

	if (argc == 2) {
		auto ifname = input_path.filename().string();
		std::string icnt = ifname.substr(4, 1); // "inst7.txt" -> "7"
		
		auto fname_1m = "res-1m-i" + icnt + ".txt";
		auto fname_5m = "res-5m-i" + icnt + ".txt";
		auto fname_un = "res-un-i" + icnt + ".txt";

		output_path_1m = input_path.parent_path().parent_path().append("res").append(fname_1m);
		output_path_5m = input_path.parent_path().parent_path().append("res").append(fname_5m);
		output_path_un = input_path.parent_path().parent_path().append("res").append(fname_un);
	}
	
	if (std::filesystem::exists(output_path_1m)) {
		best_solution_1m = solution::from_file(output_path_1m);
		std::cerr 
			<< "Old solution 1m: num_routes = " << best_solution_1m.routes.size() 
			<< ", pathlen = " << best_solution_1m.distance << std::endl;
	}

	if (std::filesystem::exists(output_path_5m)) {
		best_solution_5m = solution::from_file(output_path_5m);
		std::cerr
			<< "Old solution 5m: num_routes = " << best_solution_5m.routes.size()
			<< ", pathlen = " << best_solution_5m.distance << std::endl;
	}

	if (std::filesystem::exists(output_path_un)) {
		best_solution_un = solution::from_file(output_path_un);
		std::cerr
			<< "Old solution unlimited: num_routes = " << best_solution_un.routes.size()
			<< ", pathlen = " << best_solution_un.distance << std::endl;
	}

	input_customers();

	// runner();

	const int n_threads = 8;

	std::vector<std::thread> threads(n_threads);

	// double allowed_gap;
	// int anneal_max_iter;
	// double starting_T;
	// double swap_chance;
	// double move_chance;
	// double shorten_chance;
	// bool just_greedy;

	settings setts[] = {
		{ 0.2, 100000, 100, 0.7, 0.5, 0.001, 0 },
		{ 0.1, 10000, 10, 0.5, 0.7, 0.005, 0 },
		{ 0.1, 5000, 10, 0.5, 0.5, 0.002, 0 },
		{ 0.1, 1000000, 50, 0.5, 0.5, 0.001, 0 },
		{ 0.3, 200000, 10, 0.5, 0.5, 0.001, 0 },
		{ 0.2, 2000, 10, 0.5, 0.5, 0.001, 0 },
		{ 0.1, 10000, 10, 0.3, 0.7, 0.001, 0 },
		{ 0.1, 2000, 10, 0.5, 0.5, 0.001, 1 },
	};

	for (int i = 0; i < n_threads; i++) {
		threads[i] = std::thread(runner, setts[i]);
	}

	for (auto& t : threads) {
		t.join();
	}

	return 0;
}