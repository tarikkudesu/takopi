#!/usr/bin/env python3
"""Single-file autonomous Zappy AI integration-test client.

The first process is the team leader.  It connects to the server, creates or
uses enough connection slots, and launches worker copies of this file.  Runtime
coordination happens only through the Zappy ``Broadcast`` command.

This client supports both the conventional English protocol and this project's
lowercase French wire protocol.  The dialect is selected from the server's
initial greeting.
"""

from __future__ import annotations

import argparse
import collections
import dataclasses
import os
import re
import signal
import socket
import subprocess
import sys
import time
import unicodedata
from collections.abc import Sequence

RESOURCES: tuple[str, ...] = (
    "linemate",
    "deraumere",
    "sibur",
    "mendiane",
    "phiras",
    "thystame",
)

# Key = current level; value = resources consumed to reach current level + 1.
REQUIREMENTS: dict[int, dict[str, int]] = {
    1: {"linemate": 1},
    2: {"linemate": 1, "deraumere": 1, "sibur": 1},
    3: {"linemate": 2, "sibur": 1, "phiras": 2},
    4: {"linemate": 1, "deraumere": 1, "sibur": 2, "phiras": 1},
    5: {"linemate": 1, "deraumere": 2, "sibur": 1, "mendiane": 3},
    6: {"linemate": 1, "deraumere": 2, "sibur": 3, "phiras": 1},
    7: {
        "linemate": 2,
        "deraumere": 2,
        "sibur": 2,
        "mendiane": 2,
        "phiras": 2,
        "thystame": 1,
    },
}

INVENTORY_RE = re.compile(r"([a-z_]+)\s+(\d+)", re.IGNORECASE)
MESSAGE_RE = re.compile(r"^message\s+([0-8])\s*,\s*(.*)$", re.IGNORECASE)
LEVEL_RE = re.compile(r"^(?:current level|niveau actuel)\s*:\s*(\d+)\s*$", re.IGNORECASE)
EJECT_RE = re.compile(r"^(?:eject|deplacement)(?:\s*:\s*|\s+)([0-8])\s*$", re.IGNORECASE)

FRENCH_COMMANDS = {
    "Forward": "avance",
    "Right": "droite",
    "Left": "gauche",
    "Look": "voir",
    "Inventory": "inventaire",
    "Take": "prend",
    "Set": "pose",
    "Eject": "expulse",
    "Broadcast": "broadcast",
    "Connect_nbr": "connect_nbr",
    "Fork": "fork",
    "Incantation": "incantation",
}


def ascii_lower(text: str) -> str:
    """Lowercase text and remove accents for protocol comparisons."""
    normalized = unicodedata.normalize("NFKD", text)
    return "".join(char for char in normalized if not unicodedata.combining(char)).lower()


class ZappyError(RuntimeError):
    pass


class PlayerDead(ZappyError):
    pass


@dataclasses.dataclass(frozen=True)
class BroadcastEvent:
    direction: int
    payload: str


class Logger:
    def __init__(self, worker_id: int, quiet: bool = False) -> None:
        self.worker_id = worker_id
        self.quiet = quiet

    def __call__(self, message: str, *, force: bool = False) -> None:
        if self.quiet and not force:
            return
        stamp = time.strftime("%H:%M:%S")
        print(f"[{stamp} ai:{self.worker_id}] {message}", flush=True)


