#!/usr/bin/env bash

# ==============================================================================
# Zappy - Full Stack Launcher (Server + 3D GFX GUI + Dynamic AI Clients)
# ==============================================================================

set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

CONF_FILE="$ROOT_DIR/server/zappy.conf"
SERVER_BIN="$ROOT_DIR/server/server"
CLIENT_BIN="$ROOT_DIR/client/client"
GFX_BIN="$ROOT_DIR/zappy_gfx/gfx"
LOG_DIR="$ROOT_DIR/logs"

# Parse configuration from server/zappy.conf (with sensible fallbacks)
GAME_PORT=$(awk '/game *\{/,/\}/ { if ($1 == "port") { gsub(/;/, "", $2); print $2 } }' "$CONF_FILE")
GUI_PORT=$(awk '/gui *\{/,/\}/ { if ($1 == "port") { gsub(/;/, "", $2); print $2 } }' "$CONF_FILE")
INITIAL_CLIENTS=$(awk '/game *\{/,/\}/ { if ($1 == "clients_per_team") { gsub(/;/, "", $2); print $2 } }' "$CONF_FILE")
DEFAULT_TEAM=$(awk '/game *\{/,/\}/ { if ($1 == "teams") { gsub(/;/, "", $2); print $2 } }' "$CONF_FILE")

GAME_PORT=${GAME_PORT:-9000}
GUI_PORT=${GUI_PORT:-9002}
INITIAL_CLIENTS=${INITIAL_CLIENTS:-6}
TEAM_NAME=${1:-${DEFAULT_TEAM:-TeamRed}}
TARGET_MAX_AI=${2:-8}

SERVER_PID=""
GFX_PID=""
SUPERVISOR_PID=""
declare -a AI_PIDS=()

cleanup() {
    echo ""
    echo "[!] Stopping all Zappy processes..."
    if [[ -n "$SUPERVISOR_PID" ]] && kill -0 "$SUPERVISOR_PID" 2>/dev/null; then
        kill "$SUPERVISOR_PID" 2>/dev/null || true
    fi
    for pid in "${AI_PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
        fi
    done
    if [[ -n "$GFX_PID" ]] && kill -0 "$GFX_PID" 2>/dev/null; then
        kill "$GFX_PID" 2>/dev/null || true
    fi
    if [[ -n "$SERVER_PID" ]] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null || true
    fi
    pkill -P $$ 2>/dev/null || true
    echo "[✓] All processes stopped."
}

trap cleanup EXIT INT TERM

# Kill any leftover Zappy processes from previous runs
pkill -f "$SERVER_BIN" 2>/dev/null || true
pkill -f "$CLIENT_BIN" 2>/dev/null || true
pkill -f "$GFX_BIN" 2>/dev/null || true
sleep 0.2

echo "[*] Building projects (server, client, zappy_gfx)..."
make server client gfx

mkdir -p "$LOG_DIR"
rm -f "$LOG_DIR"/*.log "$ROOT_DIR/zappy.log"

echo "[+] Starting Zappy server (config: $CONF_FILE)..."
echo "    - Game Port : $GAME_PORT"
echo "    - GUI Port  : $GUI_PORT"
"$SERVER_BIN" all "$CONF_FILE" < /dev/null > "$LOG_DIR/server.log" 2>&1 &
SERVER_PID=$!

echo "[*] Waiting for server to listen on ports $GAME_PORT and $GUI_PORT..."
for _ in {1..30}; do
    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "[✗] Server exited prematurely. Check $ROOT_DIR/zappy.log:"
        cat "$ROOT_DIR/zappy.log" 2>/dev/null || true
        exit 1
    fi
    if ss -tln | grep -q ":$GAME_PORT " && ss -tln | grep -q ":$GUI_PORT "; then
        break
    fi
    sleep 0.2
done
echo "[✓] Server is up and listening (PID: $SERVER_PID)!"

echo "[+] Starting 3D GFX GUI client ($GFX_BIN) on port $GUI_PORT..."
"$GFX_BIN" -p "$GUI_PORT" -h 127.0.0.1 > "$LOG_DIR/gfx.log" 2>&1 &
GFX_PID=$!

echo "[+] Starting $INITIAL_CLIENTS initial AI clients for team '$TEAM_NAME' (target max: $TARGET_MAX_AI)..."
for ((i = 1; i <= INITIAL_CLIENTS; i++)); do
    BOT_LOG="$LOG_DIR/ai_bot_${i}.log"
    echo "    -> Launching AI bot #$i (log: $BOT_LOG)"
    "$CLIENT_BIN" -n "$TEAM_NAME" -p "$GAME_PORT" -h 127.0.0.1 > "$BOT_LOG" 2>&1 &
    AI_PIDS+=($!)
    sleep 0.2
done

# Background supervisor to automatically spawn new AI clients when eggs hatch
spawn_supervisor() {
    set +e
    local spawned=$INITIAL_CLIENTS
    local last_hatched=0

    while (( spawned < TARGET_MAX_AI )); do
        local current_hatched=0
        if [[ -f "$ROOT_DIR/zappy.log" ]]; then
            current_hatched=$(grep -c "hatched" "$ROOT_DIR/zappy.log" 2>/dev/null) || current_hatched=0
        elif [[ -f "$LOG_DIR/server.log" ]]; then
            current_hatched=$(grep -c "hatched" "$LOG_DIR/server.log" 2>/dev/null) || current_hatched=0
        fi

        while (( current_hatched > last_hatched && spawned < TARGET_MAX_AI )); do
            last_hatched=$((last_hatched + 1))
            spawned=$((spawned + 1))
            local bot_log="$LOG_DIR/ai_bot_${spawned}.log"
            echo "    [+] Egg hatched! Launching AI bot #$spawned for team '$TEAM_NAME' (log: $bot_log)"
            "$CLIENT_BIN" -n "$TEAM_NAME" -p "$GAME_PORT" -h 127.0.0.1 < /dev/null > "$bot_log" 2>&1 &
            sleep 0.1
        done
        sleep 0.3
    done
}

spawn_supervisor &
SUPERVISOR_PID=$!

echo ""
echo "[✓] All 3 projects are running!"
echo "    - Server PID   : $SERVER_PID (logs -> $LOG_DIR/server.log)"
echo "    - GFX GUI PID  : $GFX_PID (logs -> $LOG_DIR/gfx.log)"
echo "    - Team         : $TEAM_NAME ($INITIAL_CLIENTS initial -> up to $TARGET_MAX_AI bots)"
echo ""
echo "Press Ctrl+C or close the 3D GFX window to stop everything."

# Wait until either the server or GFX window exits
wait -n "$SERVER_PID" "$GFX_PID" 2>/dev/null || true
