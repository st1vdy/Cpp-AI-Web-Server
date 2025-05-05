//
// Created by dongye on 25-5-3.
//

#include "server/server.h"

int main() {
    int port = 8888;

    Server server(port);
    server.run();

    return 0;
}