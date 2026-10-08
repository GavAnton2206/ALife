#include "utilities.hpp"

// default types of entities to spawn
struct herbivoreEntity : entity {
    herbivoreEntity(int id, pii pos, int energy = 100, int age = 0)
        : entity(id, entityType::h, pos, energy, age) {}
    pii decideMove(const WorldState& world){
        return pos;
    }
};

struct carnivoreEntity : entity {
    carnivoreEntity(int id, pii pos, int energy = 100, int age = 0)
        : entity(id, entityType::c, pos, energy, age) {}
    pii decideMove(const WorldState& world){
        return pos;
    }
};

// counter for entityID
int getID(){
    availableID++;
    return availableID;
}

// WorldState

WorldState worldState;

// Transforming state to its output view
string cellToString(const cell& cell, const WorldState& world){
    switch(cell.type){
        case cellType::n:
            return "· ";
            break;

        case cellType::p:
            return "♣ ";
            break;

        case cellType::e:
            auto it = world.entities.find(cell.id);

            if (it == world.entities.end())
                return "? ";

            switch (it->second->type) {
                case entityType::h:
                    return "♥ ";

                case entityType::c:
                    return "♠ ";
            }
            break;
    }

    return "? ";
}

void newEntity(int id, pii pos, entityType type, WorldState& world){
    switch(type){
        case entityType::h:
            world.entities[id] = make_unique<entity>(*(new herbivoreEntity(id, pos)));
            break;

        case entityType::c:
            world.entities[id] = make_unique<entity>(*(new carnivoreEntity(id, pos)));
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
    }else{
        res.type = cellType::e;
        res.id = getID();

        newEntity(res.id, pos, (rep == "♥") ? entityType::h : entityType::c, world);

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
    printf("Entity #%d on (%d, %d) with type %s\n", ent->id, ent->pos.first, ent->pos.second, (ent->type == entityType::h) ? "H" : "C");
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

// Importing grid from file
WorldState importWorld(ifstream& file){
    WorldState res;
    
    if (!file) {
        std::cerr << "Could not open file\n";
        return res;
    }

    int n, m;
    file >> n >> m;

    // process rules
    string line;

    // import grid
    res.n = n;
    res.m = m;
    res.grid = vvc(n, vector<cell>(m));

    for(int i = 0; i < n; i++){
        for(int j = 0; j < m; j++){
            string symbol;
            file >> symbol;
            res.grid[i][j] = inputToCell({i, j}, symbol, res);
        }
    }

    return res;
}

int main(){
    ifstream file("sgrid.txt");
    worldState = importWorld(file);
    print(worldState);
    
    while(true){
        this_thread::sleep_for(chrono::milliseconds(50));
        system("clear"); 
        // advance time
        print(worldState);
    }

    return 0;
}