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
#include <memory>
#include <queue>
#include <chrono>
#include <float.h>
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

	static int distance(const Position& lhs, const Position& rhs) { return abs(lhs.x - rhs.x) + abs(lhs.y - rhs.y); }
	Position north_position() { return (this->y > 0)? Position(this->x, this->y - 1) : Position(*this); }
	Position south_position() { return (this->y < height - 1)? Position(this->x, this->y + 1) : Position(*this); }
	Position east_position() { return (this->x < width - 1) ? Position(this->x + 1, this->y) : Position(*this); }
	Position west_position() { return (this->x > 0) ? Position(this->x - 1, this->y) : Position(*this); }
	void debug() { cerr << "(" << x << "," << y << ")" << endl; }
	string print() const { return "(" + to_string(x) + "," + to_string(y) + ")"; }
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
vector<Position> vector_unique(vector<Position> positions)
{
	vector<Position> new_positions;

	unordered_map<Position, bool, HashPosition> map;

	for (auto& position : positions)
		if (!map.count(position))
		{
			new_positions.push_back(position);
			map[position] = true;
		}

	return new_positions;
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

	void debug() 
	{
		cerr << "id" << id << ", lvl" << level << " on (" << p.x << "," << p.y << ") owned by " << owner << ", obj: " << " (" << objective.target.x << "," << objective.target.y << ") " << "score: " << objective.score << endl;
	}

	bool isOwned() 
	{
		return owner == 0;
	}

	void set_objective(Objective obj) { this->objective = obj; }
};

class Building 
{
public:

	Position p;
	BuildingType t;
	int owner;

	Building(int x, int y, int t, int owner) : p(x, y), t(static_cast<BuildingType>(t)), owner(owner) {}
	Building(const Building& building) : p(building.p.x, building.p.y), t(building.t), owner(building.owner) {}

	void debug() { cerr << t << " at " << p.x << " " << p.y << " owned by " << owner << endl; }
	bool isHQ() 
	{
		return t == HQ;
	}
	bool isOwned() 
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

	void set_unit(Unit unit) { this->unit = make_shared<Unit>(unit); }
	void set_building(Building building) { this->building = make_shared<Building>(building); }
	void set_owner(int owner) { this->owner = owner; }
	void set_mine() { this->mine = true; }
	void set_void_cell() { this->void_cell = true; }
	bool is_empty() { return !unit && !building; }
	bool is_occupied() { return unit || building; }
	bool is_occupied_by_unit() { return unit? true : false; }
	bool is_occupied_by_building() { return building? true : false; }
	bool is_occupied_by_enemy_building() { return is_occupied_by_building() && building->owner == 1; }
	bool is_occupied_by_inacessible_building() { return is_occupied_by_building() && building->owner == 0 && (building->t == BuildingType::HQ || building->t == BuildingType::MINE); }
	bool is_occupied_by_enemy_tower() { return is_occupied_by_building() && building->owner == 1 && building->t == BuildingType::TOWER; }
	bool is_occupied_by_enemy_hq() { return is_occupied_by_building() && building->owner == 1 && building->t == BuildingType::HQ; }
	bool is_occupied_by_enemy_mine() { return is_occupied_by_building() && building->owner == 1 && building->t == BuildingType::MINE; }
	bool is_occupied_by_ally_mine() { return is_occupied_by_building() && building->owner == 0 && building->t == BuildingType::MINE; }
	bool is_occupied_by_ally_tower() { return is_occupied_by_building() && building->owner == 0 && building->t == BuildingType::TOWER; }
	bool is_occupied_by_mine() { return is_occupied_by_building() && building->t == BuildingType::MINE; }
	bool is_occupied_by_hq() { return is_occupied_by_building() && building->t == BuildingType::HQ; }
	bool is_occupied_by_tower() { return is_occupied_by_building() && building->t == BuildingType::TOWER; }
	bool is_occupied_by_enemy_unit() { return is_occupied_by_unit() && unit->owner == 1; }
	bool is_occupied_by_enemy_unit_of_level(int level) { return is_occupied_by_unit() && unit->owner == 1 && unit->level == level; }
	bool is_occupied_by_ally_unit() { return is_occupied_by_unit() && unit->owner == 0; }
	int level_of_enemy_unit() { return is_occupied_by_enemy_unit()? unit->level : -1; }
};

class Game
{
public:
	int turn;

	vector<Unit> units;
	vector<Building> buildings;
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

	unordered_map<Position, vector<Position>, HashPosition> adjacency_list;
	unordered_map<Position, vector<Position>, HashPosition> adjacency_list_position_enemy;
	unordered_map<Position, vector<Position>, HashPosition> adjacency_list_position_ally;

