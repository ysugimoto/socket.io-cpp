/**
 * WebSocket connection library
 */
#ifndef _WEBSOCKET_LIB_CPP_
#define _WEBSOCKET_LIB_CPP_

#include "WebSocket.h"
#include <iterator>
#include <list>
#include <thread>
#include <mutex>
#include <queue>

#define WEBSOCKET_WRITE_BUFFER_SIZE 2048

using namespace std;

// ================ WebSokcet Utility Class =========================
class WebSocketUtil
{
private:
    static WebSocketUtil *_inst;
    WebSocket* _ws;
    std::thread* _subThread;
    bool _needQuit;

public:
    WebSocketUtil();
    ~WebSocketUtil();

    std::mutex _UIWebSocketMessageQueueMutex;
    std::mutex _subThreadWebSocketMessageQueueMutex;
    std::list<WebSocket::Message*> *_UIWebSocketMessageQueue;
    std::list<WebSocket::Message*> *_subThreadWebSocketMessageQueue;

    static WebSocketUtil* getInstance();
    static int wrapOnSocketCallback(struct libwebsocket_context *ctx,
                                    struct libwebsocket *wsi,
                                    enum libwebsocket_callback_reasons reason,
                                    void *user,
                                    void *in,
                                    size_t len)
    {
        WebSocket *ws = (WebSocket*)libwebsocket_context_user(ctx);
        if ( ws )
        {
            return ws->onSocketCallback(ctx, wsi, reason, user, in, len);
        }
        return 0;
    }

    bool createThread(WebSocket* ws);
    void entryThread();
    void quitSubThread();
    void joinThreads();

    // thread messing
    void sendToUIThread(WebSocket::Message *msg);
    void sendToSubThread(WebSocket::Message *msg);

    // schedule update
    void updateHandler(float delta);
};

WebSocketUtil::WebSocketUtil():
    _ws(nullptr),
    _subThread(nullptr),
    _needQuit(false)
{
    _UIWebSocketMessageQueue = new std::list<WebSocket::Message*>();
    _subThreadWebSocketMessageQueue = new std::list<WebSocket::Message*>();

    // set schedule

}

WebSocketUtil::~WebSocketUtil()
{
    // unschedule
    joinThreads();
    if ( _subThread )
    {
        // relase mem
    }

    delete _UIWebSocketMessageQueue;
    delete _subThreadWebSocketMessageQueue;
}

WebSocketUtil* WebSocketUtil::getInstance()
{
    if ( ! _inst )
    {
        _inst = new WebSocketUtil();
    }

    return _inst;
}

bool WebSocketUtil::createThread(WebSocket* ws)
{
    _ws = ws;//const_cast<WebSocket*>(ws);
    _subThread = new std::thread(&WebSocketUtil::entryThread, this);

    return true;
}

void WebSocketUtil::entryThread()
{
    _ws->onSubThreadStarted();

    while ( !_needQuit )
    {
        if ( _ws->onSubThreadLoop() )
        {
            break;
        }
    }
}

void WebSocketUtil::quitSubThread()
{
    _needQuit = true;
}

void WebSocketUtil::sendToUIThread(WebSocket::Message *msg)
{
    std::lock_guard<std::mutex> lk(_UIWebSocketMessageQueueMutex);
    _UIWebSocketMessageQueue->push_back(msg);
}

void WebSocketUtil::sendToSubThread(WebSocket::Message *msg)
{
    std::lock_guard<std::mutex> lk(_subThreadWebSocketMessageQueueMutex);
    _subThreadWebSocketMessageQueue->push_back(msg);
}

void WebSocketUtil::joinThreads()
{
    if ( _subThread->joinable() )
    {
        _subThread->join();
    }
}

void WebSocketUtil::updateHandler(float delta)
{
    WebSocket::Message *msg = nullptr;
    _UIWebSocketMessageQueueMutex.lock();

    if ( _UIWebSocketMessageQueue->size() == 0 )
    {
        _UIWebSocketMessageQueueMutex.unlock();
        return;
    }

    msg = *(_UIWebSocketMessageQueue->begin());
    _UIWebSocketMessageQueue->pop_front();

    _UIWebSocketMessageQueueMutex.unlock();

    if ( _ws )
    {
        _ws->onUIThreadReceiveMessage(msg);
    }

    if ( msg )
    {
        delete (msg);
        msg = nullptr;
    }
}


