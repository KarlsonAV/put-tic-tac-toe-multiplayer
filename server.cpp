#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <signal.h>
#include <queue>


#define SERVER_PORT 1235
#define MAX_GAMES 5

int serverSocket;

void signalHandler(int signum) {
    std::cout << "Received signal " << signum << ". Closing server socket.\n";
    close(serverSocket);
    exit(signum);
}


enum GameStatus {
    EMPTY,
    CONTINUE,
    WIN,
    DRAW
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
    int first_player_socket = -1;
    int second_player_socket = -1;
    GameStatus state = EMPTY;

    Game() : currentPlayer('X') {
        initializeBoard();
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

    int Add(int clientSocket, int idx) {
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
                    Add(clientSocket, i);
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
};


Games games;


void* clientThread(void* arg) {
    int* args = (int*) arg;
    int clientSocket = args[0];
    int gameId = args[1];
    std::cout << clientSocket << ' ' << gameId << std::endl;
    char server_buff[24];
    char client_buff[24];
    int n;
    
    char move;
    char move_status;
    char side;

    Game *game = games.games[gameId];

    std::string encoded_board = game->EncodeBoard();

    if (game -> first_player_socket == clientSocket) {
        move = '0'; 
        move_status = '0';
        side = 'X';

        server_buff[0] = move_status;
        server_buff[1] = move;
        server_buff[2] = side;

        for (int i = 0; i < encoded_board.size(); i++) {
            server_buff[i+3] = encoded_board[i];
        }
    }

    else {
        move = '1'; 
        move_status = '0';
        side = 'O';

        server_buff[0] = move_status;
        server_buff[1] = move;
        server_buff[2] = side;

        for (int i = 0; i < encoded_board.size(); i++) {
            server_buff[i+3] = encoded_board[i];
        }
    }

    std::cout << strlen(server_buff) << std::endl;
    for (int i = 0; i < strlen(server_buff); i++) {
        std::cout << server_buff[i] << std::endl;
    }

    int sent_n = write(clientSocket, server_buff, strlen(server_buff));
    // wait until game begins
    // send game info to players

    // while((n = recv(clientSocket, server_buff, 24, 0)) > 0) {
    //     // check the move
    //         // if valid -> send game state to players
    //         // else -> send error to the current player and check the move again
    //     // check the win/draw
    //         // if win -> send win and loose info to players
    //         // if draw -> send draw info to players
    // }
    
    pthread_exit(NULL);
}


void handleConnection(int clientSocket) {
    pthread_t thread_id;
    int i = games.Find(clientSocket);

    if (i >= 0) {
        if (games.games[i] -> first_player_socket < 0) games.games[i] -> first_player_socket = clientSocket;
        else if (games.games[i] -> first_player_socket > 0) games.games[i] -> second_player_socket = clientSocket;

        int args[2] = {clientSocket, i};

        if( pthread_create(&thread_id, NULL, clientThread, (void*) args) != 0 )
        printf("Failed to create thread\n");

        pthread_detach(thread_id);
    } 
    else {
        std::cout << "All games are full" << std::endl;
    }
}


int main() {
    // Game game;

    // do {
    //     std::cout << "Player " << game.getCurrentPlayer() << "'s turn. Enter row and column (e.g., 2 3): ";
    //     int row, col;
    //     std::cin >> row >> col;
    //     GameStatus status = game.makeMove(row, col);

    //     if (status == WIN || status == DRAW) {
    //         game.displayBoard();
    //         break;
    //     }

    // } while (true);

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
    if (listen(serverSocket, 10) == 0) {
        printf("Listening\n");
    }
    else {
        printf("Error\n");
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