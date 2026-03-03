# 🔒 Encrypted Client-Server Communication in C

This project is a high-concurrency client-server application developed in C. It demonstrates the use of **sockets** for communication, **pthreads** for parallel processing, and **mutexes/condition variables** for thread safety.

The application allows a client to read a file, encrypt it in parallel using a 64-bit XOR cipher, and send the ciphertext to a server. The server, capable of handling multiple clients concurrently, receives the data, decrypts it in parallel, and saves the original content to a file.

---
## 🎯 Core Features

* **Client-Server Architecture**: Uses standard C sockets for network communication.
* **Parallel Encryption (Client)**: The client uses a specified number (`p`) of threads (`pthread`) to encrypt file blocks (8 bytes) concurrently using an XOR cipher.
* **Parallel Decryption (Server)**: The server uses `p` threads to decrypt the received blocks in parallel.
* **Thread-Safe Server**: Manages a pool of concurrent client connections (up to `l`) using mutexes and condition variables to protect shared resources.
* **Safe Data Transmission**: Encrypted (binary) data is converted to **Hexadecimal** (`toHex`) before sending to prevent issues with null terminators or other non-printable characters during socket transmission. The server converts it back (`fromHex`).
* **Signal Handling**: Critical sections (like encryption, decryption, and file writing) are protected from interruption by signals (`SIGINT`, `SIGTERM`, etc.) using `disableSig()` and `activeSignals()`.
---

## 🧩 Architecture & Core Components

The project is divided into three main C files:

* `client.c`: Handles the client-side logic. It initializes parameters, manages the core encryption workflow, and serializes the final message (`Message` struct) for sending.
* `server.c`: Manages the server-side logic. It initializes parameters, listens for new connections (`handle_server`), and spawns threads to manage individual client requests (`handle_client`) and write to files (`FileWrite`).
* `Utility.c`: A shared library containing the core logic for encryption, decryption, parallelism, and signal handling. It is included by both the client and server.
--- 

## ⚙️ How it Works: The Data Flow

### 1. Client-Side (Encryption & Sending)

1.  **Setup**: The client is initialized with a file, a 64-bit key (`K`), a parallelism level (`p`), and the server's IP/Port. These are stored in the `ClientSend` struct.
2.  **Encryption**:
    * `encryptMsg` is called to read the file.
    * The file content is divided into 8-byte (64-bit) blocks using `split_Block`.
    * `checkArr` manages the creation of `p` threads to process these blocks in parallel.
    * Each thread runs `TranslateWithKey`, which calls `xor64` to perform the `Block XOR Key` operation. The thread's work is defined by the `ThreadInfo` struct.
3.  **Transmission**:
    * The resulting encrypted blocks are reassembled (`createRes`).
    * The final ciphertext is converted to a Hex string using `toHex`.
    * A `Message` struct (containing the hex ciphertext, the original key, and the original file length) is serialized (`serializeMessage`) and sent to the server.

### 2. Server-Side (Receiving & Decryption)

1.  **Setup**: The server starts with a port, max connections (`l`), parallelism level (`p`), and an output file prefix (`s`), stored in the `SetupServer` struct.
2.  **Connection Handling**:
    * `handle_server` listens for incoming connections.
    * For each connection, `handle_client` manages the request. It tracks active clients using global counters and mutexes (`n_clients`, `active_clients`, `pthread_mutex_t`).
3.  **Decryption**:
    * The server receives the serialized message and converts the Hex ciphertext back to binary using `fromHex`.
    * `decryptMsg` is called, which uses the *exact same* parallel logic as the client (`checkArr`, `TranslateWithKey`, `xor64`) to apply the `Ciphertext-Block XOR Key` operation, retrieving the original blocks.
4.  **File Writing**:
    * The decrypted blocks are reassembled (`createRes`).
    * The server sends an acknowledgment back to the client.
    * A new thread is spawned via `threadWrite` to execute `FileWrite`, which writes the decrypted content to a new file (e.g., `s_filename.txt`). This ensures file I/O does not block the main server loop.
---

## 💻 How to Run

1.  Clone the repository:
    ```sh
    git clone https://github.com/simonemazzi/EncryptedClient-ServerCommunicationC.git
    cd EncryptedClient-ServerCommunicationC
    ```
2.  Compile the Server and Client (must link the pthread library):
    ```sh
    # Compile Server
    gcc -o server src/server.c src/Utility.c -Wall -Wextra -pthread
    
    # Compile Client
    gcc -o client src/client.c src/Utility.c -Wall -Wextra -pthread
    ```
3.  Run the Server:
    *(Arguments based on the `SetupServer` struct and project spec)*
    ```sh
    # Example: Port 8080, Max 10 connections, 4 threads, "decrypted_" prefix
    ./server 8080 10 4 "decrypted_"
    ```
4.  Run the Client in another terminal:
    *(Arguments based on the `ClientSend` struct and project spec)*
    ```sh
    # Example: input.txt, key=12345, 4 threads, localhost, Port 8080
    ./client input.txt 12345 4 "127.0.0.1" 8080
    ```
    The server will then create a file (e.g., `decrypted_input.txt`) with the original content.

## 🧰 Requirements

* C compiler (e.g., `gcc` or `clang`)
* POSIX Threads (`pthread`) library
* Standard C Libraries (for sockets, file I/O)

## 👥 Authors
- [Simone Mazzi](https://github.com/simonemazzi)

- [Lorenz Narcis Grecu](https://github.com/Lurinz)

- [Felipe Sampedro](https://github.com/Sampyx)