// ================ WebSocket Management Class =======================

WebSocket::WebSocket():
_readyState(State::INITIALIZE),
_port(80),
_host(nullptr),
_path(nullptr),
_pendingFrameDataLength(0),
_currentDataLength(0),
_currentData(nullptr),
_sslConnection(false),
_websocket(nullptr),
_websocketContext(nullptr),
_websocketProtocols(nullptr)
{
}

WebSocket::~WebSocket()
{
    close();
    // delete callbacks
    for (int i = 0; _websocketProtocols[i].callback != nullptr; ++i)
    {
        if (_websocketProtocols[i].name)
        {
            delete[] (_websocketProtocols[i].name);
            _websocketProtocols[i].name = nullptr;
        }
    }

    delete[] (_websocketProtocols);
    _websocketProtocols = nullptr;

    // TODO: util safe release
}

bool WebSocket::connect(const string& url, const vector<string>* protocols)
{
    parseUrl(url);

    size_t protocolCount = 0;
    if ( protocols && protocols->size() > 0 )
    {
        protocolCount = protocols->size();
    }
    else
    {
        protocolCount = 1;
    }

    _websocketProtocols = new libwebsocket_protocols[protocolCount+1];
    memset(_websocketProtocols, 0, sizeof(libwebsocket_protocols)*(protocolCount+1));

    if ( protocols && protocols->size() > 0 )
    {
        int i = 0;
        for (vector<string>::const_iterator iter = protocols->begin(); iter != protocols->end(); ++iter, ++i)
        {
            char* name = new char[(*iter).length()+1];
            strcpy(name, (*iter).c_str());
            _websocketProtocols[i].name = name;
            _websocketProtocols[i].callback = WebSocketUtil::wrapOnSocketCallback;
        }
    }
    else
    {
        char* name = new char[20];
        strcpy(name, "default-protocol");
        _websocketProtocols[0].name = name;
        _websocketProtocols[0].callback = WebSocketUtil::wrapOnSocketCallback;
    }

    _websocketUtil = WebSocketUtil::getInstance();
    return _websocketUtil->createThread(this);
}

void WebSocket::send(const string& message)
{
    if ( _readyState != State::OPEN )
    {
        return;
    }

    Data *dat = new Data();
    dat->bytes = new char[message.length()+1];
    strcpy(dat->bytes, message.c_str());
    dat->len = static_cast<ssize_t>(message.length());

    Message *msg = new Message();
    msg->what = WEBSOCKET_MESSAGE_TO_SUBTRHEAD_SENDING_STRING;
    msg->obj = dat;
    _websocketUtil->sendToSubThread(msg);
}

void WebSocket::close()
{
    if ( _readyState == State::CLOSING || _readyState == State::CLOSED )
    {
        return;
    }

    _readyState = State::CLOSED;
    _websocketUtil->joinThreads();

    if ( onClose )
    {
        onClose();
    }
}

WebSocket::State WebSocket::getReadyState()
{
    return _readyState;
}

