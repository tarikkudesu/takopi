#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "raylib.h"

#include "game/GameState.hpp"
#include "gfx/Renderer.hpp"
#include "network/TcpClient.hpp"
#include "protocol/MessageHandler.hpp"
#include "protocol/Parser.hpp"

namespace {

void printUsage(const char* prog) {
    std::printf(
        "USAGE: %s -p port [-h machine]\n"
        "\tport     is the port number of the Zappy server\n"
        "\tmachine  is the hostname of the server (default: 127.0.0.1)\n"
        "\t--screenshot FILE  save a screenshot after a few seconds and quit (debug)\n",
        prog);
}

} // namespace

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    int port = 0;
    const char* screenshotFile = nullptr;
    double screenshotDelay = 4.0;

    for (int i = 1; i < argc; i++) 
    {
        if (!std::strcmp(argv[i], "-help") || !std::strcmp(argv[i], "--help")) 
        {
            printUsage(argv[0]);
            return 0;
        } 
         else if (!std::strcmp(argv[i], "-p") && i + 1 < argc) 
        {
            port = std::atoi(argv[++i]);
        } 
        else if (!std::strcmp(argv[i], "-h") && i + 1 < argc) 
        {
            host = argv[++i];
        } 
        else if (!std::strcmp(argv[i], "--screenshot") && i + 1 < argc) 
        {
            screenshotFile = argv[++i];
        } 
        else 
        {
            printUsage(argv[0]);
            return 84;
        }
    }
    if (port <= 0 || port > 65535) {
        printUsage(argv[0]);
        return 84;
    }

    zappy::GameState state;
    zappy::TcpClient net;
    zappy::Parser parser;
    zappy::MessageHandler handler(state, net);
    zappy::Renderer renderer(state, net);

    if (!renderer.init(1280, 760, "Zappy - 3D Monitor")) 
    {
        std::fprintf(stderr, "Cannot open the window (no display / OpenGL 3.3 support?)\n");
        return 84;
    }

    double connectTime = 0;
    auto tryConnect = [&]() {
        if (net.connect(host, port))
            connectTime = GetTime();
        else                          
            state.reportProtocolError(net.lastError());
    };
    tryConnect();

    double shotAt = GetTime() + screenshotDelay;
    while (!renderer.shouldClose()) 
    {
        float dt = GetFrameTime();
        if (dt > 0.1f) 
        {
            dt = 0.1f;
        }
    
        std::string bytes = net.poll();
        if (!bytes.empty())
        {
            for (const auto& msg : parser.feed(bytes)) 
            {
                handler.handle(msg);
            }
        }
        
        if (net.isConnected() && !handler.hasWelcomed() && GetTime() - connectTime > 2.0)
            handler.forceWelcome();

        if (IsKeyPressed(KEY_C) && !net.isConnected()) 
        {
            state.reset();
            tryConnect();
        }

        renderer.handleInput(dt);
        state.update(dt);
        renderer.processEvents(state.consumeEvents());
        renderer.update(dt);

        renderer.beginFrame();
        renderer.draw();
        renderer.endFrame();

        if (screenshotFile && GetTime() > shotAt) 
        {
            renderer.takeScreenshot(screenshotFile);
            break;
        }
    }

    renderer.shutdown();
    return 0;
}
