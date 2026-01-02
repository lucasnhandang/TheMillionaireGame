#include "main_window.h"
#include "game_window.h"
#include "leaderboard_window.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>

using namespace std;

namespace MillionaireGame {

MainWindow::MainWindow(ProtocolHandler* protocol, const string& authToken, const string& username, const string& role)
    : protocol_(protocol), auth_token_(authToken), username_(username), role_(role) {
}

bool MainWindow::show() {
    while (true) {
        showMenu();
        
        int choice;
        cout << "Chon: ";
        cin >> choice;
        cin.ignore();
        
        switch (choice) {
            case 1:
                handleStartGame();
                break;
            case 2:
                handleResumeGame();
                break;
            case 3:
                handleLeaderboard();
                break;
            case 4:
                handleUserInfo();
                break;
            case 5:
                handleViewHistory();
                break;
            case 6:
                handleChangePassword();
                break;
            case 7:
                handleFriends();
                break;
            case 8:
                if (role_ == "admin") {
                    // Admin menu (to be implemented)
                    cout << "Chuc nang admin chua duoc trien khai." << endl;
                    waitForEnter();
                }
                break;
            case 9:
                return false; // Logout
            default:
                cout << "Lua chon khong hop le!" << endl;
                waitForEnter();
        }
    }
}

void MainWindow::showMenu() {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   MENU CHINH" << endl;
    cout << "========================================" << endl;
    cout << "Xin chao, " << username_ << "!" << endl;
    if (role_ == "admin") {
        cout << "Vai tro: Admin" << endl;
    }
    cout << "========================================" << endl;
    cout << "1. Bat dau choi moi" << endl;
    cout << "2. Tiep tuc game da luu" << endl;
    cout << "3. Bang xep hang" << endl;
    cout << "4. Thong tin nguoi choi" << endl;
    cout << "5. Lich su choi" << endl;
    cout << "6. Doi mat khau" << endl;
    cout << "7. Ban be" << endl;
    if (role_ == "admin") {
        cout << "8. Quan tri (Admin)" << endl;
    }
    cout << "9. Dang xuat" << endl;
    cout << "========================================" << endl;
}

void MainWindow::handleStartGame() {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   BAT DAU GAME MOI" << endl;
    cout << "========================================" << endl;
    
    cout << "Ban co muon ghi de game da luu? (y/n): ";
    char choice;
    cin >> choice;
    cin.ignore();
    
    bool override = (choice == 'y' || choice == 'Y');
    
    cout << "\nDang bat dau game..." << endl;
    
    ProtocolHandler::StartResponse response = protocol_->startGame(auth_token_, override);
    
    if (response.success) {
        cout << "Game da bat dau!" << endl;
        waitForEnter();
        
        // Open game window
        GameWindow gameWindow(protocol_, auth_token_);
        gameWindow.show();
    } else {
        cout << "Khong the bat dau game!" << endl;
        cout << "Ma loi: " << response.responseCode << endl;
        cout << "Thong bao: " << response.message << endl;
        waitForEnter();
    }
}

void MainWindow::handleResumeGame() {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   TIEP TUC GAME" << endl;
    cout << "========================================" << endl;
    
    cout << "Dang tai game da luu..." << endl;
    
    ProtocolHandler::ResumeResponse response = protocol_->resumeGame(auth_token_);
    
    if (response.success) {
        cout << "Game da duoc tai lai!" << endl;
        cout << "Cau hoi hien tai: " << response.questionNumber << endl;
        formatPrize(response.prize);
        cout << "Diem: " << response.totalScore << endl;
        waitForEnter();
        
        // Open game window
        GameWindow gameWindow(protocol_, auth_token_);
        gameWindow.show();
    } else {
        cout << "Khong tim thay game da luu!" << endl;
        cout << "Ma loi: " << response.responseCode << endl;
        waitForEnter();
    }
}

void MainWindow::handleLeaderboard() {
    LeaderboardWindow leaderboardWindow(protocol_, auth_token_);
    leaderboardWindow.show();
}

void MainWindow::handleUserInfo() {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   THONG TIN NGUOI CHOI" << endl;
    cout << "========================================" << endl;
    
    cout << "Nhap ten nguoi choi (de trong de xem ban): ";
    string targetUsername;
    getline(cin, targetUsername);
    
    if (targetUsername.empty()) {
        targetUsername = username_;
    }
    
    ProtocolHandler::UserInfo info = protocol_->getUserInfo(auth_token_, targetUsername);
    
    if (!info.username.empty()) {
        cout << "\nTen dang nhap: " << info.username << endl;
        cout << "Tong so game: " << info.totalGames << endl;
        formatPrize(info.highestPrize);
        cout << "Cau hoi cao nhat: " << info.finalQuestionNumber << endl;
        cout << "Diem cao nhat: " << info.totalScore << endl;
    } else {
        cout << "Khong tim thay nguoi choi!" << endl;
    }
    
    waitForEnter();
}

void MainWindow::handleViewHistory() {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   LICH SU CHOI" << endl;
    cout << "========================================" << endl;
    
    vector<ProtocolHandler::GameHistoryEntry> history = protocol_->viewHistory(auth_token_);
    
    if (history.empty()) {
        cout << "Chua co lich su choi." << endl;
    } else {
        cout << left << setw(10) << "Game ID" 
             << setw(20) << "Ngay" 
             << setw(10) << "Cau hoi" 
             << setw(15) << "Diem" 
             << setw(20) << "Giai thuong" 
             << setw(10) << "Trang thai" << endl;
        cout << string(85, '-') << endl;
        
        for (const auto& entry : history) {
            cout << left << setw(10) << entry.gameId
                 << setw(20) << entry.date
                 << setw(10) << entry.finalQuestionNumber
                 << setw(15) << entry.totalScore
                 << setw(20) << entry.finalPrize
                 << setw(10) << entry.status << endl;
        }
    }
    
    waitForEnter();
}

void MainWindow::handleChangePassword() {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   DOI MAT KHAU" << endl;
    cout << "========================================" << endl;
    
    string oldPassword, newPassword;
    cout << "Mat khau cu: ";
    getline(cin, oldPassword);
    
    cout << "Mat khau moi: ";
    getline(cin, newPassword);
    
    if (protocol_->changePassword(auth_token_, oldPassword, newPassword)) {
        cout << "\nDoi mat khau thanh cong!" << endl;
    } else {
        cout << "\nDoi mat khau that bai!" << endl;
    }
    
    waitForEnter();
}

void MainWindow::handleFriends() {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   QUAN LY BAN BE" << endl;
    cout << "========================================" << endl;
    cout << "1. Them ban be" << endl;
    cout << "2. Danh sach yeu cau ket ban" << endl;
    cout << "3. Xem danh sach ban be" << endl;
    cout << "4. Xoa ban be" << endl;
    cout << "5. Quay lai" << endl;
    cout << "========================================" << endl;
    
    int choice;
    cout << "Chon: ";
    cin >> choice;
    cin.ignore();
    
    switch (choice) {
        case 1: {
            cout << "Nhap ten nguoi choi: ";
            string friendUsername;
            getline(cin, friendUsername);
            if (protocol_->addFriend(auth_token_, friendUsername)) {
                cout << "Da gui yeu cau ket ban!" << endl;
            } else {
                cout << "Khong the gui yeu cau ket ban!" << endl;
            }
            waitForEnter();
            break;
        }
        case 2: {
            vector<pair<string, long>> requests = protocol_->getFriendRequestList(auth_token_);
            if (requests.empty()) {
                cout << "Khong co yeu cau ket ban nao." << endl;
            } else {
                for (size_t i = 0; i < requests.size(); i++) {
                    cout << (i + 1) << ". " << requests[i].first << endl;
                }
                cout << "\nChon so de chap nhan/tu choi (0 de quay lai): ";
                int idx;
                cin >> idx;
                cin.ignore();
                if (idx > 0 && idx <= static_cast<int>(requests.size())) {
                    string friendUsername = requests[idx - 1].first;
                    cout << "1. Chap nhan  2. Tu choi: ";
                    int action;
                    cin >> action;
                    cin.ignore();
                    if (action == 1) {
                        protocol_->acceptFriend(auth_token_, friendUsername);
                    } else {
                        protocol_->declineFriend(auth_token_, friendUsername);
                    }
                }
            }
            waitForEnter();
            break;
        }
        case 3: {
            vector<ProtocolHandler::FriendStatus> friends = protocol_->getFriendStatus(auth_token_);
            if (friends.empty()) {
                cout << "Chua co ban be nao." << endl;
            } else {
                for (const auto& friend_ : friends) {
                    cout << friend_.username << " - " << friend_.status << endl;
                }
            }
            waitForEnter();
            break;
        }
        case 4: {
            cout << "Nhap ten ban be: ";
            string friendUsername;
            getline(cin, friendUsername);
            if (protocol_->deleteFriend(auth_token_, friendUsername)) {
                cout << "Da xoa ban be!" << endl;
            } else {
                cout << "Khong the xoa ban be!" << endl;
            }
            waitForEnter();
            break;
        }
    }
}

void MainWindow::clearScreen() {
    system("clear");
}

void MainWindow::waitForEnter() {
    cout << "\nNhan Enter de tiep tuc...";
    cin.ignore();
    cin.get();
}

void MainWindow::formatPrize(long long prize) {
    if (prize >= 1000000000) {
        cout << "Giai thuong: " << (prize / 1000000000.0) << " ty VND" << endl;
    } else if (prize >= 1000000) {
        cout << "Giai thuong: " << (prize / 1000000.0) << " trieu VND" << endl;
    } else if (prize >= 1000) {
        cout << "Giai thuong: " << (prize / 1000.0) << " nghin VND" << endl;
    } else {
        cout << "Giai thuong: " << prize << " VND" << endl;
    }
}

} // namespace MillionaireGame

