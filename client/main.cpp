#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "socket_client.h"
#include "protocol_handler.h"
#include "json_utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#ifdef _WIN32
    #include <GL/gl.h>
#elif __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Game state
struct GameState {
    bool showLogin = true;
    bool showRegister = false;
    bool loggedIn = false;
    bool inGame = false;
    
    // Login/Register
    char username[256] = "";
    char password[256] = "";
    char registerPassword2[256] = "";
    std::string errorMessage;
    
    // Game
    std::string question;
    std::vector<std::string> options;
    int selectedAnswer = -1;
    int timeRemaining = 30;
    int currentQuestionNumber = 0;
    int totalScore = 0;
    int currentPrize = 0;
    std::vector<bool> availableLifelines = {true, true, true}; // 50/50, Phone, Audience
    
    // Prize ladder
    const int PRIZE_LADDER[15] = {
        1000000, 2000000, 3000000, 5000000, 10000000,
        20000000, 30000000, 50000000, 100000000, 200000000,
        300000000, 500000000, 1000000000, 2000000000, 1000000000
    };
};

void updateTimer(GameState& state, ProtocolHandler& protocol) {
    while (state.inGame && state.timeRemaining > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (state.inGame) {
            state.timeRemaining--;
        }
    }
}

void handleNotifications(SocketClient* client, GameState& state, ProtocolHandler& protocol) {
    SocketClient::Message msg;
    while (true) {
        if (client->getMessage(msg, 100)) {
            if (msg.type == "QUESTION_INFO") {
                state.question = MillionaireGame::JsonUtils::extractString(msg.data, "question");
                state.currentQuestionNumber = MillionaireGame::JsonUtils::extractInt(msg.data, "questionNumber", 0);
                state.timeRemaining = MillionaireGame::JsonUtils::extractInt(msg.data, "timeRemaining", 30);
                state.currentPrize = MillionaireGame::JsonUtils::extractInt(msg.data, "prize", 0);
                state.totalScore = MillionaireGame::JsonUtils::extractInt(msg.data, "totalScore", 0);
                
                // Parse options (simplified)
                state.options.clear();
                for (int i = 0; i < 4; i++) {
                    std::string opt = "Option " + std::string(1, 'A' + i);
                    state.options.push_back(opt);
                }
                
                state.inGame = true;
                state.selectedAnswer = -1;
                
                // Start timer thread
                std::thread(updateTimer, std::ref(state), std::ref(protocol)).detach();
            } else if (msg.type == "GAME_END") {
                state.inGame = false;
            } else if (msg.type == "LIFELINE_INFO") {
                std::string lifelineType = MillionaireGame::JsonUtils::extractString(msg.data, "lifelineType");
                if (lifelineType == "5050") {
                    state.availableLifelines[0] = false;
                } else if (lifelineType == "PHONE") {
                    state.availableLifelines[1] = false;
                } else if (lifelineType == "AUDIENCE") {
                    state.availableLifelines[2] = false;
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    // Parse arguments
    std::string host = "localhost";
    int port = 8080;
    
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = std::stoi(argv[2]);
    }
    
    // Setup GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        return 1;
    }
    
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(1280, 800, "Who Wants to be a Millionaire", nullptr, nullptr);
    if (window == nullptr) {
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Connect to server
    SocketClient client(host, port);
    if (!client.connect()) {
        std::cerr << "Failed to connect to server at " << host << ":" << port << std::endl;
        return 1;
    }
    
    ProtocolHandler protocol(&client);
    GameState state;
    
    // Start notification handler thread
    std::thread notificationThread(handleNotifications, &client, std::ref(state), std::ref(protocol));
    notificationThread.detach();
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Login/Register Window
        if (!state.loggedIn) {
            if (state.showLogin) {
                ImGui::Begin("Login", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
                ImGui::SetWindowPos(ImVec2(400, 300));
                
                ImGui::Text("Who Wants to be a Millionaire");
                ImGui::Separator();
                
                ImGui::InputText("Username", state.username, sizeof(state.username));
                ImGui::InputText("Password", state.password, sizeof(state.password), ImGuiInputTextFlags_Password);
                
                if (!state.errorMessage.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), state.errorMessage.c_str());
                }
                
                if (ImGui::Button("Login", ImVec2(200, 30))) {
                    ProtocolHandler::LoginResponse response = protocol.login(state.username, state.password);
                    if (response.responseCode == 200) {
                        state.loggedIn = true;
                        state.errorMessage.clear();
                    } else {
                        state.errorMessage = "Login failed: " + response.message;
                    }
                }
                
                if (ImGui::Button("Switch to Register", ImVec2(200, 30))) {
                    state.showLogin = false;
                    state.showRegister = true;
                    state.errorMessage.clear();
                }
                
                ImGui::End();
            } else if (state.showRegister) {
                ImGui::Begin("Register", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
                ImGui::SetWindowPos(ImVec2(400, 300));
                
                ImGui::Text("Create New Account");
                ImGui::Separator();
                
                ImGui::InputText("Username", state.username, sizeof(state.username));
                ImGui::InputText("Password", state.password, sizeof(state.password), ImGuiInputTextFlags_Password);
                ImGui::InputText("Confirm Password", state.registerPassword2, sizeof(state.registerPassword2), ImGuiInputTextFlags_Password);
                
                if (!state.errorMessage.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), state.errorMessage.c_str());
                }
                
                if (ImGui::Button("Register", ImVec2(200, 30))) {
                    if (strlen(state.password) < 8) {
                        state.errorMessage = "Password must be at least 8 characters";
                    } else if (strcmp(state.password, state.registerPassword2) != 0) {
                        state.errorMessage = "Passwords do not match";
                    } else {
                        int code = protocol.registerUser(state.username, state.password);
                        if (code == 201) {
                            state.errorMessage = "Registration successful! Please login.";
                            state.showRegister = false;
                            state.showLogin = true;
                        } else {
                            state.errorMessage = "Registration failed";
                        }
                    }
                }
                
                if (ImGui::Button("Switch to Login", ImVec2(200, 30))) {
                    state.showRegister = false;
                    state.showLogin = true;
                    state.errorMessage.clear();
                }
                
                ImGui::End();
            }
        } else {
            // Main game window
            ImGui::Begin("Who Wants to be a Millionaire", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::SetWindowPos(ImVec2(0, 0));
            ImGui::SetWindowSize(ImVec2(1280, 800));
            
            // Left panel - Prize ladder
            ImGui::BeginChild("PrizeLadder", ImVec2(200, 700), true);
            ImGui::Text("Prize Ladder");
            ImGui::Separator();
            for (int i = 14; i >= 0; i--) {
                bool isCheckpoint = (i == 4 || i == 9 || i == 14);
                if (isCheckpoint) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                }
                if (i + 1 == state.currentQuestionNumber) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                }
                ImGui::Text("Q%d: %d VND", i + 1, state.PRIZE_LADDER[i]);
                if (i + 1 == state.currentQuestionNumber || isCheckpoint) {
                    ImGui::PopStyleColor();
                }
            }
            ImGui::EndChild();
            
            ImGui::SameLine();
            
            // Center panel - Question
            ImGui::BeginChild("QuestionPanel", ImVec2(800, 700), true);
            
            if (!state.inGame) {
                ImGui::Text("Welcome, %s!", state.username);
                ImGui::Separator();
                
                if (ImGui::Button("Start New Game", ImVec2(200, 50))) {
                    int code = protocol.startGame(false);
                    if (code == 412) {
                        state.errorMessage = "You have a saved game. Use Resume or override.";
                    } else if (code != 200) {
                        state.errorMessage = "Failed to start game";
                    }
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Resume Game", ImVec2(200, 50))) {
                    int code = protocol.resumeGame();
                    if (code == 404) {
                        state.errorMessage = "No saved game found";
                    } else if (code != 200) {
                        state.errorMessage = "Failed to resume game";
                    }
                }
                
                if (!state.errorMessage.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), state.errorMessage.c_str());
                }
            } else {
                // Game in progress
                ImGui::Text("Question %d", state.currentQuestionNumber);
                ImGui::Text("Time: %d seconds", state.timeRemaining);
                ImGui::Text("Prize: %d VND", state.currentPrize);
                ImGui::Text("Score: %d", state.totalScore);
                ImGui::Separator();
                
                ImGui::TextWrapped("%s", state.question.c_str());
                ImGui::Separator();
                
                // Answer buttons
                for (size_t i = 0; i < state.options.size(); i++) {
                    char label[256];
                    snprintf(label, sizeof(label), "%c. %s", 'A' + i, state.options[i].c_str());
                    
                    if (state.selectedAnswer == static_cast<int>(i)) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
                    }
                    
                    if (ImGui::Button(label, ImVec2(700, 50))) {
                        state.selectedAnswer = i;
                    }
                    
                    if (state.selectedAnswer == static_cast<int>(i)) {
                        ImGui::PopStyleColor();
                    }
                }
                
                if (ImGui::Button("Submit Answer", ImVec2(200, 50))) {
                    if (state.selectedAnswer >= 0) {
                        ProtocolHandler::AnswerResponse response = protocol.answerQuestion(state.selectedAnswer);
                        if (response.responseCode == 200) {
                            if (response.gameOver) {
                                if (response.isWinner) {
                                    state.errorMessage = "Congratulations! You won!";
                                } else {
                                    state.errorMessage = "Game Over! Final Prize: " + std::to_string(response.finalPrize) + " VND";
                                }
                                state.inGame = false;
                            } else {
                                state.errorMessage = response.correct ? "Correct!" : "Wrong answer!";
                            }
                        }
                    }
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Give Up", ImVec2(150, 50))) {
                    protocol.giveUp();
                    state.inGame = false;
                }
                
                // Lifelines
                ImGui::Separator();
                ImGui::Text("Lifelines:");
                
                if (state.availableLifelines[0] && ImGui::Button("50/50", ImVec2(150, 30))) {
                    protocol.useLifeline("5050");
                }
                ImGui::SameLine();
                if (state.availableLifelines[1] && ImGui::Button("Phone a Friend", ImVec2(150, 30))) {
                    protocol.useLifeline("PHONE");
                }
                ImGui::SameLine();
                if (state.availableLifelines[2] && ImGui::Button("Ask Audience", ImVec2(150, 30))) {
                    protocol.useLifeline("AUDIENCE");
                }
                
                if (!state.errorMessage.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), state.errorMessage.c_str());
                }
            }
            
            ImGui::EndChild();
            
            ImGui::End();
        }
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
    
    // Cleanup
    protocol.logout();
    client.disconnect();
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}

