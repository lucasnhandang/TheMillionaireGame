#include "gui_app.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <regex>
#include <cstring>
#include <algorithm>
#include <cmath>

// JSON parsing helper functions (simple implementation)
namespace {
    std::string extractString(const std::string& json, const std::string& key) {
        std::string search_key = "\"" + key + "\"";
        size_t pos = json.find(search_key);
        if (pos == std::string::npos) return "";
        
        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        pos++;
        
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
            pos++;
        }
        
        if (pos >= json.length() || json[pos] != '"') return "";
        pos++;
        
        size_t end = json.find('"', pos);
        if (end == std::string::npos) return "";
        
        return json.substr(pos, end - pos);
    }
    
    int extractInt(const std::string& json, const std::string& key, int default_value = 0) {
        std::string search_key = "\"" + key + "\"";
        size_t pos = json.find(search_key);
        if (pos == std::string::npos) return default_value;
        
        pos = json.find(':', pos);
        if (pos == std::string::npos) return default_value;
        pos++;
        
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
            pos++;
        }
        
        if (pos >= json.length()) return default_value;
        
        size_t end = pos;
        while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != ']' && json[end] != ' ') {
            end++;
        }
        
        if (end == pos) return default_value;
        
        try {
            return std::stoi(json.substr(pos, end - pos));
        } catch (...) {
            return default_value;
        }
    }
    
    bool extractBool(const std::string& json, const std::string& key, bool default_value = false) {
        std::string search_key = "\"" + key + "\"";
        size_t pos = json.find(search_key);
        if (pos == std::string::npos) return default_value;
        
        pos = json.find(':', pos);
        if (pos == std::string::npos) return default_value;
        pos++;
        
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
            pos++;
        }
        
        if (pos >= json.length()) return default_value;
        
        if (json.substr(pos, 4) == "true") {
            return true;
        } else if (json.substr(pos, 5) == "false") {
            return false;
        }
        
        return default_value;
    }
}

GuiApp::GuiApp() 
    : window_(nullptr), window_width_(1280), window_height_(720),
      server_host_("localhost"), server_port_(8080),
      current_screen_(Screen::LOGIN), game_state_(GameState::IDLE),
      connected_(false), selected_answer_(-1),
      lifeline_used_5050_(false), lifeline_used_phone_(false), lifeline_used_audience_(false),
      leaderboard_page_(1), showing_global_leaderboard_(true),
      admin_correct_answer_(0), admin_question_level_(1), admin_view_page_(1) {
    
    memset(username_buf_, 0, sizeof(username_buf_));
    memset(password_buf_, 0, sizeof(password_buf_));
    memset(register_username_buf_, 0, sizeof(register_username_buf_));
    memset(register_password_buf_, 0, sizeof(register_password_buf_));
    memset(confirm_password_buf_, 0, sizeof(confirm_password_buf_));
    memset(add_friend_buf_, 0, sizeof(add_friend_buf_));
    memset(admin_question_buf_, 0, sizeof(admin_question_buf_));
    memset(admin_option_a_, 0, sizeof(admin_option_a_));
    memset(admin_option_b_, 0, sizeof(admin_option_b_));
    memset(admin_option_c_, 0, sizeof(admin_option_c_));
    memset(admin_option_d_, 0, sizeof(admin_option_d_));
}

GuiApp::~GuiApp() {
    shutdown();
}

bool GuiApp::initialize(int windowWidth, int windowHeight) {
    window_width_ = windowWidth;
    window_height_ = windowHeight;
    
    // Setup GLFW
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // GL 3.3 + GLSL 330
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window_ = glfwCreateWindow(window_width_, window_height_, "Who Wants To Be A Millionaire?", nullptr, nullptr);
    if (window_ == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable vsync
    
    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup style
    ImGui::StyleColorsDark();
    
    // Setup platform/renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Initialize network
    socket_client_ = std::make_unique<SocketClient>();
    protocol_handler_ = std::make_unique<ProtocolHandler>(socket_client_.get());
    
    protocol_handler_->setResponseCallback([this](int responseCode, const std::string& jsonData) {
        this->handleProtocolResponse(responseCode, jsonData);
    });
    
    return true;
}

void GuiApp::run() {
    if (!window_) {
        std::cerr << "Window not initialized. Call initialize() first." << std::endl;
        return;
    }
    
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Render UI
        renderFrame();
        
        // Render ImGui
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window_, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window_);
    }
}

