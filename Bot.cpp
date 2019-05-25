#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <queue>
#include <functional>
#include <utility>
#include <array>
#include <memory>
#include <unordered_set>
#include <queue>
#include <chrono>
#include <climits>
#include <float.h>
#include <assert.h>
#include <cctype>

using namespace std;

const int height = 12;
const int width = 12;
const int tower_cost = 15;
const int level_1_cost = 10;
const int level_2_cost = 20;
const int level_3_cost = 30;
const int level_1_upkeep = 1;
const int level_2_upkeep = 4;
const int level_3_upkeep = 20;

enum BuildingType
{
	HQ,
	MINE,
	TOWER
};
enum CommandType
{
	WAIT,
	MOVE,
	TRAIN,
	BUILD
};

ostream& operator<<(ostream &os, CommandType cmdType)
{
	switch (cmdType)
	{
	case WAIT:
		return os << "WAIT";
	case MOVE:
		return os << "MOVE";
	case TRAIN:
		return os << "TRAIN";
	case BUILD:
		return os << "BUILD";
	}
	return os;
}
class Stopwatch
{
public:
	Stopwatch(const string& identifier) : identifier(identifier)
	{
		start = chrono::high_resolution_clock::now();
	};

	~Stopwatch()
	{
		auto end = chrono::high_resolution_clock::now();
		chrono::duration<double> elapsed = end - start;
		long long ms = chrono::duration_cast<chrono::milliseconds>(elapsed).count();

		cerr << identifier + ": " + to_string(ms) + "ms" << endl;
	}

	string identifier;
	chrono::time_point<chrono::high_resolution_clock> start;
};
template<typename T, typename priority_t> struct MinPriorityQueue
{
	struct CompareT
	{
		bool operator()(pair<priority_t, T>& p1, pair<priority_t, T>& p2) 
		{
			return p1.first > p2.first;
		}
	};

	typedef pair<priority_t, T> PQElement;
	priority_queue<PQElement, vector<PQElement>, CompareT> elements;

	inline bool empty() const
	{
		return elements.empty();
	}

	inline void put(T item, priority_t priority)
	{
		elements.emplace(priority, item);
	}

	T pop()
	{
		T best_item = elements.top().second;
		elements.pop();
		return best_item;
	}
};
template<typename T, typename priority_t> struct MaxPriorityQueue
{
	struct CompareT
	{
		bool operator()(pair<priority_t, T>& p1, pair<priority_t, T>& p2)
		{
			return p1.first < p2.first;
		}
	};

	typedef pair<priority_t, T> PQElement;
	priority_queue<PQElement, vector<PQElement>, CompareT> elements;

	inline bool empty() const
	{
		return elements.empty();
	}

	inline void put(T item, priority_t priority)
	{
		elements.emplace(priority, item);
	}

	T pop()
	{
		T best_item = elements.top().second;
		elements.pop();
		return best_item;
	}
};

class Position
{
public:

	int x, y;

	// x is width, y is height
	Position() : x(0), y(0) {}
	Position(int x, int y) : x(x), y(y) {}
	Position(const Position& pos) : x(pos.x), y(pos.y) {}

	bool operator==(const Position& rhs) { return x == rhs.x && y == rhs.y; } const
	bool operator!=(const Position& rhs) { return x != rhs.x || y != rhs.y; } const

	inline static int distance(const Position& lhs, const Position& rhs) { return abs(lhs.x - rhs.x) + abs(lhs.y - rhs.y); }
	inline Position north_position() { return (this->y > 0)? Position(this->x, this->y - 1) : Position(*this); }
	inline Position south_position() { return (this->y < height - 1)? Position(this->x, this->y + 1) : Position(*this); }
	inline Position east_position() { return (this->x < width - 1) ? Position(this->x + 1, this->y) : Position(*this); }
	inline Position west_position() { return (this->x > 0) ? Position(this->x - 1, this->y) : Position(*this); }
	inline void debug() { cerr << "(" << x << "," << y << ")" << endl; }
	inline string print() const { return "(" + to_string(x) + "," + to_string(y) + ")"; }
};

