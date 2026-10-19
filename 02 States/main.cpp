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

// Space relations: returns the position of searched object
pii findNearestPlant(const WorldState& world, pii pos) {
    queue<pii> q;
    vector<vector<bool>> visited(world.n, vector<bool>(world.m, false));

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

            if (world.grid[next.first][next.second].type == cellType::p){
                return next;
            }

            q.push(next);
        }
    }

    return {-1, -1}; // no plants
}

pii findNearestEntity(const WorldState& world, pii pos, entityType searchedType){
    int dist = INT_MAX;
    pii targetPos = {-1, -1};

    for(auto it = world.entities.begin(); it != world.entities.end(); ++it){
        if(world.distance(it->second->id, pos) < dist && it->second->type == searchedType){
            dist = world.distance(it->second->id, pos);
            targetPos = pii(it->second->pos);
        }
    }

    return targetPos;
}

// default types of entities to spawn
struct herbivoreEntity : entity {
    herbivoreEntity(int id, pii pos, int energy = 120, int age = 0)
        : entity(id, entityType::herb, pos, energy, age) {}

    pii decideMoveProbabilistically(const WorldState& world) {
        pii foodPos = findNearestPlant(world, pos);
        pii predatorPos = findNearestEntity(world, pos, entityType::carn);

        vector<pii> possibleMoves;
        vector<double> weights;

        for (pii dir : movementDirs) {
            pii nextPos = {pos.first + dir.first, pos.second + dir.second};

            if (!world.inBounds(nextPos))
                continue;

            double score = 1.0;

            // +food
            if (world.inBounds(foodPos)) {
                int oldDist = world.distance(id, foodPos);
                int newDist = world.distance(nextPos, foodPos);

                if (newDist < oldDist)
                    score *= 8.0;

                if (newDist > oldDist)
                    score *= 0.6;
            }

            // -predators
            if (world.inBounds(predatorPos)) {
                int oldDist = world.distance(id, predatorPos);
                int newDist = world.distance(nextPos, predatorPos);

                if (oldDist <= 3) {
                    if (newDist > oldDist)
                        score *= 8.0;

                    if (newDist < oldDist)
                        score *= 0.2;
                }
            }

            // satisfaction from plant :)
            if (world.grid[nextPos.first][nextPos.second].type == cellType::p) {
                score *= 5.0;
            }

            possibleMoves.push_back(nextPos);
            weights.push_back(score);
        }

        int chosen = myRand.weightedChoice(weights);

        return possibleMoves[chosen];
    }
    
    pii decideMoveDeterministically(const WorldState& world){
        pii targetPos = findNearestPlant(world, pos);
        pii targetDir = normalizeDirection(pos, targetPos, true);

        pii scaryPos = findNearestEntity(world, pos, entityType::carn);
        pii scaryDir = normalizeDirection(pos, scaryPos, true);

        pii dir = {0, 0};

        // run from predator
        if(world.distance(id, scaryPos) <= 3 && world.inBounds(scaryPos)){
            dir = {-scaryDir.first, -scaryDir.second};
        // go to food
        }else if(world.inBounds(targetPos)){
            dir = targetDir;
        // move randomly
        }else{
            dir = {myRand.rand(-1, 1), myRand.rand(-1, 1)};
        }

        dir = normalizeDirection(dir, false);

        return {pos.first + dir.first, pos.second + dir.second};
    }

    pii decideMove(const WorldState& world) override{
        return decideMoveProbabilistically(world);
    }

    unique_ptr<entity> clone() const override {
        return make_unique<herbivoreEntity>(*this);
    }
};

struct carnivoreEntity : entity {
    carnivoreEntity(int id, pii pos, int energy = 20, int age = 0)
        : entity(id, entityType::carn, pos, energy, age) {}

    pii decideMoveProbabilistically(const WorldState& world) {
        pii preyPos = findNearestEntity(world, pos, entityType::herb);

        vector<pii> possibleMoves;
        vector<double> weights;

        for (pii dir : movementDirs) {
            pii nextPos = {pos.first + dir.first, pos.second + dir.second};

            if (!world.inBounds(nextPos))
                continue;

            double score = 1.0;

            if (world.inBounds(preyPos)) {
                int oldDist = world.distance(pos, preyPos);
                int newDist = world.distance(nextPos, preyPos);

                // go to prey
                if (newDist < oldDist)
                    score *= 10.0;

                // run from prey :/
                if (newDist > oldDist)
                    score *= 0.8;

                // satisfaction eating prey :)
                if (nextPos == preyPos)
                    score *= 10.0;
            }

            possibleMoves.push_back(nextPos);
            weights.push_back(score);
        }

        int chosen = myRand.weightedChoice(weights);

        return possibleMoves[chosen];
    }
    
