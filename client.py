import socket

HOST = "127.0.0.1"
PORT = 1235

def display_board(encoded_board: list) -> None:
    print("  1 2 3 4")
    for i in range(4):
        print(f"{i + 1} ", end="")
        for j in range(4):
            print(f"{encoded_board[i * 4 + j]} ", end="")
        print()
    print()

def check_your_move(move: str, side: str) -> bool:
    return (move == '0' and side == 'X') or (move == '1' and side == 'O')

def handle_game_status(s: socket.socket, move_status: str, encoded_board: list, side: str, move: str) -> None:
    print(f"You play: {side}.")
    display_board(encoded_board)

    if move_status == '2':
        print("CONGRATULATIONS!!! YOU WIN!!!")
    elif move_status == '3':
        print("YOU LOSE!!!")
    elif move_status == '4':
        print("IT'S A DRAW!!!")
    elif move_status == '8':
        print("YOUR OPPONENT DISCONNECTED")
    else:
        if move_status == '1':
            print("Invalid Move! Try again.")

        if check_your_move(move=move, side=side):
            print("Now is your turn.")
            user_input = ""
            while not user_input:
                user_input = input("Enter row and column (e.g., 2 3): ")

            s.send(bytes(user_input, "utf-8"))
        else:
            print("Now is the opponent's turn.")

def main():
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            print("Connecting...")
            s.connect((HOST, PORT))
            print("Finding the game...")

            while True:
                # Receive game info
                data = s.recv(256)

                # Decode data
                data_decoded = list(data.decode("utf-8"))

                # Check if data is complete
                if len(data_decoded) != 19:
                    continue

                move_status, move, side, encoded_board = data_decoded[0], data_decoded[1], data_decoded[2], data_decoded[3:]
                handle_game_status(s, move_status, encoded_board, side, move)

                if move_status in {'2', '3', '4', '8'}:
                    exit()
    except Exception as e:
        print(f"Couldn't connect to the server. Reason: {e}")
        return 1

if __name__ == "__main__":
    main()
