#include "protocol/MessageHandler.hpp"

#include <cstdlib>

namespace zappy {

namespace {

int toInt(const std::string& s) { return s.empty() ? 0 : std::atoi(s.c_str()); }

// Returns the text of `raw` after skipping `tokensToSkip` whitespace-
// separated tokens, preserving the original spacing of what remains (used
// to recover a broadcast/server message without losing embedded spaces).
std::string textAfterTokens(const std::string& raw, int tokensToSkip) {
    size_t pos = 0;
    for (int i = 0; i < tokensToSkip && pos != std::string::npos; i++) {
        while (pos < raw.size() && raw[pos] == ' ') pos++;
        pos = raw.find(' ', pos);
    }
    if (pos == std::string::npos) return {};
    while (pos < raw.size() && raw[pos] == ' ') pos++;
    return raw.substr(pos);
}

} // namespace

int MessageHandler::parsePlayerId(const std::string& token) {
    return toInt(!token.empty() && token[0] == '#' ? token.substr(1) : token);
}

void MessageHandler::handle(const Message& m) {
    if (!welcomed_ && (m.command == "WELCOME" || m.command == "BIENVENUE")) {
        net_.send("GRAPHIC\n");
        welcomed_ = true;
        return;
    }

    if      (m.command == "msz") onMsz(m);
    else if (m.command == "bct") onBct(m);
    else if (m.command == "tna") onTna(m);
    else if (m.command == "pnw") onPnw(m);
    else if (m.command == "ppo") onPpo(m);
    else if (m.command == "plv") onPlv(m);
    else if (m.command == "pin") onPin(m);
    else if (m.command == "pex") onPex(m);
    else if (m.command == "pbc") onPbc(m);
    else if (m.command == "pic") onPic(m);
    else if (m.command == "pie") onPie(m);
    else if (m.command == "pfk") onPfk(m);
    else if (m.command == "pdr") onPdr(m, false);
    else if (m.command == "pgt") onPdr(m, true);
    else if (m.command == "pdi") onPdi(m);
    else if (m.command == "enw") onEnw(m);
    else if (m.command == "eht") onEggEnd(m, 0);
    else if (m.command == "ebo") onEggEnd(m, 1);
    else if (m.command == "edi") onEggEnd(m, 2);
    else if (m.command == "sgt") onSgt(m);
    else if (m.command == "seg") onSeg(m);
    else if (m.command == "smg") onSmg(m);
    else if (m.command == "suc") state_.reportProtocolError("Server: unknown command");
    else if (m.command == "sbp") state_.reportProtocolError("Server: bad parameters");
}

void MessageHandler::onMsz(const Message& m) {
    if (m.args.size() < 2) return;
    int w = toInt(m.args[0]), h = toInt(m.args[1]);
    if (w > 0 && h > 0 && w <= 2000 && h <= 2000) state_.setMapSize(w, h);
}

void MessageHandler::onBct(const Message& m) {
    if (m.args.size() < 9) return;
    std::array<int, kResourceCount> res{};
    for (int i = 0; i < kResourceCount; i++) res[i] = toInt(m.args[2 + i]);
    state_.setTileContent(toInt(m.args[0]), toInt(m.args[1]), res);
}

void MessageHandler::onTna(const Message& m) {
    if (m.args.empty()) return;
    state_.teamIndex(m.args[0], true);
}

void MessageHandler::onPnw(const Message& m) {
    if (m.args.size() < 6) return;
    int id = parsePlayerId(m.args[0]);
    int x = toInt(m.args[1]), y = toInt(m.args[2]);
    int o = toInt(m.args[3]), lvl = toInt(m.args[4]);
    int team = state_.teamIndex(m.args[5], true);
    state_.playerJoin(id, x, y, o, lvl, team);
}

void MessageHandler::onPpo(const Message& m) {
    if (m.args.size() < 4) return;
    state_.playerMove(parsePlayerId(m.args[0]), toInt(m.args[1]), toInt(m.args[2]), toInt(m.args[3]));
}

void MessageHandler::onPlv(const Message& m) {
    if (m.args.size() < 2) return;
    state_.playerSetLevel(parsePlayerId(m.args[0]), toInt(m.args[1]));
}

void MessageHandler::onPin(const Message& m) {
    if (m.args.size() < 10) return;
    int id = parsePlayerId(m.args[0]);
    int x = toInt(m.args[1]), y = toInt(m.args[2]);
    std::array<int, kResourceCount> inv{};
    for (int i = 0; i < kResourceCount; i++) inv[i] = toInt(m.args[3 + i]);
    state_.playerSetInventory(id, x, y, inv);
}

void MessageHandler::onPex(const Message& m) {
    if (m.args.empty()) return;
    state_.playerExpel(parsePlayerId(m.args[0]));
}

void MessageHandler::onPbc(const Message& m) {
    if (m.args.empty()) return;
    state_.playerBroadcast(parsePlayerId(m.args[0]), textAfterTokens(m.raw, 2));
}

void MessageHandler::onPic(const Message& m) {
    if (m.args.size() < 3) return;
    int x = toInt(m.args[0]), y = toInt(m.args[1]), lvl = toInt(m.args[2]);
    std::vector<int> ids;
    for (size_t i = 3; i < m.args.size(); i++) ids.push_back(parsePlayerId(m.args[i]));
    state_.incantationStart(x, y, lvl, ids);
}

void MessageHandler::onPie(const Message& m) {
    if (m.args.size() < 3) return;
    state_.incantationEnd(toInt(m.args[0]), toInt(m.args[1]), toInt(m.args[2]) != 0);
}

void MessageHandler::onPfk(const Message& m) {
    if (m.args.empty()) return;
    state_.playerFork(parsePlayerId(m.args[0]));
}

void MessageHandler::onPdr(const Message& m, bool take) {
    if (m.args.size() < 2) return;
    state_.playerTakeOrDrop(parsePlayerId(m.args[0]), toInt(m.args[1]), take);
}

void MessageHandler::onPdi(const Message& m) {
    if (m.args.empty()) return;
    state_.playerDie(parsePlayerId(m.args[0]));
}

void MessageHandler::onEnw(const Message& m) {
    if (m.args.size() < 4) return;
    state_.eggLaid(parsePlayerId(m.args[0]), parsePlayerId(m.args[1]), toInt(m.args[2]), toInt(m.args[3]));
}

void MessageHandler::onEggEnd(const Message& m, int kind) {
    if (m.args.empty()) return;
    int id = parsePlayerId(m.args[0]);
    if      (kind == 0) state_.eggHatchStart(id);
    else if (kind == 1) state_.eggConnected(id);
    else                state_.eggDied(id);
}

void MessageHandler::onSgt(const Message& m) {
    if (!m.args.empty()) state_.setTimeUnit(toInt(m.args[0]));
}

void MessageHandler::onSeg(const Message& m) {
    if (!m.args.empty()) state_.setGameOver(m.args[0]);
}

void MessageHandler::onSmg(const Message& m) {
    state_.reportServerMessage(textAfterTokens(m.raw, 1));
}

} // namespace zappy