    pii decideMoveDeterministically(const WorldState& world) {
        pii targetPos = findNearestEntity(world, pos, entityType::herb);
        pii dir = normalizeDirection(pos, targetPos, true);

        pii tempPos = {pos.first + dir.first, pos.second + dir.second};
        targetPos = findNearestEntity(world, tempPos, entityType::herb);
        dir = normalizeDirection(tempPos, targetPos, true);

        return {tempPos.first + dir.first, tempPos.second + dir.second};
    }

    pii decideMove(const WorldState& world) override {
        return decideMoveProbabilistically(world);
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

    if(cell.ids.size() == 0){
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
        }
    }else{
        if(cell.ids.size() == 1){
            auto it = world.entities.find(*cell.ids.begin());

            if (it == world.entities.end())
                return "? ";

            switch (it->second->type) {
                case entityType::herb:
                    return "\033[31m♥\033[0m ";

                case entityType::carn:
                    return "\033[34m♠\033[0m ";
            }
        }else{
            return "\033[91m◎\033[0m ";
        }
    }

    return "? ";
}

void newEntity(int id, pii pos, entityType type, WorldState& world){
    switch(type){
        case entityType::herb:
            world.entities[id] = make_unique<herbivoreEntity>(id, pos);
            world.entityCount[entityType::herb].first++;
            break;

        case entityType::carn:
            world.entities[id] = make_unique<carnivoreEntity>(id, pos);
            world.entityCount[entityType::carn].first++;
            break;
    }
}

cell inputToCell(pii pos, const string& rep, WorldState& world){
    cell res;
    res.ids = {};

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
        res.type = cellType::n;
        res.ids.insert(getID());

        newEntity(*res.ids.begin(), pos, (rep == "♥") ? entityType::herb : entityType::carn, world);

        return res;
    }

    throw std::logic_error("Trying to process a wrong input character.");
    return res;
}

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
            res.grid[newPos.first][newPos.second].type = cellType::n;
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
        if(it->second->energy <= 0 || it->second->age > rules.maxAge){
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
            if(res.grid[i][j].ids.size() >= 1){
                res.grid[i][j].ids = {};
            }
        }
    }

    for(auto it = res.entities.begin(); it != res.entities.end(); ++it){
        pii pos = it->second->pos;

        if(res.inBounds(pos)){
            res.grid[pos.first][pos.second].ids.insert(it->first);
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
        return {std::move(res), rules};
    }

    int n, m;
    file >> n >> m;

    res.n = n;
    res.m = m;
    res.grid = vvc(n, vector<cell>(m));

    int delay, birthEnergy, startingEnergy, foodEnergy, entityEnergy, maxAge;
    float miraculousSpawnChance;
    int debug;
    file >> debug >> delay >> miraculousSpawnChance >> maxAge >> birthEnergy >> startingEnergy >> foodEnergy >> entityEnergy;
    rules.msDelay = delay;
    rules.maxAge = maxAge;
    rules.debug = (debug == 1);
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
    worldState = std::move(res.first);
    rules = std::move(res.second);
    print(worldState);
    
    while(true){
        this_thread::sleep_for(chrono::milliseconds(rules.msDelay));
        time++;
        system("clear"); 
        worldState = advanceTime(worldState);
        print(worldState);
        cout << '\n';
        printf("Herbivores -> \n\tAlive: %lli\tDead: %lli\n", worldState.entityCount[entityType::herb].first, worldState.entityCount[entityType::herb].second);
        printf("Carnivores -> \n\tAlive: %lli\tDead: %lli\n", worldState.entityCount[entityType::carn].first, worldState.entityCount[entityType::carn].second);
        if(worldState.entities.size() >= 1){
            printf("Gen: %d\n", time);
        }else{
            printf("Gen: %d - the world stays still...\n", time);
            break;
        }
    }

    return 0;
}