#!/usr/bin/env python3
"""
Drake Tail — USB serial settings TUI
====================================
Talks the same command language as the phone app (BLE NUS).

  python3 tools/tail_tui.py
  python3 tools/tail_tui.py -p /dev/cu.usbserial-0001

Requires Tail firmware with checkSerialBT() (USB → processBLECommand).
"""

from __future__ import annotations

import argparse
import re
import sys
import threading
import time
from collections import deque
from dataclasses import dataclass, field
from typing import Deque, Dict, Optional

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Install deps:  pip3 install -r tools/requirements.txt")
    sys.exit(1)

try:
    from rich.console import Console, Group
    from rich.layout import Layout
    from rich.live import Live
    from rich.panel import Panel
    from rich.table import Table
    from rich.text import Text
    from rich import box
except ImportError:
    print("Install deps:  pip3 install -r tools/requirements.txt")
    sys.exit(1)

# ── modes / themes (match firmware + app) ───────────────────────────────────

MODES = [
    (0, "Sound Phase"),
    (1, "Sound Pulse"),
    (2, "VU Meter"),
    (3, "Rainbow"),
    (4, "Comet"),
    (5, "Breathe"),
    (6, "Dragonfire"),
    (7, "Sparkle"),
    (8, "Wave"),
    (9, "Solid"),
    (10, "Blackout"),
]

THEMES = [
    (0, "purple", "157,78,221"),
    (1, "fire", "255,60,0"),
    (2, "ice", "80,180,255"),
    (3, "gold", "255,180,40"),
    (4, "emerald", "20,200,100"),
]

FAN_MODES = {0: "Off", 1: "On", 2: "Auto"}


@dataclass
class SuitState:
    mode: int = 0
    brightness: int = 80
    speed: int = 50
    sensitivity: int = 75
    gate: int = 100
    gain: int = 100
    sound: int = 1
    color: str = "150,0,255"
    theme: int = 0
    mic: int = 0
    head_b: int = 0
    head_t: float = 0.0
    uptime: int = 0
    seq: int = 0
    fan: int = 2
    log: Deque[str] = field(default_factory=lambda: deque(maxlen=12))
    connected: bool = False
    port: str = ""
    last_stat: float = 0.0
    raw_line: str = ""


def parse_stat(line: str, st: SuitState) -> None:
    """Parse STAT M:… B:… tokens from firmware."""
    if "STAT" not in line:
        return
    # strip [BLE] prefix if present
    if "STAT" in line:
        line = line[line.index("STAT") :]
    for part in line.split():
        if ":" not in part:
            continue
        k, _, v = part.partition(":")
        try:
            if k == "M":
                st.mode = int(v)
            elif k == "B":
                st.brightness = int(v)
            elif k == "V":
                st.speed = int(v)
            elif k == "S":
                st.sensitivity = int(v)
            elif k == "G":
                st.gate = int(v)
            elif k == "A":
                st.gain = int(v)
            elif k == "E":
                st.sound = int(v)
            elif k == "C":
                st.color = v
            elif k == "T":
                st.theme = int(v)
            elif k == "Mic":
                st.mic = int(float(v))
            elif k == "HeadB":
                st.head_b = int(float(v))
            elif k == "HeadT":
                st.head_t = float(v)
            elif k == "U":
                st.uptime = int(float(v))
            elif k == "Seq":
                st.seq = int(v)
        except ValueError:
            pass
    st.last_stat = time.time()
    st.raw_line = line.strip()[:80]


def list_serial_ports() -> list:
    ports = list(list_ports.comports())
    # Prefer USB serial (CP2102 / CH340), skip Bluetooth
    def score(p):
        d = (p.device + " " + (p.description or "")).lower()
        if "bluetooth" in d:
            return 99
        if "usbserial" in d or "usbmodem" in d or "wch" in d or "cp210" in d or "ch340" in d:
            return 0
        return 1

    ports.sort(key=score)
    return ports


