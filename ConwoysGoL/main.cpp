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

// General Grid Class
class Grid{
public:
    int n, m;
    vector<vector<state>> grid;
    vector<pos> alive;
    
    // Constructors
    Grid(const vector<vector<state>>& grid_){
        grid = grid_;
        n = grid.size();
        if(n >= 1)
            m = grid[0].size();
        else
            m = 0;
        
        initializeAlive();
    }

    Grid(int n_, int m_ = -1){
        n = n_;
        if(m_ == -1){
            m = n;
        }else{
            m = m_;
        }

        grid = vector<vector<state>>(n, vector<state>(m));
        alive.clear();
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
                newGrid[cell.fi][cell.se] = (aliveAround == 2 || aliveAround == 3) ? a : d;
            }else{
                newGrid[cell.fi][cell.se] = (aliveAround == 3) ? a : d;
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
void print(const Grid& grid){
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
vector<vector<state>> importGrid(ifstream& file, char dead = 'D'){
    if (!file) {
        std::cerr << "Could not open file\n";
        return vector<vector<state>>();
    }

    int n, m;
    file >> n >> m;

    vector<vector<state>> grid(n, vector<state>(m));

    for(int i = 0; i < n; i++){
        string line;
        file >> line;
        for(int j = 0; j < m; j++){
            grid[i][j] = (line[j] == dead) ? d : a;
        }
    }

    return grid;
}

int main(){
    ifstream file("sgrid.txt");
    vector<vector<state>> sgrid = importGrid(file);
    Grid grid(sgrid);
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