class ZappyConnection:
    """Blocking, one-command-at-a-time client with async-event filtering."""

    def __init__(self, host: str, port: int, team: str, timeout: float, language: str = "auto") -> None:
        self.host = host
        self.port = port
        self.team = team
        self.timeout = timeout
        self.requested_language = language
        self.language = language if language != "auto" else "en"
        self.sock: socket.socket | None = None
        self.buffer = bytearray()
        self.broadcasts: collections.deque[BroadcastEvent] = collections.deque()
        self.ejected: collections.deque[int] = collections.deque()
        self.level = 1
        self.elevating = False
        self.alive = True
        self.slots_at_connect = 0
        self.width = 10
        self.height = 10

    def connect(self) -> None:
        sock = socket.create_connection((self.host, self.port), self.timeout)
        sock.settimeout(self.timeout)
        self.sock = sock

        welcome = ascii_lower(self._read_line().strip())
        detected = {"welcome": "en", "bienvenue": "fr"}.get(welcome)
        if detected is None:
            raise ZappyError(f"expected WELCOME or BIENVENUE, received {welcome!r}")
        if self.requested_language != "auto" and detected != self.requested_language:
            raise ZappyError(f"server greeting indicates {detected!r}, but --language={self.requested_language!r} was requested")
        self.language = detected
        self._send_line(self.team)

        first = self._read_line().strip()
        if first.lower() == "ko":
            raise ZappyError(f"team {self.team!r} was rejected by the server")
        try:
            self.slots_at_connect = int(first)
        except ValueError as exc:
            raise ZappyError(f"invalid connection-slot response: {first!r}") from exc

        dimensions = self._read_line().split()
        if len(dimensions) != 2:
            raise ZappyError(f"invalid map dimensions: {' '.join(dimensions)!r}")
        try:
            self.width, self.height = map(int, dimensions)
        except ValueError as exc:
            raise ZappyError(f"invalid map dimensions: {dimensions!r}") from exc

    def close(self) -> None:
        if self.sock is not None:
            try:
                self.sock.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            self.sock.close()
            self.sock = None

    def _send_line(self, line: str) -> None:
        if self.sock is None:
            raise ZappyError("socket is not connected")
        self.sock.sendall(line.encode("utf-8") + b"\n")

    def _wire_command(self, command: str) -> str:
        """Translate a canonical English command to the detected wire dialect."""
        if self.language != "fr":
            return command
        verb, separator, argument = command.partition(" ")
        wire_verb = FRENCH_COMMANDS.get(verb, verb)
        if verb in {"Take", "Set"} and argument.lower() == "food":
            argument = "nourriture"
        return wire_verb + (separator + argument if separator else "")

    def _read_line(self) -> str:
        if self.sock is None:
            raise ZappyError("socket is not connected")
        while b"\n" not in self.buffer:
            try:
                chunk = self.sock.recv(4096)
            except TimeoutError as exc:
                raise ZappyError("timed out waiting for the server") from exc
            if not chunk:
                raise ZappyError("server closed the connection")
            self.buffer.extend(chunk)
        raw, _, remaining = self.buffer.partition(b"\n")
        self.buffer = bytearray(remaining)
        return raw.rstrip(b"\r").decode("utf-8", errors="replace")

    def _consume_event(self, line: str) -> bool:
        stripped = line.strip()
        comparable = ascii_lower(stripped)
        if comparable in {"dead", "mort"}:
            self.alive = False
            raise PlayerDead("player starved or was killed")

        match = MESSAGE_RE.match(stripped)
        if match:
            self.broadcasts.append(BroadcastEvent(direction=int(match.group(1)), payload=match.group(2)))
            return True

        match = EJECT_RE.match(stripped)
        if match:
            self.ejected.append(int(match.group(1)))
            return True

        if comparable in {"elevation underway", "elevation en cours"}:
            self.elevating = True
            return True

        match = LEVEL_RE.match(stripped)
        if match:
            self.level = int(match.group(1))
            self.elevating = False
            return True

        return False

    def command(self, command: str) -> str:
        """Send one command and return its synchronous response."""
        self._send_line(self._wire_command(command))
        while True:
            line = self._read_line()
            was_elevating = self.elevating
            if self._consume_event(line):
                # Elevation underway is the immediate response to Incantation.
                if command == "Incantation" and not was_elevating and self.elevating:
                    return "Elevation underway"
                continue
            return line.strip()

    def incantation(self) -> bool:
        previous_level = self.level
        first = self.command("Incantation")
        if first.lower() != "elevation underway":
            return False
        while self.alive and self.elevating:
            line = self._read_line()
            if self._consume_event(line):
                continue
            if line.strip().lower() == "ko":
                self.elevating = False
                return False
            raise ZappyError(f"unexpected response during incantation: {line!r}")
        return self.level > previous_level

    def wait_for_elevation(self) -> None:
        while self.alive and self.elevating:
            line = self._read_line()
            self._consume_event(line)


