#include <getopt.h>
#include <iostream>
#include <vector>
#include <stack>
#include <algorithm>
#include <random>
#include <limits>
#include <memory>
using namespace std;
static constexpr int MC_TRIALS = 2000;
enum class node_state: char { OPEN, HUMAN, COMPUTER };
static ostream &operator <<(ostream &os, const node_state &ns){
    switch (ns) {
    case node_state::OPEN: os << '.'; break;

    case node_state::HUMAN: os << 'H'; break;

    case node_state::COMPUTER:  os << 'C'; break;
    }
    return os;
}
/*
    Board class for the hex game.
 */
class board {
int sz;
vector<node_state> b;
public:
board(int size) : sz{size}, b(size * size, node_state::OPEN){
    if (size < 2 || size > 13) {
        throw range_error("size out of range");
    }
}

node_state operator()(int idx) const {
    return b[idx];
}

node_state &operator()(int idx){
    return b[idx];
}

node_state &operator()(int row, int col){
    return b[row * sz + col];
}

node_state operator()(int row, int col) const {
    return b[row * sz + col];
}

int size() const {
    return sz;
}

board rotated() const {
    board b(sz);

    for (int row = 0; row < sz; ++row) {
        for (int col = 0; col < sz; ++col) {
            b(col, row) = (*this)(row, col);
        }
    }
    return b;
}

};

using opens_elem = pair<double, int>;
static ostream &operator<<(ostream &os, const vector<opens_elem> &v){
    os << "[";
    for (auto e: v) {
        os << "(" << e.first << "," << e.second << ")";
    }
    os << "]";
    return os;
}

static ostream &operator <<(ostream &os, const board &b){
    static const char hexchars[] = "123456789abcdef";
    int size = b.size();

    // Computer is top to bottom.
    os << string(7 + size / 2 * 3, ' ') << "C\n";
    os << "     ";
    // column number line.
    for (auto i = 0; i < size; ++i) {
        os << hexchars[i] << "   ";
    }
    os << '\n';
    for (auto row = 0; row < size; ++row) {
        if (row == size / 2) {
            os << "  H";
        } else{
            os << "   ";
        }
        os << string(row * 2, ' ') << hexchars[row] << ' ';
        for (auto col = 0; col < size; ++col) {
            os << b(row, col);
            if (col < size - 1) {
                os << " - ";
            }
        }
        if (row == size / 2) {
            os << "  H";
        }
        os << "\n    ";
        if (row < size - 1) {
            os << string(row * 2 + 1, ' ');
            for (auto col = 0; col < size - 1; ++col) {
                os << " \\ /";
            }
            os << " \\\n";
        }
    }
    os << string(size * 2 - 1, ' ');
    // column number line.
    for (auto i = 0; i < size; ++i) {
        os << hexchars[i] << "   ";
    }
    os << '\n';
    os << string(2 * size + 5 + (size) / 2 * 3, ' ') << "C\n";
    return os;
}

enum class winner_state: char { NONE, HUMAN, COMPUTER };

static ostream &operator<<(ostream &os, winner_state ws){
    switch (ws) {
    case winner_state::NONE: os << "none"; break;

    case winner_state::HUMAN: os << "human"; break;

    case winner_state::COMPUTER: os << "computer"; break;
    }
    return os;
}

/**
   Check to see who, if anybody, has won the game.

   Using a class instead of a function because the class wraps a few functions
   and some data.

   Human is left to right and computer is top to bottom. We could do this with a
   union-find and a connected graph algorithm but that may be overkill because
   the board graphs are so regular.  For checking the computer, I mark all nodes
   in the first column that are in state HUMAN as connected and push adjacent
   nodes onto a stack. Repeatedly pop the stack, check status for state==HUMAN
   and push unvisited adjacent nodes until the stack is empty (DFS from each
   first col element).  If any nodes in the last column are marked connected,
   then the human has won.

   Swap rows and columns and run the same algorithm to see if the
   computer has won.
 */