def pick_port(cli_port: Optional[str]) -> str:
    if cli_port:
        return cli_port
    ports = list_serial_ports()
    if not ports:
        print("No serial ports found. Plug in the Tail USB cable.")
        sys.exit(1)
    # Auto-pick first non-bluetooth
    for p in ports:
        if "bluetooth" not in p.device.lower():
            return p.device
    return ports[0].device


class TailLink:
    def __init__(self, port: str, baud: int = 115200):
        self.port = port
        self.baud = baud
        self.ser: Optional[serial.Serial] = None
        self.st = SuitState(port=port)
        self._rx_thread: Optional[threading.Thread] = None
        self._stop = threading.Event()
        self._lock = threading.Lock()

    def open(self) -> None:
        self.ser = serial.Serial(self.port, self.baud, timeout=0.05)
        time.sleep(0.3)
        self.ser.reset_input_buffer()
        self.st.connected = True
        self.st.log.append(f"opened {self.port} @ {self.baud}")
        self._stop.clear()
        self._rx_thread = threading.Thread(target=self._reader, daemon=True)
        self._rx_thread.start()
        self.send("?")
        self.send("HB")

    def close(self) -> None:
        self._stop.set()
        if self._rx_thread:
            self._rx_thread.join(timeout=1.0)
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.st.connected = False

    def send(self, cmd: str) -> None:
        if not self.ser or not self.ser.is_open:
            return
        line = (cmd.strip() + "\n").encode("utf-8")
        try:
            self.ser.write(line)
            self.ser.flush()
            with self._lock:
                self.st.log.append(f"→ {cmd.strip()}")
        except serial.SerialException as e:
            with self._lock:
                self.st.log.append(f"send error: {e}")
                self.st.connected = False

    def _reader(self) -> None:
        buf = ""
        while not self._stop.is_set():
            try:
                if not self.ser or not self.ser.is_open:
                    time.sleep(0.1)
                    continue
                n = self.ser.in_waiting
                if n:
                    chunk = self.ser.read(n).decode("utf-8", errors="replace")
                    buf += chunk
                    while "\n" in buf:
                        line, buf = buf.split("\n", 1)
                        line = line.strip("\r")
                        if not line:
                            continue
                        with self._lock:
                            if "STAT" in line:
                                parse_stat(line, self.st)
                            # keep interesting log lines short
                            show = line
                            if show.startswith("[BLE]"):
                                show = show[5:].strip()
                            if len(show) > 70:
                                show = show[:67] + "…"
                            if any(
                                k in show
                                for k in (
                                    "STAT",
                                    "HBACK",
                                    "Mode",
                                    "Suit sync",
                                    "ESP-NOW",
                                    "Color",
                                    "Theme",
                                    "Brightness",
                                    "→",
                                )
                            ):
                                self.st.log.append(show)
                else:
                    time.sleep(0.03)
            except serial.SerialException:
                with self._lock:
                    self.st.connected = False
                    self.st.log.append("serial disconnected")
                break


def bar(val: int, lo: int, hi: int, width: int = 16) -> str:
    if hi <= lo:
        return "·" * width
    t = max(0.0, min(1.0, (val - lo) / (hi - lo)))
    n = int(round(t * width))
    return "█" * n + "·" * (width - n)


