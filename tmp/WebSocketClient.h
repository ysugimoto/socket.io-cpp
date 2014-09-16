/**
 * WebSocketClient connection library
 */
#ifndef _WEBSOCKET_LIB_H_
#define _WEBSOCKET_LIB_H_

#include <libwebsockets.h>
#include <string>
#include <vector>
#include <functional>

class WebSocketClientUtil;

class WebSocketClient
{

public:
    WebSocketClient();
    ~WebSocketClient();

    enum class State
    {
        INITIALIZE,
        CONNECTING,
        OPEN,
        CLOSING,
        CLOSED,
    };

    enum WEBSOCKET_MESSAGE_FLAG
    {
        WEBSOCKET_MESSAGE_TO_SUBTRHEAD_SENDING_STRING = 0,
        WEBSOCKET_MESSAGE_TO_SUBTRHEAD_SENDING_BINARY,
        WEBSOCKET_MESSAGE_TO_UITHREAD_OPEN,
        WEBSOCKET_MESSAGE_TO_UITHREAD_MESSAGE,
        WEBSOCKET_MESSAGE_TO_UITHREAD_ERROR,
        WEBSOCKET_MESSAGE_TO_UITHREAD_CLOSE,
    };

    struct Data
    {
        Data():bytes(nullptr), len(0), issued(0), isBinary(false){}

        char* bytes;
        ssize_t len;
        ssize_t issued;
        bool isBinary;
    };

    struct Message
    {
        Message(): what(0), obj(nullptr){}
        unsigned int what;
        void* obj;
    };

    bool connect(const std::string& url, const std::vector<std::string>* subProtocols = nullptr);
    void send(const std::string& message);
    void close();
    State getReadyState();

    std::function<void()> onClose;
    std::function<void()> onOpen;
    std::function<void()> onError;
    std::function<void(const WebSocketClient::Data *data)> onMessage;

    int onSocketCallback(struct libwebsocket_context *ctx,
                         struct libwebsocket *wsi,
                         int reason,
                         void *user,
                         void *in,
                         ssize_t len);

    // libwebsockets.c management implementation
    void onSubThreadStarted();
    int  onSubThreadLoop();
    void onSubTheadEnded();
    void onUIThreadReceiveMessage(Message* msg);

private:

    void parseUrl(const std::string& url);

    // status properties
    State _readyState;
    std::string _host;
    int _port;
    std::string _path;

    ssize_t _pendingFrameDataLength;
    ssize_t _currentDataLength;
    char*   _currentData;
    bool    _sslConnection;

    WebSocketClientUtil* _websocketUtil;

    struct libwebsocket*           _websocket;
    struct libwebsocket_context*   _websocketContext;
    struct libwebsocket_protocols* _websocketProtocols;
};

#endif // _WEBSOCKET_LIB_H_