int WebSocket::onSubThreadLoop()
{
    if ( _readyState == State::CLOSED || _readyState == State::CLOSING )
    {
        libwebsocket_context_destroy(_websocketContext);
        return 1;
    }

    if ( _websocketContext && _readyState != State::CLOSED && _readyState != State::CLOSING )
    {
        libwebsocket_service(_websocketContext, 0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    return 0;
}

void WebSocket::onSubThreadStarted()
{
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof info);

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = _websocketProtocols;
#ifndef LWS_NO_EXTENSIONS
    info.extensions = libwebsocket_get_internal_extensions();
#endif
    info.gid = -1;
    info.uid = -1;
    info.user = (void*)this;

    _websocketContext = libwebsocket_create_context(&info);

    if ( _websocketContext != nullptr )
    {
        _readyState = State::CONNECTING;
        std::string name;
        for (int i = 0; _websocketProtocols[i].callback != nullptr; ++i)
        {
            name += (_websocketProtocols[i].name);

            if ( _websocketProtocols[i+1].callback != nullptr )
            {
                name += ", ";
            }
        }

        _websocket = libwebsocket_client_connect(_websocketContext, _host.c_str(), _port, _sslConnection,
                                                 _path.c_str(), _host.c_str(), _host.c_str(), name.c_str(), -1);

        if ( _websocket == nullptr )
        {
            Message *msg = new Message();
            msg->what = WEBSOCKET_MESSAGE_TO_UITHREAD_ERROR;
            _readyState = State::CLOSING;
            _websocketUtil->sendToUIThread(msg);
        }
    }
}

void WebSocket::onSubTheadEnded()
{
}

int WebSocket::onSocketCallback(struct libwebsocket_context *ctx,
                                struct libwebsocket *wsi,
                                int reason,
                                void *user,
                                void *in,
                                ssize_t len)
{
    switch (reason)
    {
        case LWS_CALLBACK_DEL_POLL_FD:
        case LWS_CALLBACK_PROTOCOL_DESTROY:
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            {
                Message *msg = nullptr;
                if ( reason == LWS_CALLBACK_CLIENT_CONNECTION_ERROR
                    || (reason == LWS_CALLBACK_PROTOCOL_DESTROY && _readyState == State::CONNECTING)
                    || (reason == LWS_CALLBACK_DEL_POLL_FD && _readyState == State::CONNECTING) )
                {
                    msg = new Message();
                    msg->what = WEBSOCKET_MESSAGE_TO_UITHREAD_ERROR;
                    _readyState = State::CLOSING;
                }
                else if ( reason == LWS_CALLBACK_PROTOCOL_DESTROY && _readyState == State::CLOSING )
                {
                    msg = new Message();
                    msg->what = WEBSOCKET_MESSAGE_TO_UITHREAD_CLOSE;
                }

                if ( msg )
                {
                    _websocketUtil->sendToUIThread(msg);
                }
            }
            break;
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            {
                Message *msg = new Message();
                msg->what = WEBSOCKET_MESSAGE_TO_UITHREAD_OPEN;
                _readyState = State::OPEN;

                libwebsocket_callback_on_writable(ctx, wsi);
                _websocketUtil->sendToUIThread(msg);
            }
            break;
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            {
                std::lock_guard<std::mutex> lk(_websocketUtil->_subThreadWebSocketMessageQueueMutex);
                std::list<Message*>::iterator iter = _websocketUtil->_subThreadWebSocketMessageQueue->begin();
                int bytesWrite = 0;
                for (; iter != _websocketUtil->_subThreadWebSocketMessageQueue->end();)
                {
                    Message *threadMessage = *iter;

                    if ( threadMessage->what == WEBSOCKET_MESSAGE_TO_SUBTRHEAD_SENDING_STRING
                        || threadMessage->what == WEBSOCKET_MESSAGE_TO_SUBTRHEAD_SENDING_BINARY)
                    {
                        Data *data = (Data*)threadMessage->obj;

                        const size_t bufferSize =  WEBSOCKET_WRITE_BUFFER_SIZE;
                        size_t remaining = data->len - data->issued;
                        size_t n = std::min(remaining, bufferSize);

                        unsigned char *buf = new unsigned char[LWS_SEND_BUFFER_PRE_PADDING + n + LWS_SEND_BUFFER_POST_PADDING];
                        memcpy((char*)&buf[LWS_SEND_BUFFER_PRE_PADDING], data->bytes + data->issued, n);
                        
                        int writeProtocol;
                        if ( data->issued == 0 )
                        {
                            if ( threadMessage->what == WEBSOCKET_MESSAGE_TO_SUBTRHEAD_SENDING_STRING )
                            {
                                writeProtocol = LWS_WRITE_TEXT;
                            }
                            else
                            {
                                writeProtocol = LWS_WRITE_BINARY;
                            }

                            if ( data->len > bufferSize )
                            {
                                writeProtocol |= LWS_WRITE_NO_FIN;
                            }
                        }
                        else
                        {
                            writeProtocol = LWS_WRITE_CONTINUATION;
                            if ( remaining != n )
                            {
                                writeProtocol = LWS_WRITE_NO_FIN;
                            }
                        }

                        bytesWrite = libwebsocket_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], n, (libwebsocket_write_protocol)writeProtocol);

                        if ( bytesWrite < 0 )
                        {
                            break;
                        }
                        else if ( remaining != n )
                        {
                            data->issued += n;
                            break;
                        }

                        delete[] (data->bytes);
                        (data->bytes) = nullptr;

                        delete (data);
                        data = nullptr;

                        delete[] (buf);
                        buf = nullptr;

                        _websocketUtil->_subThreadWebSocketMessageQueue->erase(iter++);

                        delete (threadMessage);
                        threadMessage = nullptr;
                    }
                }

                libwebsocket_callback_on_writable(ctx, wsi);
            }
            break;
        case LWS_CALLBACK_CLOSED:
            {
                _websocketUtil->quitSubThread();

                if ( _readyState != State::CLOSED )
                {
                    Message *msg = new Message();
                    _readyState = State::CLOSED;
                    msg->what = WEBSOCKET_MESSAGE_TO_UITHREAD_CLOSE;
                    _websocketUtil->sendToUIThread(msg);
                }
            }
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            {
                if ( in && len > 0 )
                {
                    if ( _currentDataLength == 0 )
                    {
                        _currentData = new char[len];
                        memcpy(_currentData, in, len);
                        _currentDataLength = len;
                    }
                    else
                    {
                        char *newData = new char[_currentDataLength + len];
                        memcpy(newData, _currentData, _currentDataLength);
                        memcpy(newData + _currentDataLength, in, len);
                        _currentData = newData;
                        _currentDataLength = _currentDataLength + len;
                    }

                    _pendingFrameDataLength = libwebsockets_remaining_packet_payload(wsi);

                    if ( _pendingFrameDataLength > 0 )
                    {
                        // logging
                    }

                    if ( _pendingFrameDataLength == 0 )
                    {
                        Message *msg = new Message();
                        msg->what = WEBSOCKET_MESSAGE_TO_UITHREAD_MESSAGE;

                        char *bytes = nullptr;
                        Data *data = new Data();

                        if ( lws_frame_is_binary(wsi) )
                        {
                            bytes = new char[_currentDataLength];
                            data->isBinary = true;
                        }
                        else
                        {
                            bytes = new char[_currentDataLength+1];
                            bytes[_currentDataLength] = '\0';
                            data->isBinary = false;
                        }

                        memcpy(bytes, _currentData, _currentDataLength);

                        data->bytes = bytes;
                        data->len = _currentDataLength;
                        msg->obj = (void*)data;

                        delete [](_currentData);
                        _currentData = nullptr;
                        _currentDataLength = 0;

                        _websocketUtil->sendToUIThread(msg);
                    }
                }
            }
            break;
        default:
            break;
    }

    return 0;
}

void WebSocket::onUIThreadReceiveMessage(WebSocket::Message *msg)
{
    switch (msg->what)
    {
        case WEBSOCKET_MESSAGE_TO_UITHREAD_OPEN:
            {
                if ( onOpen )
                {
                    onOpen();
                }
            }
            break;
        case WEBSOCKET_MESSAGE_TO_UITHREAD_MESSAGE:
            {
                if ( onMessage )
                {
                    Data *data = (Data*)msg->obj;
                    onMessage(data);
                    delete [](data->bytes);
                    data->bytes = nullptr;
                    delete (data);
                    data = nullptr;
                }
            }
            break;
        case WEBSOCKET_MESSAGE_TO_UITHREAD_CLOSE:
            {
                _websocketUtil->joinThreads();
                if ( onClose )
                {
                    onClose();
                }
            }
            break;
        case WEBSOCKET_MESSAGE_TO_UITHREAD_ERROR:
            {
                if ( onError )
                {
                    onError();
                }
            }
            break;
        default:
            break;
        }
}

#endif // _WEBSOCKET_LIB_CPP_