def build_ui(st: SuitState, help_hint: str) -> Layout:
    layout = Layout()
    layout.split_column(
        Layout(name="header", size=3),
        Layout(name="body", ratio=1),
        Layout(name="footer", size=4),
    )
    layout["body"].split_row(
        Layout(name="modes", ratio=2),
        Layout(name="settings", ratio=2),
        Layout(name="status", ratio=2),
    )

    # Header
    link = "[bold green]USB LINK[/]" if st.connected else "[bold red]NO LINK[/]"
    age = ""
    if st.last_stat:
        age = f"  STAT {time.time() - st.last_stat:.1f}s ago"
    header = Text.from_markup(
        f"[bold magenta]🐉 Drake Tail TUI[/]  {link}  [dim]{st.port}[/]{age}"
    )
    layout["header"].update(Panel(header, style="magenta", box=box.ROUNDED))

    # Modes
    mt = Table(show_header=True, header_style="bold cyan", box=box.SIMPLE, expand=True)
    mt.add_column("#", width=3)
    mt.add_column("Mode")
    mt.add_column("", width=2)
    for mid, name in MODES:
        mark = "▶" if mid == st.mode else ""
        style = "bold white on purple" if mid == st.mode else ""
        mt.add_row(str(mid), name, mark, style=style)
    layout["modes"].update(Panel(mt, title="Modes  [0-9] [a]=10", border_style="cyan"))

    # Settings
    stbl = Table(show_header=False, box=box.SIMPLE, expand=True)
    stbl.add_column("Key", style="yellow", width=6)
    stbl.add_column("Setting")
    stbl.add_column("Value", justify="right")
    stbl.add_column("Bar")
    stbl.add_row("B±", "Brightness", f"{st.brightness}", bar(st.brightness, 0, 100))
    stbl.add_row("V±", "Speed", f"{st.speed}", bar(st.speed, 0, 100))
    stbl.add_row("S±", "Mic sensitivity", f"{st.sensitivity}", bar(st.sensitivity, -100, 200))
    stbl.add_row("G±", "Mic gate", f"{st.gate}", bar(st.gate, 10, 500))
    stbl.add_row("A±", "Mic gain %", f"{st.gain}", bar(st.gain, 50, 300))
    stbl.add_row("E", "Sound detect", "ON" if st.sound else "OFF", "")
    stbl.add_row("F", f"Fan {FAN_MODES.get(st.fan, '?')}", f"F{st.fan}", "")
    th = next((n for i, n, _ in THEMES if i == st.theme), "?")
    stbl.add_row("T", "Theme", f"{st.theme} {th}", "")
    stbl.add_row("C", "Color RGB", st.color, "")
    layout["settings"].update(Panel(stbl, title="Settings", border_style="yellow"))

    # Status + log
    s2 = Table(show_header=False, box=box.SIMPLE, expand=True)
    s2.add_column("k", style="dim", width=8)
    s2.add_column("v")
    s2.add_row("Mic", f"{st.mic}  {bar(st.mic, 0, 800, 12)}")
    s2.add_row("HeadT", f"{st.head_t:.1f} °F")
    s2.add_row("HeadB", f"{st.head_b}")
    s2.add_row("Uptime", f"{st.uptime}s")
    s2.add_row("HB Seq", f"{st.seq}")
    log_txt = "\n".join(st.log) if st.log else "[dim](log empty)[/]"
    layout["status"].update(
        Panel(
            Group(s2, Text(""), Text.from_markup(f"[dim]{log_txt}[/]")),
            title="Live / Log",
            border_style="green",
        )
    )

    # Footer help
    help_text = (
        "[bold]Keys[/]  "
        "0-9/a mode · [ / ] bright · ; / ' speed · e sound · "
        "t theme · f fan · l flash · r resync · h HB · ? help · "
        "q quit\n"
        f"[dim]{help_hint}[/]"
    )
    layout["footer"].update(Panel(help_text, border_style="dim", box=box.ROUNDED))
    return layout


