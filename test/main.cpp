#include "WebSocketClient.h"
#include <iostream>

int main()
{
    std::cout << "websocket start" << std::endl;
    WebSocketClient *ws = new WebSocketClient();

    ws->connect("ws://localhost:9001");

    ws->onOpen = [=]() {
        std::cout << "websocket connection opened" << std::endl;
    };
    ws->onMessage = [=](const WebSocketClient::Data *data) {
        std::cout << "websocket message received" << std::endl;
        std::cout << data->bytes << std::endl;
    };

    ws->onError = [=]() {
        std::cout << "websocket connection error" << std::endl;
    };

    return 0;
}
