#include <iostream>

enum GameStatus {
    CONTINUE,
    WIN,
    DRAW
};

class Game {
private:
    char board[4][4];
    char currentPlayer;

public:
    Game() : currentPlayer('X') {
        // Initialize the board with empty spaces
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                board[i][j] = ' ';
            }
        }
    }

    void displayBoard() const {
        std::cout << "  1 2 3 4\n";
        for (int i = 0; i < 4; ++i) {
            std::cout << i + 1 << " ";
            for (int j = 0; j < 4; ++j) {
                std::cout << board[i][j] << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    bool isValidMove(int row, int col) const {
        return (row >= 1 && row <= 4 && col >= 1 && col <= 4 && board[row - 1][col - 1] == ' ');
    }

    bool checkWin() const {
        // Check rows and columns
        for (int i = 0; i < 4; ++i) {
            if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][2] == board[i][3]) {
                return true; // Row win
            }
            if (board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[2][i] == board[3][i]) {
                return true; // Column win
            }
        }

        // Check diagonals
        if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[2][2] == board[3][3]) {
            return true; // Main diagonal win
        }
        if (board[0][3] != ' ' && board[0][3] == board[1][2] && board[1][2] == board[2][1] && board[2][1] == board[3][0]) {
            return true; // Anti-diagonal win
        }

        return false;
    }

    bool isBoardFull() const {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (board[i][j] == ' ') {
                    return false;
                }
            }
        }
        return true;
    }

    void switchPlayer() {
        currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    }

    GameStatus makeMove(int row, int col) {
        if (isValidMove(row, col)) {
            board[row - 1][col - 1] = currentPlayer;
            displayBoard();
            if (checkWin()) {
                std::cout << "Player " << currentPlayer << " wins!\n";
                return WIN;
            }

            if (isBoardFull()) {
                std::cout << "It's a draw!\n";
                return DRAW;
            }

            switchPlayer();
            return CONTINUE;
        } else {
            std::cout << "Invalid move! Try again.\n";
            return CONTINUE;
        }
    }

    char getCurrentPlayer() const {
        return currentPlayer;
    }
};

int main() {
    Game game;

    do {
        std::cout << "Player " << game.getCurrentPlayer() << "'s turn. Enter row and column (e.g., 2 3): ";
        int row, col;
        std::cin >> row >> col;
        GameStatus status = game.makeMove(row, col);

        if (status == WIN || status == DRAW) {
            game.displayBoard();
            break;
        }

    } while (true);

    return 0;
}