class check_winner {
const board &b;
const int size;
vector<char> visited;
vector<char> connected;
stack<int> to_process;
node_state ns;
void check_node(int idx){
    if (visited[idx]) {
        return;
    }

    visited[idx] = 1;
    if (ns != b(idx)) {
        return;
    }

    connected[idx] = 1;
    to_process.push(idx);
}

void above(int row, int col){
    if (row == 0) {
        return;
    }

    check_node((row - 1) * size + col);
}

void below(int row, int col){
    if (row >= size - 1) {
        return;
    }

    check_node((row + 1) * size + col);
}

void right(int row, int col){
    if (col >= size - 1) {
        return;
    }

    check_node(row * size + col + 1);
}

void left(int row, int col){
    if (col == 0) {
        return;
    }

    check_node(row * size + col - 1);
}

void above_right(int row, int col){
    if (col >= size - 1 || row == 0) {
        return;
    }

    check_node((row - 1) * size + col + 1);
}

void below_left(int row, int col){
    if (col == 0 || row >= size - 1) {
        return;
    }

    check_node((row + 1) * size + col - 1);
}

void run_adjacents(int idx){
    int row = idx / size;
    int col = idx % size;

    above(row, col);
    below(row, col);
    right(row, col);
    left(row, col);
    above_right(row, col);
    below_left(row, col);
}

public:
check_winner(const board &b) : b{b}, size(b.size()),
    visited(size * size, 0), connected(size * size, 0){
}

winner_state winner(){
    to_process = stack<int>();
    fill(visited.begin(), visited.end(), 0);
    fill(connected.begin(), connected.end(), 0);
    ns = node_state::HUMAN;
    for (auto row = 0; row < size; ++row) {
        check_node(row * size);
    }
    while (!to_process.empty()) {
        int idx = to_process.top();
        to_process.pop();
        run_adjacents(idx);
    }
    for (auto row = 0; row < size; ++row) {
        int idx = row * size + size - 1;
        if (connected[idx]) {
            return winner_state::HUMAN;
        }
    }
    fill(visited.begin(), visited.end(), 0);
    fill(connected.begin(), connected.end(), 0);
    to_process = stack<int>();
    ns = node_state::COMPUTER;
    for (auto col = 0; col < size; ++col) {
        check_node(col);
    }
    while (!to_process.empty()) {
        int idx = to_process.top();
        to_process.pop();
        run_adjacents(idx);
    }
    for (auto col = 0; col < size; ++col) {
        int idx = (size - 1) * size + col;
        if (connected[idx]) {
            return winner_state::COMPUTER;
        }
    }
    return winner_state::NONE;
}

};

/*
    Randomly fills out a board -- two idiots playing.
 */
static void fill_board(board &b, node_state next_turn = node_state::COMPUTER){
    static random_device rd;
    static default_random_engine dre(rd());
    int szs = b.size() * b.size();

    vector<int> opens;
    for (int i = 0; i < szs; ++i) {
        if (b(i) == node_state::OPEN) {
            opens.push_back(i);
        }
    }
    shuffle(opens.begin(), opens.end(), dre);
    for (int idx: opens) {
        b(idx) = next_turn;
        if (next_turn == node_state::COMPUTER) {
            next_turn = node_state::HUMAN;
        } else{
            next_turn = node_state::COMPUTER;
        }
    }
}
/*
    Randomly fill out a board MC_TRIALS times and return the percent of
    computer wins.
 */
static double rwins(const board &b,
                    node_state next_turn = node_state::COMPUTER){
    int wins{ 0 };

    for (int i = 0; i < MC_TRIALS; ++i) {
        board lb(b);
        fill_board(lb, next_turn);
        if (check_winner(lb).winner() == winner_state::COMPUTER) {
            ++wins;
        }
    }
    return (wins - MC_TRIALS / 2.0) / MC_TRIALS;
}

/**
   evaluation used at leaf nodes.
 */
static double leaf_eval(const board &b,
                        node_state next_turn = node_state::HUMAN){
    return rwins(b, next_turn);
}

/**
   evaluation used for pruning branches.
 */
static double branch_eval(const board &b,
                          node_state next_turn = node_state::HUMAN){
    return rwins(b, next_turn);
}

/*
Wraps the alpha/beta play engine and handling player moves.
 */
