#ifndef GUI_APP_H
#define GUI_APP_H

#include <string>
#include <memory>
#include <vector>
#include <map>
#include "protocol_handler.h"
#include "socket_client.h"

// ImGUI includes
#ifdef _WIN32
    #include <windows.h>
#endif
#include <GL/gl.h>
#ifdef __APPLE__
    #include <OpenGL/gl.h>
#endif
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

// Game state
enum class Screen {
    LOGIN,
    REGISTER,
    MAIN_MENU,
    GAME,
    LEADERBOARD,
    FRIENDS,
    PROFILE,
    ADMIN_PANEL
};

enum class GameState {
    IDLE,
    WAITING_QUESTION,
    SHOWING_QUESTION,
    SHOWING_RESULT,
    GAME_OVER
};

struct QuestionInfo {
    int questionId = 0;
    int questionNumber = 0;
    std::string question;
    std::vector<std::pair<std::string, std::string>> options; // label, text
    int prize = 0;
    int timeLimit = 30;
    int timeRemaining = 30;
    int gameId = 0;
    int totalScore = 0;
    std::vector<std::string> lifelinesAvailable;
};

struct GameResult {
    bool correct = false;
    int pointsEarned = 0;
    int totalScore = 0;
    int currentPrize = 0;
    bool gameOver = false;
    bool isWinner = false;
    int correctAnswer = -1;
    int safeCheckpointPrize = 0;
};

class GuiApp {
public:
    GuiApp();
    ~GuiApp();
    
    bool initialize(int windowWidth = 1280, int windowHeight = 720);
    void run();
    void shutdown();

private:
    // Window and OpenGL
    GLFWwindow* window_;
    int window_width_;
    int window_height_;
    
    // Network
    std::unique_ptr<SocketClient> socket_client_;
    std::unique_ptr<ProtocolHandler> protocol_handler_;
    std::string server_host_;
    int server_port_;
    
    // Application state
    Screen current_screen_;
    GameState game_state_;
    bool connected_;
    std::string error_message_;
    
    // Login/Register state
    char username_buf_[256];
    char password_buf_[256];
    char register_username_buf_[256];
    char register_password_buf_[256];
    char confirm_password_buf_[256];
    
    // Game state
    QuestionInfo current_question_;
    GameResult last_result_;
    int selected_answer_;
    bool lifeline_used_5050_;
    bool lifeline_used_phone_;
    bool lifeline_used_audience_;
    std::vector<int> disabled_options_; // For 50/50
    std::string phone_suggestion_;
    std::map<std::string, int> audience_poll_;
    
    // Leaderboard state
    struct LeaderboardEntry {
        std::string username;
        int finalQuestionNumber;
        int totalScore;
        int rank;
        bool isWinner;
    };
    std::vector<LeaderboardEntry> global_leaderboard_;
    std::vector<LeaderboardEntry> friend_leaderboard_;
    int leaderboard_page_;
    bool showing_global_leaderboard_;
    
    // Friend state
    struct FriendInfo {
        std::string username;
        std::string status; // online, ingame, offline
    };
    std::vector<FriendInfo> friends_;
    std::vector<std::string> friend_requests_;
    char add_friend_buf_[256];
    
    // Profile state
    struct UserProfile {
        std::string username;
        int totalGames;
        int highestPrize;
        int finalQuestionNumber;
        int totalScore;
    };
    UserProfile current_profile_;
    struct GameHistoryEntry {
        int gameId;
        std::string date;
        int finalQuestionNumber;
        int totalScore;
        int finalPrize;
        std::string status;
    };
    std::vector<GameHistoryEntry> game_history_;
    
    // Admin state
    char admin_question_buf_[512];
    char admin_option_a_[256];
    char admin_option_b_[256];
    char admin_option_c_[256];
    char admin_option_d_[256];
    int admin_correct_answer_;
    int admin_question_level_;
    int admin_view_page_;
    
    // UI Methods
    void renderFrame();
    void renderLoginScreen();
    void renderRegisterScreen();
    void renderMainMenu();
    void renderGameScreen();
    void renderLeaderboardScreen();
    void renderFriendsScreen();
    void renderProfileScreen();
    void renderAdminPanel();
    
    // Helper methods
    void showError(const std::string& error);
    void switchScreen(Screen screen);
    void connectToServer();
    void handleProtocolResponse(int responseCode, const std::string& jsonData);
    void formatMoney(int amount, char* buffer, size_t bufferSize);
    
    // GLFW callbacks
    static void glfwErrorCallback(int error, const char* description);
};

#endif // GUI_APP_H

