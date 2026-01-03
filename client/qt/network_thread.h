#ifndef NETWORK_THREAD_H
#define NETWORK_THREAD_H

#include "../src/client_core.h"
#include "../src/protocol_handler.h"
#include <QThread>
#include <QObject>
#include <QString>
#include <string>

namespace MillionaireGame {

/**
 * Network thread for handling server communication in Qt
 * Runs ClientCore in a separate thread to avoid blocking UI
 */
class NetworkThread : public QThread {
    Q_OBJECT

public:
    explicit NetworkThread(QObject* parent = nullptr);
    ~NetworkThread();

    // Connection
    bool connectToServer(const QString& host, int port);
    void disconnectFromServer();
    bool isConnected() const;

    // Protocol handler access
    ProtocolHandler* getProtocolHandler() { 
        return protocol_; 
    }

signals:
    // Connection signals
    void connectionStatusChanged(bool connected);
    void connectionNotificationReceived(const QString& message);

    // Authentication signals
    void loginResponse(bool success, int code, const QString& message, 
                      const QString& authToken, const QString& username, const QString& role);
    void registerResponse(bool success, int code, const QString& message);

    // Game signals
    void questionReceived(const QString& questionJson);
    void gameStartReceived(const QString& gameStartJson);
    void gameEndReceived(const QString& gameEndJson);
    void lifelineInfoReceived(const QString& lifelineJson);
    void answerResponseReceived(const QString& answerJson);

    // Error signals
    void errorOccurred(const QString& error);

protected:
    void run() override;

private:
    ClientCore* client_;
    ProtocolHandler* protocol_;
    QString host_;
    int port_;
    bool should_connect_;
    bool should_listen_;

    void listenForNotifications();
};

} // namespace MillionaireGame

#endif // NETWORK_THREAD_H