def parse_inventory(response: str) -> dict[str, int]:
    inventory = {"food": 0, **{name: 0 for name in RESOURCES}}
    for name, amount in INVENTORY_RE.findall(response.lower()):
        if name == "nourriture":
            name = "food"
        if name in inventory:
            inventory[name] = int(amount)
    return inventory


def parse_look(response: str) -> list[list[str]]:
    text = response.strip()
    if len(text) < 2 or (text[0], text[-1]) not in {("[", "]"), ("{", "}")}:
        return []
    inside = text[1:-1].strip()
    if not inside:
        return [[]]
    tiles = [tile.strip().lower().split() for tile in inside.split(",")]
    return [["food" if item == "nourriture" else item for item in tile] for tile in tiles]


class Explorer:
    """Deterministic serpentine walk that covers a toroidal rectangular map."""

    def __init__(self, width: int, height: int) -> None:
        self.width = max(1, width)
        self.height = max(1, height)
        self.column = 0
        self.row = 0
        self.going_right = True
        self.pending: collections.deque[str] = collections.deque()

    def next_command(self) -> str:
        if self.pending:
            return self.pending.popleft()

        if self.column < self.width - 1:
            self.column += 1
            return "Forward"

        self.column = 0
        self.row = (self.row + 1) % self.height
        if self.going_right:
            self.pending.extend(("Forward", "Right"))
            self.going_right = False
            return "Right"
        self.pending.extend(("Forward", "Left"))
        self.going_right = True
        return "Left"


