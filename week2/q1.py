import json
import copy  # use it for deepcopy if needed
import math  # for math.inf
import logging

logging.basicConfig(format='%(levelname)s - %(asctime)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S',
                    level=logging.INFO)

# Global variables in which you need to store player strategies (this is data structure that'll be used for evaluation)
# Mapping from histories (str) to probability distribution over actions
strategy_dict_x = {}
strategy_dict_o = {}

WIN_LINES=[
    [0,1,2],
    [3,4,5],
    [6,7,8],
    [0,3,6],
    [1,4,7],
    [2,5,8],
    [0,4,8],
    [2,4,6]
]
class History:
    def __init__(self, history=None):
        """
        # self.history : Eg: [0, 4, 2, 5]
            keeps track of sequence of actions played since the beginning of the game.
            Each action is an integer between 0-8 representing the square in which the move will be played as shown
            below.
              ___ ___ ____
             |_0_|_1_|_2_|
             |_3_|_4_|_5_|
             |_6_|_7_|_8_|

        # self.board
            empty squares are represented using '0' and occupied squares are either 'x' or 'o'.
            Eg: ['x', '0', 'x', '0', 'o', 'o', '0', '0', '0']
            for board
              ___ ___ ____
             |_x_|___|_x_|
             |___|_o_|_o_|
             |___|___|___|

        # self.player: 'x' or 'o'
            Player whose turn it is at the current history/board

        :param history: list keeps track of sequence of actions played since the beginning of the game.
        """
        if history is not None:
            self.history = history
            self.board = self.get_board()
        else:
            self.history = []
            self.board = ['0', '0', '0', '0', '0', '0', '0', '0', '0']
        self.player = self.current_player()

    def current_player(self):
        """ Player function
        Get player whose turn it is at the current history/board
        :return: 'x' or 'o' or None
        """
        total_num_moves = len(self.history)
        if total_num_moves < 9:
            if total_num_moves % 2 == 0:
                return 'x'
            else:
                return 'o'
        else:
            return None

    def get_board(self):
        """ Play out the current self.history and get the board corresponding to the history in self.board.

        :return: list Eg: ['x', '0', 'x', '0', 'o', 'o', '0', '0', '0']
        """
        board = ['0', '0', '0', '0', '0', '0', '0', '0', '0']
        for i in range(len(self.history)):
            if i % 2 == 0:
                board[int(self.history[i])] = 'x'
            else:
                board[int(self.history[i])] = 'o'
        return board

    def state(self):
        board=self.board
        for line in WIN_LINES:
            a,b,c=line
            if board[a]!='0' and board[a]==board[b] and board[b]==board[c]:
                if board[a]=='x':
                    return 1
                return -1
        if '0' not in board:
            return 0
        return 2


def backward_induction(history_obj):
    """
    :param history_obj: Histroy class object
    :return: best achievable utility (float) for th current history_obj
    """
    state=history_obj.state()
    if state!=2:
        return state
    
    global strategy_dict_x, strategy_dict_o

    actions=[]
    best_action="-1"
    for i in range(9):
        if history_obj.board[i]=='0':
            actions.append(str(i))

    if history_obj.player=='x':
        best_val=-math.inf
        for action in actions:
            nxt=History(history_obj.history+[action])
            cur=backward_induction(nxt)
            if cur>best_val:
                best_val=cur
                best_action=action
    else:
        best_val=math.inf
        for action in actions:
            nxt=History(history_obj.history+[action])
            cur=backward_induction(nxt)
            if cur<best_val:
                best_val=cur
                best_action=action
    

    history_string=""
    for i in history_obj.history:
        history_string+=str(i)
    if history_obj.current_player()=='x':
        strategy_dict_x[history_string]={"0":0,"1":0,"2":0,"3":0,"4":0,"5":0,"6":0,"7":0,"8":0}
        strategy_dict_x[history_string][best_action]=1
    else:
        strategy_dict_o[history_string]={"0":0,"1":0,"2":0,"3":0,"4":0,"5":0,"6":0,"7":0,"8":0}
        strategy_dict_o[history_string][best_action]=1
    return best_val
     

def solve_tictactoe():
    backward_induction(History())
    with open('./policy_x.json', 'w') as f:
        json.dump(strategy_dict_x, f)
    with open('./policy_o.json', 'w') as f:
        json.dump(strategy_dict_o, f)
    return strategy_dict_x, strategy_dict_o


if __name__ == "__main__":
    logging.info("Start")
    solve_tictactoe()
    logging.info("End")
