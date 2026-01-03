#include "game_handlers.h"
#include "../game_state_manager.h"
#include "../database.h"
#include "../logger.h"
#include "../json_utils.h"
#include "../stream_handler.h"
#include "../notification_utils.h"
#include <ctime>
#include <algorithm>
#include <sstream>

using namespace std;

namespace MillionaireGame {

namespace GameHandlers {

// Helper function to build QUESTION_INFO notification data
string buildQuestionInfoData(const Question& q, int game_id, const ClientSession& session) {
    ostringstream data;
    data << "{\"gameId\":" << game_id
         << ",\"questionId\":" << q.id
         << ",\"questionNumber\":" << session.current_question_number
         << ",\"totalQuestions\":15"
         << ",\"question\":\"" << q.question_text << "\""
         << ",\"prize\":" << session.current_prize
         << ",\"totalScore\":" << session.total_score
         << ",\"timeLimit\":30"
         << ",\"timeRemaining\":30"
         << ",\"correctAnswer\":" << q.correct_answer
         << ",\"options\":["
         << "{\"label\":\"A\",\"text\":\"" << q.option_a << "\"},"
         << "{\"label\":\"B\",\"text\":\"" << q.option_b << "\"},"
         << "{\"label\":\"C\",\"text\":\"" << q.option_c << "\"},"
         << "{\"label\":\"D\",\"text\":\"" << q.option_d << "\"}"
         << "]}";
    return data.str();
}

string handleStart(const string& request, ClientSession& session, int client_fd) {
    if (session.in_game) {
        return StreamUtils::createErrorResponse(405, "Already in a game");
    }

    bool override_saved = JsonUtils::extractBool(request, "overrideSavedGame", false);
    
    GameProgress saved_progress = GameStateManager::getInstance().loadGameProgress(session.username);
    if (saved_progress.level > 0 && !override_saved) {
        return StreamUtils::createErrorResponse(412, 
            "You have a saved game. Use RESUME to continue or set overrideSavedGame=true to start new game");
    }

    int game_id = GameStateManager::getInstance().generateGameId();
    
    session.in_game = true;
    session.game_id = game_id;
    session.current_question_number = 1;
    session.current_level = 1;
    session.current_prize = 1000000;
    session.total_score = 0;
    session.used_lifelines.clear();

    string data = "{\"gameId\":" + to_string(game_id) + 
                 ",\"timestamp\":" + to_string(time(nullptr)) + "}";
    
    // Send GAME_START notification
    string game_start_data = "{\"gameId\":" + to_string(game_id) + 
                            ",\"timestamp\":" + to_string(time(nullptr)) + "}";
    NotificationUtils::sendNotification(client_fd, "GAME_START", game_start_data);
    
    // Load and send first question
    LOG_INFO("Loading question for level " + to_string(session.current_level));
    Question q = Database::getInstance().getRandomQuestion(session.current_level);
    if (q.id > 0) {
        LOG_INFO("Question loaded: ID=" + to_string(q.id) + ", Level=" + to_string(q.level));
        string question_data = buildQuestionInfoData(q, game_id, session);
        LOG_INFO("Sending QUESTION_INFO notification to client " + to_string(client_fd));
        NotificationUtils::sendNotification(client_fd, "QUESTION_INFO", question_data);
        LOG_INFO("QUESTION_INFO notification sent");
    } else {
        LOG_WARNING("No question found for level " + to_string(session.current_level));
        LOG_WARNING("Database connected: " + string(Database::getInstance().isConnected() ? "yes" : "no"));
    }
    
    return StreamUtils::createSuccessResponse(200, data);
}

string handleAnswer(const string& request, ClientSession& session, int client_fd) {
    if (!session.in_game) {
        return StreamUtils::createErrorResponse(406, "Not in a game");
    }

    int game_id = JsonUtils::extractInt(request, "gameId", -1);
    int question_number = JsonUtils::extractInt(request, "questionNumber", -1);
    int answer_index = JsonUtils::extractInt(request, "answerIndex", -1);

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

    // TODO: Check timeout - will be implemented with game timer
    // if (GameTimer::getInstance().isTimeout(game_id)) {
    //     session.in_game = false;
    //     // Calculate safe checkpoint prize and score
    //     int safe_checkpoint_prize = 0;
    //     int safe_checkpoint_score = 0;
    //     if (session.current_question_number > 15) {
    //         safe_checkpoint_prize = 1000000000;
    //         safe_checkpoint_score = session.total_score;
    //     } else if (session.current_question_number > 10) {
    //         safe_checkpoint_prize = 100000000;
    //         safe_checkpoint_score = session.total_score;
    //     } else if (session.current_question_number > 5) {
    //         safe_checkpoint_prize = 10000000;
    //         safe_checkpoint_score = session.total_score;
    //     }
    //     return StreamUtils::createErrorResponse(408, "Question timeout");
    // }

    // Check answer against database - load question for current level
    Question current_q = Database::getInstance().getRandomQuestion(session.current_level);
    bool correct = false;
    if (current_q.id > 0) {
        correct = (answer_index == current_q.correct_answer);
    } else {
        // Fallback if question not found
        LOG_WARNING("Question not found for level " + to_string(session.current_level) + ", using fallback");
        correct = GameStateManager::getInstance().checkAnswer(session.current_level, to_string(answer_index));
    }
    int time_remaining = 15;  // Placeholder
    int lifelines_used = session.used_lifelines.size();
    int points_earned = max(0, time_remaining - (lifelines_used * 5));

    if (correct) {
        session.total_score += points_earned;
        session.current_question_number++;
        
        if (session.current_question_number > 15) {
            session.in_game = false;
            string data = "{\"gameId\":" + to_string(game_id) + 
                         ",\"correct\":true,\"questionNumber\":15" +
                         ",\"timeRemaining\":" + to_string(time_remaining) +
                         ",\"pointsEarned\":" + to_string(points_earned) +
                         ",\"totalScore\":" + to_string(session.total_score) +
                         ",\"currentPrize\":1000000000" +
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
            session.current_prize *= 2;
            string data = "{\"gameId\":" + to_string(game_id) + 
                         ",\"correct\":true" +
                         ",\"questionNumber\":" + to_string(session.current_question_number - 1) +
                         ",\"timeRemaining\":" + to_string(time_remaining) +
                         ",\"pointsEarned\":" + to_string(points_earned) +
                         ",\"totalScore\":" + to_string(session.total_score) +
                         ",\"currentPrize\":" + to_string(session.current_prize) +
                         ",\"gameOver\":false,\"isWinner\":false}";
            
            // Load and send next question
            session.current_level = session.current_question_number;
            Question next_q = Database::getInstance().getRandomQuestion(session.current_level);
            if (next_q.id > 0) {
                string question_data = buildQuestionInfoData(next_q, game_id, session);
                NotificationUtils::sendNotification(client_fd, "QUESTION_INFO", question_data);
            } else {
                LOG_WARNING("No question found for level " + to_string(session.current_level));
            }
            
            return StreamUtils::createSuccessResponse(200, data);
        }
    } else {
        session.in_game = false;
        int safe_checkpoint_prize = 0;
        int safe_checkpoint_score = 0;
        if (session.current_question_number > 15) {
            safe_checkpoint_prize = 1000000000;
            safe_checkpoint_score = session.total_score;
        } else if (session.current_question_number > 10) {
            safe_checkpoint_prize = 100000000;
            safe_checkpoint_score = session.total_score - points_earned;
        } else if (session.current_question_number > 5) {
            safe_checkpoint_prize = 10000000;
            safe_checkpoint_score = session.total_score - points_earned;
        }
        
        string data = "{\"gameId\":" + to_string(game_id) + 
                     ",\"correct\":false" +
                     ",\"questionNumber\":" + to_string(session.current_question_number) +
                     ",\"correctAnswer\":" + to_string(answer_index) +
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

    session.used_lifelines.insert(lifeline_type);
    string data = "{\"lifelineType\":\"" + lifeline_type + "}";
    
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
                 ",\"gameId\":" + to_string(game_id) + "}";
    
    // Send GAME_END notification
    string game_end_data = "{\"gameId\":" + to_string(game_id) +
                          ",\"status\":\"gave_up\"" +
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
    
    // Load and send resumed question
    session.current_level = progress.level;
    Question q = Database::getInstance().getRandomQuestion(session.current_level);
    if (q.id > 0) {
        string question_data = buildQuestionInfoData(q, session.game_id, session);
        NotificationUtils::sendNotification(client_fd, "QUESTION_INFO", question_data);
    } else {
        LOG_WARNING("No question found for level " + to_string(session.current_level));
    }
    
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

