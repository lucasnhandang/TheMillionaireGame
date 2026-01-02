#include "game_window.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <thread>
#include <chrono>

using namespace std;

namespace MillionaireGame {

GameWindow::GameWindow(ProtocolHandler* protocol, const string& authToken)
    : protocol_(protocol), auth_token_(authToken), current_game_id_(0), 
      current_question_number_(0), game_active_(false), waiting_for_question_(true) {
}

void GameWindow::show() {
    game_active_ = true;
    waiting_for_question_ = true;
    
    // Wait for first question notification
    cout << "Dang cho cau hoi dau tien..." << endl;
    
    // Simple polling for first question (in production, use proper notification system)
    for (int i = 0; i < 30 && waiting_for_question_; i++) {
        this_thread::sleep_for(chrono::seconds(1));
    }
    
    if (!game_active_) {
        return;
    }
    
    // Main game loop
    while (game_active_) {
        clearScreen();
        cout << "========================================" << endl;
        cout << "   GAME DANG CHOI" << endl;
        cout << "========================================" << endl;
        cout << "1. Tra loi" << endl;
        cout << "2. Su dung tro giup" << endl;
        cout << "3. Bo cuoc" << endl;
        cout << "4. Thoat game" << endl;
        cout << "========================================" << endl;
        
        int choice;
        cout << "Chon: ";
        cin >> choice;
        cin.ignore();
        
        switch (choice) {
            case 1:
                handleAnswer();
                break;
            case 2:
                handleLifeline();
                break;
            case 3:
                handleGiveUp();
                break;
            case 4:
                protocol_->leaveGame(auth_token_);
                game_active_ = false;
                break;
            default:
                cout << "Lua chon khong hop le!" << endl;
                waitForEnter();
        }
    }
}

void GameWindow::displayQuestion(const ProtocolHandler::QuestionInfo& question) {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   CAU HOI " << question.questionNumber << "/" << question.totalQuestions << endl;
    cout << "========================================" << endl;
    formatPrize(question.prize);
    cout << "Diem hien tai: " << question.totalScore << endl;
    cout << "Thoi gian con lai: " << question.timeRemaining << " giay" << endl;
    cout << "========================================" << endl;
    cout << "\n" << question.question << "\n" << endl;
    
    // Display options
    for (size_t i = 0; i < question.optionTexts.size(); i++) {
        if (i < question.optionLabels.size()) {
            cout << question.optionLabels[i] << ". " << question.optionTexts[i] << endl;
        } else {
            cout << static_cast<char>('A' + i) << ". " << question.optionTexts[i] << endl;
        }
    }
    
    cout << "\n========================================" << endl;
    cout << "Tro giup con lai: ";
    for (const auto& lifeline : question.lifelines) {
        cout << lifeline << " ";
    }
    cout << endl;
    cout << "========================================" << endl;
}

void GameWindow::displayLifelineInfo(const ProtocolHandler::LifelineInfo& lifeline) {
    cout << "\n========================================" << endl;
    cout << "   KET QUA TRO GIUP: " << lifeline.lifelineType << endl;
    cout << "========================================" << endl;
    
    if (lifeline.lifelineType == "5050") {
        cout << "Cac dap an con lai: ";
        for (int idx : lifeline.remainingOptions) {
            cout << static_cast<char>('A' + idx) << " ";
        }
        cout << endl;
    } else if (lifeline.lifelineType == "PHONE") {
        cout << "Goi y: " << lifeline.suggestion << endl;
    } else if (lifeline.lifelineType == "AUDIENCE") {
        cout << "Ket qua khao sat khan gia:" << endl;
        for (const auto& poll_item : lifeline.poll) {
            cout << poll_item.first << ": " << poll_item.second << "%" << endl;
        }
    }
    
    cout << "Thoi gian con lai: " << lifeline.timeRemaining << " giay" << endl;
    cout << "Tro giup con lai: ";
    for (const auto& remaining : lifeline.lifelinesLeft) {
        cout << remaining << " ";
    }
    cout << endl;
    cout << "========================================" << endl;
}

void GameWindow::handleAnswer() {
    if (current_game_id_ == 0 || current_question_number_ == 0) {
        cout << "Chua co cau hoi nao!" << endl;
        waitForEnter();
        return;
    }
    
    cout << "Chon dap an (0-3, tuong ung A-D): ";
    int answerIndex;
    cin >> answerIndex;
    cin.ignore();
    
    if (answerIndex < 0 || answerIndex > 3) {
        cout << "Dap an khong hop le!" << endl;
        waitForEnter();
        return;
    }
    
    ProtocolHandler::AnswerResponse response = protocol_->answerQuestion(
        auth_token_, current_game_id_, current_question_number_, answerIndex);
    
    clearScreen();
    cout << "========================================" << endl;
    cout << "   KET QUA" << endl;
    cout << "========================================" << endl;
    
    if (response.success) {
        if (response.correct) {
            cout << "DUNG ROI!" << endl;
            cout << "Diem nhan duoc: " << response.pointsEarned << endl;
            cout << "Tong diem: " << response.totalScore << endl;
            formatPrize(response.currentPrize);
            
            if (response.gameOver) {
                if (response.isWinner) {
                    cout << "\nCHUC MUNG! BAN DA THANG!" << endl;
                    formatPrize(response.currentPrize);
                } else {
                    cout << "\nGame ket thuc!" << endl;
                }
                game_active_ = false;
            }
        } else {
            cout << "SAI ROI!" << endl;
            cout << "Dap an dung la: " << static_cast<char>('A' + response.correctAnswer) << endl;
            formatPrize(response.finalPrize);
            cout << "Diem cuoi cung: " << response.totalScore << endl;
            game_active_ = false;
        }
    } else {
        cout << "Loi khi tra loi!" << endl;
        cout << "Ma loi: " << response.responseCode << endl;
    }
    
    waitForEnter();
}

void GameWindow::handleLifeline() {
    if (current_game_id_ == 0 || current_question_number_ == 0) {
        cout << "Chua co cau hoi nao!" << endl;
        waitForEnter();
        return;
    }
    
    cout << "Chon tro giup:" << endl;
    cout << "1. 50/50" << endl;
    cout << "2. Goi dien cho ban" << endl;
    cout << "3. Hoi khan gia" << endl;
    cout << "Chon: ";
    
    int choice;
    cin >> choice;
    cin.ignore();
    
    string lifelineType;
    switch (choice) {
        case 1:
            lifelineType = "5050";
            break;
        case 2:
            lifelineType = "PHONE";
            break;
        case 3:
            lifelineType = "AUDIENCE";
            break;
        default:
            cout << "Lua chon khong hop le!" << endl;
            waitForEnter();
            return;
    }
    
    ProtocolHandler::LifelineResponse response = protocol_->useLifeline(
        auth_token_, current_game_id_, current_question_number_, lifelineType);
    
    if (response.success) {
        cout << "Dang xu ly tro giup..." << endl;
        // Wait for LIFELINE_INFO notification
        this_thread::sleep_for(chrono::seconds(6)); // Wait for notification
        
        // In production, properly wait for and parse LIFELINE_INFO notification
        cout << "Tro giup da duoc su dung!" << endl;
    } else {
        cout << "Khong the su dung tro giup!" << endl;
        cout << "Ma loi: " << response.responseCode << endl;
        cout << "Thong bao: " << response.message << endl;
    }
    
    waitForEnter();
}

void GameWindow::handleGiveUp() {
    if (current_game_id_ == 0 || current_question_number_ == 0) {
        cout << "Chua co cau hoi nao!" << endl;
        waitForEnter();
        return;
    }
    
    cout << "Ban co chac muon bo cuoc? (y/n): ";
    char confirm;
    cin >> confirm;
    cin.ignore();
    
    if (confirm != 'y' && confirm != 'Y') {
        return;
    }
    
    ProtocolHandler::GiveUpResponse response = protocol_->giveUp(
        auth_token_, current_game_id_, current_question_number_);
    
    clearScreen();
    cout << "========================================" << endl;
    cout << "   BAN DA BO CUOC" << endl;
    cout << "========================================" << endl;
    
    if (response.success) {
        formatPrize(response.finalPrize);
        cout << "Cau hoi dung lai: " << response.finalQuestionNumber << endl;
        cout << "Diem cuoi cung: " << response.totalScore << endl;
        cout << response.message << endl;
    } else {
        cout << "Loi khi bo cuoc!" << endl;
        cout << "Ma loi: " << response.responseCode << endl;
    }
    
    game_active_ = false;
    waitForEnter();
}

void GameWindow::handleGameEnd(const ProtocolHandler::GameEndInfo& gameEnd) {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   GAME KET THUC" << endl;
    cout << "========================================" << endl;
    
    if (gameEnd.isWinner) {
        cout << "CHUC MUNG! BAN DA THANG!" << endl;
    } else {
        cout << "Game ket thuc!" << endl;
        cout << "Trang thai: " << gameEnd.status << endl;
    }
    
    formatPrize(gameEnd.finalPrize);
    cout << "Cau hoi cuoi cung: " << gameEnd.finalQuestionNumber << endl;
    cout << "Diem cuoi cung: " << gameEnd.totalScore << endl;
    
    game_active_ = false;
    waitForEnter();
}

void GameWindow::clearScreen() {
    system("clear");
}

void GameWindow::formatPrize(long long prize) {
    if (prize >= 1000000000) {
        cout << "Giai thuong: " << fixed << setprecision(1) << (prize / 1000000000.0) << " ty VND" << endl;
    } else if (prize >= 1000000) {
        cout << "Giai thuong: " << fixed << setprecision(1) << (prize / 1000000.0) << " trieu VND" << endl;
    } else if (prize >= 1000) {
        cout << "Giai thuong: " << fixed << setprecision(1) << (prize / 1000.0) << " nghin VND" << endl;
    } else {
        cout << "Giai thuong: " << prize << " VND" << endl;
    }
}

void GameWindow::waitForEnter() {
    cout << "\nNhan Enter de tiep tuc...";
    cin.ignore();
    cin.get();
}

void GameWindow::startNotificationListener() {
    // To be implemented with proper notification handling
}

void GameWindow::stopNotificationListener() {
    // To be implemented
}

void GameWindow::onNotification(const string& message) {
    if (!protocol_->isNotification(message)) {
        return;
    }
    
    string type = protocol_->getNotificationType(message);
    
    if (type == "QUESTION_INFO") {
        ProtocolHandler::QuestionInfo question = protocol_->parseQuestionInfo(message);
        current_game_id_ = question.gameId;
        current_question_number_ = question.questionNumber;
        waiting_for_question_ = false;
        displayQuestion(question);
    } else if (type == "LIFELINE_INFO") {
        ProtocolHandler::LifelineInfo lifeline = protocol_->parseLifelineInfo(message);
        displayLifelineInfo(lifeline);
    } else if (type == "GAME_START") {
        ProtocolHandler::GameStartInfo gameStart = protocol_->parseGameStart(message);
        current_game_id_ = gameStart.gameId;
    } else if (type == "GAME_END") {
        ProtocolHandler::GameEndInfo gameEnd = protocol_->parseGameEnd(message);
        handleGameEnd(gameEnd);
    }
}

} // namespace MillionaireGame

