#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <set>
#include <thread>
#include <chrono>
#include <memory>

struct cell;
struct entity;
struct imported;

using namespace std;
using pii = pair<int, int>;
using vvc = vector<vector<cell>>;

int availableID = 0;

// Type that defines type of cell
enum cellType {
    n=0, // none
    p=1, // plant,
    e=2 // entity
};

// Type that defines type of entity
enum entityType {
    h=10, // herbivore
    c=11// carnivore
};

struct cell {
    cellType type; // type of cell
    int id = -1; // id of entity

    cell(cellType type_=n, int id_=-1) : type(type_), id(id_) {}
};

class WorldState{
public:
    int n, m; // size of grid
    map<int, unique_ptr<entity>> entities; // entities
    vvc grid; // grid
    vector<int> B, S; // rules of born and survive
    
    // Constructors
    WorldState(const vvc& grid_, const vector<int>& B_ = {3}, const vector<int>& S_ = {2, 3}): grid(grid_), B(B_), S(S_){
        n = grid.size();
        if(n >= 1)
            m = grid[0].size();
        else
            m = 0;
    }

    WorldState(int n_ = 0, int m_ = -1, const vector<int>& B_ = {3}, const vector<int>& S_ = {2, 3}): B(B_), S(S_){
        n = n_;
        if(m_ == -1){
            m = n;
        }else{
            m = m_;
        }

        grid = vvc(n, vector<cell>(m, cell()));
    }

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

    // Check if a given cell is within bounds
    bool inBounds(pii Pos){
        int x = Pos.first;
        int y = Pos.second;
        return (x >= 0) && (y >= 0) && (x <= n - 1) && (y <= m - 1);
    }
};

/*
    void updateStep(){
        vector<vector<state>> newGrid = grid;
        vector<pos> newAlive = vector<pos>(); 

        set<pos> toUpdate;
        for(auto &cell : alive){
            toUpdate.insert(cell);
            vector<pos> around = cellsAround(cell);
            for(int i = 0; i < around.size(); i++){
                toUpdate.insert(around[i]);
            }
        }

        for(auto &cell: toUpdate){
            int aliveAround = 0;
            vector<pos> around = cellsAround(cell);
            for(int i = 0; i < around.size(); i++){
                aliveAround += grid[around[i].fi][around[i].se] == a ? 1 : 0;
            }

            if(grid[cell.fi][cell.se] == a){
                newGrid[cell.fi][cell.se] = (find(S.begin(), S.end(), aliveAround) != S.end()) ? a : d;
            }else{
                newGrid[cell.fi][cell.se] = (find(B.begin(), B.end(), aliveAround) != B.end()) ? a : d;
            }

            if(newGrid[cell.fi][cell.se] == a){
                newAlive.pb(cell);
            }
        }

        alive.swap(newAlive);
        grid.swap(newGrid);
    }*/

// Entity
struct entity {
    int id; // id in system

    entityType type;
    pii pos;
    int energy;
    int age;

    pii decideMove(const WorldState& world){
        return pos;
    }

    entity(int id_, entityType type_, pii pos_, int energy_ = 100, int age_ = 0) : id(id_), type(type_), pos(pos_), energy(energy_), age(age_) {}
};