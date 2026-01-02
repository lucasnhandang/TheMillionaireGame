#include "leaderboard_window.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>

using namespace std;

namespace MillionaireGame {

LeaderboardWindow::LeaderboardWindow(ProtocolHandler* protocol, const string& authToken)
    : protocol_(protocol), auth_token_(authToken) {
}

void LeaderboardWindow::show() {
    while (true) {
        showMenu();
        
        int choice;
        cout << "Chon: ";
        cin >> choice;
        cin.ignore();
        
        switch (choice) {
            case 1:
                showGlobalLeaderboard();
                break;
            case 2:
                showFriendLeaderboard();
                break;
            case 3:
                return; // Back
            default:
                cout << "Lua chon khong hop le!" << endl;
                waitForEnter();
        }
    }
}

void LeaderboardWindow::showMenu() {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   BANG XEP HANG" << endl;
    cout << "========================================" << endl;
    cout << "1. Bang xep hang toan cau" << endl;
    cout << "2. Bang xep hang ban be" << endl;
    cout << "3. Quay lai" << endl;
    cout << "========================================" << endl;
}

void LeaderboardWindow::showGlobalLeaderboard(int page) {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   BANG XEP HANG TOAN CAU" << endl;
    cout << "========================================" << endl;
    
    ProtocolHandler::LeaderboardResponse response = protocol_->getLeaderboard(auth_token_, "global", page, 20);
    
    if (response.success) {
        cout << "Trang " << response.page << " / " << ((response.total + response.limit - 1) / response.limit) << endl;
        cout << "Tong so: " << response.total << endl;
        cout << string(80, '-') << endl;
        cout << left << setw(5) << "Hang" 
             << setw(20) << "Ten dang nhap" 
             << setw(10) << "Cau hoi" 
             << setw(15) << "Diem" 
             << setw(10) << "Thang" << endl;
        cout << string(80, '-') << endl;
        
        for (const auto& entry : response.rankings) {
            cout << left << setw(5) << entry.rank
                 << setw(20) << entry.username
                 << setw(10) << entry.finalQuestionNumber
                 << setw(15) << entry.totalScore
                 << setw(10) << (entry.isWinner ? "Co" : "Khong") << endl;
        }
        
        cout << string(80, '-') << endl;
        if (response.page > 1) {
            cout << "P. Trang truoc  ";
        }
        if (response.page * response.limit < response.total) {
            cout << "N. Trang sau";
        }
        cout << endl;
    } else {
        cout << "Khong the tai bang xep hang!" << endl;
        cout << "Ma loi: " << response.responseCode << endl;
    }
    
    waitForEnter();
}

void LeaderboardWindow::showFriendLeaderboard(int page) {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   BANG XEP HANG BAN BE" << endl;
    cout << "========================================" << endl;
    
    ProtocolHandler::LeaderboardResponse response = protocol_->getLeaderboard(auth_token_, "friend", page, 20);
    
    if (response.success) {
        cout << "Trang " << response.page << " / " << ((response.total + response.limit - 1) / response.limit) << endl;
        cout << "Tong so: " << response.total << endl;
        cout << string(80, '-') << endl;
        cout << left << setw(5) << "Hang" 
             << setw(20) << "Ten dang nhap" 
             << setw(10) << "Cau hoi" 
             << setw(15) << "Diem" 
             << setw(10) << "Thang" << endl;
        cout << string(80, '-') << endl;
        
        for (const auto& entry : response.rankings) {
            cout << left << setw(5) << entry.rank
                 << setw(20) << entry.username
                 << setw(10) << entry.finalQuestionNumber
                 << setw(15) << entry.totalScore
                 << setw(10) << (entry.isWinner ? "Co" : "Khong") << endl;
        }
        
        cout << string(80, '-') << endl;
    } else {
        cout << "Khong the tai bang xep hang ban be!" << endl;
        cout << "Ma loi: " << response.responseCode << endl;
    }
    
    waitForEnter();
}

void LeaderboardWindow::clearScreen() {
    system("clear");
}

void LeaderboardWindow::waitForEnter() {
    cout << "\nNhan Enter de tiep tuc...";
    cin.ignore();
    cin.get();
}

} // namespace MillionaireGame