class player {
int branch_factor;
int search_depth;
unique_ptr<board> pb;

using alpha_beta_value = pair<double, int>;


using opens_elem = pair<double, int>;

/*
   Find and return the children of the current board.
 */
vector<opens_elem> find_children(int depth, bool maximize){
    board &b = *pb;
    int size = b.size();

    vector<opens_elem> opens;
    auto sortfn = maximize ?
                  [](opens_elem p1, opens_elem p2) {
                      return p1.first > p2.first;
                  } :
                  [](opens_elem p1, opens_elem p2) {
                      return p1.first < p2.first;
                  };
    auto eval = depth <= 1 ? leaf_eval : branch_eval;
    for (int idx = 0; idx < size * size; ++idx) {
        if (b(idx) == node_state::OPEN) {
            b(idx) = maximize ? node_state::COMPUTER : node_state::HUMAN;
            double e = eval(b,
                            (maximize ? node_state::HUMAN :
                             node_state::COMPUTER));
            b(idx) = node_state::OPEN;
            opens.push_back(opens_elem(e, idx));
        }
    }
    sort(opens.begin(), opens.end(), sortfn);
    if (depth <= 1) {
        opens.resize(min(1, int(opens.size())));
    } else{
        opens.resize(min(branch_factor, int(opens.size())));
    }
    return opens;
}
/*
    Top level of an alpha-beta search. Calls the recursive function and
    returns the best move.
 */
void computer_move(){
    (*pb)(alpha_beta(search_depth, true,
                     -numeric_limits<double>::max(),
                     numeric_limits<double>::max()).second)
        = node_state::COMPUTER;
}
/**
   Returns the (value,move) from an alpha,beta search.
 */
alpha_beta_value alpha_beta(int depth,
                            bool maximize,
                            double alpha,
                            double beta){

    node_state next_turn = maximize ? node_state::HUMAN : node_state::COMPUTER;
    board &b = *pb;

    vector<opens_elem> opens = find_children(depth, maximize);

    //    If there are no children or we are at our search depth, just return
    //    current eval.
    if (depth <= 0 || opens.size() == 0) {
        return alpha_beta_value(leaf_eval(b, next_turn), -1);
    }
    // We now have up to branch-size children to check.
    double v = maximize ? -numeric_limits<double>::max() :
               numeric_limits<double>::max();
    int move = 0;
    for (auto m: opens) {
        int idx = m.second;
        b(idx) = maximize ? node_state::COMPUTER : node_state::HUMAN;
        auto ab = alpha_beta(depth - 1, !maximize, alpha, beta);
        if (maximize) {
            if (ab.first > v) {
                move = idx;
                v = ab.first;
            }
            alpha = max(alpha, v);
        }else{
            if (ab.first < v) {
                move = idx;
                v = ab.first;
            }
            beta = min(v, beta);
        }
        b(idx) = node_state::OPEN;
        if (beta <= alpha) {
            break;
        }
    }
    return alpha_beta_value(v, move);
}

/**
   Get a legal move from the player and add it to the board.
 */
void get_move(){
    static const string hex = "123456789abcdef";
    board &b = *pb;

    while (cin) {
        cout << b;
        cout << "Input space separated row and column: ";
        string row;
        string col;
        cin >> row >> col;
        auto irow = find(hex.begin(), hex.end(), row[0]);
        if (irow != hex.end()) {
            auto icol = find(hex.begin(), hex.end(), col[0]);
            if (icol != hex.end()) {
                int r = irow - hex.begin();
                int c = icol - hex.begin();
                if (b(r, c) == node_state::OPEN) {
                    b(r, c) = node_state::HUMAN;
                    return;
                }
            }
        }
        cout << "try again\n";
        cin.ignore(numeric_limits<int>::max(), '\n');
    }
}

public:
void play(){
    board &b = *pb;
    bool human_turn = true;
    check_winner cw(b);
    winner_state ws;

    while ((ws = cw.winner()) == winner_state::NONE) {
        if (human_turn) {
            get_move();
        } else{
            computer_move();
        }
        human_turn = !human_turn;
    }
    cout << b <<  "winner: " << ws << "\n";
}

player(int board_size = 11,
       int branch_factor = 10,
       int search_depth = 10) :
    branch_factor(branch_factor),
    search_depth(search_depth),
    pb(new board(board_size)) {
}

/**
    Can't use default copying with a unique_ptr.
 */
player(const player &o) : branch_factor(o.branch_factor),
    search_depth(o.search_depth),
    pb(new board(*o.pb)){
}

player &operator=(const player &o){
    if (this != &o) {
        branch_factor = o.branch_factor;
        search_depth = o.search_depth;
        pb.reset(new board(*o.pb));
    }
    return *this;
}
void reset(){
    pb.reset(new board(pb->size()));
}
};

static void usage() {
    cout << "usage: hex [-s <board_size>] [-d <search depth>] "
        "[-f <branch_factor>]\n";
    exit(1);
}

struct {
    int board_size;
    int branch_factor;
    int search_depth;
} prog_args = {
    4, 5, 5,
};
static void get_args(int argc, char **argv){
    int c;

    while ((c = getopt(argc, argv, "d:f:s:")) != -1) {
        switch (c) {
        case 's':
            prog_args.board_size = max(3, min(18, atoi(optarg)));
            break;
        case 'f':
            prog_args.branch_factor = max(1, min(20, atoi(optarg)));
            break;
        case 'd':
            prog_args.search_depth = max(1, min(20, atoi(optarg)));
            break;
        case '?':
            usage();
        }
    }
}

/**
   board size, search depth, branch size.
 */
int main(int argc, char **argv){
    get_args(argc, argv);
    player p(prog_args.board_size,
             prog_args.branch_factor,
             prog_args.search_depth);
    p.play();
    return 0;
}

/**
   Local Variables:
   compile-command: "g++ -Wall -O3 -std=gnu++14 -o hex hex.cpp"
   End:
 */
