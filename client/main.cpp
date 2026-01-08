#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "socket_client.h"
#include "protocol_handler.h"
#include "json_utils.h"
#include "game_event.h"
#include "texture_loader.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <map>
#include <sstream>

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
    bool waitingForQuestion = false;
    bool showResultMessage = false;
    bool showResultScreen = false;
    
    // Login/Register
    char username[256] = "";
    char password[256] = "";
    char registerPassword2[256] = "";
    std::string errorMessage;
    std::string resultMessage;
    float resultMessageTime = 0.0f;

    // Home screen
    bool onHome = false;

    // Image textures (optional)
    GLuint texLogo = 0; int texLogoW = 0, texLogoH = 0; bool logoLoaded = false;
    GLuint tex5050 = 0; int tex5050W = 0, tex5050H = 0; bool t5050Loaded = false;
    GLuint texPhone = 0; int texPhoneW = 0, texPhoneH = 0; bool tPhoneLoaded = false;
    GLuint texAudience = 0; int texAudienceW = 0, texAudienceH = 0; bool tAudienceLoaded = false;
    
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
    const int PRIZE_LADDER[15] = {
        1000000, 2000000, 3000000, 5000000, 10000000,
        20000000, 30000000, 50000000, 100000000, 200000000,
        300000000, 500000000, 1000000000, 2000000000, 1000000000
    };
};

// Parse options array from QUESTION_INFO JSON
std::vector<std::string> parseOptionsArray(const std::string& json) {
    std::vector<std::string> result;
    
    // Find "options" array in JSON
    size_t optionsStart = json.find("\"options\"");
    if (optionsStart == std::string::npos) {
        return result;
    }
    
    size_t arrayStart = json.find("[", optionsStart);
    if (arrayStart == std::string::npos) {
        return result;
    }
    
    size_t arrayEnd = json.find("]", arrayStart);
    if (arrayEnd == std::string::npos) {
        return result;
    }
    
    // Extract each option's "text" field
    std::string optionsArray = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    
    // Parse each option object
    size_t pos = 0;
    while (pos < optionsArray.length()) {
        size_t textStart = optionsArray.find("\"text\"", pos);
        if (textStart == std::string::npos) break;
        
        size_t colonPos = optionsArray.find(":", textStart);
        if (colonPos == std::string::npos) break;
        
        size_t quoteStart = optionsArray.find("\"", colonPos);
        if (quoteStart == std::string::npos) break;
        
        size_t quoteEnd = optionsArray.find("\"", quoteStart + 1);
        if (quoteEnd == std::string::npos) break;
        
        std::string text = optionsArray.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        result.push_back(text);
        
        pos = quoteEnd + 1;
    }
    
    return result;
}

void updateTimer(GameState& state, ProtocolHandler* protocol, int myTimerId) {
    std::cerr << "[DEBUG] Timer thread #" << myTimerId << " started" << std::endl;
    
    while (state.timerRunning && state.timeRemaining > 0 && state.timerThreadId == myTimerId) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Only decrement if this is still the active timer and not paused
        if (state.timerRunning && state.inGame && state.timerThreadId == myTimerId && !state.timerPaused) {
            state.timeRemaining--;
            std::cerr << "[DEBUG] Timer #" << myTimerId << ": " << state.timeRemaining << "s remaining" << std::endl;
        }
    }
    
    // Timer expired - handle timeout (only if this is still the active timer)
    if (state.timeRemaining == 0 && state.inGame && state.timerRunning && state.timerThreadId == myTimerId) {
        std::cerr << "[DEBUG] Timer #" << myTimerId << " expired!" << std::endl;
        state.resultMessage = "Time's up! Game Over!";
        state.showResultMessage = true;
        state.resultMessageTime = 3.0f;
        state.inGame = false;
        state.timerRunning = false;
    }
    
    std::cerr << "[DEBUG] Timer thread #" << myTimerId << " stopped" << std::endl;
}

void handleNotifications(SocketClient* client, GameState& state, ProtocolHandler* protocol, GameEventQueue* eventQueue) {
    if (!client || !protocol || !eventQueue) return; // Demo mode or invalid
    
    std::cerr << "[DEBUG] Notification handler thread started" << std::endl;
    
    SocketClient::Message msg;
    while (true) {
        if (client->getMessage(msg, 100)) {
            std::cerr << "[DEBUG] Received notification: type=" << msg.type << ", data=" << msg.data.substr(0, 150) << std::endl;
            
            // Skip RESPONSE messages that have responseCode - they're request responses handled by ProtocolHandler
            // Only process actual notification types (QUESTION_INFO, GAME_END, LIFELINE_INFO, etc.)
            if (msg.type == "RESPONSE" && msg.data.find("\"responseCode\"") != std::string::npos) {
                // This is a request response (LOGIN, REGISTER, etc.), put it back for ProtocolHandler
                client->putMessageBack(msg);
                continue;
            }
            
            // Convert message types to events and push to queue
            if (msg.type == "GAME_START") {
                std::cerr << "[DEBUG] Pushing GAME_START event to queue" << std::endl;
                eventQueue->push(GameEvent(EVENT_GAME_START, msg.data));
            } else if (msg.type == "QUESTION_INFO") {
                std::cerr << "[DEBUG] Pushing QUESTION_INFO event to queue" << std::endl;
                eventQueue->push(GameEvent(EVENT_QUESTION_INFO, msg.data));
            } else if (msg.type == "GAME_END") {
                std::cerr << "[DEBUG] Pushing GAME_END event to queue" << std::endl;
                eventQueue->push(GameEvent(EVENT_GAME_END, msg.data));
            } else if (msg.type == "LIFELINE_INFO") {
                std::cerr << "[DEBUG] Pushing LIFELINE_INFO event to queue" << std::endl;
                eventQueue->push(GameEvent(EVENT_LIFELINE_INFO, msg.data));
            }
        }
    }
}

