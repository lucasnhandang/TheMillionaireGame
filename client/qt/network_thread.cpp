#include "network_thread.h"
#include <QDebug>
#include <iostream>

namespace MillionaireGame {

NetworkThread::NetworkThread(QObject* parent)
    : QThread(parent), client_(nullptr), protocol_(nullptr),
      port_(8080), should_connect_(false), should_listen_(false) {
}

NetworkThread::~NetworkThread() {
    disconnectFromServer();
    if (client_) {
        delete client_;
    }
    if (protocol_) {
        delete protocol_;
    }
}

bool NetworkThread::connectToServer(const QString& host, int port) {
    host_ = host;
    port_ = port;
    should_connect_ = true;
    
    if (!isRunning()) {
        start();
        return true;
    }
    return false;
}

void NetworkThread::disconnectFromServer() {
    should_listen_ = false;
    should_connect_ = false;
    
    if (client_ && client_->isConnected()) {
        client_->disconnect();
        emit connectionStatusChanged(false);
    }
}

bool NetworkThread::isConnected() const {
    return client_ && client_->isConnected();
}

void NetworkThread::run() {
    if (!client_) {
        client_ = new ClientCore();
    }
    
    if (!protocol_) {
        protocol_ = new ProtocolHandler(client_);
    }
    
    if (should_connect_) {
        std::string hostStr = host_.toStdString();
        if (client_->connect(hostStr, port_)) {
            emit connectionStatusChanged(true);
            
            // Wait for CONNECTION notification
            std::string connectionMsg;
            for (int i = 0; i < 5; i++) {
                connectionMsg = client_->receiveMessage(2);
                if (!connectionMsg.empty()) {
                    if (protocol_->isNotification(connectionMsg)) {
                        std::string type = protocol_->getNotificationType(connectionMsg);
                        if (type == "CONNECTION") {
                            emit connectionNotificationReceived(QString::fromStdString(connectionMsg));
                            break;
                        }
                    }
                }
            }
            
            // Start listening for notifications
            should_listen_ = true;
            listenForNotifications();
        } else {
            emit connectionStatusChanged(false);
            emit errorOccurred("Không thể kết nối đến server");
        }
    }
}

void NetworkThread::listenForNotifications() {
    while (should_listen_ && client_ && client_->isConnected()) {
        std::string message = client_->receiveMessage(1);
        if (!message.empty()) {
            if (protocol_->isNotification(message)) {
                std::string type = protocol_->getNotificationType(message);
                QString qMessage = QString::fromStdString(message);
                
                if (type == "CONNECTION") {
                    emit connectionNotificationReceived(qMessage);
                } else if (type == "QUESTION_INFO") {
                    emit questionReceived(qMessage);
                } else if (type == "GAME_START") {
                    emit gameStartReceived(qMessage);
                } else if (type == "GAME_END") {
                    emit gameEndReceived(qMessage);
                } else if (type == "LIFELINE_INFO") {
                    emit lifelineInfoReceived(qMessage);
                }
            } else if (protocol_->isResponse(message)) {
                // Handle responses (login, register, answer, etc.)
                // These are handled synchronously in the main thread
                // For now, we'll emit answerResponseReceived for answer responses
                if (message.find("\"requestType\":\"ANSWER\"") != std::string::npos ||
                    message.find("correct") != std::string::npos) {
                    emit answerResponseReceived(QString::fromStdString(message));
                }
            }
        }
        
        // Small sleep to avoid busy waiting
        QThread::msleep(10);
    }
}

} // namespace MillionaireGame

