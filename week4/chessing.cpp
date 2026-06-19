#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <tuple>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <chess.hpp>
using namespace chess;
using namespace std;

#define int long long
#define loop(i,n) for(int i=0;i<n;i++)
#define all(v) v.begin(),v.end()
#define vMove vector<Move>

const int MATE_SCORE=1000000;
const int MATE_BOUND=MATE_SCORE-1000;   // MATE IN X walo ka isse jyada hoga
const int INF=(int)1e18;

const int MAX_PLY=64; //iterative deepening safe limit
const int PLAY_DEPTH=8; // fallback depth when no clock is given
const int ENDGAME_THRESHOLD = 2500; //itne centipawns have to come off the board before allowing king to do masti

const int EXACT=0;
const int LOWER=1;
const int UPPER=2;

const string START_FEN="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

const int PIECE_VAL[6] = {100, 320, 330, 500, 900, 0}; // P N B R Q K

//stolen tables from ai
const int PST_PAWN[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};
const int PST_KNIGHT[64] = {
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50
};
const int PST_BISHOP[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -20,-10,-10,-10,-10,-10,-10,-20
};
const int PST_ROOK[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};
const int PST_QUEEN[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20
};
const int PST_KING_MG[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20
};
const int PST_KING_EG[64] = {
   -50,-40,-30,-20,-20,-30,-40,-50,
   -30,-20,-10,  0,  0,-10,-20,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-30,  0,  0,  0,  0,-30,-30,
   -50,-30,-30,-30,-30,-30,-30,-50
};
//

const int* PST[6] = {PST_PAWN, PST_KNIGHT, PST_BISHOP, PST_ROOK, PST_QUEEN, PST_KING_MG};//Fancy way to index the tables

struct TTEntry
{
    int score;
    int depth;
    int flag;
    Move best;
};
int evaluate(Board &board)
{
	//endgame activation?
    int npm=0;
    for(int sq=0;sq<64;sq++)
    {
        Piece p=board.at(Square(sq));
        if(p==Piece::NONE) 
			continue;

        int taip=(int)p.type(); //piece type P N B R Q K
        if(taip!=0) //skip pawns 
			npm+=PIECE_VAL[taip];
    }
    bool endgame=(npm<=ENDGAME_THRESHOLD);
	//

    int score=0;
    for(int sq=0;sq<64;sq++)
    {
        Piece p = board.at(Square(sq));
        if(p==Piece::NONE) 
			continue;

        int t=(int)p.type(); // P N B R Q K

        const int* tbl=(t==5)?(endgame?PST_KING_EG:PST_KING_MG):PST[t]; //choosing piece table correctly
        int v=PIECE_VAL[t]; 

        if(p.color()==Color::WHITE) 
			score+=v+tbl[sq^56]; //fancy way to flip stuff but yeah
        else                               
			score-=v+tbl[sq];
    }
	// score is now material + its positioning

    if(board.sideToMove()==Color::BLACK) 
		score = -score;
    return score;
}

int mvv_lva(Board &board, const Move &m)
{
    int killer=(int)board.at(m.from()).type();
    int victim;
    if(m.typeOf()==Move::ENPASSANT) 
		victim=0; 
    else                                   
		victim=(int)board.at(m.to()).type();
    if(victim>5) 
		victim=0;
    return PIECE_VAL[victim]*50-killer;
}

class Engine
{
public:
    int nodes = 0;
    unordered_map<uint64_t, TTEntry> tt; /*abhi transposition table seems an apt name 
	since it is sufficiently complicated business*/
    chrono::steady_clock::time_point start;
    int budget_ms = 0;
    bool timed = 0;
    bool stopped = 0;

    void new_search()
    {
        nodes = 0;
        stopped = 0;
        start = chrono::steady_clock::now();
    }

    int elapsed_ms()
    {
        auto now = chrono::steady_clock::now();
        return chrono::duration_cast<chrono::milliseconds>(now-start).count();
    }

    void check_time()
    {
        if( timed && ((nodes&2047)==0) && (elapsed_ms() >= budget_ms) )
            stopped = true;
    }