class Agent:
    PROTOCOL_VERSION = "ZPY1"

    def __init__(self, args: argparse.Namespace) -> None:
        self.args = args
        self.worker_id: int = args.worker_id
        self.is_leader = self.worker_id == 0
        self.log = Logger(self.worker_id, args.quiet and not self.is_leader)
        self.connection = ZappyConnection(args.host, args.port, args.team, args.socket_timeout, args.language)
        self.inventory = {"food": 10, **{name: 0 for name in RESOURCES}}
        self.explorer: Explorer | None = None
        self.children: list[subprocess.Popen[bytes]] = []
        self.stop_requested = False
        self.arrived: set[int] = set()
        self.known_workers: set[int] = {0}
        self.meet_level: int | None = None
        self.meet_direction: int | None = None
        self.last_arrived_broadcast = 0.0
        self.start_time = time.monotonic()

    @property
    def level(self) -> int:
        return self.connection.level

    @property
    def prefix(self) -> str:
        return f"{self.PROTOCOL_VERSION}|{self.args.team}|"

    def team_message(self, kind: str, *parts: object) -> str:
        suffix = "|".join(str(part) for part in parts)
        return f"{self.prefix}{kind}" + (f"|{suffix}" if suffix else "")

    def install_signal_handlers(self) -> None:
        def stop(_signum: int, _frame: object) -> None:
            self.stop_requested = True

        signal.signal(signal.SIGINT, stop)
        signal.signal(signal.SIGTERM, stop)

    def update_inventory(self) -> None:
        response = self.connection.command("Inventory")
        if response.startswith(("[", "{")):
            self.inventory = parse_inventory(response)
        self.process_events()

    def process_events(self) -> None:
        while self.connection.broadcasts:
            event = self.connection.broadcasts.popleft()
            if not event.payload.startswith(self.prefix):
                continue
            fields = event.payload[len(self.prefix) :].split("|")
            kind = fields[0]
            try:
                if kind == "HELLO" and len(fields) == 2:
                    self.known_workers.add(int(fields[1]))
                elif kind == "MEET" and len(fields) == 2 and not self.is_leader:
                    requested_level = int(fields[1])
                    if requested_level == self.level:
                        self.meet_level = requested_level
                        self.meet_direction = event.direction
                elif kind == "ARRIVED" and len(fields) == 3 and self.is_leader:
                    reported_level = int(fields[1])
                    worker_id = int(fields[2])
                    if reported_level == self.level and 0 < worker_id < self.args.players:
                        self.arrived.add(worker_id)
                elif kind == "LEVEL" and len(fields) == 2 and not self.is_leader:
                    if int(fields[1]) > self.level:
                        self.meet_direction = 0
                elif kind == "WIN":
                    self.stop_requested = True
            except ValueError:
                continue

    def broadcast(self, payload: str) -> bool:
        result = self.connection.command(f"Broadcast {payload}")
        self.process_events()
        return result.lower() == "ok"

    def take(self, resource: str) -> bool:
        result = self.connection.command(f"Take {resource}")
        self.process_events()
        if result.lower() == "ok":
            self.inventory[resource] = self.inventory.get(resource, 0) + 1
            return True
        return False

    def set_down(self, resource: str) -> bool:
        result = self.connection.command(f"Set {resource}")
        self.process_events()
        if result.lower() == "ok":
            self.inventory[resource] = max(0, self.inventory.get(resource, 0) - 1)
            return True
        return False

    def look_and_collect(self, collect_stones: bool) -> None:
        response = self.connection.command("Look")
        self.process_events()
        tiles = parse_look(response)
        if not tiles:
            return
        current = collections.Counter(tiles[0])

        food_cap = self.args.leader_food_cap if self.is_leader else self.args.food_target
        wanted_food = max(0, food_cap - self.inventory["food"])
        for _ in range(min(current.get("food", 0), wanted_food)):
            if not self.take("food"):
                break

        if collect_stones:
            for resource in RESOURCES:
                for _ in range(current.get(resource, 0)):
                    if not self.take(resource):
                        break

    def explore_once(self, collect_stones: bool) -> None:
        self.look_and_collect(collect_stones)
        if self.stop_requested:
            return
        assert self.explorer is not None
        self.connection.command(self.explorer.next_command())
        self.process_events()

    def resources_ready(self) -> bool:
        required = REQUIREMENTS.get(self.level, {})
        return all(self.inventory.get(name, 0) >= count for name, count in required.items())

    def spawn_worker(self, worker_id: int) -> None:
        command: list[str] = [
            sys.executable,
            os.path.abspath(__file__),
            "-h",
            self.args.host,
            "-p",
            str(self.args.port),
            "-n",
            self.args.team,
            "--worker-id",
            str(worker_id),
            "--players",
            str(self.args.players),
            "--food-emergency",
            str(self.args.food_emergency),
            "--food-target",
            str(self.args.food_target),
            "--meet-food",
            str(self.args.meet_food),
            "--leader-food-cap",
            str(self.args.leader_food_cap),
            "--socket-timeout",
            str(self.args.socket_timeout),
            "--language",
            self.args.language,
            "--max-runtime",
            str(self.args.max_runtime),
        ]
        if self.args.quiet:
            command.append("--quiet")
        child = subprocess.Popen(command)
        self.children.append(child)
        self.log(f"spawned worker {worker_id} (pid {child.pid})")

    def available_slots(self) -> int:
        response = self.connection.command("Connect_nbr")
        self.process_events()
        try:
            return int(response)
        except ValueError as exc:
            raise ZappyError(f"Connect_nbr returned {response!r}") from exc

    def wait_for_slot(self) -> int:
        deadline = time.monotonic() + self.args.fork_timeout
        while not self.stop_requested:
            available = self.available_slots()
            if available > 0:
                return available
            if time.monotonic() >= deadline:
                raise ZappyError(f"timed out after {self.args.fork_timeout}s waiting for a forked connection slot to hatch")
            time.sleep(self.args.fork_poll_interval)
        raise ZappyError("stopped while waiting for a connection slot")

    def wait_for_worker_ready(self, worker_id: int) -> None:
        deadline = time.monotonic() + self.args.fork_timeout
        while not self.stop_requested:
            if worker_id in self.known_workers:
                return
            child = self.children[-1]
            if child.poll() is not None:
                raise ZappyError(f"worker {worker_id} exited before joining the team (status {child.returncode})")
            # A synchronous request lets command() drain asynchronous HELLO
            # broadcasts.  Slot counts cannot prove that this worker joined:
            # another egg may hatch at the same time and mask the consumed slot.
            self.available_slots()
            if time.monotonic() >= deadline:
                raise ZappyError(f"timed out waiting for worker {worker_id} to announce readiness")
            time.sleep(self.args.fork_poll_interval)
        raise ZappyError("stopped while waiting for a worker to connect")

    def ensure_population(self) -> None:
        self.log(f"creating a {self.args.players}-player test team")
        worker_count = self.args.players - 1
        available = self.available_slots()
        forks_needed = worker_count if self.args.force_fork else max(0, worker_count - available)

        for fork_number in range(forks_needed):
            self.log(f"laying connection egg {fork_number + 1}/{forks_needed}")
            if self.connection.command("Fork").lower() != "ok":
                raise ZappyError("Fork failed while creating the test team")
            self.process_events()

        for worker_id in range(1, self.args.players):
            self.wait_for_slot()
            self.spawn_worker(worker_id)
            time.sleep(self.args.spawn_delay)
            self.wait_for_worker_ready(worker_id)
            self.log(f"worker {worker_id} joined the team")

    def clear_tile_minerals(self) -> None:
        response = self.connection.command("Look")
        self.process_events()
        tiles = parse_look(response)
        if not tiles:
            return
        current = collections.Counter(tiles[0])
        for resource in RESOURCES:
            for _ in range(current.get(resource, 0)):
                if not self.take(resource):
                    break

    def prepare_ritual(self) -> bool:
        required = REQUIREMENTS[self.level]
        self.clear_tile_minerals()
        self.update_inventory()
        if not self.resources_ready():
            self.log("ritual preparation lost a required resource; resuming collection")
            return False
        for resource, count in required.items():
            for _ in range(count):
                if not self.set_down(resource):
                    self.log(f"could not place {resource}; aborting ritual preparation")
                    return False
        return True

    def leader_loop(self) -> None:
        self.ensure_population()
        self.update_inventory()

        while not self.stop_requested and self.level < 8:
            self.enforce_runtime_limit()
            self.update_inventory()

            if self.inventory["food"] <= self.args.food_emergency:
                self.log(f"food emergency ({self.inventory['food']}); suspending progression")
                self.explore_once(collect_stones=True)
                continue

            if not self.resources_ready():
                self.explore_once(collect_stones=True)
                continue

            if self.inventory["food"] < self.args.meet_food:
                self.explore_once(collect_stones=True)
                continue

            ritual_level = self.level
            self.arrived.clear()
            self.log(f"level {ritual_level}: resources ready; assembling the team")
            last_reported_count = -1

            while not self.stop_requested and self.level == ritual_level and len(self.arrived) < self.args.players - 1:
                self.enforce_runtime_limit()
                self.broadcast(self.team_message("MEET", ritual_level))
                self.update_inventory()
                if self.inventory["food"] <= self.args.food_emergency:
                    self.log("assembly aborted: leader needs food")
                    break
                current_count = len(self.arrived) + 1
                if current_count != last_reported_count:
                    self.log(f"assembly {current_count}/{self.args.players}")
                    last_reported_count = current_count

            if len(self.arrived) < self.args.players - 1:
                continue

            self.log(f"all {self.args.players} players assembled for level {ritual_level}")
            if not self.prepare_ritual():
                continue

            self.broadcast(self.team_message("INCANT", ritual_level))
            previous_level = self.level
            succeeded = self.connection.incantation()
            self.process_events()
            if not succeeded or self.level == previous_level:
                self.log(f"level {ritual_level} incantation failed; retrying collection")
                continue

            self.log(f"team reached level {self.level}", force=True)
            self.broadcast(self.team_message("LEVEL", self.level))

        if self.level >= 8:
            self.log("level 8 reached; integration test passed", force=True)
            try:
                self.broadcast(self.team_message("WIN", 8))
            except ZappyError:
                pass

    def move_toward_broadcast(self, direction: int) -> None:
        # Zappy directions are relative sectors around the receiver:
        # 1 front, 3 left, 5 behind, 7 right; even values are diagonals.
        commands: dict[int, Sequence[str]] = {
            1: ("Forward",),
            2: ("Left", "Forward"),
            3: ("Left", "Forward"),
            4: ("Left", "Forward"),
            5: ("Left", "Left", "Forward"),
            6: ("Right", "Forward"),
            7: ("Right", "Forward"),
            8: ("Right", "Forward"),
        }
        for command in commands.get(direction, ()):
            self.connection.command(command)
            self.process_events()
        # Require a fresh bearing before moving again, preventing overshoot.
        self.meet_direction = None

    def worker_loop(self) -> None:
        self.broadcast(self.team_message("HELLO", self.worker_id))
        self.update_inventory()

        while not self.stop_requested and self.level < 8:
            self.enforce_runtime_limit()
            if self.connection.elevating:
                self.connection.wait_for_elevation()
                self.process_events()
                self.meet_level = None
                self.meet_direction = None
                self.log(f"reached level {self.level}", force=True)
                continue

            self.update_inventory()

            if self.inventory["food"] <= self.args.food_emergency:
                self.meet_level = None
                self.meet_direction = None
                self.explore_once(collect_stones=False)
                continue

            if self.meet_level == self.level and self.inventory["food"] >= self.args.meet_food:
                if self.meet_direction == 0:
                    now = time.monotonic()
                    if now - self.last_arrived_broadcast >= self.args.arrival_interval:
                        self.broadcast(self.team_message("ARRIVED", self.level, self.worker_id))
                        self.last_arrived_broadcast = now
                    else:
                        self.update_inventory()
                elif self.meet_direction is not None:
                    self.move_toward_broadcast(self.meet_direction)
                else:
                    # Inventory is a cheap command that also lets us receive the
                    # leader's next asynchronous MEET broadcast.
                    self.update_inventory()
                continue

            self.explore_once(collect_stones=False)

        if self.level >= 8:
            self.log("level 8 reached", force=True)

    def enforce_runtime_limit(self) -> None:
        if self.args.max_runtime <= 0:
            return
        elapsed = time.monotonic() - self.start_time
        if elapsed > self.args.max_runtime:
            raise ZappyError(f"maximum runtime of {self.args.max_runtime}s exceeded")

    def cleanup_children(self) -> None:
        for child in self.children:
            if child.poll() is None:
                child.terminate()
        deadline = time.monotonic() + 2.0
        for child in self.children:
            if child.poll() is None:
                try:
                    child.wait(timeout=max(0.0, deadline - time.monotonic()))
                except subprocess.TimeoutExpired:
                    child.kill()

    def run(self) -> int:
        self.install_signal_handlers()
        try:
            self.connection.connect()
            self.explorer = Explorer(self.connection.width, self.connection.height)
            role = "leader" if self.is_leader else "worker"
            self.log(f"connected as {role}; map={self.connection.width}x{self.connection.height}; protocol={self.connection.language}; initial_slots={self.connection.slots_at_connect}")
            if self.is_leader:
                self.leader_loop()
            else:
                self.worker_loop()
            return 0
        except PlayerDead as exc:
            self.log(str(exc), force=True)
            return 2
        except (OSError, ZappyError) as exc:
            self.log(f"fatal: {exc}", force=True)
            return 1
        finally:
            self.connection.close()
            if self.is_leader:
                self.cleanup_children()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        add_help=False,
        description="Autonomous single-file Zappy level-8 integration-test AI",
    )
    parser.add_argument("--help", action="help", help="show this help message and exit")
    parser.add_argument("-h", "--host", default="127.0.0.1", help="server host")
    parser.add_argument("-p", "--port", type=int, default=4242, help="server port")
    parser.add_argument("-n", "--team", default="TeamRed", help="team name")
    parser.add_argument(
        "--language",
        choices=("auto", "en", "fr"),
        default="auto",
        help="protocol language; auto detects WELCOME or BIENVENUE",
    )
    parser.add_argument("--players", type=int, default=6, help="total team processes")
    parser.add_argument("--worker-id", type=int, default=0, help=argparse.SUPPRESS)
    parser.add_argument("--food-emergency", type=int, default=6, help="food count that forces foraging")
    parser.add_argument("--food-target", type=int, default=12, help="worker food collection target")
    parser.add_argument("--meet-food", type=int, default=8, help="minimum food before rendezvous")
    parser.add_argument("--leader-food-cap", type=int, default=16, help="leader food collection cap")
    parser.add_argument("--socket-timeout", type=float, default=180.0, help="server response timeout")
    parser.add_argument("--spawn-delay", type=float, default=0.15, help="seconds between child launches")
    parser.add_argument(
        "--fork-poll-interval",
        type=float,
        default=0.25,
        help="seconds between checks for a hatched fork slot",
    )
    parser.add_argument(
        "--fork-timeout",
        type=float,
        default=180.0,
        help="maximum seconds to wait for a forked slot or worker handshake",
    )
    parser.add_argument(
        "--arrival-interval",
        type=float,
        default=0.25,
        help="seconds between ARRIVED broadcasts",
    )
    parser.add_argument(
        "--max-runtime",
        type=float,
        default=0.0,
        help="wall-clock limit in seconds; zero means unlimited",
    )
    parser.add_argument(
        "--force-fork",
        action="store_true",
        help="lay one connection egg per worker even when slots already exist",
    )
    parser.add_argument("--quiet", action="store_true", help="suppress routine worker logs")
    parser.add_argument("--self-test", action="store_true", help=argparse.SUPPRESS)
    return parser


