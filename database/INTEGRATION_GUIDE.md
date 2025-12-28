# Database Integration Guide

## Quick Start

1. **Install PostgreSQL**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install postgresql libpq-dev
   
   # macOS
   brew install postgresql
   ```

2. **Create Database**:
   ```bash
   createdb millionaire_game
   psql millionaire_game < schema.sql
   ```

3. **Implement Database Module**:
   - Copy template from `database.h.template` (if exists)
   - Implement all methods as specified in `server/docs/INTEGRATION.md`
   - Test connection independently

4. **Update Server**:
   - Add `database.cpp` to `server/Makefile`
   - Update `server/config.h` to include database config fields
   - Initialize database connection in `server_core.cpp::start()`

5. **Replace Placeholders**:
   - Search for `TODO: Replace with database call` in server code
   - Replace with `Database::getInstance().methodName()`

## Database Schema Reference

See `schema.sql` for complete schema. Key tables:

- **users**: User accounts, roles, banned status
- **questions**: Game questions with 4 options
- **game_sessions**: Active and completed game sessions
- **saved_games**: Auto-saved game progress
- **friendships**: Friend relationships
- **friend_requests**: Pending friend requests
- **leaderboard**: Cached leaderboard data

## Game Logic Integration

After database is integrated, implement game logic modules:

1. **QuestionManager**: 
   - `getRandomQuestion(int level)` - Get random question for level
   - `checkAnswer(int question_id, int answer_index)` - Validate answer

2. **GameStateManager**:
   - `startNewGame()` - Initialize new game
   - `processAnswer()` - Handle answer submission
   - `useLifeline()` - Process lifeline usage

3. **ScoringSystem**:
   - `calculateQuestionScore()` - Calculate points for question
   - `getPrizeForLevel()` - Get prize amount for level

4. **LifelineManager**:
   - `use5050()` - 50/50 lifeline
   - `usePhone()` - Phone a friend
   - `useAudience()` - Ask the audience

5. **GameTimer**:
   - `startQuestionTimer()` - Start timer for question
   - `isTimeout()` - Check if question timed out

## Testing Checklist

- [ ] Database connection works
- [ ] User authentication works
- [ ] Game session creation works
- [ ] Question retrieval works
- [ ] Answer validation works
- [ ] Scoring calculation works
- [ ] Lifeline processing works
- [ ] Leaderboard queries work
- [ ] Friend operations work
- [ ] Integration with server handlers works