    vMove order_moves(Board &board,Movelist &moves,Move tt_move)
    {
        vector<pair<int,Move>> scored;
        scored.reserve(moves.size());

        for(auto move:moves)
        {
            int s;
            if(move==tt_move)
                s=(int)1e9;                       
            else if(board.isCapture(move) || move.typeOf()==Move::PROMOTION)
                s=(int)1e6 + mvv_lva(board, move);  
            else
                s=0;                               
            scored.push_back({s,move});
        }//move order is tt, captures (mvv lva), quiets

		auto compare=[](const pair<int,Move>&a,const pair<int,Move>&b){
			return a.first>b.first;
		};
        sort(all(scored),compare);

        vMove result(scored.size());
        loop(i,scored.size())
			result[i]=scored[i].second;
        return result;
    }


    int quiescence(Board &board,int alpha,int beta,int ply)//first complete trade sequence then only return eval
    {
        nodes++;
        check_time();
        if(stopped) 
			return 0;
        if(board.isHalfMoveDraw() || board.isRepetition())
            return 0;

        bool in_check = board.inCheck();
        Movelist moves;
        int best;

        if(in_check) 
        {
            movegen::legalmoves(moves, board);
            if(moves.empty()) 
				return -(MATE_SCORE-ply); //checks if checkmate 
            best=-INF; // all possibilities open
        }
        else
        {
            int stand = evaluate(board);
            if(stand>=beta) 
				return stand;               
            if(stand>alpha) 
				alpha=stand;
            best=stand;
            movegen::legalmoves<movegen::MoveGenType::CAPTURE>(moves,board);
        }

        auto ordered = order_moves(board,moves,Move::NO_MOVE);

        for(auto move:ordered)
        {
            board.makeMove(move);
            int score = -quiescence(board,-beta,-alpha,ply+1);
            board.unmakeMove(move);
            if(stopped) 
				return 0;
            if(score>best)  
				best=score;
            if(score>alpha) 
				alpha=score;
            if(alpha>=beta) 
				break;
        }
        return best;
    }

    int alpha_beta(Board &board,int depth,int alpha,int beta,int ply,vMove& pv)
    {
        pv.clear();
        nodes++;
        check_time();
        if(stopped) 
			return 0;

        int alpha_orig = alpha;
        uint64_t h = board.hash();

        Move tt_move = Move::NO_MOVE;
        auto it = tt.find(h);
        if(it!=tt.end())
        {
            TTEntry &e = it->second;
            tt_move = e.best;
            if(e.depth>=depth && ply>0)
            {
                int s = e.score;
                if(s>MATE_BOUND) 
					s -= ply;
                if(s<-MATE_BOUND) 
					s += ply;
                if(e.flag==EXACT) 
				{ 
					pv={e.best}; 
					return s; 
				}
                if(e.flag==LOWER) 
					alpha = max(alpha, s);
                else if(e.flag==UPPER) 
					beta = min(beta,s);
                if(alpha>=beta) 
				{ 
					pv={e.best}; 
					return s; 
				}
            }
        }

        Movelist moves;
        movegen::legalmoves(moves,board);

        if(moves.empty()) //mate or stalemate
			if(board.inCheck())
				return -(MATE_SCORE-ply);
			else
				return 0; 

        if(board.isHalfMoveDraw() || board.isRepetition()) //finiteness rules
            return 0;

        if(depth<=0) 
            return quiescence(board,alpha,beta,ply);

        auto ordered=order_moves(board,moves,tt_move);

        int best_score=-INF;
        Move best_move=ordered[0];
        vMove child_pv;

        for(auto move:ordered)
        {
            board.makeMove(move);
            int score = -alpha_beta(board,depth-1,-beta,-alpha,ply+1,child_pv);
            board.unmakeMove(move);
            if(stopped) 
				return 0;

            if(score>best_score)
            {
                best_score=score;
                best_move=move;
                pv.clear();
                pv.push_back(move);
                pv.insert(pv.end(),all(child_pv));
            }
            alpha = max(alpha,best_score);
            if(alpha >= beta) break;
        }

        // ---- TT store (mate scores converted to be node-relative) ----------
		int flag=-1;
		if((best_score<=alpha_orig))
			flag=UPPER;
		else if(best_score>=beta)
			flag=LOWER;
		else
			flag=EXACT;

        int stored = best_score;
        if(stored>MATE_BOUND) 
			stored += ply;
        if(stored<-MATE_BOUND) 
			stored-=ply;
        tt[h]=TTEntry{stored,depth,flag,best_move};

        return best_score;
    }
};

