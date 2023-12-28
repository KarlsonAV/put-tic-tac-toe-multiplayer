import socket

HOST="127.0.0.1"
PORT=1235

with socket.socket(socket.AF_INET,socket.SOCK_STREAM) as s:
    s.connect((HOST,PORT))
    # recv game info
    while True:
        # if not your move skip

        # s.send(bytes(msg,"utf-8"))
        data=s.recv(2024)
        print(list(data.decode("utf-8")))

    s.close()