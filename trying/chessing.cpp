#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <tuple>
#include <chrono>
#include <algorithm>
#include <sstream>          // <-- added: needed for the UCI parser
#include <chess.hpp>

using namespace std;

#define int long long

const string PUZZLE_FILE="C:/Users/hp/OneDrive/Desktop/SOC-Chess/week3/m8n4.txt";
const int MATE_SCORE=1000000;
const int PLAY_DEPTH = 8;
const int EXACT=0;
const int LOWER=1;
const int UPPER=2;

const string START_FEN=
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

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

int material(chess::Board &board)
{
	int val[] = {100, 320, 330, 500, 900, 0}; // P N B R Q K
    int score = 0;
    for(int sq=0; sq<64; sq++)
    {
        chess::Piece p = board.at(chess::Square(sq));
        if(p == chess::Piece::NONE) continue;
        int v = val[(int)p.type()];
        if(p.color()==chess::Color::WHITE) 
			score += v;
        else                               
			score -= v;
    }
	return score;
}

int king_safety(chess::Board &board)
{
    // Opening/middlegame term only. In the endgame an active, central king
    // is GOOD, so a flat "stay home / stay castled" bonus is counterproductive.
    if(board.fullMoveNumber() > 20)
        return 0;

    const int CASTLED_BONUS   = 40;  // reward reaching a castled square
    const int LOST_RIGHTS_PEN = 40;  // punish giving up castling while uncastled

    // returns a "good for this color" value (white-positive handled by caller)
    auto eval_side = [&](chess::Color c) -> int
    {
        int ksq = board.kingSq(c).index();   // a1=0 ... h1=7, a8=56 ... h8=63

        bool kingside  = (c==chess::Color::WHITE) ? (ksq==6) : (ksq==62); // g1 / g8
        bool queenside = (c==chess::Color::WHITE) ? (ksq==2) : (ksq==58); // c1 / c8

        if(kingside || queenside)
            return CASTLED_BONUS;

        // not castled: neutral if we can still castle, bad if we threw it away
        bool hasRights = board.castlingRights().has(c);
        return hasRights ? 0 : -LOST_RIGHTS_PEN;
    };

    int score = 0;
    score += eval_side(chess::Color::WHITE);  // good for white -> add
    score -= eval_side(chess::Color::BLACK);  // good for black -> subtract
    return score;
}


int evaluate(chess::Board &board)
{	
	int finalscore=material(board)+king_safety(board);
    if(board.sideToMove()==chess::Color::BLACK)
        finalscore = -finalscore;

    return finalscore;
}


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
            int e = evaluate(board);
            move_map[key]={e,{},EXACT};
            return {e,{}};
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


string describe_score(int score)
{
    if(llabs(score)>MATE_SCORE-1000)
    {
        int plies=MATE_SCORE-llabs(score);
        int moves=(plies+1)/2;

        string sign=(score>0)?"+":"-";

        return sign+"M"+to_string(moves);
    }

    return to_string(score);
}


// ---- UCI mode: this is what Cute Chess talks to ----
void uci_loop()
{
    chess::Board board(START_FEN);
    string line;

    while(getline(cin,line))
    {
        istringstream iss(line);
        string token;
        iss>>token;

        if(token=="uci")
        {
            cout<<"id name MyEngine\n";
            cout<<"id author hp\n";
            cout<<"uciok\n"<<flush;
        }
        else if(token=="isready")
        {
            cout<<"readyok\n"<<flush;
        }
        else if(token=="ucinewgame")
        {
            board=chess::Board(START_FEN);
        }
        else if(token=="position")
        {
            string sub;
            iss>>sub;

            if(sub=="startpos")
            {
                board=chess::Board(START_FEN);
            }
            else if(sub=="fen")
            {
                string fen,part;
                for(int i=0;i<6 && iss>>part;i++)
                    fen += (i? " ":"") + part;
                board=chess::Board(fen);
            }

            // apply any moves that follow the "moves" keyword
            string mv;
            while(iss>>mv)
            {
                if(mv=="moves") continue;
                chess::Movelist ml;
                chess::movegen::legalmoves(ml,board);
                for(auto m:ml)
                    if(chess::uci::moveToUci(m)==mv){ board.makeMove(m); break; }
            }
        }
        else if(token=="go")
        {
            Engine engine(PLAY_DEPTH);
            auto [score,pv]=engine.solve(board);

            chess::Movelist ml;
            chess::movegen::legalmoves(ml,board);
            chess::Move best = pv.empty()
                ? (ml.empty()? chess::Move::NO_MOVE : ml[0])
                : pv[0];

            cout<<"bestmove "<<chess::uci::moveToUci(best)<<"\n"<<flush;
        }
        else if(token=="quit")
        {
            break;
        }
        // unknown tokens are silently ignored, as the UCI spec requires
    }
}

signed main()
{
    uci_loop();
}