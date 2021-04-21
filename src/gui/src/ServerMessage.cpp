#include "ServerMessage.h"

ServerMessage::ServerMessage(const QString& message) :
    m_message(message),
    m_clienName(parseClientName(message))
{

}

bool ServerMessage::isNewClientMessage() const
{
    return m_message.contains("unrecognised client name");
}

bool ServerMessage::isExitMessage() const
{
    return m_message.contains("process exited");
}

bool ServerMessage::isConnectedMessage() const
{
    return m_message.contains("has connected");
}

bool ServerMessage::isDisconnectedMessage() const
{
    return m_message.contains("has disconnected");
}

const QString& ServerMessage::getClientName() const
{
    return m_clienName;
}

QString ServerMessage::parseClientName(const QString& line) const
{
    QString clientName("Unknown");
    auto nameStart = line.indexOf('"') + 1;
    auto nameEnd = line.indexOf('"', nameStart);

    if (nameEnd > nameStart)
    {
       clientName = line.mid(nameStart, nameEnd - nameStart);
    }

    return clientName;
}
