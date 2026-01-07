#include "game_handlers.h"
#include "../game_state_manager.h"
#include "../question_manager.h"
#include "../scoring_system.h"
#include "../game_timer.h"
#include "../lifeline_manager.h"
#include "../json_utils.h"
#include "../stream_handler.h"
#include "../notification_utils.h"
#include "../logger.h"
#include "../../database/database.h"
#include <ctime>
#include <algorithm>
#include <sstream>

using namespace std;

namespace MillionaireGame {

namespace GameHandlers {

// Helper function to build QUESTION_INFO notification data
static string buildQuestionInfoData(const Question& q, int game_id, const ClientSession& session) {
    stringstream ss;
    ss << "{";
    ss << "\"questionId\":" << q.id << ",";
    ss << "\"questionNumber\":" << session.current_question_number << ",";
    ss << "\"question\":\"" << q.question_text << "\",";
    ss << "\"options\":[";
    ss << "{\"index\":0,\"label\":\"A\",\"text\":\"" << q.option_a << "\"},";
    ss << "{\"index\":1,\"label\":\"B\",\"text\":\"" << q.option_b << "\"},";
    ss << "{\"index\":2,\"label\":\"C\",\"text\":\"" << q.option_c << "\"},";
    ss << "{\"index\":3,\"label\":\"D\",\"text\":\"" << q.option_d << "\"}";
    ss << "],";
    ss << "\"prize\":" << session.current_prize << ",";
    ss << "\"totalQuestions\":15,";
    
    // Build lifelines array
    ss << "\"lifelines\":[";
    bool first = true;
    if (session.used_lifelines.find("5050") == session.used_lifelines.end()) {
        if (!first) ss << ",";
        ss << "\"5050\"";
        first = false;
    }
    if (session.used_lifelines.find("PHONE") == session.used_lifelines.end()) {
        if (!first) ss << ",";
        ss << "\"PHONE\"";
        first = false;
    }
    if (session.used_lifelines.find("AUDIENCE") == session.used_lifelines.end()) {
        if (!first) ss << ",";
        ss << "\"AUDIENCE\"";
        first = false;
    }
    ss << "],";
    
    ss << "\"timeLimit\":30,";
    ss << "\"timeRemaining\":30,";
    ss << "\"gameId\":" << game_id << ",";
    ss << "\"totalScore\":" << session.total_score;
    ss << "}";
    return ss.str();
}

string handleStart(const string& request, ClientSession& session, int client_fd) {
    // Check database only - database is source of truth
    GameSession active_game = Database::getInstance().getActiveGameSession(session.username);
    if (active_game.id > 0) {
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

    string data = "{\"gameId\":" + to_string(game_id) + 
                 ",\"timestamp\":" + to_string(time(nullptr)) + "}";
    
    // Send GAME_START notification
    string game_start_data = "{\"gameId\":" + to_string(game_id) + 
                            ",\"timestamp\":" + to_string(time(nullptr)) + "}";
    NotificationUtils::sendNotification(client_fd, "GAME_START", game_start_data);
    
    // Send QUESTION_INFO notification with first question
    string question_data = buildQuestionInfoData(first_question, game_id, session);
    NotificationUtils::sendNotification(client_fd, "QUESTION_INFO", question_data);
    
    return StreamUtils::createSuccessResponse(200, data);
}

string handleAnswer(const string& request, ClientSession& session, int client_fd) {
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
        
        // Update game session in database (same as wrong answer)
        Database::getInstance().endGame(game_id, "lost", safe_checkpoint_score, safe_checkpoint_prize);
        
        string data = "{\"gameId\":" + to_string(game_id) + 
                     ",\"correct\":false" +
                     ",\"questionNumber\":" + to_string(session.current_question_number) +
                     ",\"timeRemaining\":0" +
                     ",\"pointsEarned\":0" +
                     ",\"safeCheckpointPrize\":" + to_string(safe_checkpoint_prize) +
                     ",\"safeCheckpointScore\":" + to_string(safe_checkpoint_score) +
                     ",\"totalScore\":" + to_string(safe_checkpoint_score) +
                     ",\"finalPrize\":" + to_string(safe_checkpoint_prize) +
                     ",\"gameOver\":true,\"isWinner\":false}";
        
        // Send GAME_END notification (same as wrong answer)
        string game_end_data = "{\"gameId\":" + to_string(game_id) +
                              ",\"status\":\"lost\"" +
                              ",\"finalLevel\":" + to_string(session.current_question_number) +
                              ",\"finalQuestionNumber\":" + to_string(session.current_question_number) +
                              ",\"safeCheckpointPrize\":" + to_string(safe_checkpoint_prize) +
                              ",\"safeCheckpointScore\":" + to_string(safe_checkpoint_score) +
                              ",\"finalPrize\":" + to_string(safe_checkpoint_prize) +
                              ",\"totalScore\":" + to_string(safe_checkpoint_score) +
                              ",\"isWinner\":false}";
        NotificationUtils::sendNotification(client_fd, "GAME_END", game_end_data);
        
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
    int response_time = 30 - time_remaining;  // Calculate response time (30 second timer)
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
            
            // Send GAME_END notification
            string game_end_data = "{\"gameId\":" + to_string(game_id) +
                                  ",\"status\":\"won\"" +
                                  ",\"finalLevel\":15" +
                                  ",\"finalQuestionNumber\":15" +
                                  ",\"finalPrize\":1000000000" +
                                  ",\"totalScore\":" + to_string(session.total_score) +
                                  ",\"isWinner\":true}";
            NotificationUtils::sendNotification(client_fd, "GAME_END", game_end_data);
            
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
            
            // Send QUESTION_INFO notification with next question
            if (next_question.id > 0) {
                string question_data = buildQuestionInfoData(next_question, game_id, session);
                NotificationUtils::sendNotification(client_fd, "QUESTION_INFO", question_data);
            }
            
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
                     ",\"gameOver\":true,\"isWinner\":false}";
        
        // Send GAME_END notification
        string game_end_data = "{\"gameId\":" + to_string(game_id) +
                              ",\"status\":\"lost\"" +
                              ",\"finalLevel\":" + to_string(session.current_question_number) +
                              ",\"finalQuestionNumber\":" + to_string(session.current_question_number) +
                              ",\"safeCheckpointPrize\":" + to_string(safe_checkpoint_prize) +
                              ",\"safeCheckpointScore\":" + to_string(safe_checkpoint_score) +
                              ",\"finalPrize\":" + to_string(safe_checkpoint_prize) +
                              ",\"totalScore\":" + to_string(safe_checkpoint_score) +
                              ",\"isWinner\":false}";
        NotificationUtils::sendNotification(client_fd, "GAME_END", game_end_data);
        
        return StreamUtils::createSuccessResponse(200, data);
    }
}

string handleLifeline(const string& request, ClientSession& session, int client_fd) {
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
    
    // Build response data with lifeline result
    // result.result_data contains the hint JSON object
    // For 5050: {"remainingOptions":[0,2]}
    // For PHONE: {"suggestion":"I'm 85% sure it's A"}
    // For AUDIENCE: {"poll":{"A":65,"B":15,"C":10,"D":10}}
    
    // Merge result_data into response, adding lifelineType
    string data;
    if (!result.result_data.empty()) {
        // result_data is a JSON object, merge lifelineType into it
        string inner_data = result.result_data;
        if (inner_data.front() == '{' && inner_data.back() == '}') {
            // Remove outer braces
            inner_data = inner_data.substr(1, inner_data.length() - 2);
            // Add lifelineType as first field
            data = "{\"lifelineType\":\"" + lifeline_type + "\"";
            if (!inner_data.empty()) {
                data += "," + inner_data;
            }
            data += "}";
        } else {
            // Fallback: just wrap it
            data = "{\"lifelineType\":\"" + lifeline_type + "\",\"data\":" + result.result_data + "}";
        }
    } else {
        // Fallback if no result_data
        data = "{\"lifelineType\":\"" + lifeline_type + "\"}";
    }
    
    // TODO: Implement LIFELINE_INFO notification with delay
    // Delay times: 5050=5s, PHONE=10s, AUDIENCE=5s
    // This requires async/threading mechanism to send notification after delay
    // Example implementation:
    // int delay_seconds = (lifeline_type == "PHONE") ? 10 : 5;
    // std::thread([client_fd, lifeline_type, session, delay_seconds]() {
    //     std::this_thread::sleep_for(std::chrono::seconds(delay_seconds));
    //     
    //     string lifeline_data = buildLifelineInfoData(lifeline_type, session);
    //     NotificationUtils::sendNotification(client_fd, "LIFELINE_INFO", lifeline_data);
    // }).detach();
    
    return StreamUtils::createSuccessResponse(200, data);
}

string handleGiveUp(const string& request, ClientSession& session, int client_fd) {
    // Check both in-memory session and database for active game
    int game_id = JsonUtils::extractInt(request, "gameId", -1);
    
    // If session.in_game is false, check database for active game
    if (!session.in_game) {
        GameSession active_game = Database::getInstance().getActiveGameSession(session.username);
        if (active_game.id == 0) {
            return StreamUtils::createErrorResponse(406, "Not in a game");
        }
        // Restore session state from database
        session.in_game = true;
        session.game_id = active_game.id;
        session.current_question_number = active_game.current_question_number;
        session.current_prize = active_game.current_prize;
        session.total_score = active_game.total_score;
        game_id = active_game.id; // Use game_id from database if not provided
    }

    if (game_id < 0) {
        return StreamUtils::createErrorResponse(422, "Missing or invalid gameId");
    }

    if (game_id != session.game_id) {
        return StreamUtils::createErrorResponse(412, "Invalid gameId - gameId doesn't match active game");
    }

    int question_number = JsonUtils::extractInt(request, "questionNumber", -1);
    if (question_number != session.current_question_number) {
        return StreamUtils::createErrorResponse(422, 
            "Question number mismatch: expected " + to_string(session.current_question_number) + 
            ", got " + to_string(question_number));
    }

    long long final_prize = session.current_prize;
    int final_question_number = session.current_question_number;
    int total_score = session.total_score;

    // Stop timer
    GameTimer::getInstance().stopTimer(game_id);
    
    // End game in database (use 'quit' status as per schema)
    Database::getInstance().endGame(game_id, "quit", total_score, final_prize);
    
    // Update session state
    session.in_game = false;

    string data = "{\"finalPrize\":" + to_string(final_prize) + 
                 ",\"finalQuestionNumber\":" + to_string(final_question_number) + 
                 ",\"totalScore\":" + to_string(total_score) +
                 ",\"gameId\":" + to_string(game_id) + "}";
    
    // Send GAME_END notification
    string game_end_data = "{\"gameId\":" + to_string(game_id) +
                          ",\"status\":\"quit\"" +
                          ",\"finalLevel\":" + to_string(final_question_number) +
                          ",\"finalQuestionNumber\":" + to_string(final_question_number) +
                          ",\"finalPrize\":" + to_string(final_prize) +
                          ",\"totalScore\":" + to_string(total_score) + "}";
    NotificationUtils::sendNotification(client_fd, "GAME_END", game_end_data);
    
    return StreamUtils::createSuccessResponse(200, data);
}

string handleResume(const string& request, ClientSession& session, int client_fd) {
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
                 ",\"totalScore\":" + to_string(session.total_score) + "}";
    
    // TODO: Send QUESTION_INFO notification with resumed question
    // This requires database integration to load question data
    // Question q = Database::getInstance().getQuestion(session.current_level);
    // string question_data = buildQuestionInfoData(q, session.game_id, session);
    // NotificationUtils::sendNotification(client_fd, "QUESTION_INFO", question_data);
    
    return StreamUtils::createSuccessResponse(200, data);
}

string handleLeaveGame(const string& request, ClientSession& session, int client_fd) {
    if (!session.in_game) {
        return StreamUtils::createErrorResponse(406, "Not in a game");
    }

    GameStateManager::getInstance().saveGameProgress(session.username, 
        session.current_question_number, session.current_prize);
    session.in_game = false;

    string data = "{}";
    return StreamUtils::createSuccessResponse(200, data);
}

} // namespace GameHandlers

} // namespace MillionaireGame

