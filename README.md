# Who Wants To Be A Millionaire?

## Project Overview

This is a capstone project for the Network Programming course, implementing a network-based game simulation of the popular quiz show "Who Wants To Be A Millionaire?". The application enables multiple clients to connect to a server and participate in the game simultaneously, demonstrating core concepts of network programming including socket communication, client-server architecture, and concurrent connection handling.

## Course Information

- **Course**: Network Programming - 20251
- **Institution**: Hanoi University of Science and Technology (HUST)

## Team Members

- **Dang Van Nhan** (Student ID: 20225990) - Project Leader
- **Tran Kim Cuong** (Student ID: 20226017)
- **Tran Viet Anh** (Student ID: 20226013)

## Technology Stack

- **Programming Languages**: C/C++
- **Database**: PostgreSQL
- **Platform**: Linux/macOS
- **Network Libraries**: Standard socket libraries (sys/socket.h, netinet/in.h,...) and platform-specific networking APIs

## Features

- Multi-client server architecture supporting concurrent game sessions
- Real-time question delivery and answer validation
- Lifeline system (50/50, Phone a Friend, Ask the Audience)
- Progressive difficulty levels with increasing prize amounts
- Persistent game state management using PostgreSQL
- Secure client-server communication protocol
- User authentication and session management

## Project Structure

```
TheMillionaireGame/
├── server/          # Server-side implementation
├── client/          # Client-side implementation
├── database/        # Database schema and scripts
├── common/          # Shared headers and utilities
├── docs/            # Documentation
└── README.md        # This file
```

## Prerequisites

- C/C++ compiler (GCC or Clang)
- PostgreSQL database server (version 12 or higher)
- Development libraries for network programming
- Make or CMake build system

## Installation

### Database Setup

1. Install PostgreSQL on your system
2. Create a database for the game:
   ```sql
   CREATE DATABASE millionaire_game;
   ```
3. Run the database initialization scripts located in the `database/` directory

### Building the Project

1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd TheMillionaireGame
   ```

2. Configure the build system (if using CMake):
   ```bash
   mkdir build && cd build
   cmake ..
   ```

3. Compile the project:
   ```bash
   make
   ```
   Or if using CMake:
   ```bash
   cmake --build .
   ```

## Usage

### Starting the Server

```bash
./server [port]
```

The server will listen on the specified port (default: 8080) for incoming client connections.

### Running the Client

```bash
./client [server-address] [port]
```

Connect to the game server using the provided address and port.

## Architecture

The application follows a client-server model where:
- The server manages game logic, question distribution, and database interactions
- Clients connect to the server and participate in game sessions
- Communication is handled through TCP/IP sockets
- Database operations ensure data persistence and question management

## Development Notes

- The project emphasizes proper network programming practices including error handling, connection management, and protocol design
- Database integration demonstrates data persistence and query optimization
- Concurrent client handling showcases multi-threading or event-driven programming techniques

## License

This project is developed for academic purposes as part of the Network Programming course at HUST.