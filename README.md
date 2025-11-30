# The Millionaire Game

Network Programming Capstone Project - Millionaire Game Implementation

## Project Overview

A multiplayer "Who Wants to Be a Millionaire" game with client-server architecture, database persistence, and GUI interface.

## Team Structure

- **Server Developer**: Server architecture, protocol handlers, request routing
- **Member A**: Database design, game logic implementation
- **Member B**: Client application, GUI development

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
├── client/          # Client + GUI
│   ├── src/
│   └── gui/
├── docs/            # Documentation
│   ├── PROTOCOL.md
│   ├── ERROR_CODES.md
│   └── TEAM_WORKFLOW.md
└── README.md
```

## Quick Start

### Server
```bash
cd server
make
./bin/server
```

### Database Setup
```bash
cd database
createdb millionaire_game
psql millionaire_game < schema.sql
```

### Client
```bash
cd client
# Follow client/README.md for build instructions
```

## Documentation

- **Protocol Specification**: `docs/PROTOCOL.md`
- **Error Codes**: `docs/ERROR_CODES.md`
- **Team Workflow**: `docs/TEAM_WORKFLOW.md`
- **Server Integration**: `server/INTEGRATION.md`
- **Database Guide**: `database/INTEGRATION_GUIDE.md`
- **Client Guide**: `client/README.md`

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
- **Client**: (To be determined by Member B)

## License

Educational project for Network Programming course.