// UCI score string: "cp x" for normal scores, "mate y" for forced mates.
string uci_score(int score)
{
    if(llabs(score)>MATE_BOUND)
    {
        int plies=MATE_SCORE-llabs(score);
        int moves=(plies+1)/2;
        return "mate "+to_string(score>0?moves:-moves);
    }
    return "cp "+to_string(score);
}

//try all depths from 1 to i, stop if seems wont finish depth
Move search_best(Engine &engine,Board &board,int max_depth,int budget_ms,bool timed)
{
    engine.timed=timed;
    engine.budget_ms=budget_ms;
    engine.new_search();

    Movelist root;
    movegen::legalmoves(root,board);
    if(root.empty()) 
		return Move::NO_MOVE;

    Move best=root[0];

    for(int depth=1; depth<=max_depth; depth++)
    {
        vMove pv;
        int score=engine.alpha_beta(board,depth,-INF,INF,0,pv);
        if(engine.stopped) 
			break;          

        if(!pv.empty())    
			best=pv[0];

        // tell the GUI what we're thinking
        cout << "info depth " << depth<<" score "<< uci_score(score)
             << " nodes " << engine.nodes
             << " time " << engine.elapsed_ms()
             << " pv";
        for(auto m:pv) 
			cout<<" "<< uci::moveToUci(m);
        cout<<"\n"<<flush;

        if(llabs(score)>MATE_BOUND) 
			break; //found a forced mate, stop early

        //don't start a depth we can't finish
        if(timed && engine.elapsed_ms()*2>=budget_ms) 
			break;
    }
    return best;
}

// ----------------------------------------------------------------------------
//  UCI loop (stolen from ai)
// ----------------------------------------------------------------------------
void uci_loop()
{
    Board board(START_FEN);
    Engine engine;           
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
            board=Board(START_FEN);
            engine.tt.clear();
        }
        else if(token=="position")
        {
            string sub;
            iss>>sub;
            if(sub=="startpos")
            {
                board=Board(START_FEN);
            }
            else if(sub=="fen")
            {
                string fen,part;
                for(int i=0;i<6 && iss>>part;i++)
                    fen += (i? " ":"") + part;
                board=Board(fen);
            }
            string mv;
            while(iss>>mv)
            {
                if(mv=="moves") continue;
                Movelist ml;
                movegen::legalmoves(ml,board);
                for(auto m:ml)
                    if(uci::moveToUci(m)==mv){ 
						board.makeMove(m); 
						break; 
					}
            }
        }
        else if(token=="go")
        {
            // parse the clock
            int wtime=-1, btime=-1, winc=0, binc=0, movetime=-1, depth=-1, movestogo=-1;
            string t;
            while(iss>>t)
            {
                if     (t=="wtime")     iss>>wtime;
                else if(t=="btime")     iss>>btime;
                else if(t=="winc")      iss>>winc;
                else if(t=="binc")      iss>>binc;
                else if(t=="movetime")  iss>>movetime;
                else if(t=="depth")     iss>>depth;
                else if(t=="movestogo") iss>>movestogo;
            }

            int  max_depth = MAX_PLY;
            int  budget    = 0;
            bool timed     = true;

            if(movetime>0)
            {
                budget = movetime - 30;                    // small safety margin
            }
            else if(wtime>=0 || btime>=0)
            {
                bool white = (board.sideToMove()==Color::WHITE);
                int  mytime = white ? wtime : btime;
                int  myinc  = white ? winc  : binc;
                if(mytime<0) mytime=0;
                int  togo  = (movestogo>0) ? movestogo : 25;
                budget = mytime/togo + myinc/2 - 30;
            }
            else
            {
                // no clock info -> behave like the old fixed-depth engine
                timed     = false;
                max_depth = (depth>0) ? depth : PLAY_DEPTH;
            }

            if(depth>0) max_depth = min(max_depth, depth);
            if(timed && budget < 5) budget = 5;            // never go below 5ms

            Move best = search_best(engine, board, max_depth, budget, timed);
            if(best==Move::NO_MOVE)
            {
                Movelist ml;
                movegen::legalmoves(ml,board);
                if(!ml.empty()) best = ml[0];
            }
            cout<<"bestmove "<<uci::moveToUci(best)<<"\n"<<flush;
        }
        else if(token=="quit")
        {
            break;
        }
    }
}

signed main()
{
    uci_loop();
}