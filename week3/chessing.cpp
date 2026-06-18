#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <tuple>
#include <chrono>
#include <algorithm>
#include <chess.hpp>

using namespace std;

#define int long long

const string PUZZLE_FILE="C:/Users/hp/OneDrive/Desktop/SOC-Chess/week3/m8n4.txt";
const int SEARCH_DEPTH=7;
const int MATE_SCORE=1000000;

const int EXACT=0;
const int LOWER=1;
const int UPPER=2;

struct TTEntry
{
    int score;
    vector<chess::Move> line;
    int flag;
};

struct Key
{
    uint64_t hash;
    int depth;
    bool operator==(const Key &o) const
    {
        return hash==o.hash && depth==o.depth;
    }
};

struct KeyHash
{
    size_t operator()(const Key &k) const
    {
        return hash<uint64_t>()(k.hash)^((size_t)k.depth<<1);
    }
};

class PuzzleLoader
{
public:

    static bool is_fen(const string &line)
    {
        vector<string> parts;
        string cur;

        for(char c:line)
        {
            if(c==' ')
            {
                parts.push_back(cur);
                cur.clear();
            }
            else
            {
                cur.push_back(c);
            }
        }
        parts.push_back(cur);

        if(parts.size()!=6)
            return false;

        if(parts[0].find('/')==string::npos)
            return false;

        if(parts[1]!="w" && parts[1]!="b")
            return false;

        return true;
    }

    static vector<string> load_fens(const string &filename)
    {
        vector<string> puzzles;
        ifstream fin(filename);
        string line;
        while(getline(fin,line))
        {
            if(is_fen(line))
                puzzles.push_back(line);
        }

        return puzzles;
    }
};

class Engine
{
public:

    int max_depth;
    int nodes;

    unordered_map<Key,TTEntry,KeyHash> move_map;

    Engine(int depth)
    {
        max_depth=depth;
        nodes=0;
    }
    vector<chess::Move> order_moves(chess::Board &board, chess::Movelist &moves)
    {
        vector<chess::Move> checks;
        vector<chess::Move> captures;
        vector<chess::Move> others;

        for(auto move:moves)
        {
            if(board.givesCheck(move)!=chess::CheckType::NO_CHECK)
            {
                checks.push_back(move);
            }
            else if(board.isCapture(move) || move.typeOf()==chess::Move::PROMOTION)
            {
                captures.push_back(move);
            }
            else
            {
                others.push_back(move);
            }
        }

        vector<chess::Move> result;

        result.insert(result.end(),checks.begin(),checks.end());
        result.insert(result.end(),captures.begin(),captures.end());
        result.insert(result.end(),others.begin(),others.end());

        return result;
    }

    pair<int,vector<chess::Move>>
    alpha_beta(chess::Board &board,int depth,int alpha,int beta)
    {
        int alpha_orig=alpha;
        nodes++;
        Key key{board.hash(),depth};
        auto it=move_map.find(key);
        if(it!=move_map.end())
        {
            auto &[score,line,flag]=it->second;
            if(flag==EXACT)
                return {score,line};
            if(flag==LOWER)
                alpha=max(alpha,score);
            else if(flag==UPPER)
                beta=min(beta,score);
            if(alpha>=beta)
                return {score,line};
        }

        int ply=max_depth-depth;
        chess::Movelist moves;
        chess::movegen::legalmoves(moves,board);

        if(moves.empty())
        {
            int score;
            if(board.inCheck())
                score=-(MATE_SCORE-ply);
            else
                score=0;
            move_map[key]={score,{},EXACT};
            return {score,{}};
        }
        
        if(board.isHalfMoveDraw() || board.isRepetition())
        {
            move_map[key]={0,{},EXACT};
            return {0,{}};
        }

        if(depth==0)
        {
            move_map[key]={0,{},EXACT};
            return {0,{}};
        }


        auto ordered=order_moves(board,moves);

        int best_score=-(int)1e18;
        vector<chess::Move> best_line;

        for(auto move:ordered)
        {
            board.makeMove(move);

            auto [child_score,child_line]=alpha_beta(board,depth-1,-beta,-alpha);

            board.unmakeMove(move);

            int score=-child_score;

            if(score>best_score)
            {
                best_score=score;

                best_line.clear();
                best_line.push_back(move);

                best_line.insert(
                    best_line.end(),
                    child_line.begin(),
                    child_line.end()
                );
            }

            alpha=max(alpha,best_score);

            if(alpha>=beta)
                break;
        }

        int flag;

        if(best_score<=alpha_orig)
            flag=UPPER;
        else if(best_score>=beta)
            flag=LOWER;
        else
            flag=EXACT;

        move_map[key]={best_score,best_line,flag};

        return {best_score,best_line};
    }

    pair<int,vector<chess::Move>>
    solve(chess::Board &board)
    {
        return alpha_beta(board,max_depth,-(int)1e18,(int)1e18);
    }
};


/*string describe_score(int score)
{
    if(llabs(score)>MATE_SCORE-1000)
    {
        int plies=MATE_SCORE-llabs(score);
        int moves=(plies+1)/2;

        string sign=(score>0)?"+":"-";

        return sign+"M"+to_string(moves);
    }

    return to_string(score);
}*/

int states_reached=0;
int total_puzzles=0;
int total_nodes=0;
int solved=0;
void solve_puzzle(const string &fen,int depth)
{
    chess::Board board(fen);

    Engine engine(depth);

    auto [score,mainline]=engine.solve(board);
    if(llabs(score)>MATE_SCORE-1000)
        solved++;
    states_reached+=engine.move_map.size();
    total_puzzles++;
    total_nodes+=engine.nodes;
}

signed main()
{
    auto start=chrono::high_resolution_clock::now();

    auto puzzles=PuzzleLoader::load_fens(PUZZLE_FILE);
    cout<<"Program is cooking.....\n";
    cout<<"=======================================\n";
    for(auto &fen:puzzles)
    {
        solve_puzzle(fen,SEARCH_DEPTH);
    }

    cout<<"Total puzzles: "<<total_puzzles<<"\n";
    cout<<"Puzzles solved: "<<solved<<"\n";
    cout<<"Avg states reached: "
        <<(double)states_reached/total_puzzles<<"\n";
    cout<<"Avg nodes: "
        <<(double)total_nodes/total_puzzles<<"\n";

    auto end=chrono::high_resolution_clock::now();

    double secs=
        chrono::duration<double>(end-start).count();

    cout<<"Time per puzzle: "<<secs/total_puzzles<<"\n";
    cout<<"Total time: "<<secs<<"\n";
}