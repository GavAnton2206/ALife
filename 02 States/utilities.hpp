#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <set>
#include <thread>
#include <chrono>
#include <memory>
#include <random>

struct cell;
struct entity;
struct imported;

using namespace std;
using pii = pair<int, int>;
using vvc = vector<vector<cell>>;

class WorldState;

int availableID = 0;

// Type that defines type of cell
enum cellType {
    n=0, // none
    p=1, // plant,
    e=2, // entity,
    c=3, // corpse
};

// Type that defines type of entity
enum entityType {
    herb=10, // herbivore
    carn=11// carnivore
};

struct cell {
    cellType type; // type of cell
    int id = -1; // id of entity

    cell(cellType type_=n, int id_=-1) : type(type_), id(id_) {}
};

struct Rules{
    int msDelay; // delay between frames
    int foodEnergy; // energy received from eating food
};

const cell emptyCell = cell(cellType::n, -1);
const cell plantCell = cell(cellType::p, -1);

class RandomRange {
public:
    RandomRange() : engine(random_device{}()) {}

    // Generates a random integer in [a, b]
    int rand(int a, int b) {
        std::uniform_int_distribution<int> dist(a, b);
        return dist(engine);
    }

private:
    std::mt19937 engine;
};

// Entity
struct entity {
    int id; // id in system

    entityType type;

    pii pos;
    int energy;
    int age;

    virtual pii decideMove(const WorldState& world){
        return pos;
    }

    entity(int id_, entityType type_, pii pos_, int energy_ = 100, int age_ = 0) : id(id_), type(type_), pos(pos_), energy(energy_), age(age_) {}

    virtual ~entity() {}

    virtual unique_ptr<entity> clone() const {
        return make_unique<entity>(*this);
    }
};

class WorldState{
public:
    int n, m; // size of grid
    map<int, unique_ptr<entity>> entities; // entities
    vvc grid; // grid
    
    // Constructors
    WorldState(const vvc& grid_, const vector<int>& B_ = {3}, const vector<int>& S_ = {2, 3}): grid(grid_) {
        n = grid.size();
        if(n >= 1)
            m = grid[0].size();
        else
            m = 0;
    }

    WorldState(int n_ = 0, int m_ = -1, const vector<int>& B_ = {3}, const vector<int>& S_ = {2, 3}){
        n = n_;
        if(m_ == -1){
            m = n;
        }else{
            m = m_;
        }

        grid = vvc(n, vector<cell>(m, cell()));
    }

    WorldState(const WorldState& other) :n(other.n), m(other.m), grid(other.grid) {
        for (const auto& ent : other.entities) {
            entities[ent.first] = ent.second->clone();
        }
    }

    // erasing entity from dictionary
    map<int, unique_ptr<entity>>::iterator removeEntity(map<int, unique_ptr<entity>>::iterator id) {
        grid[id->second->pos.first][id->second->pos.second] = cell(cellType::c);
        return entities.erase(id);
    }

    map<int, unique_ptr<entity>>::iterator removeEntity(int id) {
        auto it = entities.find(id);

        grid[it->second->pos.first][it->second->pos.second] = cell(cellType::c);
        return entities.erase(it);
    }

    // Check if a given cell is within bounds
    bool inBounds(pii Pos) const{
        int x = Pos.first;
        int y = Pos.second;
        return (x >= 0) && (y >= 0) && (x <= n - 1) && (y <= m - 1);
    }

    WorldState(WorldState&&) noexcept = default;
    WorldState& operator=(WorldState&&) noexcept = default;

private:
    // Vectors of cells around a given, not including it
    vector<pii> cellsAround(pii Pos){
        int x = Pos.first;
        int y = Pos.second;

        vector<pii> res;
        res.reserve(8);

        vector<pii> dirs = {{-1, -1}, {-1, 0}, {-1, 1},
                            {0, -1}, {0, 1},
                            {1, -1}, {1, 0}, {1, 1}};

        for(auto &to : dirs){
            pii nP = {x + to.first, y + to.second};
            if(inBounds(nP))
                res.push_back(nP);
        }

        return res;
    }
};