void GuiApp::shutdown() {
    if (protocol_handler_) {
        if (!protocol_handler_->getAuthToken().empty()) {
            protocol_handler_->logout();
        }
    }
    
    if (socket_client_) {
        socket_client_->disconnect();
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    if (window_) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
    }
}

void GuiApp::renderFrame() {
    switch (current_screen_) {
        case Screen::LOGIN:
            renderLoginScreen();
            break;
        case Screen::REGISTER:
            renderRegisterScreen();
            break;
        case Screen::MAIN_MENU:
            renderMainMenu();
            break;
        case Screen::GAME:
            renderGameScreen();
            break;
        case Screen::LEADERBOARD:
            renderLeaderboardScreen();
            break;
        case Screen::FRIENDS:
            renderFriendsScreen();
            break;
        case Screen::PROFILE:
            renderProfileScreen();
            break;
        case Screen::ADMIN_PANEL:
            renderAdminPanel();
            break;
    }
    
    // Show error message if any
    if (!error_message_.empty()) {
        ImGui::OpenPopup("Error");
        if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", error_message_.c_str());
            if (ImGui::Button("OK")) {
                error_message_.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

void GuiApp::renderLoginScreen() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(window_width_, window_height_));
    ImGui::Begin("Login", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    
    ImGui::SetCursorPos(ImVec2(window_width_ / 2 - 150, window_height_ / 2 - 100));
    ImGui::BeginChild("LoginForm", ImVec2(300, 250));
    
    ImGui::Text("Who Wants To Be A Millionaire?");
    ImGui::Spacing();
    
    ImGui::Text("Username:");
    ImGui::InputText("##username", username_buf_, sizeof(username_buf_));
    
    ImGui::Text("Password:");
    ImGui::InputText("##password", password_buf_, ImGuiInputTextFlags_Password);
    
    ImGui::Spacing();
    
    if (ImGui::Button("Login", ImVec2(280, 30))) {
        if (strlen(username_buf_) > 0 && strlen(password_buf_) > 0) {
            connectToServer();
            if (connected_) {
                protocol_handler_->login(username_buf_, password_buf_);
            } else {
                showError("Failed to connect to server");
            }
        } else {
            showError("Please enter username and password");
        }
    }
    
    ImGui::Spacing();
    
    if (ImGui::Button("Register", ImVec2(280, 30))) {
        switchScreen(Screen::REGISTER);
    }
    
    ImGui::EndChild();
    ImGui::End();
}

void GuiApp::renderRegisterScreen() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(window_width_, window_height_));
    ImGui::Begin("Register", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    
    ImGui::SetCursorPos(ImVec2(window_width_ / 2 - 150, window_height_ / 2 - 120));
    ImGui::BeginChild("RegisterForm", ImVec2(300, 300));
    
    ImGui::Text("Create Account");
    ImGui::Spacing();
    
    ImGui::Text("Username:");
    ImGui::InputText("##reg_username", register_username_buf_, sizeof(register_username_buf_));
    
    ImGui::Text("Password:");
    ImGui::InputText("##reg_password", register_password_buf_, sizeof(register_password_buf_), 
                     ImGuiInputTextFlags_Password);
    
    ImGui::Text("Confirm Password:");
    ImGui::InputText("##confirm_password", confirm_password_buf_, sizeof(confirm_password_buf_), 
                     ImGuiInputTextFlags_Password);
    
    ImGui::Spacing();
    
    if (ImGui::Button("Register", ImVec2(280, 30))) {
        if (strlen(register_username_buf_) > 0 && strlen(register_password_buf_) > 0) {
            if (strcmp(register_password_buf_, confirm_password_buf_) == 0) {
                connectToServer();
                if (connected_) {
                    protocol_handler_->registerUser(register_username_buf_, register_password_buf_);
                } else {
                    showError("Failed to connect to server");
                }
            } else {
                showError("Passwords do not match");
            }
        } else {
            showError("Please fill all fields");
        }
    }
    
    ImGui::Spacing();
    
    if (ImGui::Button("Back to Login", ImVec2(280, 30))) {
        switchScreen(Screen::LOGIN);
    }
    
    ImGui::EndChild();
    ImGui::End();
}

void GuiApp::renderMainMenu() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(window_width_, window_height_));
    ImGui::Begin("Main Menu", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    
    ImGui::Text("Welcome, %s!", protocol_handler_->getUsername().c_str());
    if (protocol_handler_->isAdmin()) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "[ADMIN]");
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    float buttonWidth = 300.0f;
    float buttonHeight = 50.0f;
    float centerX = (window_width_ - buttonWidth) / 2.0f;
    
    ImGui::SetCursorPosX(centerX);
    if (ImGui::Button("Start New Game", ImVec2(buttonWidth, buttonHeight))) {
        protocol_handler_->startGame(false);
        game_state_ = GameState::WAITING_QUESTION;
        switchScreen(Screen::GAME);
    }
    
    ImGui::SetCursorPosX(centerX);
    if (ImGui::Button("Resume Game", ImVec2(buttonWidth, buttonHeight))) {
        protocol_handler_->resumeGame();
        game_state_ = GameState::WAITING_QUESTION;
        switchScreen(Screen::GAME);
    }
    
    ImGui::SetCursorPosX(centerX);
    if (ImGui::Button("Leaderboard", ImVec2(buttonWidth, buttonHeight))) {
        protocol_handler_->getLeaderboard("global", 1, 20);
        switchScreen(Screen::LEADERBOARD);
    }
    
    ImGui::SetCursorPosX(centerX);
    if (ImGui::Button("Friends", ImVec2(buttonWidth, buttonHeight))) {
        protocol_handler_->getFriendStatus();
        protocol_handler_->getFriendRequestList();
        switchScreen(Screen::FRIENDS);
    }
    
    ImGui::SetCursorPosX(centerX);
    if (ImGui::Button("Profile", ImVec2(buttonWidth, buttonHeight))) {
        protocol_handler_->getUserInfo(protocol_handler_->getUsername());
        protocol_handler_->getGameHistory();
        switchScreen(Screen::PROFILE);
    }
    
    if (protocol_handler_->isAdmin()) {
        ImGui::SetCursorPosX(centerX);
        if (ImGui::Button("Admin Panel", ImVec2(buttonWidth, buttonHeight))) {
            switchScreen(Screen::ADMIN_PANEL);
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::SetCursorPosX(centerX);
    if (ImGui::Button("Logout", ImVec2(buttonWidth, buttonHeight))) {
        protocol_handler_->logout();
        switchScreen(Screen::LOGIN);
    }
    
    ImGui::End();
}

void GuiApp::renderGameScreen() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(window_width_, window_height_));
    ImGui::Begin("Game", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    
    // Header info
    char prize_str[64];
    formatMoney(current_question_.prize, prize_str, sizeof(prize_str));
    ImGui::Text("Question %d / 15", current_question_.questionNumber);
    ImGui::SameLine(ImGui::GetWindowWidth() - 200);
    ImGui::Text("Prize: %s", prize_str);
    ImGui::Text("Score: %d", current_question_.totalScore);
    
    ImGui::Separator();
    ImGui::Spacing();
    
    if (game_state_ == GameState::SHOWING_QUESTION || game_state_ == GameState::WAITING_QUESTION) {
        // Timer
        ImGui::Text("Time Remaining: %d seconds", current_question_.timeRemaining);
        
        // Question
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
        ImGui::TextWrapped("%s", current_question_.question.c_str());
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Answer buttons
        for (size_t i = 0; i < current_question_.options.size(); i++) {
            bool disabled = std::find(disabled_options_.begin(), disabled_options_.end(), 
                                      static_cast<int>(i)) != disabled_options_.end();
            
            if (disabled) {
                ImGui::BeginDisabled();
            }
            
            std::string buttonLabel = current_question_.options[i].first + ": " + 
                                     current_question_.options[i].second;
            
            if (ImGui::Button(buttonLabel.c_str(), ImVec2(600, 50))) {
                if (game_state_ == GameState::SHOWING_QUESTION) {
                    protocol_handler_->answerQuestion(current_question_.gameId, 
                                                     current_question_.questionNumber, 
                                                     static_cast<int>(i));
                    selected_answer_ = static_cast<int>(i);
                    game_state_ = GameState::SHOWING_RESULT;
                }
            }
            
            if (disabled) {
                ImGui::EndDisabled();
            }
            
            ImGui::Spacing();
        }
        
        // Lifelines
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Lifelines:");
        ImGui::Spacing();
        
        if (!lifeline_used_5050_ && 
            std::find(current_question_.lifelinesAvailable.begin(), 
                     current_question_.lifelinesAvailable.end(), "5050") != 
            current_question_.lifelinesAvailable.end()) {
            if (ImGui::Button("50/50", ImVec2(150, 40))) {
                protocol_handler_->useLifeline(current_question_.gameId, 
                                              current_question_.questionNumber, "5050");
            }
            ImGui::SameLine();
        }
        
        if (!lifeline_used_phone_ && 
            std::find(current_question_.lifelinesAvailable.begin(), 
                     current_question_.lifelinesAvailable.end(), "PHONE") != 
            current_question_.lifelinesAvailable.end()) {
            if (ImGui::Button("Phone a Friend", ImVec2(150, 40))) {
                protocol_handler_->useLifeline(current_question_.gameId, 
                                              current_question_.questionNumber, "PHONE");
            }
            ImGui::SameLine();
        }
        
        if (!lifeline_used_audience_ && 
            std::find(current_question_.lifelinesAvailable.begin(), 
                     current_question_.lifelinesAvailable.end(), "AUDIENCE") != 
            current_question_.lifelinesAvailable.end()) {
            if (ImGui::Button("Ask the Audience", ImVec2(150, 40))) {
                protocol_handler_->useLifeline(current_question_.gameId, 
                                              current_question_.questionNumber, "AUDIENCE");
            }
        }
        
        // Show lifeline results
        if (!phone_suggestion_.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Phone suggestion: %s", phone_suggestion_.c_str());
        }
        
        if (!audience_poll_.empty()) {
            ImGui::Spacing();
            ImGui::Text("Audience Poll:");
            for (const auto& poll : audience_poll_) {
                ImGui::Text("  %s: %d%%", poll.first.c_str(), poll.second);
            }
        }
        
        // Action buttons
        ImGui::Spacing();
        ImGui::Separator();
        
        if (ImGui::Button("Give Up", ImVec2(150, 40))) {
            if (current_question_.gameId > 0) {
                protocol_handler_->giveUp(current_question_.gameId, current_question_.questionNumber);
            }
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Leave Game", ImVec2(150, 40))) {
            protocol_handler_->leaveGame();
            switchScreen(Screen::MAIN_MENU);
        }
    }
    
    if (game_state_ == GameState::SHOWING_RESULT) {
        if (last_result_.correct) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Correct!");
            ImGui::Text("Points earned: %d", last_result_.pointsEarned);
            ImGui::Text("Total score: %d", last_result_.totalScore);
            
            if (last_result_.gameOver) {
                if (last_result_.isWinner) {
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "CONGRATULATIONS! YOU WON!");
                    char final_prize[64];
                    formatMoney(last_result_.currentPrize, final_prize, sizeof(final_prize));
                    ImGui::Text("Final Prize: %s", final_prize);
                } else {
                    ImGui::Text("Game Over - You completed all questions!");
                }
                
                if (ImGui::Button("Back to Menu", ImVec2(200, 40))) {
                    switchScreen(Screen::MAIN_MENU);
                }
            } else {
                if (ImGui::Button("Continue", ImVec2(200, 40))) {
                    game_state_ = GameState::WAITING_QUESTION;
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Wrong Answer!");
            if (last_result_.correctAnswer >= 0) {
                ImGui::Text("Correct answer was: %s", 
                           current_question_.options[last_result_.correctAnswer].first.c_str());
            }
            
            char checkpoint_prize[64];
            formatMoney(last_result_.safeCheckpointPrize, checkpoint_prize, sizeof(checkpoint_prize));
            ImGui::Text("You receive: %s", checkpoint_prize);
            ImGui::Text("Final Score: %d", last_result_.totalScore);
            
            if (ImGui::Button("Back to Menu", ImVec2(200, 40))) {
                switchScreen(Screen::MAIN_MENU);
            }
        }
    }
    
    if (game_state_ == GameState::GAME_OVER) {
        ImGui::Text("Game Over");
        if (ImGui::Button("Back to Menu", ImVec2(200, 40))) {
            switchScreen(Screen::MAIN_MENU);
        }
    }
    
    ImGui::End();
}

