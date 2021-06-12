#include "myclient.h"
#include <QWidget>
#include <QMessageBox>

const QString MyClient::constNameUnknown = QString(".Unknown");

MyClient::MyClient(int desc, MyServer *serv, QObject *parent) :QObject(parent)
{
    _serv = serv;
    _isAutched = false;
    _name = constNameUnknown;
    _blockSize = 0;
    _sok = new QTcpSocket(this);
    _sok->setSocketDescriptor(desc);
    connect(_sok, SIGNAL(connected()), this, SLOT(onConnect()));
    connect(_sok, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
    connect(_sok, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(_sok, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));

    qDebug() << "Client connected" << desc;
}

MyClient::~MyClient()
{

}

void MyClient::onConnect()
{
}

void MyClient::onDisconnect()
{
    qDebug() << "Client disconnected";
    if (_isAutched)
    {
        emit removeUserFromGui(_name);
        _serv->doSendToAllUserLeft(_name);
        emit removeUser(this);
    }
    deleteLater();
}

void MyClient::onError(QAbstractSocket::SocketError socketError) const
{
    QWidget w;
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(&w, "Error", "The host was not found");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(&w, "Error", "The connection was refused by the peer.");
        break;
    default:
        QMessageBox::information(&w, "Error", "The following error occurred: "+_sok->errorString());
    }
}

void MyClient::onReadyRead()
{
    QDataStream in(_sok);
    //если считываем новый блок первые 2 байта это его размер
    if (_blockSize == 0) {
        //если пришло меньше 2 байт ждем пока будет 2 байта
        if (_sok->bytesAvailable() < (int)sizeof(quint16))
            return;
        //считываем размер (2 байта)
        in >> _blockSize;
        qDebug() << "_blockSize now " << _blockSize;
    }
    //ждем пока блок прийдет полностью
    if (_sok->bytesAvailable() < _blockSize)
        return;
    else
        //можно принимать новый блок
        _blockSize = 0;
    //3 байт - команда серверу
    quint8 command;
    in >> command;
    qDebug() << "Received command " << command;
    //для неавторизованный пользователей принимается только команда "запрос на авторизацию"
    if (!_isAutched && command != comAutchReq)
        return;

    switch(command)
    {
        //запрос на авторизацию
        case comAutchReq:
        {
            QString name;
            in >> name;
            if (!_serv->isNameValid(name))
            {
                doSendCommand(comErrNameInvalid);
                return;
            }
            if (_serv->isNameUsed(name))
            {
                doSendCommand(comErrNameUsed);
                return;
            }
            _name = name;
            _isAutched = true;
            doSendUsersOnline();
            emit addUserToGui(name);
            _serv->doSendToAllUserJoin(_name);
        }
        break;
        //от текущего пользователя пришло сообщение для всех
        case comMessageToAll:
        {
            QString message;
            in >> message;
            //отправляем его всем
            _serv->doSendToAllMessage(message, _name);
            //обновляем лог событий
            emit messageToGui(message, _name, QStringList());
        }
        break;
        //от текущего пользователя пришло сообщение для некоторых пользователей
        case comMessageToUsers:
        {
            QString users_in;
            in >> users_in;
            QString message;
            in >> message;
            //разбиваем строку на имена
            QStringList users = users_in.split(",");
            //отправляем нужным
            _serv->doSendMessageToUsers(message, users, _name);
            //обновляем интерфейс
            emit messageToGui(message, _name, users);
        }
        break;
    }

}

void MyClient::doSendCommand(quint8 comm) const
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    out << comm;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    _sok->write(block);
    qDebug() << "Send to" << _name << "command: " << comm;
}

void MyClient::doSendUsersOnline() const
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    out << comUsersOnline;
    QStringList l = _serv->getUsersOnline();
    QString s;
    for (int i = 0; i < l.length(); ++i)
        if (l.at(i) != _name)
            s += l.at(i)+(QString)",";
    s.remove(s.length()-1, 1);
    out << s;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    _sok->write(block);
    qDebug() << "Send user list to" << _name << ":" << s;
}


