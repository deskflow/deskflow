#ifndef SERVERMESSAGE_H
#define SERVERMESSAGE_H

#include <QString>

class ServerMessage
{
    QString m_message;
    QString m_clienName;

public:
    explicit ServerMessage(const QString& message);

    bool isNewClientMessage() const;
    bool isExitMessage() const;
    bool isConnectedMessage() const;
    bool isDisconnectedMessage() const;

    const QString& getClientName() const;

private:
    QString parseClientName(const QString& line) const;

};

#endif // SERVERMESSAGE_H
