#include "game_handlers.h"
#include "../game_state_manager.h"
#include "../json_utils.h"
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

    int game_id = GameStateManager::getInstance().generateGameId();
    
    session.in_game = true;
    session.game_id = game_id;
    session.current_question_number = 1;
    session.current_level = 1;
    session.current_prize = 1000000;
    session.total_score = 0;
    session.used_lifelines.clear();

    string data = "{\"message\":\"Game started\",\"gameId\":" + to_string(game_id) + 
                 ",\"timestamp\":" + to_string(time(nullptr)) + "}";
    return StreamUtils::createSuccessResponse(200, data);
}

string handleAnswer(const string& request, ClientSession& session) {
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

    bool correct = GameStateManager::getInstance().checkAnswer(session.current_level, to_string(answer_index));
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

    session.used_lifelines.insert(lifeline_type);
    string data = "{\"message\":\"Lifeline processed\"}";
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

