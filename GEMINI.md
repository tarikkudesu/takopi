# Zappy Project Documentation

## Architecture & Components

Zappy is composed of three main binaries built from the root `Makefile`:
1. **Server (`server/server`)**: Epoll-based TCP server running the game world, tick clock, food consumption, elevation rules, and GUI protocol.
2. **AI Client (`client/client`)**: Autonomous client implementing an event-driven state machine (Solo, Forage, LeaderGather, JoinLeader, Incanting, Survive) and team-wide broadcast coordination.
3. **GFX (`zappy_gfx/gfx`)**: Graphical client connecting to the server GUI port to visualize map tiles, bots, eggs, resources, and incantations.

---

## Key Diagnostic & Fix Summary

### 1. Packet Pipeline Desynchronization (Off-by-One in Followers)
- **Problem**: When `elevation en cours` arrived as an unsolicited server event, followers forcibly cleared `__hasPending = false`. If a command like `voir` was in flight, its eventual server reply was matched with the subsequent command (e.g. `avance` or `inventaire`). This offset the client-server request/reply pipeline by 1 packet permanently, causing followers to interpret vision strings as move replies or inventory, and `"ok"` as vision strings.
- **Fix**:
  - Removed premature `__hasPending = false` on `isElevationEnCours`.
  - Added type-safe reply matching in [Client.cpp](file:///home/ooulcaid/Desktop/zappy/client/srcs/Game/Client.cpp) (`CMD_VOIR` rejects `"ok"`/`"ko"`; movement/action commands reject vision strings `{...}`).

### 2. Level 2 & Level 3 Cooperative Leadership Deadlock
- **Problem**: In [StrategyStates.cpp](file:///home/ooulcaid/Desktop/zappy/client/srcs/Game/StrategyStates.cpp), `hasEnoughToLead` was restricted to `ctx->getLevel() >= 4`. At Level 3 (needs 5 stones: 2 linemate, 1 sibur, 2 phiras), unless a single bot hoovered all 5 stones into its own inventory, neither bot declared leadership, neither broadcast `OFFER 3`, and both wandered in `StateForage` forever.
- **Fix**:
  - Allowed cooperative leading for `ctx->getLevel() >= 2` with `inv[NOURRITURE] >= 12`.
  - Adjusted `tolerance` per level in [ElevationManager.cpp](file:///home/ooulcaid/Desktop/zappy/client/srcs/Game/ElevationManager.cpp):
    - Level 2 (3 stones needed): tolerance = 1 (leads with 2 stones).
    - Level 3 (5 stones needed): tolerance = 2 (leads with 3 stones).
    - Level 4 (5 stones needed, 4 players): tolerance = 3 (leads with 2 stones).
    - Level 5 & 6 (7 stones needed): tolerance = 4 (leads with 3 stones).
    - Level 7 (11 stones needed, 6 players): tolerance = 6 (leads with 5 stones).

### 3. Phantom Leader Lock-in
- **Problem**: In [Strategy.cpp](file:///home/ooulcaid/Desktop/zappy/client/srcs/Game/Strategy.cpp), when a leader leveled up or vanished, followers in `StateJoinLeader` refused offers from active leaders unless `msg.senderId > __targetLeaderId`. If the previous leader ID happened to be higher, followers remained locked to a phantom leader.
- **Fix**: Followers switch to any active leader with open slots if their current leader heartbeat has timed out (`__leaderHeartbeat > 15`) or `__targetLeaderId == -1`. Also reset `__targetLeaderId` on incantation failure (`ko`).

### 4. Movement & Speed Optimizations (<5 min win condition)
- **Proactive Early Egg Laying**: Initial team has 6 players, needing exactly 2 eggs for the 8-player cap. Bots lay the 2 eggs right at Level 1/2 as soon as food $\ge 14$. Eggs hatch in 12 seconds, allowing Bots #7 and #8 to grow up alongside the team rather than spawning late alone.
- **Direct Corridor Vision Navigation**: In [SoundNavigator.cpp](file:///home/ooulcaid/Desktop/zappy/client/srcs/Game/SoundNavigator.cpp), tiles within the forward corridor of ring $\ge 2$ now march straight forward (`avance`) instead of turning 90° back-and-forth, eliminating ~50% of turn actions.
- **Continuous Follower Transit**: Follower moving towards sound in [StrategyStates.cpp](file:///home/ooulcaid/Desktop/zappy/client/srcs/Game/StrategyStates.cpp) continues `avance` between audio pulses instead of halting for `voir` (saves 7 ticks per step). Stops looting food while en route.
- **Immediate Tile Priming**: Followers deposit missing stones onto the tile *before* broadcasting `HERE`, priming the tile for immediate incantation.
- **Reduced Inventory Calls**: Polling interval increased from 20 to 60 ticks since take/put update local inventory directly.
- **Long-Sweep Wandering**: Wander ratio updated to 80% `avance` / 10% `droite` / 10% `gauche` on the 10x10 toroid map, replacing erratic spinning.

### 5. Elimination of Blind Marching Packs & Regroup-Scatter Loops
- **Blind Marching Forward**:
  - *Problem*: In `StateForage`, `if (here.players > 1) return "avance"` forced bots sharing a tile to immediately advance without checking inventory, taking resources, or initiating gathers. Multiple bots travelling together saw `here.players > 1` on every step, marching continuously in an infinite forward loop without taking any actions.
  - *Fix*: Removed premature `avance` on occupied tiles. Bots now take food and stones, declare leadership if eligible, or step away only if a completed ritual or active leader of another group is present.
- **Regroup & Scatter Churn**:
  - *Problem*: In `StateLeaderGather`, if stones were incomplete, the leader aborted after only 10 ticks (before arriving followers could finish posing stones). In `StateJoinLeader`, followers stepped off after 15 ticks if stones were not yet placed. This caused bots to arrive, wait 10-15 ticks, abandon the tile, leave dropped stones behind, forage, meet again, and repeat.
  - *Fix*: Increased gather wait timeout to 40 ticks, allowing all arriving followers sufficient time to execute `pose`. Followers now remain on tile with the leader until ritual completes or leader officially broadcasts `ABORT`.
- **Destructive Expulse Elimination & Arrival Capacity Guard**:
  - *Problem*: When an extra follower arrived on a ritual tile (`here.players > targetPlayers`), the leader sent `expulse`, which expulsed *all* players on the tile indiscriminately, resetting the gather to 1 player and scattering the squad.
  - *Fix*: Removed leader `expulse`. When a follower arrives on a leader tile that is already over capacity (`here.players > targetPlayers && ctx->getGatherWaitTicks() == 0`), the extra follower automatically steps off on its own, allowing the existing squad to incant undisturbed.
- **Leadership Usurpation & Churn**:
  - *Problem*: Foraging bots with higher IDs continuously usurped active leaders (`ctx->getMyId() > activeLeaderId`), causing leaders to yield and followers to reverse direction constantly.
  - *Fix*: Foraging bots now declare leadership *only* if `!leaderActive`. One leader organizes the elevation undisturbed while all other teammates join as followers.

### 6. Elimination of Infinite Abort-Relead Loops & Empty-Handed Gatherings
- **Immediate Abort-Relead Churn**:
  - *Problem*: In `StateLeaderGather`, when a gather aborted due to missing stones, the leader transitioned to `StateForage`. On the very next tick in `StateForage`, `!leaderActive` was true and `hasEnoughToLead` still returned true with the same incomplete inventory. The bot immediately re-declared itself leader without taking a single step or picking up new stones, trapping the squad in an endless gather $\rightarrow$ abort $\rightarrow$ re-lead loop every 10 seconds.
  - *Fix*: Added a 60-tick `__gatherCooldown` after any aborted gather, forcing the leader to wander and forage for missing stones instead of immediately re-leading.
- **Key Stone Guard in Leadership Qualifications**:
  - *Problem*: `hasEnoughToLead` allowed bots to declare leadership for Level 5 with 0 mendiane (needed 3) and Level 7 with 0 thystame (needed 1). The leader would freeze on tile waiting for followers to bring all 3 mendiane or thystame.
  - *Fix*: Enforced strict key-stone prerequisites in [ElevationManager.cpp](file:///home/ooulcaid/Desktop/zappy/client/srcs/Game/ElevationManager.cpp): Level 5 leaders must possess $\ge 1$ mendiane; Level 7 leaders must possess $\ge 1$ thystame.
- **Universal Missing Stone Filter for Followers**:
  - *Problem*: In [Strategy.cpp](file:///home/ooulcaid/Desktop/zappy/client/srcs/Game/Strategy.cpp), checking if followers carried needed stones was restricted to `msg.targetLevel <= 3`. For Levels 4, 5, 6, 7, followers with empty inventories abandoned foraging to join leaders who were also missing those stones, convening 4 bots with 0 mendiane on the ritual tile.
  - *Fix*: Made missing-stone verification universal across all levels. Followers only join a leader missing stones if they carry at least one of those needed stones. Teammates without the required stones continue foraging until they find them.

### 7. Enforcing >75% Stones Prerequisite for Leaders & Multi-bot Squad Enrollment
- **Strict >75% Stone Threshold for Leadership**:
  - *Problem*: Leaders declared leadership with loose stone tolerances (e.g. only 54% of stones at Level 7 or 57% at Level 5), summoning followers to ritual tiles when nearly half the stones were missing, leading to squad lockups.
  - *Fix*: In [ElevationManager.cpp](file:///home/ooulcaid/Desktop/zappy/client/srcs/Game/ElevationManager.cpp), `hasEnoughToLead` now strictly enforces `stoneRatio > 0.75` (more than 75% of total required stones for that level) along with mandatory rare stones (e.g. 100% thystame for Level 7, $\ge 2$ mendiane for Level 5, sibur/phiras for Levels 3, 4, 6).
- **Explicit Squad Enrollment (`JOIN` / Enrolled IDs)**:
  - *Problem*: When a leader broadcast `OFFER`, all available followers rushed to the leader at once because `slotsNeeded` remained unchanged until someone stepped onto the tile. This caused 5-6 bots to crowd a 2-player or 4-player ritual tile, causing the server to reject the incantation.
  - *Fix*: Followers now send `JOIN` upon receiving `OFFER`. The leader accepts up to `players_needed - 1` followers in `__enrolledFollowers`, broadcasts enrolled IDs in `OFFER`, and sets `slotsNeeded = targetPlayers - 1 - enrolledFollowers.size()`. Followers abort back to `StateForage` if `slotsNeeded == 0` and they are not enrolled.
- **Universal Missing-Stone Filter & Tile Priming**:
  - *Problem*: Followers without needed stones rushed to leaders who were also missing those stones.
  - *Fix*: If ritual stones are missing on the leader tile, only followers carrying at least one of those missing stones can join. If stones on tile are complete, any teammate can join to satisfy the player quota.
- **Immediate Step-Off for Overcrowded / Unenrolled Bots**:
  - *Problem*: Passing bots or extra followers lingered on occupied leader tiles, blocking the server's exact-player-count incantation check.
  - *Fix*: Foraging bots on occupied leader tiles immediately step away (`avance`). Any follower on the tile that is not enrolled or where `here.players > targetPlayers` steps off immediately to forage.

- **Two-Team Simulation & Scaled Map (12x12)**:
  - Scaled server map from 10x10 to **12x12** in [zappy.conf](file:///home/ooulcaid/Desktop/zappy/server/zappy.conf) (+2 tiles in width and height).
  - Updated [run_zappy.sh](file:///home/ooulcaid/Desktop/zappy/run_zappy.sh) to dynamically parse and launch AI clients for both teams (`TeamRed` and `TeamBlue`), each starting with 6 bots and dynamically scaling to 8 bots as eggs hatch via a team-aware supervisor.

---

## Build & Run

```bash
# Build all 3 binaries
make

# Run the complete game simulation
./run_zappy.sh
```
