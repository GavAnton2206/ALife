#include "utilities.hpp"

// Global variables
Rules rules;
WorldState worldState;
RandomRange myRand;

// default types of entities to spawn
struct herbivoreEntity : entity {
    herbivoreEntity(int id, pii pos, int energy = 100, int age = 0)
        : entity(id, entityType::herb, pos, energy, age) {}
    
    pii decideMove(const WorldState& world) override{
        vector<pii> dir = {{-1, 0}, {1, 0}, {0, 1}, {0, -1}};
        return {pos.first + dir[myRand.rand(0, 3)].first, pos.second + dir[myRand.rand(0, 3)].second};
    }

    unique_ptr<entity> clone() const override {
        return make_unique<herbivoreEntity>(*this);
    }
};

struct carnivoreEntity : entity {
    carnivoreEntity(int id, pii pos, int energy = 100, int age = 0)
        : entity(id, entityType::carn, pos, energy, age) {}
    
    pii decideMove(const WorldState& world) override{
        vector<pii> dir = {{-1, 0}, {1, 0}, {0, 1}, {0, -1}};
        return {pos.first + dir[myRand.rand(0, 3)].first, pos.second + dir[myRand.rand(0, 3)].second};
    }

    unique_ptr<entity> clone() const override {
        return make_unique<carnivoreEntity>(*this);
    }
};

// counter for entityID
int getID(){
    availableID++;
    return availableID;
}

// Transforming state to its output view
string cellToString(const cell& cell, const WorldState& world){
    switch(cell.type){
        case cellType::n:
            return "\033[90m·\033[0m ";
            break;

        case cellType::p:
            return "\033[32m♣\033[0m ";
            break;

        case cellType::c:
            return "\033[35m☠\033[0m ";
            break;

        case cellType::e:
            auto it = world.entities.find(cell.id);

            if (it == world.entities.end())
                return "? ";

            switch (it->second->type) {
                case entityType::herb:
                    return "\033[31m♥\033[0m ";

                case entityType::carn:
                    return "\033[34m♠\033[0m ";
            }
            break;
    }

    return "? ";
}

void newEntity(int id, pii pos, entityType type, WorldState& world){
    switch(type){
        case entityType::herb:
            world.entities[id] = make_unique<herbivoreEntity>(id, pos);
            break;

        case entityType::carn:
            world.entities[id] = make_unique<carnivoreEntity>(id, pos);
            break;
    }
}

cell inputToCell(pii pos, const string& rep, WorldState& world){
    cell res;
    res.id = -1;

    // , e}, {"♣", p}, {"♥", h}, {"♠", c}};
    if(rep == "·"){
        res.type = cellType::n;
        return res;
    }else if(rep == "♣"){
        res.type = cellType::p;
        return res;
    }else if(rep == "☠"){
        res.type = cellType::c;
        return res;
    }else{
        res.type = cellType::e;
        res.id = getID();

        newEntity(res.id, pos, (rep == "♥") ? entityType::herb : entityType::carn, world);

        return res;
    }

    throw std::logic_error("Trying to process a wrong input character.");
    return res;
}

/*
map<int, string> toStr = {{e,  "· "}, {p, "♣ "}, {h, "♥ "}, {c, "♠ "}};
map<string, int> toType = {{"·", e}, {"♣", p}, {"♥", h}, {"♠", c}};*/

// Showing the grid to user
void print(const unique_ptr<entity>& ent){
    printf("Entity #%d on (%d, %d) with type %s\n", ent->id, ent->pos.first, ent->pos.second, (ent->type == entityType::herb) ? "H" : "C");
}

void print(const WorldState& world) {
    /*
    for(auto it = world.entities.begin(); it != world.entities.end(); ++it){
        print(it->second);
    }

    cout << "\n";*/

    for (int i = 0; i < world.n; i++) {
        for (int j = 0; j < world.m; j++) {
            cout << cellToString(world.grid[i][j], world);
        }
        cout << '\n';
    }
    cout << '\n';
}

WorldState advanceTime(const WorldState& world){
    WorldState res = WorldState(world);

    // move entities
    for(auto ent = world.entities.begin(); ent != world.entities.end(); ++ent){
        pii oldPos = ent->second->pos;
        pii newPos = ent->second->decideMove(world);

        if(!world.inBounds(newPos)){
            newPos = oldPos;
        }

        res.entities[ent->second->id]->age += 1;
        res.entities[ent->second->id]->energy -= 1;

        if(world.grid[newPos.first][newPos.second].type == cellType::p && ent->second->type == entityType::herb){
            res.entities[ent->second->id]->energy += rules.foodEnergy;
        }
        
        res.entities[ent->second->id]->pos = newPos;
        res.grid[oldPos.first][oldPos.second] = emptyCell;
        res.grid[newPos.first][newPos.second] = cell(cellType::e, ent->second->id);
    }

    // check energy levels
    for(auto it = res.entities.begin(); it != res.entities.end();){
        printf("Entity %d has %d energy\n", it->second->id, it->second->energy);
        if(it->second->energy <= 0){
            it = res.removeEntity(it);
        }else{
            ++it;
        }
    }

    return res;
}

// Importing grid from file
pair<WorldState, Rules> importWorld(ifstream& file){
    WorldState res;
    Rules rules;
    
    if (!file) {
        std::cerr << "Could not open file\n";
        return {move(res), rules};
    }

    int n, m;
    file >> n >> m;

    res.n = n;
    res.m = m;
    res.grid = vvc(n, vector<cell>(m));

    int delay, foodEnergy;
    file >> delay >> foodEnergy;
    rules.msDelay = delay;
    rules.foodEnergy = foodEnergy;

    // process rules
    string line;

    // import grid
    for(int i = 0; i < n; i++){
        for(int j = 0; j < m; j++){
            string symbol;
            file >> symbol;
            res.grid[i][j] = inputToCell({i, j}, symbol, res);
        }
    }

    return {res, rules};
}

int main(){
    myRand = RandomRange();

    int time = 0;

    ifstream file("sgrid.txt");
    pair<WorldState, Rules> res = importWorld(file);
    worldState = move(res.first);
    rules = move(res.second);
    print(worldState);
    
    while(true){
        this_thread::sleep_for(chrono::milliseconds(rules.msDelay));
        time++;
        system("clear"); 
        worldState = advanceTime(worldState);
        print(worldState);
        if(worldState.entities.size() >= 1){
            cout << '\n';
            printf("Gen: %d\n", time);
        }else{
            cout << '\n';
            printf("Gen: %d - the world stays still...\n", time);
            break;
        }
    }

    return 0;
}