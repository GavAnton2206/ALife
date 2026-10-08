#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <set>
#include <thread>
#include <chrono>

using namespace std;
using pos = pair<int, int>;

#define fi first
#define se second
#define pb push_back


// State that defines dead or alvie cell
enum state {
    d,
    a
};

// String that defines output view of dead and alive cell
const string dS = "· ";
const string aS = "█ ";

// Transforming state to its output view
map<state, string> toStr = {{d, dS}, {a, aS}};

// Struct for importing the information
struct imported {
    vector<vector<state>> grid;
    vector<int> S; // survive when
    vector<int> B; // born when
};

// General WorldState Class
class WorldState{
public:
    int n, m;
    vector<vector<state>> grid;
    vector<pos> alive;
    vector<int> B, S;
    
    // Constructors
    WorldState(const vector<vector<state>>& grid_, const vector<int>& B_ = {3}, const vector<int>& S_ = {2, 3}): grid(grid_), B(B_), S(S_){
        n = grid.size();
        if(n >= 1)
            m = grid[0].size();
        else
            m = 0;
        
        initializeAlive();
    }

    WorldState(int n_, int m_ = -1, const vector<int>& B_ = {3}, const vector<int>& S_ = {2, 3}): B(B_), S(S_){
        n = n_;
        if(m_ == -1){
            m = n;
        }else{
            m = m_;
        }

        grid = vector<vector<state>>(n, vector<state>(m));
        alive.clear();
    }

    void setRules(const vector<int>& B_, const vector<int>& S_){
        B = B_;
        S = S_;
    }

    // Update-related functions
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
    }

private:
    // Counting which cells are alive
    void initializeAlive(){
        alive.clear();

        for(int i = 0; i < n; i++){
            for(int j = 0; j < m; j++){
                if(grid[i][j] == a){
                    alive.pb({i, j});
                }
            }
        }
    }

    // Vectors of cells around a given, not including it
    vector<pos> cellsAround(pos Pos){
        int x = Pos.fi;
        int y = Pos.se;

        vector<pos> res;
        res.reserve(8);

        vector<pos> dirs = {{-1, -1}, {-1, 0}, {-1, 1},
                            {0, -1}, {0, 1},
                            {1, -1}, {1, 0}, {1, 1}};

        for(auto &to : dirs){
            pos nP = {x + to.fi, y + to.se};
            if(inBounds(nP))
                res.pb(nP);
        }

        return res;
    }

    // Check if a given cell is within bounds
    bool inBounds(pos Pos){
        int x = Pos.fi;
        int y = Pos.se;
        return (x >= 0) && (y >= 0) && (x <= n - 1) && (y <= m - 1);
    }
};

// Showing the grid to user
void print(const WorldState& grid){
    for(int i = 0; i < grid.n; i++){
        for(int j = 0; j < grid.m; j++){
            cout << toStr[grid.grid[i][j]];
        }
        cout << '\n';
    }
    cout << '\n' << '\n';
}

void print(const vector<vector<state>>& grid, int n, int m){
    for(int i = 0; i < n; i++){
        for(int j = 0; j < m; j++){
            cout << toStr[grid[i][j]];
        }
        cout << '\n';
    }
    cout << '\n' << '\n';
}

// Importing grid from file
imported importWorld(ifstream& file, char dead = 'D'){
    imported res;
    
    if (!file) {
        std::cerr << "Could not open file\n";
        return res;
    }

    int n, m;
    file >> n >> m;

    // process rules
    string line;
    file >> line;
    for(int j = 1; j < line.size(); j++){
        res.B.pb(line[j] - '0');
    }

    file >> line;
    for(int j = 1; j < line.size(); j++){
        res.S.pb(line[j] - '0');
    }

    // import grid
    res.grid = vector<vector<state>>(n, vector<state>(m));

    for(int i = 0; i < n; i++){
        file >> line;
        for(int j = 0; j < m; j++){
            res.grid[i][j] = (line[j] == dead) ? d : a;
        }
    }

    return res;
}

int main(){
    ifstream file("sgrid.txt");
    imported fileI = importWorld(file);
    WorldState grid(fileI.grid, fileI.B, fileI.S);
    print(grid);

    int n;
    while(true){
        this_thread::sleep_for(chrono::milliseconds(50));
        system("clear"); 
        grid.updateStep();
        print(grid);
    }

    return 0;
}