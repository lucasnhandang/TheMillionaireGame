#include "login_window.h"
#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;

namespace MillionaireGame {

LoginWindow::LoginWindow(ProtocolHandler* protocol) : protocol_(protocol) {
}

bool LoginWindow::show() {
    clearScreen();
    
    cout << "========================================" << endl;
    cout << "   WHO WANTS TO BE A MILLIONAIRE" << endl;
    cout << "========================================" << endl;
    cout << endl;
    
    while (true) {
        showLoginMenu();
        
        int choice;
        cout << "Chon: ";
        cin >> choice;
        cin.ignore(); // Clear newline
        
        switch (choice) {
            case 1:
                if (handleLogin()) {
                    return true;
                }
                break;
            case 2:
                handleRegister();
                break;
            case 3:
                return false; // Exit
            default:
                cout << "Lua chon khong hop le!" << endl;
                waitForEnter();
        }
    }
}

void LoginWindow::showLoginMenu() {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   DANG NHAP / DANG KY" << endl;
    cout << "========================================" << endl;
    cout << "1. Dang nhap" << endl;
    cout << "2. Dang ky" << endl;
    cout << "3. Thoat" << endl;
    cout << "========================================" << endl;
}

bool LoginWindow::handleLogin() {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   DANG NHAP" << endl;
    cout << "========================================" << endl;
    
    string username, password;
    cout << "Ten dang nhap: ";
    getline(cin, username);
    
    cout << "Mat khau: ";
    // Simple password input (in production, use proper password masking)
    getline(cin, password);
    
    cout << "\nDang dang nhap..." << endl;
    
    ProtocolHandler::LoginResponse response = protocol_->login(username, password);
    
    if (response.success) {
        auth_token_ = response.authToken;
        username_ = response.username;
        role_ = response.role;
        
        cout << "\nDang nhap thanh cong!" << endl;
        cout << "Xin chao, " << username_ << "!" << endl;
        if (role_ == "admin") {
            cout << "Ban la Admin." << endl;
        }
        waitForEnter();
        return true;
    } else {
        cout << "\nDang nhap that bai!" << endl;
        cout << "Ma loi: " << response.responseCode << endl;
        cout << "Thong bao: " << response.message << endl;
        waitForEnter();
        return false;
    }
}

bool LoginWindow::handleRegister() {
    clearScreen();
    cout << "========================================" << endl;
    cout << "   DANG KY" << endl;
    cout << "========================================" << endl;
    
    string username, password;
    cout << "Ten dang nhap: ";
    getline(cin, username);
    
    cout << "Mat khau (toi thieu 8 ky tu, co chu hoa, chu thuong, so): ";
    getline(cin, password);
    
    cout << "\nDang dang ky..." << endl;
    
    ProtocolHandler::RegisterResponse response = protocol_->registerUser(username, password);
    
    if (response.success) {
        cout << "\nDang ky thanh cong!" << endl;
        cout << response.message << endl;
        waitForEnter();
        return true;
    } else {
        cout << "\nDang ky that bai!" << endl;
        cout << "Ma loi: " << response.responseCode << endl;
        cout << "Thong bao: " << response.message << endl;
        waitForEnter();
        return false;
    }
}

string LoginWindow::getAuthToken() const {
    return auth_token_;
}

string LoginWindow::getUsername() const {
    return username_;
}

string LoginWindow::getRole() const {
    return role_;
}

void LoginWindow::clearScreen() {
    system("clear"); // Linux clear command
}

void LoginWindow::waitForEnter() {
    cout << "\nNhan Enter de tiep tuc...";
    cin.ignore();
    cin.get();
}

} // namespace MillionaireGame

