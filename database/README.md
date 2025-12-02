# Database Module

## Overview

This folder contains database schema, migrations, and integration code for the Millionaire Game server.

## Responsibilities (Member A)

- **Database Schema Design**: Design and implement PostgreSQL database schema
- **Database Integration**: Create `database.h/cpp` module to integrate with server
- **Game Logic**: Implement game logic modules (QuestionManager, GameStateManager, ScoringSystem, LifelineManager, GameTimer)

## Folder Structure

```
database/
├── schema.sql              # Database schema definition
├── mock_data.sql           # Sample data for testing
├── migrations/             # Database migration scripts
│   └── 001_initial_schema.sql
├── database.h              # Database class interface
├── database.cpp            # Database implementation
├── game_logic/             # Game logic modules
│   ├── question_manager.h/cpp
│   ├── game_state_manager.h/cpp
│   ├── scoring_system.h/cpp
│   ├── lifeline_manager.h/cpp
│   └── game_timer.h/cpp
├── README.md               # This file
├── INTEGRATION_GUIDE.md    # Detailed integration steps
├── INTEGRATION_COMPLETE.md # Complete integration status and fixes
├── TEST_GUIDE.md           # Comprehensive testing guide with test cases
└── QUICK_TEST_REFERENCE.md # Quick reference for common test commands
```

## Integration Steps

1. **Review Server Integration Guide**: See `../server/docs/INTEGRATION.md` for detailed integration instructions

2. **Create Database Schema**: 
   - Design tables: users, questions, game_sessions, saved_games, friendships, friend_requests, leaderboard
   - See `schema.sql` template

3. **Implement Database Module**:
   - Create `database.h/cpp` with singleton pattern
   - Implement all methods referenced in `../server/docs/INTEGRATION.md`
   - Use `libpq` for PostgreSQL connection

4. **Implement Game Logic Modules**:
   - QuestionManager: Question retrieval and answer validation
   - GameStateManager: Game state transitions
   - ScoringSystem: Score calculation
   - LifelineManager: Lifeline processing
   - GameTimer: Question timeout handling

5. **Update Server Makefile**: Add database and game logic source files

6. **Replace Placeholders**: Find all `TODO: Replace with database call` in server code and replace with actual calls

## Database Connection

Add database connection settings to `server/config.json`:

```json
{
  "db_host": "localhost",
  "db_port": 5432,
  "db_name": "millionaire_game",
  "db_user": "game_user",
  "db_password": "password"
}
```

## Testing

- Test database connection independently
- Test each game logic module separately
- Integration test with server handlers

### Testing Documentation

- **`TEST_GUIDE.md`**: Comprehensive testing guide with detailed test cases for all features (authentication, game flow, scoring, lifelines, leaderboard, friends, admin operations). Includes step-by-step instructions, expected responses, and database verification queries.

- **`QUICK_TEST_REFERENCE.md`**: Quick reference guide for common test commands and database verification queries. Useful for rapid testing during development.

- **`INTEGRATION_COMPLETE.md`**: Complete integration status document detailing all fixes, known issues, build requirements, and server management instructions. Includes bug fixes, compilation issues resolved, and next steps.

## Notes

- All database operations should be thread-safe
- Use prepared statements to prevent SQL injection
- Log all database operations for debugging
- Handle connection errors gracefully

