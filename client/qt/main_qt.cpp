#include <QApplication>
#include <QStackedWidget>
#include <QMessageBox>
#include "login_register_page.h"
#include "landing_page.h"
#include "ingame_page.h"
#include "result_page.h"
#include "network_thread.h"
#include <iostream>

using namespace MillionaireGame;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Default server configuration
    QString host = "localhost";
    int port = 8080;
    
    // Parse command line arguments
    if (argc >= 2) {
        host = QString::fromLocal8Bit(argv[1]);
    }
    if (argc >= 3) {
        port = QString::fromLocal8Bit(argv[2]).toInt();
    }
    
    // Create network thread
    NetworkThread* network_thread = new NetworkThread();
    
    // Create main stacked widget
    QStackedWidget* stack = new QStackedWidget();
    stack->setWindowTitle("Who Wants to Be a Millionaire");
    stack->resize(1400, 900);
    
    // Create pages
    LoginRegisterPage* login_page = new LoginRegisterPage(network_thread, stack);
    LandingPage* landing_page = nullptr;
    InGamePage* ingame_page = nullptr;
    ResultPage* result_page = nullptr;
    
    int login_index = stack->addWidget(login_page);
    int landing_index = -1;
    int ingame_index = -1;
    int result_index = -1;
    
    // Connect login page signals
    QObject::connect(login_page, &LoginRegisterPage::loginSuccessful,
                    [&](const QString& authToken, const QString& username, const QString& role) {
        // Create landing page if not exists
        if (!landing_page) {
            landing_page = new LandingPage(network_thread, username, role, stack);
            landing_index = stack->addWidget(landing_page);
            
            // Connect landing page signals
            QObject::connect(landing_page, &LandingPage::playRequested, [&]() {
                if (!ingame_page) {
                    ingame_page = new InGamePage(network_thread, authToken, stack);
                    ingame_index = stack->addWidget(ingame_page);
                    
                    // Connect ingame page signals
                    QObject::connect(ingame_page, &InGamePage::gameEnded,
                                    [&](long long finalPrize, int finalQuestion, bool isWinner) {
                        if (!result_page) {
                            result_page = new ResultPage(finalPrize, finalQuestion, isWinner, stack);
                            result_index = stack->addWidget(result_page);
                            
                            // Connect result page signals
                            QObject::connect(result_page, &ResultPage::backToMenuRequested, [&]() {
                                stack->setCurrentIndex(landing_index);
                            });
                            
                            QObject::connect(result_page, &ResultPage::playAgainRequested, [&]() {
                                // Delete old ingame page and create new one
                                if (ingame_page) {
                                    stack->removeWidget(ingame_page);
                                    delete ingame_page;
                                    ingame_page = nullptr;
                                }
                                
                                ingame_page = new InGamePage(network_thread, authToken, stack);
                                ingame_index = stack->addWidget(ingame_page);
                                
                                QObject::connect(ingame_page, &InGamePage::gameEnded,
                                                [&](long long finalPrize, int finalQuestion, bool isWinner) {
                                    if (result_page) {
                                        stack->removeWidget(result_page);
                                        delete result_page;
                                    }
                                    result_page = new ResultPage(finalPrize, finalQuestion, isWinner, stack);
                                    result_index = stack->addWidget(result_page);
                                    
                                    QObject::connect(result_page, &ResultPage::backToMenuRequested, [&]() {
                                        stack->setCurrentIndex(landing_index);
                                    });
                                    
                                    QObject::connect(result_page, &ResultPage::playAgainRequested, [&]() {
                                        stack->setCurrentIndex(ingame_index);
                                    });
                                    
                                    stack->setCurrentIndex(result_index);
                                });
                                
                                stack->setCurrentIndex(ingame_index);
                            });
                        } else {
                            // Update existing result page
                            stack->removeWidget(result_page);
                            delete result_page;
                            result_page = new ResultPage(finalPrize, finalQuestion, isWinner, stack);
                            result_index = stack->addWidget(result_page);
                            
                            QObject::connect(result_page, &ResultPage::backToMenuRequested, [&]() {
                                stack->setCurrentIndex(landing_index);
                            });
                            
                            QObject::connect(result_page, &ResultPage::playAgainRequested, [&]() {
                                if (ingame_page) {
                                    stack->removeWidget(ingame_page);
                                    delete ingame_page;
                                    ingame_page = nullptr;
                                }
                                
                                ingame_page = new InGamePage(network_thread, authToken, stack);
                                ingame_index = stack->addWidget(ingame_page);
                                
                                QObject::connect(ingame_page, &InGamePage::gameEnded,
                                                [&](long long finalPrize, int finalQuestion, bool isWinner) {
                                    stack->removeWidget(result_page);
                                    delete result_page;
                                    result_page = new ResultPage(finalPrize, finalQuestion, isWinner, stack);
                                    result_index = stack->addWidget(result_page);
                                    
                                    QObject::connect(result_page, &ResultPage::backToMenuRequested, [&]() {
                                        stack->setCurrentIndex(landing_index);
                                    });
                                    
                                    QObject::connect(result_page, &ResultPage::playAgainRequested, [&]() {
                                        stack->setCurrentIndex(ingame_index);
                                    });
                                    
                                    stack->setCurrentIndex(result_index);
                                });
                                
                                stack->setCurrentIndex(ingame_index);
                            });
                        }
                        
                        stack->setCurrentIndex(result_index);
                    });
                }
                stack->setCurrentIndex(ingame_index);
            });
            
            QObject::connect(landing_page, &LandingPage::leaderboardRequested, [&]() {
                // TODO: Implement leaderboard page
                QMessageBox::information(landing_page, "Leaderboard", "Leaderboard feature coming soon!");
            });
            
            QObject::connect(landing_page, &LandingPage::friendRequested, [&]() {
                // TODO: Implement friend page
                QMessageBox::information(landing_page, "Friends", "Friend feature coming soon!");
            });
            
            QObject::connect(landing_page, &LandingPage::logoutRequested, [&]() {
                // Clean up pages
                if (ingame_page) {
                    stack->removeWidget(ingame_page);
                    delete ingame_page;
                    ingame_page = nullptr;
                }
                if (result_page) {
                    stack->removeWidget(result_page);
                    delete result_page;
                    result_page = nullptr;
                }
                if (landing_page) {
                    stack->removeWidget(landing_page);
                    delete landing_page;
                    landing_page = nullptr;
                }
                landing_index = -1;
                ingame_index = -1;
                result_index = -1;
                
                // Return to login
                stack->setCurrentIndex(login_index);
            });
            
            QObject::connect(landing_page, &LandingPage::quitRequested, [&]() {
                app.quit();
            });
        }
        
        stack->setCurrentIndex(landing_index);
    });
    
    QObject::connect(login_page, &LoginRegisterPage::quitRequested, [&]() {
        app.quit();
    });
    
    // Connect to server
    std::cout << "Đang kết nối đến server " << host.toStdString() << ":" << port << "..." << std::endl;
    network_thread->connectToServer(host, port);
    
    // Show login page
    stack->setCurrentIndex(login_index);
    stack->show();
    
    int result = app.exec();
    
    // Cleanup
    network_thread->disconnectFromServer();
    network_thread->wait();
    delete network_thread;
    delete stack;
    
    return result;
}

