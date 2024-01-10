#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <sstream>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <signal.h>
#include <queue>


#define SERVER_PORT 1234
#define MAX_GAMES 5

// SERVER Messages
#define WAIT '9'
#define FIRST_PLAYER_MOVE '0'
#define SECOND_PLAYER_MOVE '1'
#define VALID_MOVE '0'
#define INVALID_MOVE '1'
#define SIDE_X 'X'
#define SIDE_O 'O'
#define WIN_MOVE '2'
#define LOSE_MOVE '3'
#define DRAW_MOVE '4'
#define DISCONNECTED_MOVE '8'


int serverSocket;

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ". Closing server socket.\n";
    close(serverSocket);
    exit(0);
}

enum GameStatus {
    EMPTY,
    FINISHED,
    WAITING,
    ONGOING,
    DISCONNECTED
};

enum MoveStatus {
    CONTINUE,
    WIN,
    DRAW,
    INVALID
};

class Game {
private:
    char board[4][4];
    char currentPlayer;

    void initializeBoard() {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                board[i][j] = ' ';
            }
        }
    }

public:
    int first_player_socket;
    int second_player_socket;
    GameStatus state;

    Game() : currentPlayer('X') {
        first_player_socket = -1;
        second_player_socket = -1;
        state = EMPTY;
        initializeBoard();
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

    MoveStatus makeMove(int row, int col) {
        if (isValidMove(row, col)) {
            board[row - 1][col - 1] = currentPlayer;
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
            return INVALID;
        }
    }

    std::string EncodeBoard() {
        std::string boardEncoded;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                boardEncoded += board[i][j];
            }
        }

        return boardEncoded;
    }

    char getCurrentPlayer() const {
        return currentPlayer;
    }

    void setFirstPlayer(int clientSocket) {
        first_player_socket = clientSocket;
    }

    void setSecondPlayer(int clientSocket) {
        second_player_socket = clientSocket;
    }
};


class Games {
public:
    Game* games[MAX_GAMES];
    std::queue<int> games_queue;

    Games() {
        for (int i = 0; i < MAX_GAMES; i++) {
            games[i] = nullptr;
        }
    }

    int Add(int idx) {
        games[idx] = new Game();
        games_queue.push(idx);

        return 0;
    }

    int Find(int clientSocket) {
        // Add game if there is no games
        int i = 0;
        int nullptrCount = 0;
        if (games_queue.empty()) {
            for (; i < MAX_GAMES; i++) {
                if (games[i] == nullptr) {
                    Add(i);
                    break;
                }
                else {
                    nullptrCount++;
                }
            }
            if (nullptrCount == MAX_GAMES) return -1; // return -1 if all games are full
        }
        else {
            // get the first game from the games queue and pop it
            i = games_queue.front(); 
            games_queue.pop();
        }
        return i; // idx of the found game
    }

    int Pair(int clientSocket, int idx) {
        if (idx <= MAX_GAMES && games[idx] != nullptr) {
            if (games[idx] -> first_player_socket > 0) {
                games[idx] -> second_player_socket = clientSocket;
                return 0;
            }
        }
        return 1;
    }

    void Delete(int idx) {
        delete games[idx];
        games[idx] = nullptr;
    }
};


Games games;

bool extractRowAndColumn(const char* input, int& row, int& column) {
    if (strlen(input) < 3) return false;

    std::istringstream iss(input);
    return (iss >> row >> column) ? true : false;
}

void sendMoveResult(int socket, char result, char playerMove, char playerSide, Game* game) {
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));

    buffer[0] = result;
    buffer[1] = playerMove;
    buffer[2] = playerSide;

    // Copy encoded board to buffer
    std::string encodedBoard = game->EncodeBoard();

    for (int i = 0; i < encodedBoard.size(); i++) {
        buffer[i + 3] = encodedBoard[i];
    }

    write(socket, buffer, strlen(buffer));
}

