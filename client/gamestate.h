#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <string>
#include <vector>
#include <map>
#include <chrono>

struct GameState {
    bool showLogin = true;
    bool showRegister = false;
    bool loggedIn = false;
    bool inGame = false;
    bool waitingForQuestion = false;
    bool showResultMessage = false;
    bool showResultScreen = false;
    
    // Login/Register
    std::string username;
    std::string password;
    std::string registerPassword2;
    std::string errorMessage;
    std::string resultMessage;
    float resultMessageTime = 0.0f;

    // Home screen
    bool onHome = false;
    
    // Game
    std::string question;
    std::vector<std::string> options;
    int selectedAnswer = -1;
    int timeRemaining = 30;
    int currentQuestionNumber = 0;
    int totalScore = 0;
    int currentPrize = 0;
    bool timerRunning = false;
    int timerThreadId = 0;  // Unique ID for each timer thread to prevent multiple timers
    std::vector<bool> availableLifelines = {true, true, true}; // 50/50, Phone, Audience

    // Reveal sequence
    std::chrono::steady_clock::time_point revealStart;
    bool revealActive = false;
    int answersRevealed = 0;       // 0..4
    bool timerStartedForThisQuestion = false;
    
    // Lifeline processing
    bool lifelineProcessing = false;
    std::string lifelineLoadingMessage;  // "Eliminating...", "Calling...", "Surveying"
    std::chrono::steady_clock::time_point lifelineStartTime;
    std::string lifelineType;  // "5050", "PHONE", "AUDIENCE"
    std::vector<int> lifeline5050Remaining;  // For 5050: remaining option indices
    std::string lifelinePhoneSuggestion;  // For PHONE: suggestion string
    std::map<char, int> lifelineAudiencePoll;  // For AUDIENCE: poll percentages (A, B, C, D)
    bool timerPaused = false;
    int pausedTimeRemaining = 0;

    // Final result
    long long finalPrize = 0;
    
    // Prize ladder
    static const int PRIZE_LADDER[15];
};

#endif // GAMESTATE_H