void GuiApp::renderLeaderboardScreen() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(window_width_, window_height_));
    ImGui::Begin("Leaderboard", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    
    if (ImGui::Button("Back", ImVec2(100, 30))) {
        switchScreen(Screen::MAIN_MENU);
    }
    
    ImGui::Spacing();
    
    if (ImGui::Button("Global", ImVec2(150, 30))) {
        showing_global_leaderboard_ = true;
        protocol_handler_->getLeaderboard("global", leaderboard_page_, 20);
    }
    ImGui::SameLine();
    if (ImGui::Button("Friends", ImVec2(150, 30))) {
        showing_global_leaderboard_ = false;
        protocol_handler_->getLeaderboard("friend", leaderboard_page_, 20);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    auto& leaderboard = showing_global_leaderboard_ ? global_leaderboard_ : friend_leaderboard_;
    
    if (ImGui::BeginTable("Leaderboard", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Rank");
        ImGui::TableSetupColumn("Username");
        ImGui::TableSetupColumn("Questions");
        ImGui::TableSetupColumn("Score");
        ImGui::TableHeadersRow();
        
        for (const auto& entry : leaderboard) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%d", entry.rank);
            ImGui::TableNextColumn();
            if (entry.isWinner) {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", entry.username.c_str());
            } else {
                ImGui::Text("%s", entry.username.c_str());
            }
            ImGui::TableNextColumn();
            ImGui::Text("%d", entry.finalQuestionNumber);
            ImGui::TableNextColumn();
            ImGui::Text("%d", entry.totalScore);
        }
        
        ImGui::EndTable();
    }
    
    ImGui::Spacing();
    
    if (ImGui::Button("Previous")) {
        if (leaderboard_page_ > 1) {
            leaderboard_page_--;
            protocol_handler_->getLeaderboard(showing_global_leaderboard_ ? "global" : "friend", 
                                             leaderboard_page_, 20);
        }
    }
    ImGui::SameLine();
    ImGui::Text("Page %d", leaderboard_page_);
    ImGui::SameLine();
    if (ImGui::Button("Next")) {
        leaderboard_page_++;
        protocol_handler_->getLeaderboard(showing_global_leaderboard_ ? "global" : "friend", 
                                         leaderboard_page_, 20);
    }
    
    ImGui::End();
}

void GuiApp::renderFriendsScreen() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(window_width_, window_height_));
    ImGui::Begin("Friends", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    
    if (ImGui::Button("Back", ImVec2(100, 30))) {
        switchScreen(Screen::MAIN_MENU);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::Text("Add Friend:");
    ImGui::InputText("##add_friend", add_friend_buf_, sizeof(add_friend_buf_));
    ImGui::SameLine();
    if (ImGui::Button("Add")) {
        if (strlen(add_friend_buf_) > 0) {
            protocol_handler_->addFriend(add_friend_buf_);
            add_friend_buf_[0] = '\0';
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::Text("Friend Requests:");
    for (const auto& req : friend_requests_) {
        ImGui::Text("%s", req.c_str());
        ImGui::SameLine();
        if (ImGui::Button(("Accept##" + req).c_str())) {
            protocol_handler_->acceptFriend(req);
        }
        ImGui::SameLine();
        if (ImGui::Button(("Decline##" + req).c_str())) {
            protocol_handler_->declineFriend(req);
        }
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::Text("Friends List:");
    if (ImGui::BeginTable("Friends", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Username");
        ImGui::TableSetupColumn("Status");
        ImGui::TableHeadersRow();
        
        for (const auto& friend_info : friends_) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", friend_info.username.c_str());
            ImGui::TableNextColumn();
            ImVec4 color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            if (friend_info.status == "online") {
                color = ImVec4(0, 1, 0, 1);
            } else if (friend_info.status == "ingame") {
                color = ImVec4(1, 1, 0, 1);
            }
            ImGui::TextColored(color, "%s", friend_info.status.c_str());
        }
        
        ImGui::EndTable();
    }
    
    ImGui::End();
}

void GuiApp::renderProfileScreen() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(window_width_, window_height_));
    ImGui::Begin("Profile", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    
    if (ImGui::Button("Back", ImVec2(100, 30))) {
        switchScreen(Screen::MAIN_MENU);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::Text("Username: %s", current_profile_.username.c_str());
    ImGui::Text("Total Games: %d", current_profile_.totalGames);
    
    char highest_prize[64];
    formatMoney(current_profile_.highestPrize, highest_prize, sizeof(highest_prize));
    ImGui::Text("Highest Prize: %s", highest_prize);
    ImGui::Text("Best Questions Answered: %d", current_profile_.finalQuestionNumber);
    ImGui::Text("Best Score: %d", current_profile_.totalScore);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::Text("Game History:");
    if (ImGui::BeginTable("History", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Date");
        ImGui::TableSetupColumn("Questions");
        ImGui::TableSetupColumn("Score");
        ImGui::TableSetupColumn("Prize");
        ImGui::TableSetupColumn("Status");
        ImGui::TableHeadersRow();
        
        for (const auto& game : game_history_) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", game.date.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%d", game.finalQuestionNumber);
            ImGui::TableNextColumn();
            ImGui::Text("%d", game.totalScore);
            ImGui::TableNextColumn();
            char prize_str[64];
            formatMoney(game.finalPrize, prize_str, sizeof(prize_str));
            ImGui::Text("%s", prize_str);
            ImGui::TableNextColumn();
            ImGui::Text("%s", game.status.c_str());
        }
        
        ImGui::EndTable();
    }
    
    ImGui::End();
}

void GuiApp::renderAdminPanel() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(window_width_, window_height_));
    ImGui::Begin("Admin Panel", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    
    if (ImGui::Button("Back", ImVec2(100, 30))) {
        switchScreen(Screen::MAIN_MENU);
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    if (ImGui::BeginTabBar("AdminTabs")) {
        if (ImGui::BeginTabItem("Add Question")) {
            ImGui::Text("Question:");
            ImGui::InputTextMultiline("##question", admin_question_buf_, sizeof(admin_question_buf_), 
                                     ImVec2(600, 100));
            
            ImGui::Text("Options:");
            ImGui::InputText("A:", admin_option_a_, sizeof(admin_option_a_));
            ImGui::InputText("B:", admin_option_b_, sizeof(admin_option_b_));
            ImGui::InputText("C:", admin_option_c_, sizeof(admin_option_c_));
            ImGui::InputText("D:", admin_option_d_, sizeof(admin_option_d_));
            
            ImGui::Text("Correct Answer (0-3):");
            ImGui::InputInt("##correct", &admin_correct_answer_);
            if (admin_correct_answer_ < 0) admin_correct_answer_ = 0;
            if (admin_correct_answer_ > 3) admin_correct_answer_ = 3;
            
            ImGui::Text("Level (1-15):");
            ImGui::InputInt("##level", &admin_question_level_);
            if (admin_question_level_ < 1) admin_question_level_ = 1;
            if (admin_question_level_ > 15) admin_question_level_ = 15;
            
            if (ImGui::Button("Add Question", ImVec2(200, 40))) {
                std::vector<std::pair<std::string, std::string>> options;
                options.push_back({"A", admin_option_a_});
                options.push_back({"B", admin_option_b_});
                options.push_back({"C", admin_option_c_});
                options.push_back({"D", admin_option_d_});
                
                protocol_handler_->addQuestion(admin_question_buf_, options, 
                                              admin_correct_answer_, admin_question_level_);
            }
            
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("View Questions")) {
            ImGui::Text("Level:");
            ImGui::SameLine();
            int level_input = admin_question_level_;
            ImGui::InputInt("##view_level", &level_input);
            if (level_input != admin_question_level_) {
                admin_question_level_ = level_input;
                protocol_handler_->viewQuestions(admin_view_page_, 20, admin_question_level_);
            }
            
            ImGui::SameLine();
            if (ImGui::Button("View All")) {
                protocol_handler_->viewQuestions(admin_view_page_, 20, 0);
            }
            
            ImGui::Spacing();
            
            // Questions list would go here - simplified for now
            if (ImGui::Button("Refresh", ImVec2(150, 30))) {
                protocol_handler_->viewQuestions(admin_view_page_, 20, admin_question_level_);
            }
            
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Ban User")) {
            static char ban_username[256] = "";
            static char ban_reason[512] = "";
            
            ImGui::Text("Username:");
            ImGui::InputText("##ban_user", ban_username, sizeof(ban_username));
            
            ImGui::Text("Reason:");
            ImGui::InputTextMultiline("##ban_reason", ban_reason, sizeof(ban_reason), 
                                     ImVec2(400, 100));
            
            if (ImGui::Button("Ban User", ImVec2(200, 40))) {
                if (strlen(ban_username) > 0) {
                    protocol_handler_->banUser(ban_username, ban_reason);
                    ban_username[0] = '\0';
                    ban_reason[0] = '\0';
                }
            }
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

void GuiApp::showError(const std::string& error) {
    error_message_ = error;
}

void GuiApp::switchScreen(Screen screen) {
    current_screen_ = screen;
    if (screen == Screen::GAME) {
        selected_answer_ = -1;
        disabled_options_.clear();
        phone_suggestion_.clear();
        audience_poll_.clear();
    }
}

void GuiApp::connectToServer() {
    if (!socket_client_) {
        socket_client_ = std::make_unique<SocketClient>();
        protocol_handler_ = std::make_unique<ProtocolHandler>(socket_client_.get());
        protocol_handler_->setResponseCallback([this](int responseCode, const std::string& jsonData) {
            this->handleProtocolResponse(responseCode, jsonData);
        });
    }
    
    if (!socket_client_->isConnected()) {
        connected_ = socket_client_->connect(server_host_, server_port_);
        if (connected_) {
            socket_client_->startListening();
        }
    } else {
        connected_ = true;
    }
}

void GuiApp::handleProtocolResponse(int responseCode, const std::string& jsonData) {
    if (responseCode == 200 || responseCode == 201) {
        // Handle success responses based on current screen/state
        if (current_screen_ == Screen::LOGIN) {
            std::string token = extractString(jsonData, "authToken");
            std::string username = extractString(jsonData, "username");
            std::string role = extractString(jsonData, "role");
            
            if (!token.empty()) {
                protocol_handler_->setAuthToken(token);
                protocol_handler_->setUsername(username);
                protocol_handler_->setRole(role);
                switchScreen(Screen::MAIN_MENU);
            }
        } else if (current_screen_ == Screen::REGISTER) {
            showError("Registration successful! Please login.");
            switchScreen(Screen::LOGIN);
        } else if (current_screen_ == Screen::GAME) {
            // Handle game-related responses
            if (game_state_ == GameState::WAITING_QUESTION) {
                // Check if this is QUESTION_INFO notification
                int questionNumber = extractInt(jsonData, "questionNumber");
                if (questionNumber > 0) {
                    current_question_.questionNumber = questionNumber;
                    current_question_.question = extractString(jsonData, "question");
                    current_question_.gameId = extractInt(jsonData, "gameId");
                    current_question_.prize = extractInt(jsonData, "prize");
                    current_question_.timeRemaining = extractInt(jsonData, "timeRemaining");
                    current_question_.timeLimit = extractInt(jsonData, "timeLimit");
                    current_question_.totalScore = extractInt(jsonData, "totalScore");
                    
                    // Parse options
                    current_question_.options.clear();
                    // Simplified option parsing - in production use proper JSON parser
                    std::regex optionRegex("\"label\":\"([A-D])\",\"text\":\"([^\"]+)\"");
                    std::sregex_iterator iter(jsonData.begin(), jsonData.end(), optionRegex);
                    std::sregex_iterator end;
                    for (; iter != end; ++iter) {
                        current_question_.options.push_back({iter->str(1), iter->str(2)});
                    }
                    
                    // Parse lifelines
                    current_question_.lifelinesAvailable.clear();
                    if (jsonData.find("\"5050\"") != std::string::npos) {
                        current_question_.lifelinesAvailable.push_back("5050");
                    }
                    if (jsonData.find("\"PHONE\"") != std::string::npos) {
                        current_question_.lifelinesAvailable.push_back("PHONE");
                    }
                    if (jsonData.find("\"AUDIENCE\"") != std::string::npos) {
                        current_question_.lifelinesAvailable.push_back("AUDIENCE");
                    }
                    
                    game_state_ = GameState::SHOWING_QUESTION;
                }
            }
            
            // Handle ANSWER response
            bool correct = extractBool(jsonData, "correct");
            if (jsonData.find("\"correct\"") != std::string::npos) {
                last_result_.correct = correct;
                last_result_.pointsEarned = extractInt(jsonData, "pointsEarned");
                last_result_.totalScore = extractInt(jsonData, "totalScore");
                last_result_.currentPrize = extractInt(jsonData, "currentPrize");
                last_result_.gameOver = extractBool(jsonData, "gameOver");
                last_result_.isWinner = extractBool(jsonData, "isWinner");
                last_result_.correctAnswer = extractInt(jsonData, "correctAnswer", -1);
                last_result_.safeCheckpointPrize = extractInt(jsonData, "safeCheckpointPrize");
                
                game_state_ = GameState::SHOWING_RESULT;
            }
            
            // Handle LIFELINE response
            std::string lifelineType = extractString(jsonData, "lifelineType");
            if (!lifelineType.empty()) {
                if (lifelineType == "5050") {
                    lifeline_used_5050_ = true;
                    // Parse remainingOptions
                    disabled_options_.clear();
                    // Simplified parsing
                } else if (lifelineType == "PHONE") {
                    lifeline_used_phone_ = true;
                    phone_suggestion_ = extractString(jsonData, "suggestion");
                } else if (lifelineType == "AUDIENCE") {
                    lifeline_used_audience_ = true;
                    // Parse poll - simplified
                }
            }
        } else if (current_screen_ == Screen::LEADERBOARD) {
            // Parse leaderboard data
            auto& leaderboard = showing_global_leaderboard_ ? global_leaderboard_ : friend_leaderboard_;
            leaderboard.clear();
            // Simplified parsing - in production use proper JSON parser
        } else if (current_screen_ == Screen::FRIENDS) {
            // Parse friends data
            friends_.clear();
            friend_requests_.clear();
            // Simplified parsing
        } else if (current_screen_ == Screen::PROFILE) {
            // Parse profile data
            current_profile_.username = extractString(jsonData, "username");
            current_profile_.totalGames = extractInt(jsonData, "totalGames");
            current_profile_.highestPrize = extractInt(jsonData, "highestPrize");
            current_profile_.finalQuestionNumber = extractInt(jsonData, "finalQuestionNumber");
            current_profile_.totalScore = extractInt(jsonData, "totalScore");
        }
    } else {
        // Handle error responses
        std::string message = extractString(jsonData, "message");
        if (message.empty()) {
            message = "Error code: " + std::to_string(responseCode);
        }
        showError(message);
        
        if (responseCode == 401 || responseCode == 402) {
            // Authentication error - return to login
            switchScreen(Screen::LOGIN);
        }
    }
}

void GuiApp::formatMoney(int amount, char* buffer, size_t bufferSize) {
    if (amount >= 1000000000) {
        double billions = amount / 1000000000.0;
        snprintf(buffer, bufferSize, "%.2fB VND", billions);
    } else if (amount >= 1000000) {
        double millions = amount / 1000000.0;
        snprintf(buffer, bufferSize, "%.2fM VND", millions);
    } else if (amount >= 1000) {
        double thousands = amount / 1000.0;
        snprintf(buffer, bufferSize, "%.2fK VND", thousands);
    } else {
        snprintf(buffer, bufferSize, "%d VND", amount);
    }
}

void GuiApp::glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

