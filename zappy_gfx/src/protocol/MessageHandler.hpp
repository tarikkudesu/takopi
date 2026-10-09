#pragma once

#include "game/GameState.hpp"
#include "network/TcpClient.hpp"
#include "protocol/Message.hpp"

namespace zappy {

// Applies every server command from the Table of Commands (RFC 4242) onto a
// GameState, and completes the handshake by sending "GRAPHIC" once the
// server greets us with WELCOME/BIENVENUE.
class MessageHandler {
public:
    MessageHandler(GameState& state, TcpClient& net) : state_(state), net_(net) {}

    void handle(const Message& msg);

    bool hasWelcomed() const { return welcomed_; }
    // Fallback for a server that never sends WELCOME: send GRAPHIC anyway.
    void forceWelcome() {
        if (!welcomed_) 
        {
            net_.send("GRAPHIC\n");
            welcomed_ = true;
        }
    }

private:
    GameState& state_;
    TcpClient& net_;
    bool welcomed_ = false;

    static int parsePlayerId(const std::string& token);

    void onMsz(const Message& m);
    void onBct(const Message& m);
    void onTna(const Message& m);
    void onPnw(const Message& m);
    void onPpo(const Message& m);
    void onPlv(const Message& m);
    void onPin(const Message& m);
    void onPex(const Message& m);
    void onPbc(const Message& m);
    void onPic(const Message& m);
    void onPie(const Message& m);
    void onPfk(const Message& m);
    void onPdr(const Message& m, bool take);
    void onPdi(const Message& m);
    void onEnw(const Message& m);
    void onEggEnd(const Message& m, int kind); // eht=0, ebo=1, edi=2
    void onSgt(const Message& m);
    void onSeg(const Message& m);
    void onSmg(const Message& m);
};

} // namespace zappy