def run_tui(port: str) -> None:
    console = Console()
    link = TailLink(port)
    try:
        link.open()
    except serial.SerialException as e:
        console.print(f"[red]Cannot open {port}: {e}[/]")
        console.print("Close any [cyan]screen[/] / Serial Monitor using the port first.")
        sys.exit(1)

    # Keyboard: Unix termios raw-ish
    import tty
    import termios
    import select as sel

    fd = sys.stdin.fileno()
    old = termios.tcgetattr(fd)
    hint = "Polling HB every 2s · same cmds as phone app"
    last_hb = 0.0

    # shift/adjust helpers
    def adj(attr: str, delta: int, lo: int, hi: int, prefix: str) -> None:
        with link._lock:
            cur = getattr(link.st, attr)
            cur = max(lo, min(hi, cur + delta))
            setattr(link.st, attr, cur)
            val = cur
        link.send(f"{prefix}{val}")

    try:
        tty.setcbreak(fd)
        with Live(build_ui(link.st, hint), console=console, refresh_per_second=8) as live:
            while True:
                now = time.time()
                if now - last_hb >= 2.0:
                    link.send("HB")
                    last_hb = now

                # non-blocking key
                if sel.select([sys.stdin], [], [], 0.05)[0]:
                    ch = sys.stdin.read(1)
                    if not ch:
                        continue
                    if ch in ("q", "Q", "\x03"):  # q or Ctrl-C
                        break
                    if ch in "0123456789":
                        link.send(f"M{ch}")
                    elif ch in ("a", "A"):
                        link.send("M10")
                    elif ch == "[":
                        adj("brightness", -5, 0, 100, "B")
                    elif ch == "]":
                        adj("brightness", 5, 0, 100, "B")
                    elif ch == ";":
                        adj("speed", -5, 0, 100, "V")
                    elif ch == "'":
                        adj("speed", 5, 0, 100, "V")
                    elif ch == ",":
                        adj("sensitivity", -10, -500, 4000, "S")
                    elif ch == ".":
                        adj("sensitivity", 10, -500, 4000, "S")
                    elif ch == "e":
                        with link._lock:
                            on = link.st.sound
                        link.send("e" if on else "E")
                    elif ch in ("t", "T"):
                        with link._lock:
                            nt = (link.st.theme + 1) % 5
                        link.send(f"T{nt}")
                    elif ch in ("f", "F"):
                        with link._lock:
                            nf = (link.st.fan + 1) % 3
                            link.st.fan = nf
                        link.send(f"F{nf}")
                    elif ch in ("l", "L"):
                        link.send("L")
                    elif ch in ("r", "R"):
                        link.send("R")
                    elif ch in ("h", "H"):
                        link.send("HB")
                    elif ch == "?":
                        link.send("?")
                    elif ch == "z":
                        # reboot — require shift-ish: only lowercase z with confirm via double
                        link.send("Z")
                        hint = "Reboot sent (Z)"
                    elif ch == "p":
                        # purple original solid
                        link.send("C150,0,255")
                    elif ch == "1" and False:
                        pass

                with link._lock:
                    # copy for render
                    snap = SuitState(
                        mode=link.st.mode,
                        brightness=link.st.brightness,
                        speed=link.st.speed,
                        sensitivity=link.st.sensitivity,
                        gate=link.st.gate,
                        gain=link.st.gain,
                        sound=link.st.sound,
                        color=link.st.color,
                        theme=link.st.theme,
                        mic=link.st.mic,
                        head_b=link.st.head_b,
                        head_t=link.st.head_t,
                        uptime=link.st.uptime,
                        seq=link.st.seq,
                        fan=link.st.fan,
                        log=deque(link.st.log, maxlen=12),
                        connected=link.st.connected,
                        port=link.st.port,
                        last_stat=link.st.last_stat,
                        raw_line=link.st.raw_line,
                    )
                live.update(build_ui(snap, hint))
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old)
        link.close()
        console.print("[dim]Tail TUI closed.[/]")


def main() -> None:
    ap = argparse.ArgumentParser(description="Drake Tail USB settings TUI")
    ap.add_argument("-p", "--port", help="Serial port (default: auto USB)")
    ap.add_argument("-l", "--list", action="store_true", help="List ports and exit")
    args = ap.parse_args()

    if args.list:
        for p in list_serial_ports():
            print(f"{p.device:30}  {p.description}")
        return

    port = pick_port(args.port)
    print(f"Opening {port} … (close other serial monitors first)")
    run_tui(port)


if __name__ == "__main__":
    main()