def self_test() -> int:
    defaults = build_parser().parse_args([])
    assert defaults.food_emergency < defaults.meet_food
    assert defaults.meet_food <= defaults.food_target <= defaults.leader_food_cap
    assert parse_inventory("[ food 10, linemate 2, sibur 1 ]")["food"] == 10
    assert parse_inventory("[ food 10, linemate 2, sibur 1 ]")["sibur"] == 1
    assert parse_inventory("{nourriture 9, phiras 2}")["food"] == 9
    assert parse_look("[ food linemate, sibur,  ]") == [
        ["food", "linemate"],
        ["sibur"],
        [],
    ]
    assert parse_look("{ nourriture deraumere,  }") == [
        ["food", "deraumere"],
        [],
    ]
    french = ZappyConnection("localhost", 4242, "TEST", 1, "fr")
    assert french._wire_command("Forward") == "avance"
    assert french._wire_command("Take food") == "prend nourriture"
    assert french._wire_command("Set linemate") == "pose linemate"
    assert french._wire_command("Connect_nbr") == "connect_nbr"
    assert EJECT_RE.match("deplacement 3")
    explorer = Explorer(3, 2)
    assert [explorer.next_command() for _ in range(5)] == [
        "Forward",
        "Forward",
        "Right",
        "Forward",
        "Right",
    ]
    print("self-test passed")
    return 0


def validate_args(args: argparse.Namespace) -> None:
    if not 1 <= args.port <= 65535:
        raise SystemExit("port must be between 1 and 65535")
    if args.players < 1:
        raise SystemExit("players must be at least 1")
    if not 0 <= args.worker_id < args.players:
        raise SystemExit("worker-id must be in [0, players)")
    if "\n" in args.team or "\r" in args.team or "|" in args.team:
        raise SystemExit("team must not contain newlines or '|'")
    if args.food_emergency < 1:
        raise SystemExit("food-emergency must be positive")
    if args.meet_food <= args.food_emergency:
        raise SystemExit("meet-food must be greater than food-emergency")
    if args.spawn_delay < 0:
        raise SystemExit("spawn-delay must not be negative")
    if args.fork_poll_interval <= 0:
        raise SystemExit("fork-poll-interval must be positive")
    if args.fork_timeout <= 0:
        raise SystemExit("fork-timeout must be positive")


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.self_test:
        return self_test()
    validate_args(args)
    return Agent(args).run()


if __name__ == "__main__":
    raise SystemExit(main())