bool operator==(const Position& lhs, const Position& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
class HashPosition
{
public:
	size_t operator()(const Position& position) const
	{
		return ((position.x + position.y) * (position.x + position.y + 1) / 2) + position.y;
	}
};
auto find_max_in_map(unordered_map<Position, double, HashPosition> map) { return max_element(map.begin(), map.end(), [](const pair<Position, double>& p1, const pair<Position, double>& p2) { return p1.second < p2.second; }); }
void print_vector_positions(const vector<Position>& positions, string tag)
{
	string str = tag + ": ";
	for (auto& position : positions)
		str += position.print() + ", ";
	cerr << str << endl;
}
void print_hashmap_positions(const unordered_map<Position, double, HashPosition>& positions, string tag)
{
	string str = tag + ": ";
	for (auto& position : positions)
		str += position.first.print() + ": " + to_string(position.second) + ", ";
	cerr << str << endl;
}
void print_hashmap_vector_positions(const unordered_map<Position, vector<Position>, HashPosition>& positions, string tag)
{
	cerr << tag + ": " << endl;
	for (auto& position_vector : positions)
	{
		string str = position_vector.first.print() + ": ";
		for (auto& position : position_vector.second)
			str += position.print() + ", ";
		cerr << str << endl;
	}
}
void print_vector_vector(vector<vector<double>> vv)
{
	for (auto& row : vv)
	{
		for (auto& cell : row)
			cerr << ((cell != -DBL_MAX) ? to_string(cell) : "#") << " ";
		cerr << endl;
	}
}
void print_vector_vector(int vv[][height])
{
	for (int i = 0; i < width; ++i)
	{
		for (int j = 0; j < width; ++j)
			cerr << vv[i][j] << " ";
		cerr << endl;
	}
}

class Objective
{
public:
	Position target;
	double score;

	Objective() : score(-DBL_MAX) {}
	Objective(double score) : score(score) {}
	Objective(Position pos, double score) : target(pos), score(score) {}
};

class Command 
{
public:

	CommandType t;
	Position p;
	int idOrLevel;
	string building;

	Command(CommandType t, int idOrLevel, const Position &p) : t(t), idOrLevel(idOrLevel), p(p) {}
	Command(CommandType t, string building, const Position &p) : t(t), idOrLevel(-1), building(building), p(p) {}

	void print() 
	{
		if (idOrLevel >= 0)
			cout << t << " " << idOrLevel << " " << p.x << " " << p.y << ";";
		else
			cout << t << " " << building << " " << p.x << " " << p.y << ";";
	}
};

class Unit 
{
public:

	int id;
	int owner;
	int level;
	Position p;
	Objective objective;

	Unit(int x, int y, int id, int level, int owner) : p(x, y), id(id), level(level), owner(owner), objective(Objective(-DBL_MAX)) {}

	inline void debug() 
	{
		cerr << "id" << id << ", lvl" << level << " on (" << p.x << "," << p.y << ") owned by " << owner << ", obj: " << " (" << objective.target.x << "," << objective.target.y << ") " << "score: " << objective.score << endl;
	}
	inline bool isOwned()
	{
		return owner == 0;
	}
	inline void set_objective(Objective obj) { this->objective = obj; }
};

class Building 
{
public:

	Position p;
	BuildingType t;
	int owner;

	Building(int x, int y, int t, int owner) : p(x, y), t(static_cast<BuildingType>(t)), owner(owner) {}
	Building(const Building& building) : p(building.p.x, building.p.y), t(building.t), owner(building.owner) {}

	inline void debug() { cerr << t << " at " << p.x << " " << p.y << " owned by " << owner << endl; }
	inline bool isHQ()
	{
		return t == HQ;
	}
	inline bool isOwned()
	{
		return owner == 0;
	}
};

class Cell
{
public:
	Position position;
	shared_ptr<Unit> unit;
	shared_ptr<Building> building;
	double mine;
	int owner;
	bool void_cell;

	Cell() : owner(0), mine(false), void_cell(false) {}
	Cell(int x, int y) : owner(0), mine(false), void_cell(false) { this->position = Position(x, y); }

	inline void set_unit(shared_ptr<Unit> unit) { this->unit = unit; }
	inline void set_building(shared_ptr<Building> building) { this->building = building; }
	inline void set_owner(int owner) { this->owner = owner; }
	inline void set_mine() { this->mine = true; }
	inline bool get_mine() { return this->mine; }
	inline void set_void_cell() { this->void_cell = true; }
	inline bool is_empty() { return !unit && !building; }
	inline bool is_occupied() { return unit || building; }
	inline bool is_occupied_by_unit() { return unit? true : false; }
	inline bool is_occupied_by_building() { return building? true : false; }
	inline bool is_occupied_by_enemy_building() { return is_occupied_by_building() && building->owner == 1; }
	inline bool is_occupied_by_inacessible_building() { return is_occupied_by_building() && building->owner == 0 && (building->t == BuildingType::HQ || building->t == BuildingType::MINE); }
	inline bool is_occupied_by_enemy_tower() { return is_occupied_by_building() && building->owner == 1 && building->t == BuildingType::TOWER; }
	inline bool is_occupied_by_ally_hq() { return is_occupied_by_building() && building->owner == 0 && building->t == BuildingType::HQ; }
	inline bool is_occupied_by_enemy_hq() { return is_occupied_by_building() && building->owner == 1 && building->t == BuildingType::HQ; }
	inline bool is_occupied_by_enemy_mine() { return is_occupied_by_building() && building->owner == 1 && building->t == BuildingType::MINE; }
	inline bool is_occupied_by_ally_mine() { return is_occupied_by_building() && building->owner == 0 && building->t == BuildingType::MINE; }
	inline bool is_occupied_by_ally_tower() { return is_occupied_by_building() && building->owner == 0 && building->t == BuildingType::TOWER; }
	inline bool is_occupied_by_mine() { return is_occupied_by_building() && building->t == BuildingType::MINE; }
	inline bool is_occupied_by_hq() { return is_occupied_by_building() && building->t == BuildingType::HQ; }
	inline bool is_occupied_by_tower() { return is_occupied_by_building() && building->t == BuildingType::TOWER; }
	inline bool is_occupied_by_enemy_unit() { return is_occupied_by_unit() && unit->owner == 1; }
	inline bool is_occupied_by_enemy_unit_of_level(int level) { return is_occupied_by_unit() && unit->owner == 1 && unit->level == level; }
	inline bool is_occupied_by_ally_unit() { return is_occupied_by_unit() && unit->owner == 0; }
	inline int level_of_enemy_unit() { return is_occupied_by_enemy_unit()? unit->level : 0; }
	inline int level_of_ally_unit() { return is_occupied_by_ally_unit() ? unit->level : 0; }
};

class Game
{
public:
	int turn;

	vector<shared_ptr<Unit>> units;
	vector<shared_ptr<Building>> buildings;
	vector<Position> mine_spots;
	vector<Command> commands;

	Position center;
	shared_ptr<Building> hq_ally;
	shared_ptr<Building> hq_enemy;

	vector<shared_ptr<Unit>> units_in_order;
	vector<shared_ptr<Unit>> units_ally;
	vector<shared_ptr<Building>> buildings_ally;

	vector<shared_ptr<Unit>> units_enemy;
	vector<shared_ptr<Building>> buildings_enemy;

	vector<Position> positions_ally;
	vector<Position> positions_enemy;

	int distances[width * height][width * height];

	unordered_map<Position, vector<Position>, HashPosition> adjacency_list;
	unordered_map<Position, vector<Position>, HashPosition> adjacency_list_position_enemy;
	unordered_map<Position, vector<Position>, HashPosition> adjacency_list_position_enemy_for_cut;
	unordered_map<Position, vector<Position>, HashPosition> adjacency_list_position_ally;
	unordered_map<Position, vector<Position>, HashPosition> adjacency_list_position_ally_for_cut;

	vector<vector<Cell>> cells;
	char cells_info[width][height]; // stores the chars representing the cell type
	int cells_used_objective[width][height]; // used for objective
	int cells_used_movement[width][height]; // used for movement
	int cells_level_ally[width][height]; // level required to move to cell
	int cells_level_enemy[width][height]; // level required to move to cell

	int gold_ally, income_ally;
	int gold_enemy, income_enemy;

	double score_ally[width][height];
	double score_enemy[width][height];
	double cuts_ally[width][height];
	double cuts_enemy[width][height];

	// Utilities
	inline Cell& get_cell(const Position& position) { return cells[position.y][position.x]; }
	inline int get_cells_used_movement(const Position& position) { return cells_used_movement[position.y][position.x]; }
	inline int get_cells_level_ally(const Position& position) { return cells_level_ally[position.y][position.x]; }
	inline int get_cells_level_enemy(const Position& position) { return cells_level_enemy[position.y][position.x]; }
	inline int get_cells_used_objective(const Position& position) { return cells_used_objective[position.y][position.x]; }
	inline char get_cell_info(const Position& position) { return cells_info[position.y][position.x]; }
	inline double get_cuts_ally(const Position& position) { return cuts_ally[position.y][position.x]; }
	inline double get_cuts_enemy(const Position& position) { return cuts_enemy[position.y][position.x]; }
	inline double get_score_enemy(const Position& position) { return score_enemy[position.y][position.x]; }
	inline double get_score_ally(const Position& position) { return score_ally[position.y][position.x]; }
	inline vector<Position>& get_adjacency_list(const Position& position) { return adjacency_list.at(position); }
	inline vector<Position>& get_adjacency_list_position_enemy(const Position& position) { return adjacency_list_position_enemy.at(position); }

	inline bool can_train_level3() { return gold_ally >= 30 && income_ally >= 20; }
	inline bool can_train_level2() { return gold_ally >= 20 && income_ally >= 4; }
	inline bool can_train_level1() { return gold_ally >= 10 && income_ally >= 1; }
	inline bool can_train_level(int level)
	{
		switch (level)
		{
		case 1: 
			return can_train_level1();
			break;
		case 2: 
			return can_train_level2();
			break;
		case 3:
			return can_train_level3();
			break;
		default:
			return false;
			break;
		}
	}
	inline int cost_of_unit(int level)
	{
		switch (level)
		{
		case 1:
			return level_1_cost;
			break;
		case 2:
			return level_2_cost;
			break;
		case 3:
			return level_3_cost;
			break;
		default:
			return 999;
			break;
		}
	}
	inline int upkeep_of_unit(int level)
	{
		switch (level)
		{
		case 1:
			return level_1_upkeep;
			break;
		case 2:
			return level_2_upkeep;
			break;
		case 3:
			return level_3_upkeep;
			break;
		default:
			return 999;
			break;
		}
	}
	inline int nbr_units_ally_of_level(int level)
	{
		int n = 0;
		for (auto& unit : units_ally)
			if (unit->level == level)
				n++;

		return n;
	}
	inline int nbr_mines_ally()
	{
		int n = 0;
		for (auto& building : buildings_ally)
			if (building->owner == 0 && building->t == BuildingType::MINE)
				n++;
		return n;
	}
	inline int nbr_towers_ally()
	{
		int n = 0;
		for (auto& building : buildings_ally)
			if (building->owner == 0 && building->t == BuildingType::TOWER)
				n++;
		return n;
	}
	inline shared_ptr<Building> getHQ()
	{
		for (auto& b : buildings)
			if (b->isHQ() && b->isOwned())
				return b;
	}
	inline shared_ptr<Building> getOpponentHQ()
	{
		for (auto &b : buildings)
			if (b->isHQ() && !b->isOwned())
				return b;
	}
	inline bool is_position_attainable(const Position& position)
	{
		if (get_cell_info(Position(position.x, position.y)) == 'O')
			return true;

		if (get_cell_info(Position(position.x, min(position.y + 1, width - 1))) == 'O')
			return true;

		if (get_cell_info(Position(position.x, max(position.y - 1, 0))) == 'O')
			return true;

		if (get_cell_info(Position(min(position.x + 1, height - 1), position.y)) == 'O')
			return true;

		if (get_cell_info(Position(max(position.x - 1, 0), position.y)) == 'O')
			return true;

		return false;
	}
	inline void refresh_gamestate_for_movement(shared_ptr<Unit> unit, const Position& destination)
	{
		cells_used_movement[destination.y][destination.x] = 1;
		income_ally += (cells_info[destination.y][destination.x] != 'O');
		cells_info[destination.y][destination.x] = 'O';
		unit->p = destination;
		update_gamestate();
	}
	inline void refresh_gamestate_for_spawn(shared_ptr<Unit> unit, const Position& destination)
	{
		cells_used_movement[destination.y][destination.x] = 1;
		income_ally += (cells_info[destination.y][destination.x] != 'O');
		gold_ally -= cost_of_unit(unit->level);
		income_ally -= upkeep_of_unit(unit->level);
		cells_info[destination.y][destination.x] = 'O';
		units.push_back(unit);
		update_gamestate();
	}
	inline void refresh_gamestate_for_building(shared_ptr<Building> building)
	{
		income_ally += 4 * (building->t == BuildingType::MINE);
		gold_ally -= tower_cost * (building->t == BuildingType::TOWER);
		buildings.push_back(building);
		update_gamestate();
	}
	inline int get_distance(const Position& pos1, const Position& pos2) { return distances[pos1.x + height * pos1.y][pos2.x + height * pos2.y]; }
	inline vector<Position> get_frontier_ally(int distance)
	{
		vector<Position> frontier;
		for (auto& position_ally : positions_ally)
		for (auto& position_enemy : positions_enemy)
			if (get_distance(position_ally, position_enemy) <= distance)
			{
				frontier.push_back(position_ally);
				break;
			}
		return frontier;
	}
	inline vector<Position> get_frontier_enemy(int distance)
	{
		vector<Position> frontier;
		for (auto& position_enemy : positions_enemy)
		for (auto& position_ally : positions_ally)
			if (get_distance(position_ally, position_enemy) <= distance)
			{
				frontier.push_back(position_enemy);
				break;
			}
		return frontier;
	}
	inline vector<Position> get_frontier_spawn_ally(int distance)
	{
		vector<Position> frontier;
		for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
		for (auto& position_ally : positions_ally)
		if (get_distance(Position(i, j), position_ally) == distance && (get_cell_info(Position(i, j)) == '.' || get_cell_info(Position(i, j)) == 'X' || get_cell_info(Position(i, j)) == 'x'))
		{
			frontier.push_back(Position(i, j));
			break;
		}
		return frontier;
	}
	inline vector<Position> get_frontier_spawn_enemy(int distance)
	{
		vector<Position> frontier;
		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
				for (auto& position_enemy : positions_enemy)
					if (get_distance(Position(i, j), position_enemy) == distance && (get_cell_info(Position(i, j)) == '.' || get_cell_info(Position(i, j)) == 'O' || get_cell_info(Position(i, j)) == 'o'))
					{
						frontier.push_back(Position(i, j));
						break;
					}
		return frontier;
	}


	// Main functions
	void debug()
	{
		Stopwatch s("Debug");

		for_each(units_ally.begin(), units_ally.end(), [](shared_ptr<Unit>& u) { u->debug(); });

		//print_vector_vector(cells_level_ally);
	}
	void init() 
	{
		int numberMineSpots;
		cin >> numberMineSpots; 
		cin.ignore();
		for (int i = 0; i < numberMineSpots; i++) 
		{
			int x;
			int y;
			cin >> x >> y; cin.ignore();
			mine_spots.push_back(Position(x, y));
		}

		center = Position(5, 5);
		turn = 0;
	}
	void update_game()
	{
		Stopwatch s("Update game");

		turn++;

		units.clear();
		buildings.clear();
		commands.clear();

		cin >> gold_ally; cin.ignore();
		cin >> income_ally; cin.ignore();

		cin >> gold_enemy; cin.ignore();
		cin >> income_enemy; cin.ignore();

		cerr << "Gold: " << gold_ally << endl;

		for (int i = 0; i < 12; i++)
		{
			string line;
			cin >> line; cin.ignore();
			for (int j = 0; j < line.size(); j++)
				cells_info[i][j] = line[j];
			cerr << line << endl;
		}

		int buildingCount;
		cin >> buildingCount; cin.ignore();
		for (int i = 0; i < buildingCount; i++)
		{
			int owner;
			int buildingType;
			int x;
			int y;
			cin >> owner >> buildingType >> x >> y; cin.ignore();
			buildings.push_back(make_shared<Building>(Building(x, y, buildingType, owner)));
		}

		int unitCount;
		cin >> unitCount; cin.ignore();
		for (int i = 0; i < unitCount; i++)
		{
			int owner;
			int unitId;
			int level;
			int x;
			int y;
			cin >> owner >> unitId >> level >> x >> y; cin.ignore();
			units.push_back(make_shared<Unit>(Unit(x, y, unitId, level, owner)));
		}

		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
				cells_used_objective[j][i] = cells_used_movement[j][i] = 0;

		if (turn <= 1)
		{
			hq_ally = getHQ();
			hq_enemy = getOpponentHQ();
			floyd_warshall();
		}
	}
	void update_gamestate()
	{
		compute_adjacency_list_enemy();

		for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
			if (cells_info[j][i] == 'X' && !find_path_to_destination(Position(i, j), hq_enemy->p))
			{
				cells_info[j][i] = 'x';
				cerr << "Inactivating cell " << Position(i, j).print() << endl;
			}

		bool repeat = true;
		while (repeat)
		{
			repeat = false;
			for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
				if (cells_info[j][i] == 'o')
				if (cells_info[min(j + 1, width)][i] == 'O' || cells_info[max(j - 1, 0)][i] == 'O' || cells_info[j][min(i + 1, height)] == 'O' || cells_info[j][max(i - 1, 0)] == 'O')
				{
					cells_info[j][i] = 'O';
					cerr << "Reactivating cell " << Position(i, j).print() << endl;
					repeat = true;
				}
		}
		

		// Cells
		cells.clear();
		cells = vector<vector<Cell>>(width, vector<Cell>(height));

		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
			{
				cells[j][i] = Cell(i, j);

				if (cells_info[j][i] == '#')
					cells[j][i].set_void_cell();
			}
			

		// Units
		units_ally.clear();
		units_ally.reserve(units.size());
		units_enemy.clear();
		units_enemy.reserve(units.size());
		for (auto& unit : units)
		{
			if (unit->isOwned())
			{
				units_ally.push_back(unit);
				cells[unit->p.y][unit->p.x].set_unit(unit);
			}
			else if (get_cell_info(unit->p) == 'X')
			{
				units_enemy.push_back(unit);
				cells[unit->p.y][unit->p.x].set_unit(unit);
			}
		}
		
		// Buildings
		buildings_ally.clear();
		buildings_ally.reserve(buildings.size());
		buildings_enemy.clear();
		buildings_enemy.reserve(buildings.size());
		for (auto& building : buildings)
		{
			if (building->isOwned())
			{
				buildings_ally.push_back(building);
				cells[building->p.y][building->p.x].set_building(building);
			}
			else if (get_cell_info(building->p) == 'X' || get_cell_info(building->p) == 'x')
			{
				buildings_enemy.push_back(building);
				cells[building->p.y][building->p.x].set_building(building);
			}
		}

		// Mines
		for (auto& mine : mine_spots)
			cells[mine.y][mine.x].set_mine();


		// Level required to move to cell
		for (auto& row : cells)
			for (auto& cell : row)
			{
				// Ally
				if (cell.is_occupied_by_ally_hq())
					cells_level_ally[cell.position.y][cell.position.x] = 9;
				else if (cell.is_occupied_by_enemy_hq())
					cells_level_ally[cell.position.y][cell.position.x] = 1;
				else if (cell.is_occupied_by_enemy_mine())
					cells_level_ally[cell.position.y][cell.position.x] = 1;
				else if (cell.is_occupied_by_ally_mine())
					cells_level_ally[cell.position.y][cell.position.x] = 9;
				else if (cell.is_occupied_by_ally_tower())
					cells_level_ally[cell.position.y][cell.position.x] = 9;
				else if (cell.is_occupied_by_enemy_unit())
					cells_level_ally[cell.position.y][cell.position.x] = min(3, cell.level_of_enemy_unit() + 1);
				else if (cell.is_occupied_by_ally_unit())
					cells_level_ally[cell.position.y][cell.position.x] = 9;
				else if (cell.void_cell)
					cells_level_ally[cell.position.y][cell.position.x] = 9;
				else
					cells_level_ally[cell.position.y][cell.position.x] = 1;

				// Enemy
				if (cell.is_occupied_by_enemy_hq())
					cells_level_enemy[cell.position.y][cell.position.x] = 9;
				else if (cell.is_occupied_by_ally_hq())
					cells_level_enemy[cell.position.y][cell.position.x] = 1;
				else if (cell.is_occupied_by_ally_mine())
					cells_level_enemy[cell.position.y][cell.position.x] = 1;
				else if (cell.is_occupied_by_enemy_mine())
					cells_level_enemy[cell.position.y][cell.position.x] = 9;
				else if (cell.is_occupied_by_enemy_tower())
					cells_level_enemy[cell.position.y][cell.position.x] = 9;
				else if (cell.is_occupied_by_ally_unit())
					cells_level_enemy[cell.position.y][cell.position.x] = min(3, cell.level_of_ally_unit() + 1);
				else if (cell.is_occupied_by_enemy_unit())
					cells_level_enemy[cell.position.y][cell.position.x] = 9;
				else if (cell.void_cell)
					cells_level_enemy[cell.position.y][cell.position.x] = 9;
				else
					cells_level_enemy[cell.position.y][cell.position.x] = 1;
			}

		for (auto& row : cells)
			for (auto& cell : row)
				if (cell.is_occupied_by_enemy_tower())
				{
					cells_level_ally[cell.position.y][cell.position.x] = 3;

					if (get_cell_info(Position(cell.position.x, min(cell.position.y + 1, width - 1))) == 'X')
						cells_level_ally[min(cell.position.y + 1, width - 1)][cell.position.x] = 3;

					if (get_cell_info(Position(cell.position.x, max(cell.position.y - 1, 0))) == 'X')
						cells_level_ally[max(cell.position.y - 1, 0)][cell.position.x] = 3;

					if (get_cell_info(Position(min(cell.position.x + 1, height - 1), cell.position.y)) == 'X')
						cells_level_ally[cell.position.y][min(cell.position.x + 1, height - 1)] = 3;

					if (get_cell_info(Position(max(cell.position.x - 1, 0), cell.position.y)) == 'X')
						cells_level_ally[cell.position.y][max(cell.position.x - 1, 0)] = 3;
				}

		for (auto& row : cells)
			for (auto& cell : row)
				if (cell.is_occupied_by_ally_tower())
				{
					cells_level_enemy[cell.position.y][cell.position.x] = 3;

					if (get_cell_info(Position(cell.position.x, min(cell.position.y + 1, width - 1))) == 'O')
						cells_level_enemy[min(cell.position.y + 1, width - 1)][cell.position.x] = 3;

					if (get_cell_info(Position(cell.position.x, max(cell.position.y - 1, 0))) == 'O')
						cells_level_enemy[max(cell.position.y - 1, 0)][cell.position.x] = 3;

					if (get_cell_info(Position(min(cell.position.x + 1, height - 1), cell.position.y)) == 'O')
						cells_level_enemy[cell.position.y][min(cell.position.x + 1, height - 1)] = 3;

					if (get_cell_info(Position(max(cell.position.x - 1, 0), cell.position.y)) == 'O')
						cells_level_enemy[cell.position.y][max(cell.position.x - 1, 0)] = 3;
				}


		// Adjacency list
		adjacency_list.clear();
		for (auto& row : cells)
			for (auto& cell : row)
			{
				vector<Position> positions;
				
				Position north_position = cell.position.north_position();
				if (!get_cell(north_position).is_occupied_by_inacessible_building() && !get_cell(north_position).void_cell && north_position != cell.position)
					positions.push_back(north_position);

				Position south_position = cell.position.south_position();
				if (!get_cell(south_position).is_occupied_by_inacessible_building() && !get_cell(south_position).void_cell && south_position != cell.position)
					positions.push_back(south_position);

				Position east_position = cell.position.east_position();
				if (!get_cell(east_position).is_occupied_by_inacessible_building() && !get_cell(east_position).void_cell && east_position != cell.position)
					positions.push_back(east_position);

				Position west_position = cell.position.west_position();
				if (!get_cell(west_position).is_occupied_by_inacessible_building() && !get_cell(west_position).void_cell && west_position != cell.position)
					positions.push_back(west_position);

				adjacency_list[cell.position] = positions;
			}
		compute_adjacency_list_enemy();
		compute_adjacency_list_ally();


		// Positions
		positions_ally.clear();
		positions_enemy.clear();
		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
			{
				if (cells_info[j][i] == 'X')
					positions_enemy.push_back(Position(i, j));
				else if (cells_info[j][i] == 'O')
					positions_ally.push_back(Position(i, j));
			}
				

		// Scores
		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
				score_ally[j][i] = score_enemy[j][i] = 0;

		for (auto& enemy : units_enemy)
			for (int i = 0; i < width; i++)
				for (int j = 0; j < height; j++)
					if (get_distance(enemy->p, Position(i, j)) <= 3 && get_cell_info(enemy->p) == 'X')
						score_enemy[j][i] += enemy->level;

		for (auto& ally : units_ally)
			for (int i = 0; i < width; i++)
				for (int j = 0; j < height; j++)
					if (get_distance(ally->p, Position(i, j)) <= 3)
						score_ally[j][i] += ally->level;
	}
	void compute_adjacency_list_enemy()
	{
		adjacency_list_position_enemy.clear();
		for (int i = 0; i < width; ++i)
		for (int j = 0; j < height; ++j)
		{
			vector<Position> positions;
			Position position(i, j);

			if (get_cell_info(position) != 'X')
				continue;

			Position north_position = position.north_position();
			if (get_cell_info(north_position) == 'X' && north_position != position)
				positions.push_back(north_position);

			Position south_position = position.south_position();
			if (get_cell_info(south_position) == 'X' && south_position != position)
				positions.push_back(south_position);

			Position east_position = position.east_position();
			if (get_cell_info(east_position) == 'X' && east_position != position)
				positions.push_back(east_position);

			Position west_position = position.west_position();
			if (get_cell_info(west_position) == 'X' && west_position != position)
				positions.push_back(west_position);

			adjacency_list_position_enemy[position] = positions;
		}
	}
	void compute_adjacency_list_enemy_for_cut()
	{
		adjacency_list_position_enemy_for_cut.clear();
		for (int i = 0; i < width; ++i)
			for (int j = 0; j < height; ++j)
			{
				vector<Position> positions;
				Position position(i, j);

				char info = get_cell_info(position);
				if (info != 'X' && info != 'x' && info != '.')
					continue;

				Position north_position = position.north_position();
				char north_info = get_cell_info(north_position);
				if ((north_info == 'X' || north_info == 'x' || north_info == '.') && north_position != position)
					positions.push_back(north_position);

				Position south_position = position.south_position();
				char south_info = get_cell_info(south_position);
				if ((south_info == 'X' || south_info == 'x' || south_info == '.') && south_position != position)
					positions.push_back(south_position);

				Position east_position = position.east_position();
				char east_info = get_cell_info(east_position);
				if ((east_info == 'X' || east_info == 'x' || east_info == '.') && east_position != position)
					positions.push_back(east_position);

				Position west_position = position.west_position();
				char west_info = get_cell_info(west_position);
				if ((west_info == 'X' || west_info == 'x' || west_info == '.') && west_position != position)
					positions.push_back(west_position);

				adjacency_list_position_enemy_for_cut[position] = positions;
			}
	}
	void compute_adjacency_list_ally_for_cut()
	{
		adjacency_list_position_ally_for_cut.clear();
		for (int i = 0; i < width; ++i)
			for (int j = 0; j < height; ++j)
			{
				vector<Position> positions;
				Position position(i, j);

				char info = get_cell_info(position);
				if (info != 'O' && info != 'o' && info != '.')
					continue;

				Position north_position = position.north_position();
				char north_info = get_cell_info(north_position);
				if ((north_info == 'O' || north_info == 'o' || north_info == '.') && north_position != position)
					positions.push_back(north_position);

				Position south_position = position.south_position();
				char south_info = get_cell_info(south_position);
				if ((south_info == 'O' || south_info == 'o' || south_info == '.') && south_position != position)
					positions.push_back(south_position);

				Position east_position = position.east_position();
				char east_info = get_cell_info(east_position);
				if ((east_info == 'O' || east_info == 'o' || east_info == '.') && east_position != position)
					positions.push_back(east_position);

				Position west_position = position.west_position();
				char west_info = get_cell_info(west_position);
				if ((west_info == 'O' || west_info == 'o' || west_info == '.') && west_position != position)
					positions.push_back(west_position);

				adjacency_list_position_ally_for_cut[position] = positions;
			}
	}
	void compute_adjacency_list_ally()
	{
		adjacency_list_position_ally.clear();
		for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
		{
			vector<Position> positions;
			Position position(i, j);

			if (cells_info[j][i] != 'O')
				continue;

			Position north_position = position.north_position();
			if (get_cell_info(north_position) == 'O' && north_position != position)
				positions.push_back(north_position);

			Position south_position = position.south_position();
			if (get_cell_info(south_position) == 'O' && south_position != position)
				positions.push_back(south_position);

			Position east_position = position.east_position();
			if (get_cell_info(east_position) == 'O' && east_position != position)
				positions.push_back(east_position);

			Position west_position = position.west_position();
			if (get_cell_info(west_position) == 'O' && west_position != position)
				positions.push_back(west_position);

			adjacency_list_position_ally[position] = positions;
		}
	}
	void send_commands()
	{
		for_each(commands.begin(), commands.end(), [](Command &c) { c.print(); });
		cout << "WAIT" << endl;
	}


	// Buildings
	void build_towers()
	{
		Stopwatch s("Towers");

		//int n = nbr_towers_ally();

		//if (n >= 4)
		//	return;

		compute_adjacency_list_ally_for_cut();
		
		double cuts[width][height] = {};
		for (auto& position : get_frontier_spawn_enemy(1))
		{
			auto pair = search({ position }, 4, false);

			if (pair.first > 0.0)
			{
				string s1 = "";
				for (auto& p : pair.second)
					s1 += p.print() + ", ";
				cerr << "Chain against me: " << s1 << "Score: " << pair.first << " Cost:" << get_cut_cost(pair.second, false) << endl;

				for (auto& position : pair.second)
					for (int i = 0; i < width; i++)
					for (int j = 0; j < height; j++)
						if (get_distance(Position(i, j), position) <= 1 && get_cell_info(Position(i, j)) == 'O')
							cuts[j][i] += pair.first;
			}
		}

		double scores[width][height] = {};
		for (auto& position : get_frontier_ally(3))
		{
			//Position position = Position(i, j);
			int i = position.x;
			int j = position.y;

			if (get_cell_info(position) == 'O' && !get_cell(position).is_occupied() && !get_cell(position).get_mine())
			{
				double score = max(3.0 - get_cells_level_enemy(position), 0.0) * 10.0;

				Position north_position = position.north_position();
				if (get_cell_info(north_position) == 'O' && !get_cell(north_position).void_cell && north_position != position)
					score += max(3.0 - get_cells_level_enemy(north_position), 0.0) * 10.0;

				Position south_position = position.south_position();
				if (get_cell_info(south_position) == 'O' && !get_cell(south_position).void_cell && south_position != position)
					score += max(3.0 - get_cells_level_enemy(south_position), 0.0) * 10.0;

				Position east_position = position.east_position();
				if (get_cell_info(east_position) == 'O' && !get_cell(east_position).void_cell && east_position != position)
					score += max(3.0 - get_cells_level_enemy(east_position), 0.0) * 10.0;

				Position west_position = position.west_position();
				if (get_cell_info(west_position) == 'O' && !get_cell(west_position).void_cell && west_position != position)
					score += max(3.0 - get_cells_level_enemy(west_position), 0.0) * 10.0;

				score += cuts[j][i] * 5.0;

				// assume we are on the offensive then, no need for towers
				if (get_distance(position, hq_ally->p) > 13)
					score = 0.0;

				scores[j][i] = score;
			}
			else
				scores[j][i] = -DBL_MAX;
		}

		Position max_position = hq_ally->p;
		double max_score = -DBL_MAX;

		for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
			if (scores[j][i] > max_score)
			{
				max_score = scores[j][i];
				max_position = Position(i, j);
			}

		cerr << "Best tower cell: " << max_position.print() << " score: " << max_score << endl;

		bool close_to_enemy = false;
		for (auto& position_ally : positions_ally)
			for (auto& position_enemy : positions_enemy)
				if (get_distance(position_ally, position_enemy) <= 2)
				{
					close_to_enemy = true;
					goto can_spawn_tower;
				}
		can_spawn_tower:
		if (!close_to_enemy)
			return;

		if (gold_ally >= tower_cost && max_score > 60.0)
		{
			commands.push_back(Command(BUILD, "TOWER", max_position));
			refresh_gamestate_for_building(make_shared<Building>(Building(max_position.x, max_position.y, TOWER, 0)));
		}
	}


	// Training new units
	double get_training_score(const shared_ptr<Unit>& unit, const Position& pos)
	{
		if (unit->level < get_cells_level_ally(pos) || cells_used_objective[pos.y][pos.x] || cells_used_movement[pos.y][pos.x])
			// not objective if not enough level, cell is already used, cell is already owned
			return -DBL_MAX;
		else
		{
			int distance_to_hq_ally = get_distance(pos, hq_ally->p);
			int distance_to_enemy_hq = get_distance(pos, hq_enemy->p);

			bool enemy_on_cell = get_cell(pos).is_occupied_by_enemy_unit();
			bool enemy_building_on_cell = get_cell(pos).is_occupied_by_enemy_building();
			bool enemy_territory = get_cell_info(pos) == 'X';
			bool is_ally_territory = cells_info[pos.y][pos.x] == 'O';

			double score = 0.0;

			score += (turn <= 3) ? -distance_to_hq_ally * 10.0 : distance_to_hq_ally;
			score -= distance_to_enemy_hq;
			score += enemy_on_cell * 20.0;
			score += enemy_building_on_cell * 20.0;
			score += enemy_territory * 10.0;
			score += is_ally_territory ? -100.0 : 0.0;

			return score;
		}
	}
	unordered_map<Position, double, HashPosition> find_training_positions(int level)
	{
		vector<Position> positions_available;

		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
			{
				Position pos = Position(i, j);

				bool enemy_lower_level_or_none = (level >= 2) ? get_cell(pos).is_occupied_by_enemy_unit_of_level(level - 1) : true;

				if (get_cells_level_ally(pos) > level || get_cells_used_movement(pos) || !enemy_lower_level_or_none)
					continue;

				bool attainable = false;
				for (int k = 0; k < width; k++)
					for (int l = 0; l < height; l++)
						if (get_distance(pos, Position(k, l)) == 1 && get_cell_info(Position(k, l)) == 'O')
						{
							attainable = true;
							break;
						}

				if (attainable)
					positions_available.push_back(pos);
			};

		//string s1 = "can spawn to:";
		//for (auto& p : positions_available)
		//	s1 += p.print() + ", ";

		//cerr << s1 << endl;

		unordered_map<Position, double, HashPosition> positions_for_spawn;
		for (auto& pos : positions_available)
			if (level == 1)
				positions_for_spawn[pos] = get_training_score(make_shared<Unit>(Unit(pos.x, pos.y, 999, level, 0)), pos);
			else if (level == 2)
				positions_for_spawn[pos] = get_score_enemy(pos);

		return positions_for_spawn;
	}
	inline bool need_train_units(int level) { return nbr_units_ally_of_level(level) <= ((level == 1) ? 8 : 0); }
	void train_units()
	{
		for (int level : {2, 1})
		{
			Stopwatch s("Train units of level" + to_string(level));

			while (need_train_units(level) && can_train_level(level))
			{
				unordered_map<Position, double, HashPosition> training_positions = find_training_positions(level);

				if (training_positions.size())
				{
					Position training_position = find_max_in_map(training_positions)->first;
					refresh_gamestate_for_spawn(make_shared<Unit>(Unit(training_position.x, training_position.y, 999, level, 0)), training_position);
					commands.push_back(Command(TRAIN, level, training_position));
					cerr << "Training level" << level << " on " << training_position.print() << endl;
				}
				else
					break;
			}
		}
	}


	// Pathing
	void move_units()
	{
		Stopwatch s("Generate Moves");

		fill_cuts_for_move();
		assign_objective_to_units();

		for (auto& unit : units_in_order)
		{
			Position destination = get_path(unit, unit->objective.target, false);
			
			cerr << "Path for: " << unit->id << ", want to move to " << destination.print() << endl;

			if (unit_can_move_to_destination(unit, destination))
			{
				commands.push_back(Command(MOVE, unit->id, destination));
				refresh_gamestate_for_movement(unit, destination);
			}
		}
	}
	void fill_cuts_for_move()
	{
		MaxPriorityQueue<Position, double> cuts = find_cuts(true);

		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
				cuts_ally[j][i] = -1.0;

		while (!cuts.empty())
		{
			cuts_ally[cuts.elements.top().second.y][cuts.elements.top().second.x] = cuts.elements.top().first;
			cuts.elements.pop();
		}

		cuts = find_cuts(false);

		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
				cuts_enemy[j][i] = -1.0;

		while (!cuts.empty())
		{
			cuts_enemy[cuts.elements.top().second.y][cuts.elements.top().second.x] = cuts.elements.top().first;
			cuts.elements.pop();
		}

		//cerr << "My cuts:" << endl;
		//for (auto& row : cuts_enemy)
		//{
		//	for (auto& cell : row)
		//		cerr << cell;
		//	cerr << endl;
		//}
	}
	bool unit_can_move_to_destination(const shared_ptr<Unit>& unit, const Position& target)
	{
		return (
			get_cells_level_ally(target) <= unit->level &&
			get_distance(target, unit->p) <= 1 &&
			!get_cell(target).is_occupied_by_inacessible_building() &&
			!get_cell(target).is_occupied_by_ally_unit()
			);
	}
	Position get_path(const shared_ptr<Unit>& unit, Position target, bool debug)
	{
		if (unit->p == target || (get_distance(unit->p, target) == 1))
			return target;

		vector<Position> optimal_path = dijkstra(unit, target, debug);

		if (optimal_path.size() > 1)
			return optimal_path.at(1);
		else
			return unit->p;
	}
	double compute_next_step_score(const shared_ptr<Unit>& unit, const Position& target, const Position& current, const Position& next)
	{
		double score = 1.0;

		// Too low level to move to position
		//if (unit->level < get_cells_level(next))
		//	score += 1000.0;

		if (get_cell_info(next) == '#')
			score += 1000.0;

		// Someone already moving there
		if (get_cells_used_movement(next))
			score += 1000.0;

		return score;
	}
	vector<Position> dijkstra(const shared_ptr<Unit>& unit, const Position& target, bool debug)
	{
		unordered_map<Position, Position, HashPosition> came_from;
		came_from[unit->p] = unit->p;

		MinPriorityQueue<Position, double> frontier;
		frontier.put(unit->p, 0.0);

		unordered_map<Position, double, HashPosition> cost_so_far;
		cost_so_far[unit->p] = 0.0;

		while (!frontier.empty())
		{
			Position current = frontier.pop();

			//if (debug)
			//{
			//	cerr << "current: " << current.print() << " ";
			//	auto tt = get_adjacency_list(current);
			//	cerr << tt.size();
			//	for (auto& t : tt)
			//		cerr << t.print();
			//}

			if (current == target)
				return reconstruct_path(unit->p, target, came_from);

			for (const Position& next : get_adjacency_list(current))
			{
				double new_cost = cost_so_far[current] + compute_next_step_score(unit, target, current, next);

				//if (debug)
				//	cerr << "next: " << next.print() << " s: " << new_cost << ", ";

				if ((cost_so_far.find(next) == cost_so_far.end()) || (new_cost < cost_so_far[next]))
				{
					cost_so_far[next] = new_cost;
					came_from[next] = current;
					frontier.put(next, new_cost);
				}

				//if (debug)
				//	cerr << endl;
			}
		}

		return reconstruct_path(unit->p, target, came_from);
	}
	vector<Position> reconstruct_path(Position source, Position target, unordered_map<Position, Position, HashPosition> came_from)
	{
		vector<Position> path;
		Position current = target;
		while (current != source)
		{
			path.push_back(current);
			current = came_from[current];
		}
		path.push_back(source);
		reverse(path.begin(), path.end());
		return path;
	}
	Objective find_target(const shared_ptr<Unit>& unit)
	{
		Position target = hq_enemy->p;
		double max_score = -DBL_MAX;

		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
			{
				Position pos(i, j);

				double score = get_score(unit, pos);

				if (score > max_score)
				{
					target = pos;
					max_score = score;
				}
			}

		return Objective(target, max_score);
	}
	double get_score(const shared_ptr<Unit>& unit, const Position& pos)
	{
		if (unit->level < get_cells_level_ally(pos) || cells_used_objective[pos.y][pos.x] || cells_used_movement[pos.y][pos.x] || cells_info[pos.y][pos.x] == 'O')
			// not objective if not enough level, cell is already used, cell is already owned
			return -DBL_MAX;
		else
		{
			int distance_to_hq_ally = get_distance(pos, hq_ally->p);
			int distance_to_enemy_hq = get_distance(pos, hq_enemy->p);
			int distance = get_distance(unit->p, pos);

			bool enemy_on_cell = get_cell(pos).is_occupied_by_enemy_unit();
			bool enemy_building_on_cell = get_cell(pos).is_occupied_by_enemy_building();
			bool enemy_territory = get_cell_info(pos) == 'X';
			bool enemy_territory_inactive = get_cell_info(pos) == 'x';
			bool is_empty = (get_cell_info(pos) == '.');
			bool cut_ally_distance_one = get_cuts_ally(pos) > 0 && distance <= 1;
			bool cut_enemy_distance_one = get_cuts_enemy(pos) > 0 && distance <= 1;

			double score = 0.0;

			// more weight if going into cluster of allies or where less weight?

			score += enemy_on_cell * 25.0 / distance;
			score += enemy_building_on_cell * 20.0 / distance;
			score += enemy_territory * 15.0 / distance;
			score += enemy_territory_inactive * 12.5 / distance;
			score += is_empty * 10.0 / distance;

			score += cut_ally_distance_one * get_cuts_ally(pos) * 10.0;
			score += cut_enemy_distance_one * get_cuts_enemy(pos) * 8.0;

			score -= distance_to_enemy_hq;
			score -= distance;

			return score;
		}
	}
	void assign_objective_to_units() 
	{
		Stopwatch s("Assign objective to units");

		units_in_order.clear();

		set<shared_ptr<Unit>> units;
		for (auto& unit : units_ally)
			units.insert(unit);

		while (units.size())
		{
			shared_ptr<Unit> best_unit = *units.begin();
			Objective best_objective = Objective(-DBL_MAX);

			for (const auto& unit : units)
			{
				Objective objective = find_target(unit);

				if (objective.score > best_objective.score)
				{
					best_unit = unit;
					best_objective = objective;
				}
			}

			//best_unit->debug();
			//best_objective.target.debug();
			//cerr << endl;

			if (best_objective.score > -DBL_MAX)
			{
				cells_used_objective[best_objective.target.y][best_objective.target.x] = 1;
				best_unit->set_objective(best_objective);
				units_in_order.push_back(best_unit);
			}

			units.erase(best_unit);
		}
	}


	// Cuts
	void train_units_on_cuts()
	{
		Stopwatch s("Train on cuts");

		MaxPriorityQueue<Position, double> cuts = find_cuts(true);

		while (!cuts.empty())
		{
			Position cut = cuts.elements.top().second;
			double gain = cuts.elements.top().first;
			int level_required = get_cells_level_ally(cut);
			double cost = (double)(level_required * 10);
			double score = gain - cost;

			cerr << "Cut: " << cut.print() << ", gain " << gain << ", cost: " << cost << ", score: " << score << endl;

			if (score > 0.0 && level_required <= 3 && can_train_level(level_required))
			{
				commands.push_back(Command(TRAIN, level_required, cut));
				refresh_gamestate_for_spawn(make_shared<Unit>(Unit(cut.x, cut.y, 999, level_required, 0)), cut);
			}

			cuts.elements.pop();
		}
	}
	MaxPriorityQueue<Position, double> find_cuts(bool find_enemies)
	{
		vector<Position> attainable_articulation_points = get_attainable_articulation_points(find_enemies);
		unordered_map<Position, vector<Position>, HashPosition>& adjacency_list_positions = find_enemies ? adjacency_list_position_enemy : adjacency_list_position_ally;

		//string str = "Articulation Points: ";
		//for (auto& attainable_articulation_point : attainable_articulation_points)
		//	str += attainable_articulation_point.print() + " ";
		//cerr << str << endl;

		MaxPriorityQueue<Position, double> scores;
		for (auto& articulation_point : attainable_articulation_points)
		{
			double score = 0;
			for (auto& neighbor : adjacency_list_positions[articulation_point])
			{
				vector<Position> graph = find_graph_from_source(neighbor, articulation_point, find_enemies);
				score += score_graph(graph);

				//string string1 = "graph: ";
				//for (auto& pos : graph)
				//	string1 += pos.print() + ", ";
				//cerr << string1 << endl;
			}
			score += score_graph({ articulation_point });
			scores.put(articulation_point, score);
		}

		//string string2 = "Cuts: ";
		//while (!scores.empty())
		//{
		//	string2 += scores.elements.top().second.print() + ": " + to_string(scores.elements.top().first) + ", ";
		//	scores.elements.pop();
		//}
		//cerr << string2;

		return scores;
	}
	double score_graph(const vector<Position>& positions)
	{
		double score = 0.0;
		for (auto& position : positions)
		{
			Cell& cell = get_cell(position);

			if (cell.is_occupied_by_unit())
				score += cell.unit->level * 10.0;
			else if (cell.is_occupied_by_mine())
				score += 4.0;
			else if (cell.is_occupied_by_tower())
				score += 15.0;

			score += 1.0;
		}

		return score;
	}
	vector<Position> find_graph_from_source(const Position& source, const Position& forbidden, bool find_enemies)
	{
		unordered_map<Position, vector<Position>, HashPosition>& adj_list = find_enemies ? adjacency_list_position_enemy : adjacency_list_position_ally;
		Position hq = find_enemies ? hq_enemy->p : hq_ally->p;

		unordered_map<Position, bool, HashPosition> visited;
		visited[source] = true;

		queue<Position> frontier;
		frontier.push(source);

		vector<Position> graph;
		graph.push_back(source);

		while (!frontier.empty())
		{
			Position current = frontier.front();
			frontier.pop();

			if (current == hq)
				return vector<Position>();

			for (const Position& next : adj_list[current])
			{
				if (!visited.count(next) && !(next == forbidden))
				{
					visited[next] = true;
					frontier.push(next);
					graph.push_back(next);
				}
			}
		}

		return graph;
	}
	vector<Position> get_attainable_articulation_points(bool find_enemies)
	{
		vector<Position> articulation_points = get_articulation_points(find_enemies);
		vector<Position> attainable_articulation_points;

		for (auto& articulation_point : articulation_points)
		{
			if (find_enemies)
			{
				for (auto& position : positions_ally)
					if (get_distance(articulation_point, position) <= 1)
					{
						attainable_articulation_points.push_back(articulation_point);
						break;
					}
			}
			else
			{
				for (auto& position : positions_enemy)
					if (get_distance(articulation_point, position) <= 1)
					{
						attainable_articulation_points.push_back(articulation_point);
						break;
					}
			}
		}

		return attainable_articulation_points;
	}
	vector<Position> get_articulation_points(bool find_enemies)
	{
		unordered_map<Position, bool, HashPosition> visited;
		unordered_map<Position, int, HashPosition> disc;
		unordered_map<Position, int, HashPosition> low;
		unordered_map<Position, Position, HashPosition> parent;
		unordered_map<Position, bool, HashPosition> ap;

		vector<Position> positions = find_enemies ? positions_enemy : positions_ally;
		unordered_map<Position, vector<Position>, HashPosition> adj_list = find_enemies ? adjacency_list_position_enemy : adjacency_list_position_ally;

		for (auto& pos : positions)
		{
			parent[pos] = Position(-1, -1);
			visited[pos] = false;
			ap[pos] = false;
		}

		for (auto& pos : positions)
			if (!visited[pos])
				articulation_point_inner(pos, visited, disc, low, parent, ap, adj_list);

		vector<Position> articulation_points;
		for (auto& a : ap)
			if (a.second)
				articulation_points.push_back(a.first);

		return articulation_points;
	}
	void articulation_point_inner(
		Position position, 
		unordered_map<Position, bool, HashPosition>& visited,
		unordered_map<Position, int, HashPosition>& disc,
		unordered_map<Position, int, HashPosition>& low,
		unordered_map<Position, Position, HashPosition>& parent,
		unordered_map<Position, bool, HashPosition>& ap,
		unordered_map<Position, vector<Position>, HashPosition>& adjacency_list_ap
	)
	{
		static int time = 0;
		int children = 0; // Count of children in DFS Tree 
		visited[position] = true;
		disc[position] = low[position] = ++time;

		for (auto& next_position : adjacency_list_ap[position])
		{
			if (!visited[next_position])
			{
				children++;
				parent[next_position] = position;
				articulation_point_inner(next_position, visited, disc, low, parent, ap, adjacency_list_ap);

				// (1) u is root of DFS tree and has two or more chilren. 
				if (parent[position] == Position(-1, -1) && children > 1)
					ap[position] = true;

				// (2) If u is not root and low value of one of its child is more than discovery value of u. 
				if (parent[position] != Position(-1, -1) && low[next_position] >= disc[position])
					ap[position] = true;

				// Check if the subtree rooted with next_position has a connection to one of the ancestors of position 
				low[position] = min(low[position], low[next_position]);
			}
			// Update low value of u for parent function calls.
			else if (next_position != parent[position])
			{
				low[position] = min(low[position], disc[next_position]);
			}
		}
	}


	// Chainkill
	void attempt_chainkill()
	{
		Stopwatch s("Chainkills");

		unordered_map<Position, double, HashPosition> chainkills = dijkstra_chainkill_all_costs(hq_enemy->p);

		if (!chainkills.size())
			return;

		//for (auto& t : chainkills)
		//{
		//	string s1 = "";
		//	s1 += t.first.print() + ": " + to_string(t.second);
		//	cerr << s1 << endl;
		//}

		double chainkill_cost = DBL_MAX;
		Position chainkill_start;
		for (auto& position : chainkills)
		{
			if (position.second < chainkill_cost && is_position_attainable(position.first))
			{
				chainkill_cost = position.second;
				chainkill_start = position.first;
			}
		}

		cerr << "Chainkill start: " << chainkill_start.print() << " cost: " << chainkill_cost << endl;

		if (chainkill_cost <= gold_ally)
		{
			vector<Position> chainkill_path = dijkstra_chainkill_path(chainkill_start, hq_enemy->p);

			//string s1;
			//for (auto& t : chainkill_path)
			//	s1 += t.print() + ", ";

			//cerr << s1 << endl;

			double score = get_cut_cost(chainkill_path, true);

			if (score >= gold_ally)
				return;

			for (auto& t : chainkill_path)
				commands.push_back(Command(TRAIN, get_cells_level_ally(t), t));

			cerr << "WILL CHAINKILL!" << endl;
		}
	}
	unordered_map<Position, double, HashPosition> dijkstra_chainkill_all_costs(const Position& source)
	{
		MinPriorityQueue<Position, double> frontier;
		frontier.put(source, 0.0);

		unordered_map<Position, double, HashPosition> cost_so_far;
		cost_so_far[source] = 10.0;

		while (!frontier.empty())
		{
			Position current = frontier.pop();

			for (const Position& next : get_adjacency_list(current))
			if (get_cell_info(next) != 'O')
			{
				double new_cost = cost_so_far[current] + get_cells_level_ally(next) * 10.0;

				if ((cost_so_far.find(next) == cost_so_far.end()) || (new_cost < cost_so_far[next]))
				{
					cost_so_far[next] = new_cost;
					frontier.put(next, new_cost);
				}
			}
		}

		return cost_so_far;
	}
	vector<Position> dijkstra_chainkill_path(const Position& source, const Position& target)
	{
		MinPriorityQueue<Position, double> frontier;
		frontier.put(source, 0.0);

		unordered_map<Position, double, HashPosition> cost_so_far;
		cost_so_far[source] = 10.0;

		unordered_map<Position, Position, HashPosition> came_from;
		came_from[source] = source;

		while (!frontier.empty())
		{
			Position current = frontier.pop();

			for (const Position& next : get_adjacency_list(current))
				if (get_cell_info(next) != 'O')
				{
					double new_cost = cost_so_far[current] + get_cells_level_ally(next) * 10.0;

					if ((cost_so_far.find(next) == cost_so_far.end()) || (new_cost < cost_so_far[next]))
					{
						cost_so_far[next] = new_cost;
						frontier.put(next, new_cost);
						came_from[next] = current;
					}
				}
		}

		return reconstruct_path(source, target, came_from);
	}
	inline double get_cut_cost(const vector<Position> cut, bool my_pov)
	{
		double cost = 0.0;

		//bool just_captured_tower = false;
		//for (auto& position : cut)
		//{
		//	if (just_captured_tower && !get_cell(position).is_occupied_by_enemy_tower())
		//	{
		//		cost += (get_cell(position).is)
		//		just_captured_tower = false;
		//	}
		//	else
		//		cost += get_cells_level_ally(position) * 10.0;

		//	if (get_cell(position).is_occupied_by_enemy_tower())
		//		just_captured_tower = true;
		//	else
		//		just_captured_tower = false;
		//}

		if (my_pov)
		{
			for (auto& position : cut)
				cost += get_cells_level_ally(position) * 10.0;
		}
		else
		{
			for (auto& position : cut)
				cost += get_cells_level_enemy(position) * 10.0; 
		}
		
		return cost;
	}


	// Distances
	void floyd_warshall()
	{
		Stopwatch s("All distance");

		const int dim = width * height;

		for (int i = 0; i < dim; i++)
		for (int j = 0; j < dim; j++)
		{
			Position pos1 = Position(i % width, i / width);
			Position pos2 = Position(j % width, j / width);

			int distance = INT_MAX;
			if (Position::distance(pos1, pos2) == 1 && get_cell_info(pos1) != '#' && get_cell_info(pos2) != '#')
				distance = 1;
			else if (Position::distance(pos1, pos2) == 0)
				distance = 0;

			distances[i][j] = distance;
		}

		for (int k = 0; k < dim; ++k)
			for (int i = 0; i < dim; ++i)
				for (int j = 0; j <= i; ++j)
					if (distances[i][k] != INT_MAX && distances[k][j] != INT_MAX && distances[i][k] + distances[k][j] < distances[i][j])
						distances[j][i] = distances[i][j] = distances[i][k] + distances[k][j];
		
		//cerr << "Distances" << endl;

		//for (auto& row : distances)
		//{
		//	for (auto& cell : row)
		//		cerr << ((cell < INT_MAX) ? to_string(cell) : "#") << " ";

		//	cerr << endl;
		//}
	}


	// Simulation
	void search_cuts()
	{
		Stopwatch s("Find cuts");

		bool need_refresh = true;
		unordered_map<shared_ptr<vector<Position>>, double> cuts;
		while(true)
		{
			if (need_refresh)
			{
				compute_adjacency_list_enemy_for_cut();

				cuts.clear();
				for (auto& position : get_frontier_spawn_ally(1))
				{
					auto pair = search({ position }, 4, true);

					if (pair.first > 0.0)
						cuts.emplace(make_shared<vector<Position>>(pair.second), pair.first);
				}

				if (cuts.empty())
					return;

				need_refresh = false;
			}
			
			auto best_cut = max_element(cuts.begin(), cuts.end(), [](const pair<shared_ptr<vector<Position>>, double>& p1, const pair<shared_ptr<vector<Position>>, double>& p2) { return p1.second < p2.second; });
			vector<Position> positions = *(best_cut->first);
			double score = best_cut->second;
			double cost = get_cut_cost(positions, true);

			//for (auto& t : cuts)
			//{
			//	string s1 = "";
			//	for (auto& p : *(t.first))
			//		s1 += p.print() + ", ";
			//	cerr << "Chain: " << s1 << "Score: " << t.second << " Cost:" << get_cut_cost(*(t.first)) << endl;
			//}

			if (score < 0.0)
				return;

			if (cost <= (double)gold_ally)
			{
				string s1 = "";
				for (auto& t : positions)
					s1 += t.print() + ", ";
				cerr << "CUTTING! " << s1 << "Score: " << score << " Cost:" << cost << endl;

				for (auto& position : positions)
				{
					int level_required = get_cells_level_ally(position);
					commands.push_back(Command(TRAIN, level_required, position));
					refresh_gamestate_for_spawn(make_shared<Unit>(Unit(position.x, position.y, 999, level_required, 0)), position);
				}
				need_refresh = true;
			}

			cuts.erase(best_cut);
		}
	}
	pair<double, vector<Position>> search(const vector<Position>& forbidden, int depth, bool my_pov)
	{
		auto& adj_list = my_pov ? adjacency_list_position_enemy_for_cut : adjacency_list_position_ally_for_cut;

		if (depth > 0)
		{
			double max_score = score_cut(forbidden, my_pov);
			vector<Position> max_cut = forbidden;

			for (auto& child : adj_list[forbidden.back()])
			if(find(forbidden.begin(), forbidden.end(), child) == forbidden.end())
			{
				vector<Position> new_forbidden = forbidden;
				new_forbidden.push_back(child);

				auto pair = search(new_forbidden, depth - 1, my_pov);

				if (pair.first > max_score)
				{
					max_score = pair.first;
					max_cut = pair.second;
				}
			}

			return make_pair(max_score, max_cut);
		}
		else
			return make_pair(score_cut(forbidden, my_pov), forbidden);
	}
	double score_cut(const vector<Position>& forbidden, bool my_pov)
	{
		if (!is_valid_cut(forbidden))
			return -DBL_MAX;

		double cut_cost = get_cut_cost(forbidden, my_pov);

		if (cut_cost > (my_pov ? gold_ally : gold_enemy + income_enemy))
			return -DBL_MAX;

		vector<Position> tree_from_hq = graph_with_excluded_nodes_including(my_pov ? hq_enemy->p : hq_ally->p, forbidden, my_pov);

		vector<Position>& positions = (my_pov ? positions_enemy : positions_ally);
		if (tree_from_hq.size() != positions.size())
		{
			double cut_gain = 0.0;
			unordered_set<Position, HashPosition> positions_to_check(positions.begin(), positions.end());
			while (positions_to_check.size())
			{
				Position position = *(positions_to_check.begin());
				positions_to_check.erase(position);

				if (find(tree_from_hq.begin(), tree_from_hq.end(), position) == tree_from_hq.end())
				{
					vector<Position> cut_graph = graph_with_excluded_nodes(position, forbidden, my_pov);
					cut_gain += score_graph(cut_graph);

					//string s0 = "cut graph, gain" + to_string(cut_gain) + ", ";
					//for (auto& t : cut_graph)
					//	s0 += t.print() + ", ";
					//cerr << s0 << endl;

					for (auto& position_to_remove : cut_graph)
						positions_to_check.erase(position_to_remove);
				}
			}

			cut_gain += score_graph(forbidden);

			//string s1 = "";
			//for (auto& t : forbidden)
			//	s1 += t.print() + ", ";
			//cerr << s1 << " gain: " << cut_gain << " cost: " << cut_cost << " score: " << cut_gain - cut_cost << endl;
			//string s2 = "Tree from enemy HQ";
			//for (auto& t : tree_from_hq)
			//	s2 += t.print() + ", ";
			//cerr << "size: " << tree_from_hq.size() << " total: " << positions_ally.size() << " " << s2 << endl;
			//string s3 = "";
			//for (auto& t : forbidden)
			//	s3 += t.print() + ", ";
			//cerr << s3 << " gain: " << cut_gain << " cost: " << cut_cost << " score: " << cut_gain - cut_cost << endl;

			return cut_gain - cut_cost;
		}
		else
			return -DBL_MAX;
	}
	vector<Position> graph_with_excluded_nodes(const Position& source, const vector<Position>& forbidden, bool my_pov)
	{
		auto& adj_list = my_pov ? adjacency_list_position_enemy : adjacency_list_position_ally;

		bool visited[width][height] = {};
		visited[source.y][source.x] = true;

		queue<Position> frontier;
		frontier.push(source);

		vector<Position> graph;
		graph.reserve(width * height);
		graph.push_back(source);

		while (!frontier.empty())
		{
			Position current = frontier.front();
			frontier.pop();

			for (const Position& next : adj_list[current])
			{
				if (!visited[next.y][next.x] && (find(forbidden.begin(), forbidden.end(), next) == forbidden.end()))
				{
					visited[next.y][next.x] = true;
					frontier.push(next);
					graph.push_back(next);
				}
			}
		}

		return graph;
	}
	vector<Position> graph_with_excluded_nodes_including(const Position& source, const vector<Position>& forbidden, bool my_pov)
	{
		auto& adj_list = my_pov ? adjacency_list_position_enemy : adjacency_list_position_ally;

		bool visited[width][height] = {};
		visited[source.y][source.x] = true;

		queue<Position> frontier;
		frontier.push(source);

		vector<Position> graph;
		graph.reserve(width * height);
		graph.push_back(source);

		while (!frontier.empty())
		{
			Position current = frontier.front();
			frontier.pop();

			for (const Position& next : adj_list[current])
			{
				if (!visited[next.y][next.x])
				{
					bool next_in_forbidden = find(forbidden.begin(), forbidden.end(), next) != forbidden.end();
					bool current_in_forbidden = find(forbidden.begin(), forbidden.end(), current) != forbidden.end();

					if (!current_in_forbidden || (current_in_forbidden && next_in_forbidden))
					{
						visited[next.y][next.x] = true;
						graph.push_back(next);
						frontier.push(next);
					}
				}
			}
		}

		return graph;
	}
	bool is_valid_cut(const vector<Position>& forbidden)
	{
		for (auto& pos1 : forbidden)
		{
			bool adjacent_exists = false;
			for (auto& pos2 : forbidden)
				if (Position::distance(pos1, pos2) <= 1)
				{
					adjacent_exists = true;
					break;
				}
			
			if (!adjacent_exists)
				return false;
		}
		return true;
	}
	bool find_path_to_destination(const Position& source, const Position& destination)
	{
		bool visited[width][height] = {};
		visited[source.y][source.x] = true;

		queue<Position> frontier;
		frontier.push(source);

		while (!frontier.empty())
		{
			Position current = frontier.front();
			frontier.pop();

			for (const Position& next : adjacency_list_position_enemy[current])
			{
				if (next == destination)
					return true;

				if (!visited[next.y][next.x])
				{
					visited[next.y][next.x] = true;
					frontier.push(next);
				}
			}
		}

		return false;
	}
};

int main()
{
	Game g;
	g.init();

	while (true)
	{
		g.update_game();
		{
			Stopwatch s("Turn total time");

			g.update_gamestate();

			g.move_units();
			g.attempt_chainkill();
			g.search_cuts();
			
			g.build_towers();

			g.train_units_on_cuts();
			g.train_units();

			g.debug();

			g.send_commands();
		}
	}

	return 0;
}