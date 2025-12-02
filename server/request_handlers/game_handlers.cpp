#include "game_handlers.h"
#include "../game_state_manager.h"
#include "../question_manager.h"
#include "../scoring_system.h"
#include "../game_timer.h"
#include "../lifeline_manager.h"
#include "../json_utils.h"
#include "../logger.h"
#include "../../database/database.h"
#include <ctime>
#include <algorithm>

using namespace std;

namespace MillionaireGame {

namespace GameHandlers {

string handleStart(const string& request, ClientSession& session) {
    if (session.in_game) {
        return StreamUtils::createErrorResponse(405, "Already in a game");
    }

    bool override_saved = JsonUtils::extractBool(request, "overrideSavedGame", false);
    
    GameProgress saved_progress = GameStateManager::getInstance().loadGameProgress(session.username);
    if (saved_progress.level > 0 && !override_saved) {
        return StreamUtils::createErrorResponse(412, 
            "You have a saved game. Use RESUME to continue or set overrideSavedGame=true to start new game");
    }

    // Create game session in database
    int game_id = Database::getInstance().createGameSession(session.username);
    if (game_id == 0) {
        return StreamUtils::createErrorResponse(500, "Failed to create game session");
    }
    
    // Get random question for level 0 (easy) - first question
    Question first_question = QuestionManager::getInstance().getRandomQuestion(0);
    if (first_question.id == 0) {
        return StreamUtils::createErrorResponse(500, "Failed to get question");
    }
    
    // Add question to game_questions table
    Database::getInstance().addGameQuestion(game_id, 1, first_question.id);
    
    session.in_game = true;
    session.game_id = game_id;
    session.current_question_number = 1;
    session.current_level = 0;  // Start with easy (level 0)
    session.current_prize = ScoringSystem::getInstance().getPrizeForLevel(0, 1);
    session.total_score = 0;
    session.used_lifelines.clear();
    
    // Start timer for first question
    GameTimer::getInstance().startQuestionTimer(game_id);

    string data = "{\"message\":\"Game started\",\"gameId\":" + to_string(game_id) + 
                 ",\"timestamp\":" + to_string(time(nullptr)) + "}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleAnswer(const string& request, ClientSession& session) {
    LOG_INFO("handleAnswer called - in_game=" + string(session.in_game ? "true" : "false"));
    
    if (!session.in_game) {
        return StreamUtils::createErrorResponse(406, "Not in a game");
    }

    int game_id = JsonUtils::extractInt(request, "gameId", -1);
    int question_number = JsonUtils::extractInt(request, "questionNumber", -1);
    int answer_index = JsonUtils::extractInt(request, "answerIndex", -1);
    
    LOG_INFO("handleAnswer: game_id=" + to_string(game_id) + ", question_number=" + to_string(question_number) + ", answer_index=" + to_string(answer_index));

    if (game_id < 0) {
        return StreamUtils::createErrorResponse(422, "Missing or invalid gameId");
    }

    if (game_id != session.game_id) {
        return StreamUtils::createErrorResponse(412, "Invalid gameId - gameId doesn't match active game");
    }

    if (question_number != session.current_question_number) {
        return StreamUtils::createErrorResponse(422, 
            "Question number mismatch: expected " + to_string(session.current_question_number) + 
            ", got " + to_string(question_number));
    }

    if (answer_index < 0 || answer_index > 3) {
        return StreamUtils::createErrorResponse(422, "Invalid answerIndex: must be 0-3");
    }

    // Check timeout
    if (GameTimer::getInstance().isTimeout(game_id)) {
        session.in_game = false;
        GameTimer::getInstance().stopTimer(game_id);
        
        long long safe_checkpoint_prize = ScoringSystem::getInstance().getSafeCheckpointPrize(session.current_question_number);
        int safe_checkpoint_score = session.total_score;
        
        string data = "{\"gameId\":" + to_string(game_id) + 
                     ",\"correct\":false" +
                     ",\"questionNumber\":" + to_string(session.current_question_number) +
                     ",\"timeRemaining\":0" +
                     ",\"pointsEarned\":0" +
                     ",\"safeCheckpointPrize\":" + to_string(safe_checkpoint_prize) +
                     ",\"safeCheckpointScore\":" + to_string(safe_checkpoint_score) +
                     ",\"totalScore\":" + to_string(safe_checkpoint_score) +
                     ",\"finalPrize\":" + to_string(safe_checkpoint_prize) +
                     ",\"gameOver\":true,\"isWinner\":false,\"timeout\":true}";
        // For timeout, we return error response but include game data
        return "{\"responseCode\":408,\"data\":" + data + "}";
    }
    
    int time_remaining = GameTimer::getInstance().getRemainingTime(game_id);
    if (time_remaining < 0) time_remaining = 0;
    
    // Get the question assigned to this game (not a new random one!)
    Question current_question = Database::getInstance().getGameQuestion(game_id, question_number);
    if (current_question.id == 0) {
        LOG_ERROR("Failed to get question for game_id=" + to_string(game_id) + ", question_number=" + to_string(question_number));
        return StreamUtils::createErrorResponse(500, "Failed to get question for this game");
    }
    
    // Check answer directly (we already have the question object)
    bool correct = (current_question.correct_answer == answer_index);
    
    // Debug: Log the comparison
    string debug_msg = "DEBUG: question_id=" + to_string(current_question.id) + 
                       ", correct_answer=" + to_string(current_question.correct_answer) + 
                       ", answer_index=" + to_string(answer_index) + 
                       ", correct=" + (correct ? "true" : "false");
    LOG_INFO(debug_msg);
    
    // Also add debug info to response for immediate visibility
    string debug_data = ",\"debug\":{\"questionId\":" + to_string(current_question.id) +
                       ",\"correctAnswer\":" + to_string(current_question.correct_answer) +
                       ",\"answerIndex\":" + to_string(answer_index) +
                       ",\"match\":" + (correct ? "true" : "false") + "}";
    
    // time_remaining already calculated above in timeout check
    if (time_remaining < 0) time_remaining = 0;
    int lifelines_used = session.used_lifelines.size();
    int points_earned = ScoringSystem::getInstance().calculateQuestionScore(time_remaining, lifelines_used);

    // Record answer in database
    int response_time = 60 - time_remaining;  // Calculate response time (60 second timer)
    Database::getInstance().addGameAnswer(game_id, question_number, answer_index, correct, response_time);
    
    if (correct) {
        session.total_score += points_earned;
        session.current_question_number++;
        
        if (session.current_question_number > 15) {
            session.in_game = false;
            GameTimer::getInstance().stopTimer(game_id);
            
            // Update game session in database
            GameSession db_session;
            db_session.id = game_id;
            db_session.status = "won";
            db_session.current_question_number = 15;
            db_session.total_score = session.total_score;
            long long final_prize = ScoringSystem::getInstance().getPrizeForLevel(2, 15);
            db_session.final_prize = final_prize;
            Database::getInstance().endGame(game_id, "won", session.total_score, final_prize);
            
            string data = "{\"gameId\":" + to_string(game_id) + 
                         ",\"correct\":true,\"questionNumber\":15" +
                         ",\"timeRemaining\":" + to_string(time_remaining) +
                         ",\"pointsEarned\":" + to_string(points_earned) +
                         ",\"totalScore\":" + to_string(session.total_score) +
                         ",\"currentPrize\":" + to_string(final_prize) +
                         ",\"finalPrize\":" + to_string(final_prize) +
                         ",\"gameOver\":true,\"isWinner\":true}";
            return StreamUtils::createSuccessResponse(200, data);
        } else {
            // Determine next level and get next question
            int next_level = (session.current_question_number <= 5) ? 0 : 
                            (session.current_question_number <= 10) ? 1 : 2;
            session.current_level = next_level;
            session.current_prize = ScoringSystem::getInstance().getPrizeForLevel(next_level, session.current_question_number);
            
            // Get next random question for the new level
            Question next_question = QuestionManager::getInstance().getRandomQuestion(next_level);
            if (next_question.id > 0) {
                Database::getInstance().addGameQuestion(game_id, session.current_question_number, next_question.id);
            }
            
            // Update game session in database
            GameSession db_session;
            db_session.id = game_id;
            db_session.status = "active";
            db_session.current_question_number = session.current_question_number;
            db_session.current_level = session.current_level;
            db_session.current_prize = session.current_prize;
            db_session.total_score = session.total_score;
            Database::getInstance().updateGameSession(db_session);
            
            // Restart timer for next question
            GameTimer::getInstance().startQuestionTimer(game_id);
            
            string data = "{\"gameId\":" + to_string(game_id) + 
                         ",\"correct\":true" +
                         ",\"questionNumber\":" + to_string(session.current_question_number - 1) +
                         ",\"timeRemaining\":" + to_string(time_remaining) +
                         ",\"pointsEarned\":" + to_string(points_earned) +
                         ",\"totalScore\":" + to_string(session.total_score) +
                         ",\"currentPrize\":" + to_string(session.current_prize) +
                         ",\"gameOver\":false,\"isWinner\":false}";
            return StreamUtils::createSuccessResponse(200, data);
        }
    } else {
        session.in_game = false;
        GameTimer::getInstance().stopTimer(game_id);
        
        long long safe_checkpoint_prize = ScoringSystem::getInstance().getSafeCheckpointPrize(session.current_question_number);
        int safe_checkpoint_score = session.total_score;  // Don't subtract points_earned for wrong answer
        
        int correct_answer = current_question.correct_answer;
        
        // Update game session in database
        Database::getInstance().endGame(game_id, "lost", safe_checkpoint_score, safe_checkpoint_prize);
        
        string data = "{\"gameId\":" + to_string(game_id) + 
                     ",\"correct\":false" +
                     ",\"questionNumber\":" + to_string(session.current_question_number) +
                     ",\"correctAnswer\":" + to_string(correct_answer) +
                     ",\"pointsEarned\":0" +
                     ",\"safeCheckpointPrize\":" + to_string(safe_checkpoint_prize) +
                     ",\"safeCheckpointScore\":" + to_string(safe_checkpoint_score) +
                     ",\"totalScore\":" + to_string(safe_checkpoint_score) +
                     ",\"finalPrize\":" + to_string(safe_checkpoint_prize) +
                     ",\"gameOver\":true,\"isWinner\":false" +
                     debug_data + "}";
        return StreamUtils::createSuccessResponse(200, data);
    }
}

string handleLifeline(const string& request, ClientSession& session) {
    if (!session.in_game) {
        return StreamUtils::createErrorResponse(406, "Not in a game");
    }

    int game_id = JsonUtils::extractInt(request, "gameId", -1);
    int question_number = JsonUtils::extractInt(request, "questionNumber", -1);
    string lifeline_type = JsonUtils::extractString(request, "lifelineType");

    if (game_id < 0) {
        return StreamUtils::createErrorResponse(422, "Missing or invalid gameId");
    }

    if (game_id != session.game_id) {
        return StreamUtils::createErrorResponse(412, "Invalid gameId - gameId doesn't match active game");
    }

    if (question_number != session.current_question_number) {
        return StreamUtils::createErrorResponse(422, 
            "Question number mismatch: expected " + to_string(session.current_question_number) + 
            ", got " + to_string(question_number));
    }

    if (lifeline_type != "5050" && lifeline_type != "PHONE" && lifeline_type != "AUDIENCE") {
        return StreamUtils::createErrorResponse(422, "Invalid lifelineType");
    }

    if (session.used_lifelines.find(lifeline_type) != session.used_lifelines.end()) {
        return StreamUtils::createErrorResponse(407, "Lifeline already used");
    }
    
    // Determine level for current question
    int level = (session.current_question_number <= 5) ? 0 : 
                (session.current_question_number <= 10) ? 1 : 2;
    
    // Get current question
    Question current_question = QuestionManager::getInstance().getRandomQuestion(level);
    if (current_question.id == 0) {
        return StreamUtils::createErrorResponse(500, "Failed to get question");
    }
    
    // Use lifeline
    LifelineResult result;
    if (lifeline_type == "5050") {
        result = LifelineManager::getInstance().use5050(game_id, current_question.id);
    } else if (lifeline_type == "PHONE") {
        result = LifelineManager::getInstance().usePhone(game_id, current_question.id);
    } else if (lifeline_type == "AUDIENCE") {
        result = LifelineManager::getInstance().useAudience(game_id, current_question.id);
    } else {
        return StreamUtils::createErrorResponse(422, "Invalid lifelineType");
    }
    
    if (!result.success) {
        return StreamUtils::createErrorResponse(500, "Failed to process lifeline");
    }
    
    session.used_lifelines.insert(lifeline_type);
    
    string data = "{\"message\":\"Lifeline processed\",\"lifelineType\":\"" + lifeline_type + 
                 "\",\"delaySeconds\":" + to_string(result.delay_seconds) +
                 ",\"result\":" + result.result_data + "}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleGiveUp(const string& request, ClientSession& session) {
    if (!session.in_game) {
        return StreamUtils::createErrorResponse(406, "Not in a game");
    }

    int game_id = JsonUtils::extractInt(request, "gameId", -1);
    int question_number = JsonUtils::extractInt(request, "questionNumber", -1);

    if (game_id < 0) {
        return StreamUtils::createErrorResponse(422, "Missing or invalid gameId");
    }

    if (game_id != session.game_id) {
        return StreamUtils::createErrorResponse(412, "Invalid gameId - gameId doesn't match active game");
    }

    if (question_number != session.current_question_number) {
        return StreamUtils::createErrorResponse(422, 
            "Question number mismatch: expected " + to_string(session.current_question_number) + 
            ", got " + to_string(question_number));
    }

    int final_prize = session.current_prize;
    int final_question_number = session.current_question_number;
    int total_score = session.total_score;

    session.in_game = false;

    string data = "{\"finalPrize\":" + to_string(final_prize) + 
                 ",\"finalQuestionNumber\":" + to_string(final_question_number) + 
                 ",\"totalScore\":" + to_string(total_score) +
                 ",\"gameId\":" + to_string(game_id) +
                 ",\"message\":\"You gave up and took the prize.\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleResume(const string& request, ClientSession& session) {
    if (session.in_game) {
        return StreamUtils::createErrorResponse(405, "User already in a game");
    }

    GameProgress progress = GameStateManager::getInstance().loadGameProgress(session.username);
    if (progress.level == 0) {
        return StreamUtils::createErrorResponse(404, "No saved game found");
    }

    session.in_game = true;
    session.game_id = progress.level;
    session.current_question_number = progress.level;
    session.current_prize = progress.prize;

    string data = "{\"questionNumber\":" + to_string(progress.level) + 
                 ",\"prize\":" + to_string(progress.prize) + 
                 ",\"gameId\":" + to_string(session.game_id) +
                 ",\"totalScore\":" + to_string(session.total_score) +
                 ",\"message\":\"Game resumed successfully\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleLeaveGame(const string& request, ClientSession& session) {
    if (!session.in_game) {
        return StreamUtils::createErrorResponse(406, "Not in a game");
    }

    GameStateManager::getInstance().saveGameProgress(session.username, 
        session.current_question_number, session.current_prize);
    session.in_game = false;

    string data = "{\"message\":\"Left game successfully. Game state saved. Use RESUME to continue later.\"}";
    return StreamUtils::createSuccessResponse(200, data);
}

} // namespace GameHandlers

} // namespace MillionaireGame

