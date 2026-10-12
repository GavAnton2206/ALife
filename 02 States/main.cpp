#include "utilities.hpp"

// Global variables
Rules rules;
WorldState worldState;
RandomRange myRand;

void print(const char* format, ...) {
    if (!rules.debug)
        return;

    va_list args;
    va_start(args, format);

    vprintf(format, args);

    va_end(args);
}

pii findNearestPlant(const WorldState& world, pii pos) {
    queue<pii> q;
    vector<vector<bool>> visited(world.n, vector<bool>(world.m, false));
    vector<vector<pii>> firstStep(world.n, vector<pii>(world.m));

    vector<pii> dirs = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    q.push(pos);
    visited[pos.first][pos.second] = true;

    while (!q.empty()) {
        pii cur = q.front();
        q.pop();

        for (auto d : dirs) {
            pii next = {cur.first + d.first, cur.second + d.second};

            if (!world.inBounds(next)) continue;
            if (visited[next.first][next.second]) continue;

            visited[next.first][next.second] = true;

            if (cur == pos)
                firstStep[next.first][next.second] = d;
            else
                firstStep[next.first][next.second] = firstStep[cur.first][cur.second];

            if (world.grid[next.first][next.second].type == cellType::p)
                return firstStep[next.first][next.second];

            q.push(next);
        }
    }

    return {0, 0}; // no plants
}

pii findNearestHerbivore(const WorldState& world, pii pos){
    int dist = INT_MAX;
    pii herbPos = {-1, 1};

    for(auto it = world.entities.begin(); it != world.entities.end(); ++it){
        if(world.distance(it->second->id, pos) < dist && it->second->type == entityType::herb){
            dist = world.distance(it->second->id, pos);
            herbPos = pii(it->second->pos);
        }
    }

    pii dir = {sign(herbPos.first - pos.first), sign(herbPos.second - pos.second)};

    if(dir.first != 0){
        dir.second = 0;
    }

    return {pos.first + dir.first, pos.second + dir.second};
}

// default types of entities to spawn
struct herbivoreEntity : entity {
    herbivoreEntity(int id, pii pos, int energy = 120, int age = 0)
        : entity(id, entityType::herb, pos, energy, age) {}
    
    pii decideMove(const WorldState& world) override{
        pii dir = findNearestPlant(world, pos);

        return {pos.first + dir.first, pos.second + dir.second};
    }

    unique_ptr<entity> clone() const override {
        return make_unique<herbivoreEntity>(*this);
    }
};

struct carnivoreEntity : entity {
    carnivoreEntity(int id, pii pos, int energy = 20, int age = 0)
        : entity(id, entityType::carn, pos, energy, age) {}
    
    pii decideMove(const WorldState& world) override{
        pii newPos = findNearestHerbivore(world, pos);

        return newPos;
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
    print("Entity #%d on (%d, %d) with type %s\n", ent->id, ent->pos.first, ent->pos.second, (ent->type == entityType::herb) ? "H" : "C");
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

    map<int, pii> oldPositions;
    map<int, pii> newPositions;

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
            res.grid[newPos.first][newPos.second] = emptyCell;
        }
        
        res.entities[ent->second->id]->pos = newPos;

        oldPositions[ent->first] = oldPos;
        newPositions[ent->first] = newPos;
    }

    for(auto a = world.entities.begin(); a != world.entities.end(); ++a){
        for(auto b = next(a); b != world.entities.end(); ++b){
            int id1 = a->first;
            int id2 = b->first;

            if(oldPositions[id1] == newPositions[id2] && oldPositions[id2] == newPositions[id1]){
                if(a->second->type == entityType::carn && b->second->type == entityType::herb){
                    res.removeEntity(id2);
                    res.entities.at(id1)->energy += rules.entityEnergy;
                }else if(a->second->type == entityType::herb && b->second->type == entityType::carn){
                    res.removeEntity(id1);
                    res.entities.at(id2)->energy += rules.entityEnergy;
                }
            }
        }
    }

    // miraculous birth
    if(((float)(myRand.rand(0, 100) % 100))/100.0 >= 1 - rules.miraculousSpawnChance){
        int newID = getID();
        newEntity(newID, myRand.rand_pos(world.n, world.m), myRand.rand_entity(), res);
        res.entities[newID]->age = 0;
        res.entities[newID]->energy = rules.startingEnergy;
    }

    // check energy levels
    map<pii, vector<int>> collisions;

    for(auto it = res.entities.begin(); it != res.entities.end();){
        // death
        if(it->second->energy <= 0){
            print("Entity #%d died\n", it->second->id);
            it = res.removeEntity(it);
        // reproduction
        }else if(it->second->energy >= 120 && it->second->type == entityType::herb){
            int newID = getID();
            newEntity(newID, {it->second->pos.first, it->second->pos.second + 1}, entityType::herb, res);
            res.entities[newID]->age = 0;
            res.entities[newID]->energy = rules.startingEnergy;

            it->second->energy += rules.birthEnergy;
            print("Entity #%d reproduced, creating #%d\n", it->second->id, newID);

            ++it;
        }else{
            ++it;
        }
    }

    for(auto it = res.entities.begin(); it != res.entities.end(); ++it){
        if(collisions.find(it->second->pos) != collisions.end()){
            collisions[it->second->pos].push_back(it->second->id);
        }else{
            collisions[it->second->pos] = vector<int>();
            collisions[it->second->pos].push_back(it->second->id);
        }

        print("Entity #%d has %d energy at position (%d, %d)\n", it->second->id, it->second->energy, it->second->pos.first, it->second->pos.second);
    }

    for(auto collision : collisions){
        if(collision.second.size() >= 2){
            print("Collision at (%d, %d): ",
                collision.first.first,
                collision.first.second);

            for(int id : collision.second){
                print("%d (%s) ", id,
                    res.entities.at(id)->type == entityType::herb
                    ? "Herbivore" : "Carnivore");
            }

            bool existsCarnivore = false;
            for(auto ent : collision.second){
                if(res.entities[ent]->type == entityType::carn){
                    existsCarnivore = true;
                }
            }

            if(!existsCarnivore)
                continue;

            for(auto ent : collision.second){
                if(res.entities[ent]->type == entityType::herb){
                    res.removeEntity(ent);
                }else{
                    res.entities[ent]->energy += rules.entityEnergy;
                }
            }
        }
    }

    // update displaying grid
    for(int i = 0; i < res.n; i++){
        for(int j = 0; j < res.m; j++){
            if(res.grid[i][j].type == cellType::e){
                res.grid[i][j] = emptyCell;
            }
        }
    }

    for(auto it = res.entities.begin(); it != res.entities.end(); ++it){
        pii pos = it->second->pos;

        if(res.inBounds(pos)){
            res.grid[pos.first][pos.second] = cell(cellType::e, it->first);
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

    int delay, birthEnergy, startingEnergy, foodEnergy, entityEnergy;
    float miraculousSpawnChance;
    int debug;
    file >> debug >> delay >> miraculousSpawnChance >> birthEnergy >> startingEnergy >> foodEnergy >> entityEnergy;
    rules.msDelay = delay;
    rules.debug = (debug == '1');
    rules.miraculousSpawnChance = miraculousSpawnChance;
    rules.birthEnergy = birthEnergy;
    rules.startingEnergy = startingEnergy;
    rules.foodEnergy = foodEnergy;
    rules.entityEnergy = entityEnergy;

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