	vector<vector<Cell>> cells;
	vector<vector<char>> cells_info; // stores the chars representing the cell type
	vector<vector<int>> cells_used_objective; // used for objective
	vector<vector<int>> cells_used_movement; // used for movement
	vector<vector<int>> cells_level; // level required to move to cell

	int gold_ally, income_ally;
	int gold_enemy, income_enemy;

	vector<vector<double>> score_enemy;

	// Utilities
	Cell& get_cell(const Position& position) { return cells[position.y][position.x]; }
	int get_cells_used_movement(const Position& position) { return cells_used_movement[position.y][position.x]; }
	int get_cells_level(const Position& position) { return cells_level[position.y][position.x]; }
	int get_cells_used_objective(const Position& position) { return cells_used_objective[position.y][position.x]; }
	char get_cell_info(const Position& position) { return cells_info[position.y][position.x]; }
	double get_score_enemy(const Position& position) { return score_enemy[position.y][position.x]; }
	vector<Position>& get_adjacency_list(const Position& position) { return adjacency_list.at(position); }
	vector<Position>& get_adjacency_list_position_enemy(const Position& position) { return adjacency_list_position_enemy.at(position); }

	bool can_train_level3() { return gold_ally >= 30 && income_ally >= 20; }
	bool can_train_level2() { return gold_ally >= 20 && income_ally >= 4; }
	bool can_train_level1() { return gold_ally >= 10 && income_ally >= 1; }
	bool can_train_level(int level) 
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
	int cost_of_unit(int level)
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
	int upkeep_of_unit(int level)
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
	int nbr_units_ally_of_level(int level)
	{
		int n = 0;
		for (auto& unit : units_ally)
			if (unit->level == level)
				n++;

		return n;
	}
	int nbr_mines_ally()
	{
		int n = 0;
		for (auto& building : buildings_ally)
			if (building->owner == 0 && building->t == BuildingType::MINE)
				n++;
		return n;
	}

	const Building& getHQ()
	{
		for (auto& b : buildings)
			if (b.isHQ() && b.isOwned())
				return b;
	}
	const Building& getOpponentHQ()
	{
		for (auto &b : buildings)
			if (b.isHQ() && !b.isOwned())
				return b;
	}


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
		for (auto& row : cells_level)
		{
			for (auto& cell : row)
				cerr << cell;

			cerr << endl;
		}

		cerr << "Enemy score" << endl;
		for (auto& row : score_enemy)
		{
			for (auto& cell : row)
				cerr << cell;

			cerr << endl;
		}
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

		units.clear();
		buildings.clear();
		commands.clear();

		cin >> gold_ally; cin.ignore();
		cin >> income_ally; cin.ignore();

		cin >> gold_enemy; cin.ignore();
		cin >> income_enemy; cin.ignore();

