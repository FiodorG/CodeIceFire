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
	inline int level_of_enemy_unit() { return is_occupied_by_enemy_unit()? unit->level : -1; }
	inline int level_of_ally_unit() { return is_occupied_by_ally_unit() ? unit->level : -1; }
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

	//array<array<int, width * height>, width * height> distances;
	int distances[width * height][width * height];

	unordered_map<Position, vector<Position>, HashPosition> adjacency_list;
	unordered_map<Position, vector<Position>, HashPosition> adjacency_list_position_enemy;
	unordered_map<Position, vector<Position>, HashPosition> adjacency_list_position_ally;

	vector<vector<Cell>> cells;
	char cells_info[width][height]; // stores the chars representing the cell type
	vector<vector<int>> cells_used_objective; // used for objective
	vector<vector<int>> cells_used_movement; // used for movement
	int cells_level_ally[width][height]; // level required to move to cell
	int cells_level_enemy[width][height]; // level required to move to cell

	int gold_ally, income_ally;
	int gold_enemy, income_enemy;

	vector<vector<double>> score_enemy;

	// Utilities
	inline Cell& get_cell(const Position& position) { return cells[position.y][position.x]; }
	inline int get_cells_used_movement(const Position& position) { return cells_used_movement[position.y][position.x]; }
	inline int get_cells_level_ally(const Position& position) { return cells_level_ally[position.y][position.x]; }
	inline int get_cells_level_enemy(const Position& position) { return cells_level_enemy[position.y][position.x]; }
	inline int get_cells_used_objective(const Position& position) { return cells_used_objective[position.y][position.x]; }
	inline char get_cell_info(const Position& position) { return cells_info[position.y][position.x]; }
	inline double get_score_enemy(const Position& position) { return score_enemy[position.y][position.x]; }
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
	inline int get_distance(const Position& pos1, const Position& pos2) { return distances[pos1.x + height * pos1.y][pos2.x + height * pos2.y]; }

	// Main functions
	void debug()
	{
		Stopwatch s("Debug");

		for_each(units_ally.begin(), units_ally.end(), [](shared_ptr<Unit>& u) { u->debug(); });
		//for_each(units_enemy.begin(), units_enemy.end(), [](shared_ptr<Unit>& u) { u->debug(); });
		//for_each(buildings_ally.begin(), buildings_ally.end(), [](shared_ptr<Building>& u) { u->debug(); });
		//for_each(buildings_enemy.begin(), buildings_enemy.end(), [](shared_ptr<Building>& u) { u->debug(); });

		//for (auto& row : cells)
		//{
		//	for (auto& cell : row)
		//	{
		//		if (cell.is_occupied())
		//			cerr << "x ";
		//		else
		//			cerr << cell.position.x << "," << cell.position.y << " ";
		//	}

		//	cerr << endl;
		//}

		//for (auto& row : cells_info)
		//{
		//	for (auto& cell : row)
		//		cerr << cell;

		//	cerr << endl;
		//}

		//for (auto& row : cells_used)
		//{
		//	for (auto& cell : row)
		//		cerr << cell;

		//	cerr << endl;
		//} 

		cerr << "Level" << endl;
		for (auto& row : cells_level_enemy)
		{
			for (auto& cell : row)
				cerr << cell;

			cerr << endl;
		}

		//cerr << "Enemy score" << endl;
		//for (auto& row : score_enemy)
		//{
		//	for (auto& cell : row)
		//		cerr << cell;

		//	cerr << endl;
		//}

		// seed = -6736300506288822300
		//cerr << get_distance(Position(5, 5), Position(6, 6)) << " vs " << Position::distance(Position(5, 5), Position(6, 6)) << endl;

		//cerr << "nbr: " << nbr_units_ally_of_level(1) << endl;

		//cerr << "My cuts:" << endl;
		//MaxPriorityQueue<Position, double> cuts = find_cuts(false);

		//while (!cuts.empty())
		//{
		//	Position cut = cuts.elements.top().second;
		//	double gain = cuts.elements.top().first;
		//	int level_required = get_cells_level_enemy(cut);
		//	double cost = (double)(level_required * 10);
		//	double score = gain - cost;

		//	cerr << "Cut: " << cut.print() << ", gain " << gain << ", cost: " << cost << ", score: " << score << endl;

		//	cuts.elements.pop();
		//}
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

		hq_ally = getHQ();
		hq_enemy = getOpponentHQ();

		if (turn == 1)
			floyd_warshall();
	}
	void update_gamestate()
	{
		//Stopwatch s("Update gamestate");

		// Cells used from previous units
		cells_used_objective = vector<vector<int>>(width, vector<int>(height, 0));
		cells_used_movement = vector<vector<int>>(width, vector<int>(height, 0));

		cells.clear();
		cells = vector<vector<Cell>>(width, vector<Cell>(height));

		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
				cells[j][i] = Cell(i, j);

		// Units
		units_ally.clear();
		units_enemy.clear();

		for (auto& unit : units)
		{
			if (unit->isOwned())
				units_ally.push_back(unit);
			else
				units_enemy.push_back(unit);

			cells[unit->p.y][unit->p.x].set_unit(unit);
		}
		
		// Buildings
		buildings_ally.clear();
		buildings_enemy.clear();

		for (auto& building : buildings)
		{
			if (building->isOwned())
				buildings_ally.push_back(building);
			else
				buildings_enemy.push_back(building);

			cells[building->p.y][building->p.x].set_building(building);
		}

		// Mines
		for (auto& mine : mine_spots)
			cells[mine.y][mine.x].set_mine();

		// Voids
		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
				if (cells_info[j][i] == '#')
					cells[j][i].set_void_cell();

		// Level required to move to cell
		for (auto& row : cells)
			for (auto& cell : row)
			{
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
			{
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

		adjacency_list_position_enemy.clear();
		for (auto& position : positions_enemy)
		{
			vector<Position> positions;

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

		adjacency_list_position_ally.clear();
		for (auto& position : positions_ally)
		{
			vector<Position> positions;

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

		score_enemy= vector<vector<double>>(width, vector<double>(height, 0.0));
		for (auto& enemy : units_enemy)
		{
			for (int i = 0; i < width; i++)
				for (int j = 0; j < height; j++)
					if (get_distance(enemy->p, Position(i, j)) <= 3)
						score_enemy[j][i] += enemy->level;
		}
	}
	void send_commands()
	{
		Stopwatch s("Send commands");

		for_each(commands.begin(), commands.end(), [](Command &c) { c.print(); });
		cout << "WAIT" << endl;
	}


	// Buildings
	void build_mines()
	{
		Stopwatch s("Build mines");

		int nbr_mines = nbr_mines_ally();

		if (nbr_mines >= 2)
			return;

		unordered_map<Position, double, HashPosition> position_for_mines;
		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
				if (cells_info[j][i] == 'O' && cells[j][i].mine && !cells[j][i].is_occupied_by_building())
					position_for_mines[Position(i, j)] = -get_distance(hq_ally->p, Position(i, j));

		while (position_for_mines.size())
		{
			auto max = max_element(position_for_mines.begin(), position_for_mines.end(), [](const pair<Position, double>& p1, const pair<Position, double>& p2) { return p1.second < p2.second; });
			Position position_for_mine = max->first;

			if (gold_ally >= 20 + nbr_mines * 4)
			{
				gold_ally -= 20 + nbr_mines * 4;
				nbr_mines++;
				commands.push_back(Command(BUILD, "MINE", position_for_mine));
			}
			else
				return;

			position_for_mines.erase(position_for_mine);
		}
	}
	void build_towers()
	{
		Position pos;

		if (hq_ally->p.x == 0)
			pos = Position(1, 1);
		else
			pos = Position(10, 10);

		for (auto& enemy : units_enemy)
			if (get_distance(enemy->p, hq_ally->p) <= 10)
			{
				commands.push_back(Command(BUILD, "TOWER", pos));
				return;
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
		Stopwatch s("Find Training Position");

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
	inline bool need_train_units(int level) { return nbr_units_ally_of_level(level) <= ((level == 1) ? 8 : 4); }
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

		assign_objective_to_units();

		for (auto& unit : units_in_order)
		{
			Position destination = get_path(unit, unit->objective.target, false);
			cerr << "Path for: " << unit->id << ", moving to " << destination.print() << endl;

			if (unit_can_move_to_destination(unit, destination))
			{
				commands.push_back(Command(MOVE, unit->id, destination));
				refresh_gamestate_for_movement(unit, destination);
			}
		}
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


	// Objectives
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
			bool empty_at_distance_one = get_cell_info(pos) == '.' && distance == 1;

			double score = 0.0;

			//score += distance_to_hq_ally * (distance_to_hq_ally <= 3) * 100.0;
			score -= distance_to_enemy_hq/* * (distance_to_enemy_hq <= 6)*/;
			score += empty_at_distance_one * 10.0;
			score += (hq_enemy->p == pos) * 10.0;
			score += enemy_on_cell * 10.0;
			score += enemy_building_on_cell * 10.0;
			score += enemy_territory * 5.0;
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

			if (score > 0 && level_required <= 3 && can_train_level(level_required))
			{
				commands.push_back(Command(TRAIN, level_required, cut));
				refresh_gamestate_for_spawn(make_shared<Unit>(Unit(cut.x, cut.y, 999, level_required, 0)), cut);
				// fill the cut with inactive cells
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
				score += score_graph(graph, find_enemies);

				//string string1 = "graph: ";
				//for (auto& pos : graph)
				//	string1 += pos.print() + ", ";
				//cerr << string1 << endl;
			}
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
	double score_graph(vector<Position>& positions, bool find_enemies)
	{
		Position hq = find_enemies ? hq_ally->p : hq_enemy->p;

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
			else if (cell.is_occupied_by_hq())
				score += 100.0;
			
			// if close to hq, should definitely do the cut
			if (get_distance(position, hq) <= 3)
				score += 20.0;

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

		unordered_map<Position, double, HashPosition> chainkills = dijkstra_chainkill_all_costs(hq_enemy->p, false);

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

			double score = 0.0;
			for (auto& t : chainkill_path)
				score += get_cells_level_ally(t) * 10.0;

			assert(score == chainkill_cost);

			for (auto& t : chainkill_path)
				commands.push_back(Command(TRAIN, get_cells_level_ally(t), t));
		}
	}
	unordered_map<Position, double, HashPosition> dijkstra_chainkill_all_costs(const Position& source, bool debug)
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

			g.train_units_on_cuts();
			g.train_units();

			g.debug();

			//g.build_mines();
			g.build_towers();

			g.send_commands();
		}
	}

	return 0;
}