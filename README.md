# The Millionaire Game

Network Programming Capstone Project - Millionaire Game Implementation

## Project Overview

A multiplayer "Who Wants to Be a Millionaire" game with client-server architecture, database persistence, and GUI interface.

## Project Structure

```
TheMillionaireGame/
├── server/          # Server code (C++)
│   ├── server_core.h/cpp
│   ├── request_handlers/
│   ├── session_manager.h/cpp
│   ├── auth_manager.h/cpp
│   └── ...
├── database/        # Database + Game Logic (C++)
│   ├── schema.sql
│   ├── database.h/cpp
│   └── game_logic/
├── client/          # Client + GUI (Qt5)
│   ├── main.cpp
│   ├── mainwindow.h/cpp
│   └── ...
├── docs/            # Documentation
│   ├── PROTOCOL.md
│   ├── ERROR_CODES.md
│   └── TEAM_WORKFLOW.md
└── README.md
```

---

## 🚀 Complete Setup Guide (Start to Finish)

This guide will walk you through setting up and running the entire project from scratch.

### Prerequisites

- **Linux/Ubuntu** or **macOS**
- **PostgreSQL** database server
- **C++11** compiler (g++ or clang++)
- **Qt5** (for client GUI)
- **CMake** and **Make**

---

## Step 1: Database Setup

The database must be set up **before** starting the server.

### 1.1 Install PostgreSQL

**On Ubuntu/Linux:**
```bash
sudo apt-get update
sudo apt-get install -y postgresql postgresql-contrib libpq-dev
sudo systemctl start postgresql
sudo systemctl enable postgresql
```

**On macOS:**
```bash
brew install postgresql@14
brew services start postgresql@14
```

### 1.2 Reset and Create Database

**Important:** If the database already exists, this will drop it and recreate it fresh.

```bash
# Terminate any existing connections to the database
psql -U postgres -d postgres -c "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = 'millionaire_game' AND pid <> pg_backend_pid();"

# Drop the database if it exists
psql -U postgres -d postgres -c "DROP DATABASE IF EXISTS millionaire_game;"

# Create a new database
createdb -U postgres millionaire_game
```

**On Linux (using sudo):**
```bash
sudo -u postgres psql -d postgres -c "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = 'millionaire_game' AND pid <> pg_backend_pid();"
sudo -u postgres psql -d postgres -c "DROP DATABASE IF EXISTS millionaire_game;"
sudo -u postgres createdb millionaire_game
```

### 1.3 Import Database

```bash
cd database
psql -U postgres -d millionaire_game < schema.sql
psql -U postgres -d millionaire_game < mock_data.sql
psql -U postgres -d millionaire_game < fix_prize_values.sql
```

**On Linux (using sudo):**
```bash
cd database
sudo -u postgres psql millionaire_game < schema.sql
sudo -u postgres psql millionaire_game < mock_data.sql
sudo -u postgres psql millionaire_game < fix_prize_values.sql
```

### 1.4 Verify Database Setup

```bash
# Check tables exist
psql -U postgres -d millionaire_game -c "\dt"

# Check questions count
psql -U postgres -d millionaire_game -c "SELECT COUNT(*) FROM questions;"

# Check questions by level
psql -U postgres -d millionaire_game -c "SELECT level, COUNT(*) FROM questions GROUP BY level ORDER BY level;"
```

**On Linux (using sudo):**
```bash
# Check tables exist
sudo -u postgres psql millionaire_game -c "\dt"

# Check questions count
sudo -u postgres psql millionaire_game -c "SELECT COUNT(*) FROM questions;"

# Check questions by level
sudo -u postgres psql millionaire_game -c "SELECT level, COUNT(*) FROM questions GROUP BY level ORDER BY level;"
```

### 1.5 Create Admin User (Optional)

To test admin features, you can promote an existing user to admin role:

```bash
# Make a user admin (replace 'admin' with your username)
psql -U postgres -d millionaire_game -c "UPDATE users SET role = 'admin' WHERE username = 'admin';"
```

**On Linux (using sudo):**
```bash
sudo -u postgres psql millionaire_game -c "UPDATE users SET role = 'admin' WHERE username = 'admin';"
```

**Note:** You must first register/create the user through the client application before promoting them to admin.

---

## Step 2: Server Setup and Start

### 2.1 Install Server Dependencies

**On Ubuntu/Linux:**
```bash
sudo apt-get update
sudo apt-get install -y build-essential g++ make cmake
sudo apt-get install -y libpq-dev
```

**On macOS:**
```bash
brew install postgresql libpq
```

### 2.2 Configure Server

Create or edit `server/config.json`:

```bash
cd server
cat > config.json << EOF
{
  "port": 8080,
  "log_file": "server.log",
  "log_level": "INFO",
  "max_clients": 100,
  "ping_timeout_seconds": 60,
  "connection_timeout_seconds": 300,
  "db_host": "localhost",
  "db_port": 5432,
  "db_name": "millionaire_game",
  "db_user": "postgres",
  "db_password": ""
}
EOF
```

### 2.3 Build Server

```bash
cd server
make clean
make
```

**Verify build:**
```bash
ls -lh bin/server
chmod +x bin/server
```

### 2.4 Start Server

**Run in foreground (for testing)**
```bash
cd server
./bin/server
```

### 2.5 Find Server IP Address

**On the server machine, find its IP address:**

**Linux/Ubuntu:**
```bash
hostname -I
# or
ip addr show | grep "inet " | grep -v 127.0.0.1
```

**macOS:**
```bash
ifconfig | grep "inet " | grep -v 127.0.0.1
```