// Process events from the queue (called in main UI thread)
void processGameEvents(GameEventQueue* eventQueue, GameState& state, ProtocolHandler* protocol) {
    GameEvent event;
    while (eventQueue->pop(event)) {
        std::cerr << "[DEBUG] Processing event type: " << event.type << std::endl;
        
        switch (event.type) {
            case EVENT_GAME_START: {
                std::cerr << "[DEBUG] Processing GAME_START event" << std::endl;
                
                // Reset all game state
                state.inGame = true;
                state.onHome = false;
                state.waitingForQuestion = true;
                state.availableLifelines = {true, true, true};
                state.totalScore = 0;
                state.currentQuestionNumber = 0;
                state.selectedAnswer = -1;
                state.timeRemaining = 30;
                state.question.clear();
                state.options.clear();
                state.errorMessage.clear();
                state.timerRunning = false;
                state.timerStartedForThisQuestion = false;
                state.revealActive = false;
                break;
            }
            
            case EVENT_QUESTION_INFO: {
                std::cerr << "[DEBUG] Processing QUESTION_INFO event" << std::endl;
                
                // Extract all question data
                std::string newQuestion = MillionaireGame::JsonUtils::extractString(event.data, "question");
                int newQuestionNumber = MillionaireGame::JsonUtils::extractInt(event.data, "questionNumber", 0);
                int newTimeRemaining = MillionaireGame::JsonUtils::extractInt(event.data, "timeRemaining", 30);
                int newPrize = MillionaireGame::JsonUtils::extractInt(event.data, "prize", 0);
                int newTotalScore = MillionaireGame::JsonUtils::extractInt(event.data, "totalScore", 0);
                
                // Parse options array
                std::vector<std::string> newOptions = parseOptionsArray(event.data);
                
                // If parsing failed, use placeholder
                if (newOptions.size() != 4) {
                    std::cerr << "[WARN] Failed to parse options, using placeholders" << std::endl;
                    newOptions.clear();
                    newOptions.push_back("Option A");
                    newOptions.push_back("Option B");
                    newOptions.push_back("Option C");
                    newOptions.push_back("Option D");
                }
                
                std::cerr << "[DEBUG] Question loaded: " << newQuestion << std::endl;
                std::cerr << "[DEBUG] Question empty? " << (newQuestion.empty() ? "YES" : "NO") << std::endl;
                std::cerr << "[DEBUG] Options count: " << newOptions.size() << std::endl;
                if (newOptions.size() >= 4) {
                    std::cerr << "[DEBUG] Options: " << newOptions[0] << ", " << newOptions[1] 
                              << ", " << newOptions[2] << ", " << newOptions[3] << std::endl;
                }
                
                // Stop any existing timer first by incrementing the timer ID
                state.timerRunning = false;
                state.timerThreadId++;  // Invalidate old timer threads
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give old timer time to check and exit
                
                // Reset lifeline state for new question
                state.lifeline5050Remaining.clear();
                state.lifelinePhoneSuggestion.clear();
                state.lifelineAudiencePoll.clear();
                state.lifelineType.clear();
                state.lifelineProcessing = false;
                state.lifelineLoadingMessage.clear();
                
                // Now update all state
                state.question = newQuestion;
                state.currentQuestionNumber = newQuestionNumber;
                state.timeRemaining = 30;  // Always reset to 30s for new question
                state.currentPrize = newPrize;
                state.totalScore = newTotalScore;
                state.options = newOptions;
                state.selectedAnswer = -1;
                state.inGame = true;
                state.waitingForQuestion = false;  // CRITICAL: Set to false so question is displayed
                state.revealActive = true;
                state.answersRevealed = 0;
                state.timerStartedForThisQuestion = false;
                state.timerPaused = false;
                state.pausedTimeRemaining = 0;
                state.revealStart = std::chrono::steady_clock::now();
                
                // CRITICAL: Update protocol handler's question number so answerQuestion sends correct value
                if (protocol) {
                    protocol->currentQuestionNumber = newQuestionNumber;
                }
                
                std::cerr << "[DEBUG] State updated: inGame=" << state.inGame 
                          << ", waitingForQuestion=" << state.waitingForQuestion 
                          << ", questionNumber=" << state.currentQuestionNumber << std::endl;
                
                // Do not start timer yet. We will start after reveal is completed (all options shown).
                break;
            }
            
            case EVENT_GAME_END: {
                std::cerr << "[DEBUG] Processing GAME_END event" << std::endl;
                std::cerr << "[DEBUG] GAME_END data: " << event.data << std::endl;
                
                std::string status = MillionaireGame::JsonUtils::extractString(event.data, "status");
                long long finalPrize = MillionaireGame::JsonUtils::extractInt(event.data, "finalPrize", 0);
                int totalScore = MillionaireGame::JsonUtils::extractInt(event.data, "totalScore", 0);
                bool isWinner = MillionaireGame::JsonUtils::extractBool(event.data, "isWinner", false);
                
                std::cerr << "[DEBUG] Game ended: status=" << status << ", finalPrize=" << finalPrize 
                          << ", totalScore=" << totalScore << ", isWinner=" << isWinner << std::endl;
                
                // CRITICAL: Clear all game state to prepare for next game
                state.timerRunning = false;
                state.timerStartedForThisQuestion = false;
                state.timerPaused = false;
                state.inGame = false;
                state.waitingForQuestion = false;
                state.onHome = false;
                state.question.clear();
                state.options.clear();
                state.currentQuestionNumber = 0;
                state.selectedAnswer = -1;
                state.lifeline5050Remaining.clear();
                state.lifelinePhoneSuggestion.clear();
                state.lifelineAudiencePoll.clear();
                state.lifelineType.clear();
                state.lifelineProcessing = false;
                state.lifelineLoadingMessage.clear();
                state.availableLifelines = {true, true, true};
                
                if (isWinner) {
                    state.resultMessage = "🎉 Congratulations! You WON! Prize: " + std::to_string(finalPrize) + " VND";
                } else if (status == "lost") {
                    state.resultMessage = "Game Over! Final Prize: " + std::to_string(finalPrize) + " VND";
                } else if (status == "quit") {
                    state.resultMessage = "You gave up. Prize taken: " + std::to_string(finalPrize) + " VND";
                }
                state.finalPrize = finalPrize;
                state.totalScore = totalScore;  // Store total score for display
                state.showResultMessage = true;
                state.resultMessageTime = 5.0f;
                state.showResultScreen = true;
                break;
            }
            
            case EVENT_LIFELINE_INFO: {
                std::cerr << "[DEBUG] Processing LIFELINE_INFO event" << std::endl;
                std::cerr << "[DEBUG] LIFELINE_INFO data: " << event.data << std::endl;
                std::string lifelineType = MillionaireGame::JsonUtils::extractString(event.data, "lifelineType");
                int timeRemaining = MillionaireGame::JsonUtils::extractInt(event.data, "timeRemaining", state.timeRemaining);
                
                // Mark lifeline as used
                if (lifelineType == "5050") {
                    state.availableLifelines[0] = false;
                    // Parse remainingOptions array - extract JSON array directly
                    state.lifeline5050Remaining.clear();
                    std::string searchKey = "\"remainingOptions\"";
                    size_t pos = event.data.find(searchKey);
                    if (pos != std::string::npos) {
                        pos = event.data.find('[', pos);
                        if (pos != std::string::npos) {
                            size_t end = event.data.find(']', pos);
                            if (end != std::string::npos) {
                                std::string arrayStr = event.data.substr(pos + 1, end - pos - 1);
                                std::stringstream ss(arrayStr);
                                std::string token;
                                while (std::getline(ss, token, ',')) {
                                    // Remove whitespace
                                    token.erase(0, token.find_first_not_of(" \t"));
                                    token.erase(token.find_last_not_of(" \t") + 1);
                                    if (!token.empty()) {
                                        int idx = std::stoi(token);
                                        state.lifeline5050Remaining.push_back(idx);
                                    }
                                }
                            }
                        }
                    }
                } else if (lifelineType == "PHONE") {
                    state.availableLifelines[1] = false;
                    // Parse suggestion string
                    state.lifelinePhoneSuggestion = MillionaireGame::JsonUtils::extractString(event.data, "suggestion");
                } else if (lifelineType == "AUDIENCE") {
                    state.availableLifelines[2] = false;
                    // Parse poll object - extract JSON object directly
                    state.lifelineAudiencePoll.clear();
                    std::string searchKey = "\"poll\"";
                    size_t pos = event.data.find(searchKey);
                    if (pos != std::string::npos) {
                        pos = event.data.find('{', pos);
                        if (pos != std::string::npos) {
                            // Find matching closing brace
                            int braceCount = 0;
                            size_t end = pos;
                            for (size_t i = pos; i < event.data.length(); i++) {
                                if (event.data[i] == '{') braceCount++;
                                if (event.data[i] == '}') {
                                    braceCount--;
                                    if (braceCount == 0) {
                                        end = i;
                                        break;
                                    }
                                }
                            }
                            if (end > pos) {
                                std::string pollStr = event.data.substr(pos + 1, end - pos - 1);
                                std::stringstream ss(pollStr);
                                std::string token;
                                while (std::getline(ss, token, ',')) {
                                    // Token format: "A":65
                                    size_t colonPos = token.find(':');
                                    if (colonPos != std::string::npos) {
                                        // Extract key (remove quotes)
                                        std::string key = token.substr(0, colonPos);
                                        key.erase(0, key.find_first_not_of(" \t\""));
                                        key.erase(key.find_last_not_of(" \t\"") + 1);
                                        // Extract value
                                        std::string value = token.substr(colonPos + 1);
                                        value.erase(0, value.find_first_not_of(" \t"));
                                        value.erase(value.find_last_not_of(" \t") + 1);
                                        if (key.length() == 1 && key[0] >= 'A' && key[0] <= 'D') {
                                            state.lifelineAudiencePoll[key[0]] = std::stoi(value);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                // Stop loading and show result
                state.lifelineProcessing = false;
                state.lifelineLoadingMessage.clear();
                
                // Resume timer with remaining time
                if (state.timerPaused) {
                    state.timeRemaining = timeRemaining;
                    state.timerPaused = false;
                    // Always restart timer after lifeline (timer should be running during question)
                    state.timerRunning = true;
                    state.timerThreadId++;
                    int newTimerId = state.timerThreadId;
                    std::thread(updateTimer, std::ref(state), protocol, newTimerId).detach();
                    std::cerr << "[DEBUG] Timer resumed after lifeline: " << timeRemaining << "s remaining" << std::endl;
                } else {
                    // Timer was not paused, but we should still update timeRemaining and ensure timer is running
                    state.timeRemaining = timeRemaining;
                    if (!state.timerRunning) {
                        state.timerRunning = true;
                        state.timerThreadId++;
                        int newTimerId = state.timerThreadId;
                        std::thread(updateTimer, std::ref(state), protocol, newTimerId).detach();
                    }
                }
                break;
            }
            
            default:
                break;
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
    
    // OpenGL version - macOS requires 3.2+
#ifdef __APPLE__
    // macOS requires OpenGL 3.2+ and uses core profile
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); // Required on macOS
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Required on macOS
#else
    // Linux/Windows - use 3.0
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    
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
    
    // Check for demo mode
    bool demoMode = false;
    if (argc > 1 && std::string(argv[1]) == "--demo") {
        demoMode = true;
        std::cout << "Running in DEMO MODE (no server connection)" << std::endl;
    }
    
    // Connect to server (skip in demo mode)
    SocketClient* client = nullptr;
    ProtocolHandler* protocol = nullptr;
    
    if (!demoMode) {
        client = new SocketClient(host, port);
        if (!client->connect()) {
            std::cerr << "Failed to connect to server at " << host << ":" << port << std::endl;
            std::cerr << "Tip: Make sure server is running, or use --demo flag for demo mode" << std::endl;
            return 1;
        }
        protocol = new ProtocolHandler(client);
    }
    
    GameState state;
    
    // Create event queue for thread-safe communication
    GameEventQueue eventQueue;
    
    // Start notification handler thread (skip in demo mode)
    std::thread* notificationThread = nullptr;
    if (!demoMode && client && protocol) {
        notificationThread = new std::thread(handleNotifications, client, std::ref(state), protocol, &eventQueue);
        notificationThread->detach();
    }
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Process any game events from notification thread (in UI thread - SAFE!)
        if (!demoMode && protocol) {
            processGameEvents(&eventQueue, state, protocol);
        }
        
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
                    if (demoMode) {
                        // Demo mode - fake login
                        state.loggedIn = true;
                        state.errorMessage.clear();
                        state.username[0] = '\0'; // Clear for demo
                        state.onHome = true;
                    } else if (protocol) {
                        ProtocolHandler::LoginResponse response = protocol->login(state.username, state.password);
                        if (response.responseCode == 200) {
                            state.loggedIn = true;
                            state.errorMessage.clear();
                            state.onHome = true;
                        } else {
                            state.errorMessage = "Login failed: " + response.message;
                        }
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
                        if (demoMode) {
                            // Demo mode - fake registration
                            state.errorMessage = "Registration successful! Please login.";
                            state.showRegister = false;
                            state.showLogin = true;
                            state.onHome = false;
                        } else if (protocol) {
                            int code = protocol->registerUser(state.username, state.password);
                            if (code == 201) {
                                state.errorMessage = "Registration successful! Please login.";
                                state.showRegister = false;
                                state.showLogin = true;
                                state.onHome = false;
                            } else {
                                state.errorMessage = "Registration failed";
                            }
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
            // Load textures once (if present)
            static bool texturesChecked = false;
            if (!texturesChecked) {
                texturesChecked = true;
                state.logoLoaded = LoadTextureFromAny({
                    "client/assets/millionaire_logo.png",
                    "assets/millionaire_logo.png",
                    "millionaire_logo.png"
                }, &state.texLogo, &state.texLogoW, &state.texLogoH);

                state.t5050Loaded = LoadTextureFromAny({
                    "client/assets/lifeline_5050.png",
                    "assets/lifeline_5050.png"
                }, &state.tex5050, &state.tex5050W, &state.tex5050H);
                state.tPhoneLoaded = LoadTextureFromAny({
                    "client/assets/lifeline_phone.png",
                    "assets/lifeline_phone.png"
                }, &state.texPhone, &state.texPhoneW, &state.texPhoneH);
                state.tAudienceLoaded = LoadTextureFromAny({
                    "client/assets/lifeline_audience.png",
                    "assets/lifeline_audience.png"
                }, &state.texAudience, &state.texAudienceW, &state.texAudienceH);
            }

            // Main window full screen
            ImGui::Begin("Who Wants to be a Millionaire", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::SetWindowPos(ImVec2(0, 0));
            ImGui::SetWindowSize(ImVec2(1280, 800));
            
            // If result screen requested
            if (state.showResultScreen) {
                ImGui::SetCursorPos(ImVec2(0, 0));
                ImGui::BeginChild("ResultScreen", ImVec2(1280, 800), false);
                ImGui::SetCursorPos(ImVec2(200, 120));
                ImGui::SetWindowFontScale(1.8f);
                ImGui::Text("Game Result");
                ImGui::SetWindowFontScale(1.0f);

                ImGui::SetCursorPos(ImVec2(200, 180));
                ImGui::Separator();

                ImGui::SetCursorPos(ImVec2(200, 240));
                ImGui::SetWindowFontScale(1.6f);
                ImGui::Text("Total Winnings:");
                ImGui::SetWindowFontScale(2.0f);
                ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%lld VND", state.finalPrize);
                ImGui::SetWindowFontScale(1.0f);
                
                ImGui::SetCursorPos(ImVec2(200, 300));
                ImGui::SetWindowFontScale(1.6f);
                ImGui::Text("Total Score:");
                ImGui::SetWindowFontScale(2.0f);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%d points", state.totalScore);
                ImGui::SetWindowFontScale(1.0f);

                ImGui::SetCursorPos(ImVec2(200, 380));
                if (ImGui::Button("Back to Home", ImVec2(250, 50))) {
                    state.showResultScreen = false;
                    state.onHome = true;
                    state.inGame = false;
                }
                ImGui::EndChild();
                ImGui::End();
                goto frame_end; // render result screen only
            }

            if (state.onHome && !state.inGame) {
                // Homepage layout: Logo left, menu right
                ImGui::BeginChild("HomeLeft", ImVec2(700, 800), false);
                ImGui::SetCursorPos(ImVec2(120, 120));
                if (state.logoLoaded) {
                    ImGui::Image(ImTextureRef((ImTextureID)(intptr_t)state.texLogo), ImVec2((float)state.texLogoW, (float)state.texLogoH));
                } else {
                    ImGui::SetWindowFontScale(2.5f);
                    ImGui::Text("MILLIONAIRE");
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::TextDisabled("(Place millionaire_logo.png in client/assets/)");
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("HomeRight", ImVec2(580, 800), false);
                ImGui::SetCursorPos(ImVec2(150, 180));
                if (ImGui::Button("Play Game", ImVec2(280, 48))) {
                    if (demoMode) {
                        state.inGame = false; // start via button below to trigger protocol flow
                    }
                    if (demoMode) {
                        // Start local demo game
                        state.inGame = true;
                        state.onHome = false;
                        state.currentQuestionNumber = 1;
                        state.question = "Demo Question: What is the capital of Vietnam?";
                        state.options = {"Hanoi", "Ho Chi Minh City", "Da Nang", "Hue"};
                        state.timeRemaining = 30;
                        state.currentPrize = 1000000;
                        state.waitingForQuestion = false;
                        state.revealActive = true;
                        state.answersRevealed = 0;
                        state.revealStart = std::chrono::steady_clock::now();
                    } else if (protocol) {
                        int code = protocol->startGame(false);
                        if (code == 200) {
                            state.inGame = true;
                            state.onHome = false;
                            state.waitingForQuestion = true;
                            state.errorMessage.clear();
                        } else {
                            state.errorMessage = "Failed to start game (code " + std::to_string(code) + ")";
                        }
                    }
                }
                ImGui::SetCursorPos(ImVec2(150, 240));
                ImGui::Button("Leaderboard", ImVec2(280, 48));
                ImGui::SetCursorPos(ImVec2(150, 300));
                ImGui::Button("Settings", ImVec2(280, 48));
                ImGui::SetCursorPos(ImVec2(150, 360));
                ImGui::Button("Instructions", ImVec2(280, 48));
                ImGui::SetCursorPos(ImVec2(150, 420));
                ImGui::Button("Friends", ImVec2(280, 48));
                ImGui::EndChild();

                ImGui::End();
                goto frame_end;
            }
            
            // Left panel - Prize ladder
            ImGui::BeginChild("PrizeLadder", ImVec2(200, 700), true);
            ImGui::Text("Prize Ladder");
            ImGui::Separator();
            for (int i = 14; i >= 0; i--) {
                bool isCheckpoint = (i == 4 || i == 9 || i == 14);
                bool isCurrent = (i + 1 == state.currentQuestionNumber);
                int pushCount = 0;
                
                // Current question takes priority over checkpoint color
                if (isCurrent) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.0f, 1.0f)); // Orange
                    pushCount++;
                } else if (isCheckpoint) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow
                    pushCount++;
                }
                
                ImGui::Text("Q%d: %d VND", i + 1, state.PRIZE_LADDER[i]);
                
                // Pop exactly what we pushed
                if (pushCount > 0) {
                    ImGui::PopStyleColor(pushCount);
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
                    if (demoMode) {
                        // Demo mode - fake game start
                        state.inGame = true;
                        state.onHome = false;
                        state.currentQuestionNumber = 1;
                        state.question = "Demo Question: What is the capital of Vietnam?";
                        state.options = {"Hanoi", "Ho Chi Minh City", "Da Nang", "Hue"};
                        state.timeRemaining = 30;
                        state.currentPrize = 1000000;
                        state.errorMessage.clear();
                        state.revealActive = true;
                        state.answersRevealed = 0;
                        state.revealStart = std::chrono::steady_clock::now();
                    } else if (protocol) {
                        std::cerr << "[DEBUG] Starting new game..." << std::endl;
                        int code = protocol->startGame(false);
                        std::cerr << "[DEBUG] Start game response code: " << code << std::endl;
                        
                        if (code == 200) {
                            // Game started - switch to game screen immediately
                            state.errorMessage.clear();
                            state.inGame = true;
                            state.onHome = false;
                            state.waitingForQuestion = true;
                            // Notification handler will update state when GAME_START/QUESTION_INFO arrive
                        } else if (code == 412) {
                            state.errorMessage = "You have a saved game. Use Resume or override.";
                        } else {
                            state.errorMessage = "Failed to start game (code " + std::to_string(code) + ")";
                        }
                    }
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Resume Game", ImVec2(200, 50))) {
                    if (demoMode) {
                        state.errorMessage = "No saved game in demo mode";
                    } else if (protocol) {
                        int code = protocol->resumeGame();
                        if (code == 404) {
                            state.errorMessage = "No saved game found";
                        } else if (code != 200) {
                            state.errorMessage = "Failed to resume game";
                        }
                    }
                }
                
                if (!state.errorMessage.empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), state.errorMessage.c_str());
                }
            } else {
                // Game in progress
                // Display question if we have question data, otherwise show waiting message
                if (!state.question.empty() && state.currentQuestionNumber > 0) {
                    // We have question data, display it
                    ImGui::Text("Question %d of 15", state.currentQuestionNumber);

                    // Walk Away (top-left of question panel)
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(10.0f);
                    if (ImGui::Button("WALK AWAY", ImVec2(120, 30))) {
                        if (demoMode) {
                            int prevIndex = std::max(0, state.currentQuestionNumber - 2);
                            state.finalPrize = prevIndex >= 0 ? state.PRIZE_LADDER[prevIndex] : 0;
                            state.inGame = false;
                            state.showResultScreen = true;
                            state.resultMessage = "You walked away!";
                            state.showResultMessage = true;
                            state.resultMessageTime = 3.0f;
                        } else if (protocol) {
                            int code = protocol->giveUp();
                            if (code != 200) {
                                state.errorMessage = "Error giving up (code " + std::to_string(code) + ")";
                            }
                        }
                    }
                    
                    // Timer with color coding
                    if (state.timeRemaining <= 10) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red
                    } else if (state.timeRemaining <= 20) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.0f, 1.0f)); // Orange
                    }

                    // Draw circular timer
                    {
                        ImGui::SameLine();
                        ImVec2 center = ImGui::GetCursorScreenPos();
                        center.x += 40; center.y += 28;
                        float radius = 24.0f;
                        ImDrawList* draw_list = ImGui::GetWindowDrawList();
                        float pct = state.timeRemaining / 30.0f;
                        constexpr float kPi = 3.14159265358979323846f;
                        draw_list->AddCircleFilled(center, radius, IM_COL32(30, 30, 30, 255), 64);
                        draw_list->AddCircle(center, radius, IM_COL32(80, 80, 80, 255), 64, 2.0f);
                        // Arc progress
                        int segments = 48;
                        for (int i = 0; i < segments; ++i) {
                            float a0 = (-kPi/2) + (i / (float)segments) * 2*kPi;
                            float a1 = (-kPi/2) + ((i+1) / (float)segments) * 2*kPi;
                            if ((i+1) / (float)segments > pct) break;
                            draw_list->AddTriangleFilled(
                                center,
                                ImVec2(center.x + (float)std::cos(a0)*radius, center.y + (float)std::sin(a0)*radius),
                                ImVec2(center.x + (float)std::cos(a1)*radius, center.y + (float)std::sin(a1)*radius),
                                IM_COL32(255, 165, 0, 200));
                        }
                        // Number
                        char tbuf[8]; snprintf(tbuf, sizeof(tbuf), "%d", state.timeRemaining);
                        ImVec2 ts = ImGui::CalcTextSize(tbuf);
                        draw_list->AddText(ImVec2(center.x - ts.x*0.5f, center.y - ts.y*0.5f), IM_COL32(255,255,255,255), tbuf);
                        ImGui::Dummy(ImVec2(80, 56));
                    }

                    // Start timer after reveal completes
                    if (state.revealActive) {
                        float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - state.revealStart).count();
                        int answersToShow = 0;
                        if (elapsed >= 1.0f) {
                            answersToShow = (int)((elapsed - 1.0f) / 0.5f) + 1;
                            if (answersToShow > 4) answersToShow = 4;
                        }
                        if (answersToShow != state.answersRevealed) {
                            state.answersRevealed = answersToShow;
                        }
                        if (state.answersRevealed >= 4 && !state.timerStartedForThisQuestion) {
                            state.timerRunning = true;
                            state.timerThreadId++;
                            state.timerStartedForThisQuestion = true;
                            int newTimerId = state.timerThreadId;
                            std::thread(updateTimer, std::ref(state), protocol, newTimerId).detach();
                        }
                        if (state.answersRevealed >= 4) state.revealActive = false;
                    }

                    if (state.timeRemaining <= 20) {
                        ImGui::PopStyleColor();
                    }
                    
                    ImGui::Text("Prize: %d VND", state.currentPrize);
                    ImGui::Text("Score: %d points", state.totalScore);
                    ImGui::Separator();
                    
                    // Display question
                    if (!state.question.empty()) {
                        ImGui::TextWrapped("%s", state.question.c_str());
                    }
                    ImGui::Separator();
                    
                    // Answer buttons with reveal
                    size_t maxToShow = state.revealActive ? (size_t)state.answersRevealed : state.options.size();
                    if (maxToShow > state.options.size()) maxToShow = state.options.size();
                    for (size_t i = 0; i < maxToShow; i++) {
                        char label[512];
                        snprintf(label, sizeof(label), "%c. %s", 'A' + (int)i, state.options[i].c_str());
                        
                        // Check if this option should be disabled (5050 lifeline eliminated it)
                        bool isDisabled = false;
                        if (!state.lifeline5050Remaining.empty() && state.lifelineType == "5050") {
                            // Disable if not in remainingOptions
                            bool found = false;
                            for (int idx : state.lifeline5050Remaining) {
                                if (idx == static_cast<int>(i)) {
                                    found = true;
                                    break;
                                }
                            }
                            isDisabled = !found;
                        }
                        
                        // Check if this button was selected BEFORE rendering
                        bool wasSelected = (state.selectedAnswer == static_cast<int>(i));
                        
                        // Apply disabled style (gray) if eliminated by 5050
                        if (isDisabled) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                        } else if (wasSelected) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
                        }
                        
                        // Disable button if eliminated
                        if (isDisabled) {
                            ImGui::BeginDisabled();
                        }
                        
                        if (ImGui::Button(label, ImVec2(700, 50))) {
                            if (!isDisabled) {
                                state.selectedAnswer = i;
                            }
                        }
                        
                        if (isDisabled) {
                            ImGui::EndDisabled();
                        }
                        
                        // Pop style colors
                        if (isDisabled) {
                            ImGui::PopStyleColor(4);
                        } else if (wasSelected) {
                            ImGui::PopStyleColor(3);
                        }
                    }
                    if (state.revealActive) {
                        ImGui::TextDisabled("Revealing options...");
                    }
                    
                    ImGui::Spacing();
                    
                    if (ImGui::Button("Submit Answer", ImVec2(200, 50))) {
                        if (state.selectedAnswer >= 0) {
                            if (demoMode) {
                                // Demo mode - fake answer
                                if (state.selectedAnswer == 0) {
                                    state.resultMessage = "✓ Correct! (Demo Mode)";
                                    state.showResultMessage = true;
                                    state.resultMessageTime = 2.0f;
                                    state.currentQuestionNumber++;
                                    if (state.currentQuestionNumber > 15) {
                                        state.resultMessage = "🎉 Congratulations! You won! (Demo)";
                                        state.resultMessageTime = 5.0f;
                                        state.inGame = false;
                                    } else {
                                        state.question = "Demo Question " + std::to_string(state.currentQuestionNumber);
                                        state.selectedAnswer = -1;
                                    }
                                } else {
                                    state.resultMessage = "✗ Wrong answer! Game Over! (Demo)";
                                    state.showResultMessage = true;
                                    state.resultMessageTime = 5.0f;
                                    state.inGame = false;
                                }
                            } else if (protocol) {
                                std::cerr << "[DEBUG] Submitting answer: " << state.selectedAnswer << std::endl;
                                
                                ProtocolHandler::AnswerResponse response = protocol->answerQuestion(state.selectedAnswer);
                                std::cerr << "[DEBUG] Answer response code: " << response.responseCode << std::endl;
                                
                                if (response.responseCode == 200) {
                                    if (response.gameOver) {
                                        // Game over - notification handler will show the result
                                        state.timerRunning = false;
                                        state.inGame = false;
                                    } else if (response.correct) {
                                        // Correct answer - wait for next QUESTION_INFO notification
                                        std::cerr << "[DEBUG] Answer correct! Waiting for next question..." << std::endl;
                                        state.resultMessage = "✓ Correct! +"+std::to_string(response.pointsEarned)+" points";
                                        state.showResultMessage = true;
                                        state.resultMessageTime = 2.0f;
                                        state.waitingForQuestion = true;
                                        state.timerRunning = false;
                                        state.totalScore = response.totalScore;
                                        std::cerr << "[DEBUG] State: waitingForQuestion=" << state.waitingForQuestion 
                                                  << ", inGame=" << state.inGame << std::endl;
                                    }
                                } else if (response.responseCode == 408) {
                                    // Timeout
                                    state.resultMessage = "⏰ Time's up! Game Over!";
                                    state.showResultMessage = true;
                                    state.resultMessageTime = 5.0f;
                                    state.timerRunning = false;
                                    state.inGame = false;
                                } else {
                                    state.errorMessage = "Error submitting answer (code " + std::to_string(response.responseCode) + ")";
                                }
                            }
                        } else {
                            state.errorMessage = "Please select an answer first!";
                        }
                    }
                    
                    ImGui::SameLine();
                    if (ImGui::Button("Give Up", ImVec2(150, 50))) {
                        if (demoMode) {
                            state.resultMessage = "You gave up! (Demo Mode)";
                            state.showResultMessage = true;
                            state.resultMessageTime = 3.0f;
                            state.inGame = false;
                        } else if (protocol) {
                            std::cerr << "[DEBUG] Player giving up" << std::endl;
                            int code = protocol->giveUp();
                            if (code == 200) {
                                state.timerRunning = false;
                                // Notification handler will show the result
                            } else {
                                state.errorMessage = "Error giving up (code " + std::to_string(code) + ")";
                            }
                        }
                    }
                    
                    // Lifelines - ONLY show when we have a question (not waiting)
                    ImGui::Separator();
                    ImGui::Text("Lifelines:");
                    
                    if (state.availableLifelines[0]) {
                        bool used = false;
                        if (state.t5050Loaded) {
                            if (ImGui::ImageButton("##ll5050", ImTextureRef((ImTextureID)(intptr_t)state.tex5050), ImVec2(48, 48))) used = true;
                        } else {
                            if (ImGui::Button("50/50", ImVec2(150, 30))) used = true;
                        }
                        if (used) {
                            if (demoMode) {
                                state.errorMessage = "50/50 used! (Demo - removes 2 wrong answers)";
                                state.availableLifelines[0] = false;
                            } else if (protocol) {
                                std::cerr << "[DEBUG] Player using 50/50 lifeline" << std::endl;
                                // Pause timer
                                state.timerPaused = true;
                                state.pausedTimeRemaining = state.timeRemaining;
                                state.timerRunning = false;
                                // Start loading
                                state.lifelineProcessing = true;
                                state.lifelineLoadingMessage = "Eliminating 2 wrong answers...";
                                state.lifelineType = "5050";
                                state.lifelineStartTime = std::chrono::steady_clock::now();
                                protocol->useLifeline("5050");
                            }
                        }
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                        ImGui::Button("50/50 (Used)", ImVec2(150, 30));
                        ImGui::PopStyleColor();
                    }
                    
                    ImGui::SameLine();
                    if (state.availableLifelines[1]) {
                        bool used = false;
                        if (state.tPhoneLoaded) {
                            if (ImGui::ImageButton("##llphone", ImTextureRef((ImTextureID)(intptr_t)state.texPhone), ImVec2(48, 48))) used = true;
                        } else {
                            if (ImGui::Button("Phone Friend", ImVec2(150, 30))) used = true;
                        }
                        if (used) {
                            if (demoMode) {
                                state.errorMessage = "Friend says: I think it's A! (Demo)";
                                state.availableLifelines[1] = false;
                            } else if (protocol) {
                                std::cerr << "[DEBUG] Player using Phone a Friend lifeline" << std::endl;
                                // Pause timer
                                state.timerPaused = true;
                                state.pausedTimeRemaining = state.timeRemaining;
                                state.timerRunning = false;
                                // Start loading
                                state.lifelineProcessing = true;
                                state.lifelineLoadingMessage = "Calling...";
                                state.lifelineType = "PHONE";
                                state.lifelineStartTime = std::chrono::steady_clock::now();
                                protocol->useLifeline("PHONE");
                            }
                        }
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                        ImGui::Button("Phone (Used)", ImVec2(150, 30));
                        ImGui::PopStyleColor();
                    }
                    
                    ImGui::SameLine();
                    if (state.availableLifelines[2]) {
                        bool used = false;
                        if (state.tAudienceLoaded) {
                            if (ImGui::ImageButton("##llaud", ImTextureRef((ImTextureID)(intptr_t)state.texAudience), ImVec2(48, 48))) used = true;
                        } else {
                            if (ImGui::Button("Ask Audience", ImVec2(150, 30))) used = true;
                        }
                        if (used) {
                            if (demoMode) {
                                state.errorMessage = "Audience poll: A: 65%, B: 15%, C: 10%, D: 10% (Demo)";
                                state.availableLifelines[2] = false;
                            } else if (protocol) {
                                std::cerr << "[DEBUG] Player using Ask Audience lifeline" << std::endl;
                                // Pause timer
                                state.timerPaused = true;
                                state.pausedTimeRemaining = state.timeRemaining;
                                state.timerRunning = false;
                                // Start loading
                                state.lifelineProcessing = true;
                                state.lifelineLoadingMessage = "Surveying...";
                                state.lifelineType = "AUDIENCE";
                                state.lifelineStartTime = std::chrono::steady_clock::now();
                                protocol->useLifeline("AUDIENCE");
                            }
                        }
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                        ImGui::Button("Audience (Used)", ImVec2(150, 30));
                        ImGui::PopStyleColor();
                    }
                    
                    if (!state.errorMessage.empty()) {
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", state.errorMessage.c_str());
                    }
                    
                    // Lifeline results area (below lifeline buttons)
                    ImGui::Separator();
                    if (state.lifelineProcessing) {
                        // Show loading message
                        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "%s", state.lifelineLoadingMessage.c_str());
                    } else if (!state.lifelineType.empty()) {
                        // Show lifeline result
                        if (state.lifelineType == "5050" && !state.lifeline5050Remaining.empty()) {
                            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "50/50: Two wrong answers eliminated!");
                        } else if (state.lifelineType == "PHONE" && !state.lifelinePhoneSuggestion.empty()) {
                            ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Friend says: %s", state.lifelinePhoneSuggestion.c_str());
                        } else if (state.lifelineType == "AUDIENCE" && !state.lifelineAudiencePoll.empty()) {
                            ImGui::Text("Audience Poll Results:");
                            // Draw column chart
                            float chartWidth = 600.0f;
                            float chartHeight = 150.0f;
                            float barWidth = chartWidth / 4.0f - 10.0f;
                            ImVec2 chartPos = ImGui::GetCursorScreenPos();
                            ImDrawList* draw_list = ImGui::GetWindowDrawList();
                            
                            // Draw background
                            draw_list->AddRectFilled(chartPos, ImVec2(chartPos.x + chartWidth, chartPos.y + chartHeight), 
                                                    IM_COL32(40, 40, 40, 255));
                            
                            // Draw bars for each option
                            char labels[] = {'A', 'B', 'C', 'D'};
                            for (int i = 0; i < 4; i++) {
                                char label = labels[i];
                                int percent = 0;
                                if (state.lifelineAudiencePoll.find(label) != state.lifelineAudiencePoll.end()) {
                                    percent = state.lifelineAudiencePoll[label];
                                }
                                
                                float barHeight = (percent / 100.0f) * (chartHeight - 40.0f);
                                float x = chartPos.x + i * (chartWidth / 4.0f) + 5.0f;
                                float y = chartPos.y + chartHeight - 20.0f - barHeight;
                                
                                // Draw bar
                                ImU32 barColor = IM_COL32(100, 150, 255, 255);
                                draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + barWidth, chartPos.y + chartHeight - 20.0f), barColor);
                                
                                // Draw label
                                char labelStr[16];
                                snprintf(labelStr, sizeof(labelStr), "%c", label);
                                ImVec2 labelSize = ImGui::CalcTextSize(labelStr);
                                draw_list->AddText(ImVec2(x + barWidth/2.0f - labelSize.x/2.0f, chartPos.y + chartHeight - 15.0f), 
                                                  IM_COL32(255, 255, 255, 255), labelStr);
                                
                                // Draw percentage
                                char percentStr[16];
                                snprintf(percentStr, sizeof(percentStr), "%d%%", percent);
                                ImVec2 percentSize = ImGui::CalcTextSize(percentStr);
                                draw_list->AddText(ImVec2(x + barWidth/2.0f - percentSize.x/2.0f, y - 20.0f), 
                                                  IM_COL32(255, 255, 255, 255), percentStr);
                            }
                            
                            ImGui::Dummy(ImVec2(chartWidth, chartHeight));
                        }
                    }

                    // Bottom bar with prize
                    ImGui::SetCursorPosY(660);
                    ImGui::Separator();
                    ImGui::SetCursorPosY(670);
                    ImGui::SetCursorPosX(280);
                    ImGui::SetWindowFontScale(1.4f);
                    ImGui::TextColored(ImVec4(0.9f, 0.75f, 0.2f, 1.0f), "%d VND", state.currentPrize);
                    ImGui::SetWindowFontScale(1.0f);
                    }
                } else {
                    // No question data yet - show waiting message
                    ImGui::Text("Waiting for question...");
                }
                
                ImGui::EndChild();
            
            ImGui::End();
        }
        