void* clientThread(void* arg) {
    int* args = (int*) arg;
    int clientSocket = args[0];
    int gameId = args[1];
    delete[] args;

    Game *game = games.games[gameId];
    std::cout << "Player " << clientSocket << " Joins game " << gameId << std::endl;
    
    char client_buff[256];
    int n;

    // wait until game begins
    while (game -> state == WAITING) continue;

    // send initial game info
    sendMoveResult(clientSocket, VALID_MOVE, FIRST_PLAYER_MOVE, (game->first_player_socket == clientSocket) ? SIDE_X : SIDE_O, game);
   
    int row, column;
    bool validRowAndColumn;
    MoveStatus moveStatus;

    memset(&client_buff, 0, sizeof(client_buff));

    while (games.games[gameId] != nullptr && (n = recv(clientSocket, client_buff, sizeof(client_buff), 0)) > 0) {
        validRowAndColumn = extractRowAndColumn(client_buff, row, column);

        if (game -> first_player_socket == clientSocket) {
            if (validRowAndColumn) {
                moveStatus = game -> makeMove(row, column);

                if (moveStatus == INVALID) {
                    sendMoveResult(clientSocket, INVALID_MOVE, FIRST_PLAYER_MOVE, SIDE_X, game);
                }

                else if (moveStatus == CONTINUE) {
                    sendMoveResult(clientSocket, VALID_MOVE, SECOND_PLAYER_MOVE, SIDE_X, game);
                    sendMoveResult(game -> second_player_socket, VALID_MOVE, SECOND_PLAYER_MOVE, SIDE_O, game);
                }
                
                else if (moveStatus == WIN) {
                    game -> state = FINISHED;
                    sendMoveResult(clientSocket, WIN_MOVE, SECOND_PLAYER_MOVE, SIDE_X, game);
                    sendMoveResult(game -> second_player_socket, LOSE_MOVE, SECOND_PLAYER_MOVE, SIDE_O, game);
                }

                else if (moveStatus == DRAW) {
                    game -> state = FINISHED;
                    sendMoveResult(clientSocket, DRAW_MOVE, SECOND_PLAYER_MOVE, SIDE_X, game);
                    sendMoveResult(game -> second_player_socket, DRAW_MOVE, SECOND_PLAYER_MOVE, SIDE_O, game);
                }
            }
            else {
                sendMoveResult(clientSocket, INVALID_MOVE, FIRST_PLAYER_MOVE, SIDE_X, game);
            }
        }

        else if (game -> second_player_socket == clientSocket) {
            if (validRowAndColumn) {
                moveStatus = game -> makeMove(row, column);
                
                if (moveStatus == INVALID) {
                    sendMoveResult(clientSocket, INVALID_MOVE, SECOND_PLAYER_MOVE, SIDE_O, game);
                }

                else if (moveStatus == CONTINUE) {
                    sendMoveResult(clientSocket, VALID_MOVE, FIRST_PLAYER_MOVE, SIDE_O, game);
                    sendMoveResult(game -> first_player_socket, VALID_MOVE, FIRST_PLAYER_MOVE, SIDE_X, game);
                }

                else if (moveStatus == WIN) {
                    game -> state = FINISHED;
                    sendMoveResult(clientSocket, WIN_MOVE, FIRST_PLAYER_MOVE, SIDE_O, game);
                    sendMoveResult(game -> first_player_socket, LOSE_MOVE, FIRST_PLAYER_MOVE, SIDE_X, game);
                }

                else if (moveStatus == DRAW) {
                    game -> state = FINISHED;
                    sendMoveResult(clientSocket, DRAW_MOVE, FIRST_PLAYER_MOVE, SIDE_O, game);
                    sendMoveResult(game -> first_player_socket, DRAW_MOVE, FIRST_PLAYER_MOVE, SIDE_X, game);
                }
            }
            else {
                sendMoveResult(clientSocket, INVALID_MOVE, SECOND_PLAYER_MOVE, SIDE_O, game);
            }
        }
        memset(&client_buff, 0, sizeof(client_buff));
        if (game -> state == FINISHED && (games.games[gameId] != nullptr)) {
            games.Delete(gameId);
            std::cout << "Game " << gameId << " has finished." << std::endl;
            break;
        }
    }

    // Delete game if opponent disconnected
    if (game -> state == DISCONNECTED) {
        games.Delete(gameId);
        std::cout << "Game " << gameId << " has finished due to Disconnection." << std::endl;
    }
    
    // Handle disconnection
    else if (game -> state == ONGOING) {
        game -> state = DISCONNECTED;
        int opponentPlayer;

        if (clientSocket == game -> first_player_socket) {
            opponentPlayer = game -> second_player_socket;
            sendMoveResult(opponentPlayer, DISCONNECTED_MOVE, SECOND_PLAYER_MOVE, SIDE_O, game);

        }
        else {
            opponentPlayer = game -> first_player_socket;
            sendMoveResult(opponentPlayer, DISCONNECTED_MOVE, FIRST_PLAYER_MOVE, SIDE_X, game);
        }
    }

    std::cout << "Client " << clientSocket << " exit Thread." << std::endl;
    close(clientSocket);
    pthread_exit(NULL);
}


void handleConnection(int clientSocket) {
    pthread_t thread_id;
    int i = -1;
    
    while (i < 0) {
        i = games.Find(clientSocket);
        if (i >= 0) {
            if (games.games[i] -> first_player_socket < 0) {
                games.games[i] -> first_player_socket = clientSocket;
                games.games[i] -> state = WAITING;
            }
            else if (games.games[i] -> first_player_socket > 0) {
                games.games[i] -> second_player_socket = clientSocket;
                games.games[i] -> state = ONGOING;
            }

            int* args = new int[2];
            args[0] = clientSocket;
            args[1] = i;

            if( pthread_create(&thread_id, NULL, clientThread, (void*) args) != 0 ) {
                std::cout << "Failed to create thread" << std::endl;
                delete[] args;
            }
            
            pthread_detach(thread_id);
        } 
        else {
            usleep(2000);
        }
    }
    
}


int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    int server_port = SERVER_PORT;

    //Create the socket. 
    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    
    signal(SIGINT, signalHandler);
    // Configure settings of the server address struct
    // Address family = Internet 
    serverAddr.sin_family = AF_INET;

    //Set port number, using htons function to use proper byte order 
    serverAddr.sin_port = htons(server_port);

    //Set IP address to localhost 
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    //Set all bits of the padding field to 0 
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

    //Bind the address struct to the socket 
    bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    //Listen on the socket
    if (listen(serverSocket, MAX_GAMES * 2 + 2) == 0) {
        std::cout << "Listening" << std::endl;
    }
    else {
        std::cout << "Error" << std::endl;
        return 1;
    }

    while(1)
    {
        //Accept call creates a new socket for the incoming connection
        addr_size = sizeof serverStorage;
        clientSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);
        handleConnection(clientSocket);
    }
    
    close(serverSocket);
    return 0;
}