		cells_info = vector<vector<char>>(width, vector<char>(height, '#'));
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
			buildings.push_back(Building(x, y, buildingType, owner));
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
			units.push_back(Unit(x, y, unitId, level, owner));
		}

		hq_ally = make_shared<Building>(getHQ());
		hq_enemy = make_shared<Building>(getOpponentHQ());
	}
	void update_gamestate()
	{
		Stopwatch s("Update gamestate");

		turn++;

		// Cells used from previous units
		cells_used_objective = vector<vector<int>>(width, vector<int>(height, 0));
		cells_used_movement = vector<vector<int>>(width, vector<int>(height, 0));

		cells.clear(); // does this clear the two level cells with all shared_ptr well?
		cells = vector<vector<Cell>>(width, vector<Cell>(height));

		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
				cells[j][i] = Cell(i, j);

		// Units
		units_ally.clear();
		units_enemy.clear();

		for (Unit& unit : units)
		{
			if (unit.isOwned())
				units_ally.push_back(make_shared<Unit>(unit));
			else
				units_enemy.push_back(make_shared<Unit>(unit));

			cells[unit.p.y][unit.p.x].set_unit(unit);
		}
		
		// Buildings
		buildings_ally.clear();
		buildings_enemy.clear();

		for (Building& building : buildings)
		{
			if (building.isOwned())
				buildings_ally.push_back(make_shared<Building>(building));
			else
				buildings_enemy.push_back(make_shared<Building>(building));

			cells[building.p.y][building.p.x].set_building(building);
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
		cells_level = vector<vector<int>>(width, vector<int>(height, 0));
		for (auto& row : cells)
			for (auto& cell : row)
			{
				if (cell.is_occupied_by_enemy_hq())
					cells_level[cell.position.y][cell.position.x] = 1;
				else if (cell.is_occupied_by_enemy_tower())
				{
					cells_level[cell.position.y][cell.position.x] = 3;

					if (get_cell_info(Position(cell.position.x, min(cell.position.y + 1, width - 1))) == 'X')
						cells_level[min(cell.position.y + 1, width - 1)][cell.position.x] = 3;

					if (get_cell_info(Position(cell.position.x, max(cell.position.y - 1, 0))) == 'X')
						cells_level[max(cell.position.y - 1, 0)][cell.position.x] = 3;	

					if (get_cell_info(Position(min(cell.position.x + 1, height - 1), cell.position.y)) == 'X')
						cells_level[cell.position.y][min(cell.position.x + 1, height - 1)] = 3;

					if (get_cell_info(Position(max(cell.position.x - 1, 0), cell.position.y)) == 'X')
						cells_level[cell.position.y][max(cell.position.x - 1, 0)] = 3;
				}
				else if (cell.is_occupied_by_enemy_mine())
					cells_level[cell.position.y][cell.position.x] = 1;
				else if (cell.is_occupied_by_ally_mine())
					cells_level[cell.position.y][cell.position.x] = 9;
				else if (cell.is_occupied_by_ally_tower())
					cells_level[cell.position.y][cell.position.x] = 9;
				else if (cell.is_occupied_by_enemy_unit())
					cells_level[cell.position.y][cell.position.x] = min(3, cell.level_of_enemy_unit() + 1);
				else if (cell.is_occupied_by_ally_unit())
					cells_level[cell.position.y][cell.position.x] = 9;
				else if (cell.void_cell)
					cells_level[cell.position.y][cell.position.x] = 9;
				else
					cells_level[cell.position.y][cell.position.x] = 1;
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
					if (Position::distance(enemy->p, Position(i, j)) <= 3)
						score_enemy[j][i] += enemy->level;
		}
	}
	void send_commands()
	{
		Stopwatch s("Send commands");

		for_each(commands.begin(), commands.end(), [](Command &c) { c.print(); });
		cout << "WAIT" << endl;
	}

	void build_mines()
	{
		Stopwatch s("Build mines");

		int nbr_mines = nbr_mines_ally();

		if (nbr_mines >= 3)
			return;

		unordered_map<Position, double, HashPosition> position_for_mines;
		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
				if (cells_info[j][i] == 'O' && cells[j][i].mine && !cells[j][i].is_occupied_by_building())
					position_for_mines[Position(i, j)] = -Position::distance(hq_ally->p, Position(i, j));

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
			if (Position::distance(enemy->p, hq_ally->p) <= 4)
			{
				commands.push_back(Command(BUILD, "TOWER", pos));
				return;
			}
	}


	// Training new units
	double get_training_score(const shared_ptr<Unit>& unit, const Position& pos)
	{
		if (unit->level < get_cells_level(pos) || cells_used_objective[pos.y][pos.x] || cells_used_movement[pos.y][pos.x] || cells_info[pos.y][pos.x] == 'O')
			// not objective if not enough level, cell is already used, cell is already owned
			return -DBL_MAX;
		else
		{
			int distance_to_hq_ally = Position::distance(pos, hq_ally->p);
			int distance_to_enemy_hq = Position::distance(pos, hq_enemy->p);

			bool enemy_on_cell = get_cell(pos).is_occupied_by_enemy_unit();
			bool enemy_building_on_cell = get_cell(pos).is_occupied_by_enemy_building();
			bool enemy_territory = get_cell_info(pos) == 'X';
			bool is_enemy_hq = (hq_enemy->p == pos);

			double score = 0.0;

			score += (turn <= 3) ? -distance_to_hq_ally * 10.0 : distance_to_hq_ally;
			score -= distance_to_enemy_hq;
			score += is_enemy_hq * 10.0;
			score += enemy_on_cell * 20.0;
			score += enemy_building_on_cell * 20.0;
			score += enemy_territory * 5.0;

			return score;
		}
	}
	unordered_map<Position, double, HashPosition> find_training_position_level_1(int level)
	{
		Stopwatch s("Find Training Position");

		vector<Position> positions_available;

		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
			{
				Position pos = Position(i, j);

				if (get_cell_info(pos) == 'O' || get_cells_level(pos) > level || get_cells_used_movement(pos))
					continue;

				bool attainable = false;
				for (int k = 0; k < width; k++)
					for (int l = 0; l < height; l++)
						if (Position::distance(pos, Position(k, l)) == 1 && get_cell_info(Position(k, l)) == 'O')
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
			positions_for_spawn[pos] = get_training_score(make_shared<Unit>(Unit(pos.x, pos.y, 999, 1, 0)), pos);

		return positions_for_spawn;
	}
	void train_units_level_1()
	{
		Stopwatch s("Train units level 1");

		int level = 1;
		bool can_recruit_lvl1 = nbr_units_ally_of_level(level) <= 8;

		unordered_map<Position, double, HashPosition> training_positions = find_training_position_level_1(level);

		//string s1;
		//for (auto& t : training_positions)
		//	s1 += t.first.print() + " " + to_string(t.second) + " ";
		//cerr << s1;

		while (training_positions.size())
		{
			auto max = max_element(training_positions.begin(), training_positions.end(), [](const pair<Position, double>& p1, const pair<Position, double>& p2) { return p1.second < p2.second; });
			Position training_position = max->first;

			if (can_train_level1() && can_recruit_lvl1)
			{
				commands.push_back(Command(TRAIN, level, training_position));
				gold_ally -= level_1_cost;
				income_ally -= level_1_upkeep;
			}
			else
				return;

			training_positions.erase(training_position);

			for (auto& pos : get_adjacency_list(training_position))
				if (get_cells_level(pos) <= level && !get_cells_used_movement(pos))
					training_positions[pos] = get_score(make_shared<Unit>(Unit(pos.x, pos.y, 999, level, 0)), pos);
		}
	}
	unordered_map<Position, double, HashPosition> find_training_position_level_2(int level)
	{
		Stopwatch s("Find Training Position");

		vector<Position> positions_available;

		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
			{
				Position pos = Position(i, j);

				if (get_cell_info(pos) == 'O' || get_cells_level(pos) > level || get_cells_used_movement(pos) || !get_cell(pos).is_occupied_by_enemy_unit_of_level(1))
					continue;

				bool attainable = false;
				for (int k = 0; k < width; k++)
					for (int l = 0; l < height; l++)
						if (Position::distance(pos, Position(k, l)) == 1 && get_cell_info(Position(k, l)) == 'O')
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
			positions_for_spawn[pos] = get_score_enemy(pos);

		return positions_for_spawn;
	}
	void train_units_level_2()
	{
		Stopwatch s("Train units level 2");

		int level = 2;
		bool can_recruit_lvl2 = nbr_units_ally_of_level(level) <= 4;

		unordered_map<Position, double, HashPosition> training_positions = find_training_position_level_2(level);

		//string s1;
		//for (auto& t : training_positions)
		//	s1 += t.first.print() + " " + to_string(t.second) + " ";
		//cerr << s1;

		while (training_positions.size())
		{
			auto max = max_element(training_positions.begin(), training_positions.end(), [](const pair<Position, double>& p1, const pair<Position, double>& p2) { return p1.second < p2.second; });
			Position training_position = max->first;

			if (can_train_level2() && can_recruit_lvl2)
			{
				commands.push_back(Command(TRAIN, level, training_position));
				gold_ally -= level_2_cost;
				income_ally -= level_2_upkeep;
			}
			else
				return;

			training_positions.erase(training_position);

			for (auto& pos : get_adjacency_list(training_position))
				if (get_cells_level(pos) <= level && !get_cells_used_movement(pos))
					training_positions[pos] = get_score(make_shared<Unit>(Unit(pos.x, pos.y, 999, level, 0)), pos);
		}
	}

	// Pathing
	void generate_moves()
	{
		Stopwatch s("Generate Moves");

		for (auto& unit : units_in_order)
		{
			cerr << "Finding path for: " << unit->id << ", ";
			Position target = unit->objective.target;
			Position destination = get_path(unit, target, false);
			cells_used_movement[destination.y][destination.x] = 1;
			cerr << "moving to " << destination.print() << endl;
			commands.push_back(Command(MOVE, unit->id, destination));
		}
	}
	Position get_path(const shared_ptr<Unit>& unit, Position target, bool debug)
	{
		if (unit->p == target || (Position::distance(unit->p, target) == 1))
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
		if (unit->level < get_cells_level(next))
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
		if (unit->level < get_cells_level(pos) || cells_used_objective[pos.y][pos.x] || cells_used_movement[pos.y][pos.x] || cells_info[pos.y][pos.x] == 'O')
			// not objective if not enough level, cell is already used, cell is already owned
			return -DBL_MAX;
		else
		{
			int distance_to_hq_ally = Position::distance(pos, hq_ally->p);
			int distance_to_enemy_hq = Position::distance(pos, hq_enemy->p);
			int distance = Position::distance(unit->p, pos);

			bool enemy_on_cell = get_cell(pos).is_occupied_by_enemy_unit();
			bool enemy_building_on_cell = get_cell(pos).is_occupied_by_enemy_building();
			bool enemy_territory = get_cell_info(pos) == 'X';

			double score = 0.0;

			score += distance_to_hq_ally * (distance_to_hq_ally <= 3) * 100.0;
			score -= distance_to_enemy_hq * (distance_to_enemy_hq <= 6);
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

		MaxPriorityQueue<Position, double> cuts = find_cuts();

		while (!cuts.empty())
		{
			Position cut = cuts.elements.top().second;
			double gain = cuts.elements.top().first;
			int level_required = get_cells_level(cut);
			double cost = (double)(level_required * 10);
			double score = gain - cost;

			cerr << "Cut: " << cut.print() << ", gain " << gain << ", cost: " << cost << ", score: " << score << endl;

			if (score > 0 && level_required <= 3 && can_train_level(level_required))
			{
				commands.push_back(Command(TRAIN, level_required, cut));
				cells_used_movement[cut.y][cut.x] = 1;
				gold_ally -= cost_of_unit(level_required);
				income_ally -= upkeep_of_unit(level_required);
			}

			cuts.elements.pop();
		}
	}
	MaxPriorityQueue<Position, double> find_cuts()
	{
		vector<Position> attainable_articulation_points = get_attainable_articulation_points(true);

		//string str = "Articulation Points: ";
		//for (auto& attainable_articulation_point : attainable_articulation_points)
		//	str += attainable_articulation_point.print() + " ";
		//cerr << str << endl;

		MaxPriorityQueue<Position, double> scores;
		for (auto& articulation_point : attainable_articulation_points)
		{
			double score = 0;
			for (auto& neighbor : adjacency_list_position_enemy[articulation_point])
			{
				vector<Position> graph = find_graph_from_source(neighbor, articulation_point, true);
				score += score_graph(graph);

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
	double score_graph(vector<Position>& positions)
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
			else if (cell.is_occupied_by_hq())
				score += 100.0;
			
			// if close to hq, should definitely do the cut
			if (Position::distance(position, hq_ally->p) <= 3)
				score += 20.0;

			score += 1.0;
		}

		return score;
	}
	vector<Position> find_graph_from_source(const Position& source, const Position& forbidden, bool find_enemies)
	{
		unordered_map<Position, vector<Position>, HashPosition> adj_list = find_enemies ? adjacency_list_position_enemy : adjacency_list_position_ally;

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

			if (current == hq_enemy->p)
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
					if (Position::distance(articulation_point, position) <= 1)
					{
						attainable_articulation_points.push_back(articulation_point);
						break;
					}
					
			}
			else
			{
				for (auto& position : positions_enemy)
					if (Position::distance(articulation_point, position) <= 1)
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
		if (turn != 20)
			return;

		unordered_map<Position, double, HashPosition> all_paths = dijkstra_all_paths(hq_enemy->p, false);

		for (auto& t : all_paths)
		{
			string s1 = "";
			s1 += t.first.print() + ": " + to_string(t.second);
			cerr << s1 << endl;
		}
	}
	unordered_map<Position, double, HashPosition> dijkstra_all_paths(const Position& source, bool debug)
	{
		MinPriorityQueue<Position, double> frontier;
		frontier.put(source, 0.0);

		unordered_map<Position, double, HashPosition> cost_so_far;
		cost_so_far[source] = 0.0;

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

			for (const Position& next : get_adjacency_list(current))
			if (get_cell_info(next) != 'O')
			{
				double new_cost = cost_so_far[current] + get_cells_level(next);

				//if (debug)
				//	cerr << "next: " << next.print() << " s: " << new_cost << ", ";

				if ((cost_so_far.find(next) == cost_so_far.end()) || (new_cost < cost_so_far[next]))
				{
					cost_so_far[next] = new_cost;
					frontier.put(next, new_cost);
				}

				//if (debug)
				//	cerr << endl;
			}
		}

		return cost_so_far;
	}

};

int main()
{
	Game g;
	g.init();

	while (true)
	{
		g.update_game();
		g.update_gamestate();

		//g.attempt_chainkill();
		g.assign_objective_to_units();
		g.generate_moves();

		g.train_units_on_cuts();
		g.train_units_level_2();
		g.train_units_level_1();

		g.debug();

		g.build_mines();
		g.build_towers();

		g.send_commands();
	}

	return 0;
}