frame_end:
        // Toast notification for result messages
        if (state.showResultMessage && state.resultMessageTime > 0.0f) {
            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - 200, 100), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.9f);
            
            ImGui::Begin("##Toast", nullptr, 
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                ImGuiWindowFlags_NoSavedSettings);
            
            // Color based on message content
            if (state.resultMessage.find("Correct") != std::string::npos || 
                state.resultMessage.find("won") != std::string::npos ||
                state.resultMessage.find("WON") != std::string::npos) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green
            } else if (state.resultMessage.find("Wrong") != std::string::npos || 
                       state.resultMessage.find("Game Over") != std::string::npos ||
                       state.resultMessage.find("Time") != std::string::npos) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow
            }
            
            ImGui::SetWindowFontScale(1.5f);
            ImGui::TextWrapped("%s", state.resultMessage.c_str());
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();
            
            ImGui::End();
            
            // Decrease timer
            state.resultMessageTime -= io.DeltaTime;
            if (state.resultMessageTime <= 0.0f) {
                state.showResultMessage = false;
            }
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
    if (!demoMode && protocol) {
        protocol->logout();
        delete protocol;
    }
    if (!demoMode && client) {
        client->disconnect();
        delete client;
    }
    if (notificationThread) {
        delete notificationThread;
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}