**Common examples:**
- `localhost` or `127.0.0.1` - if client and server are on same machine
- `192.168.1.100` - typical local network IP
- `10.0.0.5` - another local network IP

**Share this IP address with clients** - they will need it to connect.

---

## Step 3: Client Setup and Start

### 3.1 Install Client Dependencies

**On Ubuntu/Linux:**
```bash
sudo apt-get update
sudo apt-get install -y build-essential g++ make cmake
sudo apt-get install -y qtbase5-dev qt5-qmake
sudo apt-get install -y libqt5widgets5 libqt5core5a libqt5gui5
```

**On macOS:**
```bash
brew install qt@5
```

### 3.2 Build Client

```bash
cd client
make clean
make
```

**Verify build:**
```bash
ls -lh build/MillionaireGameClient
```

### 3.3 Start Client

**Option 1: Connect to localhost (same machine as server)**
```bash
cd client
./build/MillionaireGameClient
# or
make run
```

**Option 2: Connect to remote server**
```bash
cd client
./build/MillionaireGameClient <SERVER_IP> <PORT>
```

**Examples:**
```bash
# Connect to server at IP 192.168.1.100 on port 8080
./build/MillionaireGameClient 192.168.1.100 8080

# Connect to server at IP 10.0.0.5 on port 8080
./build/MillionaireGameClient 10.0.0.5 8080

# Connect to localhost (default port 8080)
./build/MillionaireGameClient localhost
```

### 3.4 Client Connection Tips

- **Same machine:** Use `localhost` or `127.0.0.1`
- **Same network:** Use the server's local IP (e.g., `192.168.1.100`)
- **Different network:** Use the server's public IP (requires port forwarding/firewall rules)
- **Default port:** `8080` (can be changed in server `config.json`)

---

## 📋 Complete Workflow Example

### Terminal 1: Database (one-time setup)
```bash
# 1. Reset database (terminate connections and drop if exists)
psql -U postgres -d postgres -c "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = 'millionaire_game' AND pid <> pg_backend_pid();"
psql -U postgres -d postgres -c "DROP DATABASE IF EXISTS millionaire_game;"
createdb -U postgres millionaire_game

# 2. Import schema
cd database
psql -U postgres -d millionaire_game < schema.sql

# 3. Add mock data
psql -U postgres -d millionaire_game < mock_data.sql

# 4. Fix prize values
psql -U postgres -d millionaire_game < fix_prize_values.sql

# 5. (Optional) Create admin user after registering through client
# psql -U postgres -d millionaire_game -c "UPDATE users SET role = 'admin' WHERE username = 'admin';"
```

**On Linux, use `sudo -u postgres` prefix:**
```bash
sudo -u postgres psql -d postgres -c "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = 'millionaire_game' AND pid <> pg_backend_pid();"
sudo -u postgres psql -d postgres -c "DROP DATABASE IF EXISTS millionaire_game;"
sudo -u postgres createdb millionaire_game
cd database
sudo -u postgres psql millionaire_game < schema.sql
sudo -u postgres psql millionaire_game < mock_data.sql
sudo -u postgres psql millionaire_game < fix_prize_values.sql
# (Optional) Create admin user after registering through client
# sudo -u postgres psql millionaire_game -c "UPDATE users SET role = 'admin' WHERE username = 'admin';"
```

### Terminal 2: Server
```bash
# 1. Configure server
cd server
cat > config.json << EOF
{
  "port": 8080,
  "db_host": "localhost",
  "db_port": 5432,
  "db_name": "millionaire_game",
  "db_user": "postgres",
  "db_password": ""
}
EOF

# 2. Build server
make

# 3. Find server IP
hostname -I  # Note this IP for clients

# 4. Start server
./bin/server
```

### Terminal 3: Client
```bash
# 1. Build client
cd client
make

# 2. Connect to server (replace <SERVER_IP> with actual IP from Terminal 2)
./build/MillionaireGameClient <SERVER_IP> 8080

# Or if on same machine:
./build/MillionaireGameClient localhost
```

---

## 🔧 Troubleshooting

### Server won't start
- **Check PostgreSQL is running:** `sudo systemctl status postgresql`
- **Check database exists:** `sudo -u postgres psql -l | grep millionaire_game`
- **Check config.json:** Verify database credentials match your setup
- **Check port availability:** `netstat -tuln | grep 8080`

### Client can't connect
- **Verify server is running:** `ps aux | grep server`
- **Check server IP:** Make sure you're using the correct IP address
- **Check firewall:** `sudo ufw allow 8080/tcp` (Linux)
- **Test connection:** `telnet <SERVER_IP> 8080`

### Database connection errors
- **Check PostgreSQL is running:** `sudo systemctl start postgresql`
- **Verify credentials:** Check `server/config.json` matches your database setup
- **Check pg_hba.conf:** Ensure local connections are allowed

### Client build errors (Qt5)
- **macOS:** Ensure Qt5 PATH is set: `export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"`
- **Linux:** Install Qt5: `sudo apt-get install -y qtbase5-dev qt5-qmake`
- **Verify qmake:** `qmake --version`

---

## Features

- User authentication and registration
- Game session management
- Question system with 15 levels
- Lifelines (50/50, Phone a Friend, Ask the Audience)
- Leaderboard (global and friends)
- Friend system
- Admin panel (question management, user banning)
- Auto-save game progress

## Technology Stack

- **Server**: C++11, TCP sockets, multi-threading
- **Database**: PostgreSQL
- **Client**: Qt5 (C++)

## License

Educational project for Network Programming course.
