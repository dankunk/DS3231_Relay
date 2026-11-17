#!/usr/bin/env python3
"""
sync_rtc.py: set the DS3231 on the Arduino relay timer from this computer over USB.

    py sync_rtc.py                     # find the Arduino, check health, set the clock
    py sync_rtc.py --check             # only measure and report, don't change anything
    py sync_rtc.py --battery-replaced  # also record that the coin cell was replaced today

Where the time comes from:
    1. An internet time server (NTP) if this computer can reach one: accurate to a few ms.
    2. Otherwise this computer's own clock (with a warning).

How USB delay is handled:
    The script measures the USB round trip with PING, sends "SET <time>" early,
    and sends the final newline at (target second - one-way delay). The Arduino
    writes the RTC the instant that newline arrives. Afterwards it measures the
    RTC's remaining error with TICK, so you see exactly how close it got
    (typically within a few milliseconds).

Requires pyserial:   py -m pip install pyserial
"""

import argparse
import datetime as dt
import math
import socket
import statistics
import struct
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:  # pragma: no cover
    print("The 'pyserial' package is missing. Install it with:\n\n    py -m pip install pyserial\n")
    sys.exit(2)

BAUD = 57600
BYTE_TIME = 10.0 / BAUD          # seconds to send one byte over the Arduino's UART
BATTERY_LIFE_YEARS = 3           # recommended CR2032 replacement interval
DRIFT_WARN_PPM = 5.0             # a healthy DS3231 is within about +/-2 ppm
NTP_SERVERS = ["time.google.com", "time.cloudflare.com", "pool.ntp.org", "time.nist.gov", "time.windows.com"]
NTP_EPOCH = 2208988800           # seconds from 1900-01-01 to 1970-01-01

# USB vendor IDs of common Arduino boards and USB-serial chips
KNOWN_VIDS = {0x2341: "Arduino", 0x2A03: "Arduino", 0x1A86: "CH340", 0x0403: "FTDI", 0x10C4: "CP210x"}


class SyncError(Exception):
    pass


# ───────────────────────── reference clock ─────────────────────────
class RefClock:
    """This computer's clock plus a correction (from NTP), read via a
    high-resolution counter so it can't jump during the sync."""

    def __init__(self, offset=0.0, source="this computer's clock", uncertainty=None):
        self.offset, self.source, self.uncertainty = offset, source, uncertainty
        self._pc0, self._t0 = time.perf_counter(), time.time()

    def at(self, pc):
        return self._t0 + (pc - self._pc0) + self.offset

    def now(self):
        return self.at(time.perf_counter())


def sntp_query(server, timeout=2.0):
    """One SNTP request. Returns (offset, round_trip_delay) in seconds."""
    packet = b"\x23" + 47 * b"\0"  # LI=0, version 4, mode 3 (client)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.settimeout(timeout)
        addr = socket.getaddrinfo(server, 123, socket.AF_INET, socket.SOCK_DGRAM)[0][4]
        t1 = time.time()
        s.sendto(packet, addr)
        data, _ = s.recvfrom(512)
        t4 = time.time()
    if len(data) < 48 or data[1] == 0:
        raise OSError("bad NTP reply")

    def ts(off):
        sec, frac = struct.unpack("!II", data[off:off + 8])
        return sec - NTP_EPOCH + frac / 2**32

    t2, t3 = ts(32), ts(40)
    return ((t2 - t1) + (t3 - t4)) / 2, (t4 - t1) - (t3 - t2)


def get_reference_clock(use_ntp=True):
    if use_ntp:
        samples = []
        for server in NTP_SERVERS:
            for _ in range(3):
                try:
                    off, delay = sntp_query(server)
                    samples.append((delay, off, server))
                except OSError:
                    break
            if len(samples) >= 3:
                break
        if samples:
            delay, off, server = min(samples)  # lowest network delay = most trustworthy
            return RefClock(off, f"internet time ({server})", uncertainty=delay / 2)
        print("[!!] Could not reach an internet time server (no internet, or UDP port 123 blocked).")
    print("[!!] Using this computer's clock. Make sure it is correct:")
    print("     Windows: Settings > Time & language > Date & time > 'Sync now'.")
    return RefClock(0.0, "this computer's clock (not verified)", uncertainty=None)


# ───────────────────────── Arduino link ─────────────────────────
def find_port():
    ports = list(list_ports.comports())
    matches = [p for p in ports if p.vid in KNOWN_VIDS]
    if len(matches) == 1:
        return matches[0].device
    if not matches:
        listing = "\n".join(f"    {p.device}  {p.description}" for p in ports) or "    (none)"
        raise SyncError("No Arduino found. Is the USB cable plugged in? Serial ports seen:\n" + listing +
                        "\nIf your Arduino is listed, run again with --port COMx")
    listing = "\n".join(f"    {p.device}  {p.description}" for p in matches)
    raise SyncError("More than one Arduino-like port found. Pick one with --port:\n" + listing)


def open_serial(port):
    ser = serial.Serial()
    ser.port, ser.baudrate, ser.timeout = port, BAUD, 0.05
    # Try not to reset the Arduino when the port opens (works on many boards;
    # if it does reset, we simply wait for it to boot).
    ser.dtr = False
    ser.rts = False
    ser.open()
    return ser


class Device:
    def __init__(self, ser, verbose=False):
        self.ser, self.verbose = ser, verbose

    def send(self, text):
        self.ser.write(text.encode("ascii"))
        self.ser.flush()

    def read_reply(self, prefix, timeout):
        """Read lines until one starts with '@<prefix>' (or '@ERR').
        Returns (line, perf_counter at arrival)."""
        deadline = time.perf_counter() + timeout
        buf = b""
        while time.perf_counter() < deadline:
            chunk = self.ser.readline()
            if not chunk:
                continue
            buf += chunk
            if not buf.endswith(b"\n"):
                continue
            arrived = time.perf_counter()
            line = buf.decode("ascii", "replace").strip()
            buf = b""
            if line.startswith("@" + prefix):
                return line, arrived
            if line.startswith("@ERR"):
                raise SyncError("Arduino says: " + line[1:])
            if self.verbose and line:
                print("     arduino>", line)
        raise SyncError(f"No '{prefix}' reply from the Arduino.")

    def wait_ready(self):
        """Wait for the sketch to answer (it may be rebooting after the port opened)."""
        try:
            self.read_reply("READY", 2.5)
        except SyncError as e:
            if "Arduino says" in str(e):
                raise
        for _ in range(3):
            try:
                self.ser.reset_input_buffer()
                self.send("PING\n")
                self.read_reply("PONG", 1.0)
                return
            except SyncError as e:
                if "Arduino says" in str(e):
                    raise
                time.sleep(0.5)
        raise SyncError("The Arduino isn't answering. Is ds3231_trigger_relay (v2) uploaded to it?\n"
                        "Also close the Arduino IDE's Serial Monitor; only one program can use the port.")

    def status(self):
        line, _ = self.read_cmd("STATUS\n", "STATUS", 3.0)
        info = {}
        for part in line.split()[1:]:
            key, _, value = part.partition("=")
            info[key] = value
        return info

    def read_cmd(self, cmd, prefix, timeout):
        self.send(cmd)
        return self.read_reply(prefix, timeout)


def measure_latency(dev, n=12):
    """One-way USB+UART latency, estimated as half the PING round trip
    (minus the time to clock the bytes through the UART)."""
    rtts = []
    for _ in range(n):
        t = time.perf_counter()
        _, arrived = dev.read_cmd("PING\n", "PONG", 1.0)
        rtts.append(arrived - t)
    rtt = statistics.median(rtts)
    one_way = max(0.0, (rtt - (len("PING\n") + len("@PONG\r\n")) * BYTE_TIME) / 2)
    return one_way, rtt


def measure_rtc_error(dev, clock, one_way, n=3):
    """How far ahead (+) or behind (-) of true time the RTC is, in seconds.
    The Arduino reports the instant its seconds roll over; we timestamp that."""
    errors = []
    for _ in range(n):
        line, arrived = dev.read_cmd("TICK\n", "TICK", 3.0)
        rtc_second = int(line.split()[1])
        sent_at = clock.at(arrived) - (one_way + (len(line) + 2) * BYTE_TIME)
        errors.append(rtc_second - sent_at)
    return statistics.median(errors)


def wait_until(clock, target):
    while True:
        remaining = target - clock.now()
        if remaining <= 0:
            return
        if remaining > 0.03:
            time.sleep(remaining - 0.02)  # coarse sleep, then spin for precision


def set_rtc(dev, clock, one_way):
    target = math.floor(clock.now()) + 2
    dev.send(f"SET {target}")                         # everything except the newline…
    wait_until(clock, target - (one_way + BYTE_TIME))
    dev.send("\n")                                    # …which lands exactly on the second
    dev.read_reply("OK SET", 3.0)
    return target


# ───────────────────────── reporting helpers ─────────────────────────
def utc_str(t):
    return dt.datetime.fromtimestamp(t, dt.timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")


def fmt_err(seconds):
    if abs(seconds) < 1:
        return f"{seconds * 1000:+.0f} ms"
    if abs(seconds) < 120:
        return f"{seconds:+.2f} s"
    return f"{seconds:+.0f} s ({seconds / 3600:+.2f} h)"


def parse_date(text):
    if text.lower() == "today":
        return int(time.time())
    d = dt.datetime.strptime(text, "%Y-%m-%d").replace(hour=12, tzinfo=dt.timezone.utc)
    return int(d.timestamp())


def battery_report(battery_utc, now, lost_power):
    if lost_power:
        print("[!!] REPLACE THE COIN CELL NOW: the RTC lost all power since its last sync.")
        print("     After replacing it, run:  py sync_rtc.py --battery-replaced")
        return
    if not battery_utc:
        print("[--] Coin cell install date unknown. Next time you replace it, run with --battery-replaced.")
        return
    age_years = (now - battery_utc) / (365.25 * 86400)
    due = battery_utc + BATTERY_LIFE_YEARS * 365.25 * 86400
    due_str = dt.datetime.fromtimestamp(due, dt.timezone.utc).strftime("%Y-%m-%d")
    if age_years >= BATTERY_LIFE_YEARS:
        print(f"[!!] Coin cell is {age_years:.1f} years old: replace it (then run with --battery-replaced).")
    else:
        print(f"[OK] Coin cell is {age_years:.1f} years old. Replace by about {due_str}.")


# ───────────────────────── main ─────────────────────────
def run(args, ser=None):
    print("=== DS3231 relay timer: clock sync ===\n")
    clock = get_reference_clock(use_ntp=not args.no_ntp)
    unc = f" (+/-{clock.uncertainty * 1000:.0f} ms)" if clock.uncertainty is not None else ""
    print(f"Time source: {clock.source}{unc}")
    print(f"This computer's clock is off by {fmt_err(-clock.offset)}" if clock.uncertainty is not None else "")

    if ser is None:
        port = args.port or find_port()
        print(f"Arduino port: {port}")
        ser = open_serial(port)
    dev = Device(ser, verbose=args.verbose)
    try:
        dev.wait_ready()
        before = dev.status()
        if not before.get("fw", "").startswith("2"):
            print(f"[!!] Arduino sketch version {before.get('fw')} is unexpected; upload the v2 sketch.")

        one_way, rtt = measure_latency(dev)
        print(f"USB round trip: {rtt * 1000:.1f} ms (compensated automatically)\n")

        # ---- health check (before touching anything) ----
        print("--- Before sync ---")
        lost_power = before.get("lost_power") == "1"
        now = clock.now()
        try:
            err_before = measure_rtc_error(dev, clock, one_way)
        except SyncError:
            err_before = None
        print(f"RTC time:        {utc_str(int(before.get('utc', 0)))}")
        print(f"Lost-power flag: {'SET' if lost_power else 'clear'}")
        print(f"RTC temperature: {before.get('temp')} C")
        last_sync = int(before.get("last_sync", 0) or 0)
        if err_before is None:
            print("[!!] RTC seconds aren't ticking: oscillator stopped. Setting it will restart it.")
        elif lost_power or before.get("valid") != "1":
            print(f"RTC error:       {fmt_err(err_before)} (clock was not set / lost power)")
        else:
            print(f"RTC error:       {fmt_err(err_before)}")
            whole_h = round(err_before / 3600)
            if whole_h and abs(err_before - whole_h * 3600) < 120:
                print(f"[--] That's about {whole_h:+d} h: the RTC was holding LOCAL time (old Pico method).")
                print("     Expected once. From now on it holds UTC and the Arduino handles DST.")
            elif last_sync:
                days = (now - last_sync) / 86400
                ppm = err_before / (now - last_sync) * 1e6 if now > last_sync else 0
                flag = "[!!]" if abs(ppm) > DRIFT_WARN_PPM else "[OK]"
                print(f"{flag} Drift since last sync ({days:.0f} days ago): {ppm:+.2f} ppm "
                      f"(~{abs(ppm) * 31.5576:.0f} s/year; a healthy DS3231 is within +/-2 ppm)")

        if args.check:
            print()
            battery_report(int(before.get("battery", 0) or 0), now, lost_power)
            print("\n--check: nothing was changed.")
            return 0

        # ---- set ----
        print("\n--- Setting clock ---")
        target = set_rtc(dev, clock, one_way)
        print(f"RTC set to {utc_str(target)}")
        err_after = measure_rtc_error(dev, clock, one_way)
        bound = rtt / 2 + (clock.uncertainty or 0)
        print(f"[OK] Measured RTC error after sync: {fmt_err(err_after)}  "
              f"(worst case about +/-{bound * 1000:.0f} ms incl. USB timing"
              f"{'' if clock.uncertainty is not None else ', plus any error in this computer clock'})")

        # ---- battery date ----
        battery = int(before.get("battery", 0) or 0)
        new_battery = None
        if args.battery_replaced:
            new_battery = parse_date(args.battery_replaced)
        elif not battery and sys.stdin.isatty():
            try:
                ans = input("\nWhen was the coin cell installed? (YYYY-MM-DD, 'today', or Enter to skip): ").strip()
            except EOFError:
                ans = ""
            if ans:
                try:
                    new_battery = parse_date(ans)
                except ValueError:
                    print("[--] Didn't understand that date; skipped.")
        if new_battery:
            dev.read_cmd(f"BATTERY {new_battery}\n", "OK BATTERY", 3.0)
            battery = new_battery
            lost_power = False  # the warning was about the old cell

        # ---- confirm what the Arduino now believes ----
        after = dev.status()
        print("\n--- Arduino now reports ---")
        if after.get("valid") != "1":
            raise SyncError("The Arduino still reports the clock as invalid. Check the DS3231 wiring.")
        off = int(after["offset"])
        print(f"Local time:   {after['local'].replace('T', ' ')} (UTC{off:+d}, "
              f"{'daylight saving' if after.get('dst') == '1' else 'standard time'})")
        if "next_change_local" in after:
            print(f"Next change:  {after['next_change_local'].replace('T', ' ')} local (automatic)")
        here = dt.datetime.now().astimezone().utcoffset().total_seconds() / 3600
        if here == off:
            print("[OK] Matches this computer's time zone.")
        else:
            print(f"[--] This computer is on UTC{here:+g}. Fine if it isn't set to Mountain time; "
                  f"otherwise check STD_OFFSET_HOURS in the sketch.")
        print()
        battery_report(battery, clock.now(), lost_power)
        print("\nDone. The relay schedule is running. You can unplug the laptop.")
        return 0
    finally:
        ser.close()


def main(argv=None):
    p = argparse.ArgumentParser(description="Set the DS3231 on the Arduino relay timer over USB.")
    p.add_argument("--port", help="serial port, e.g. COM3 or /dev/ttyACM0 (default: auto-detect)")
    p.add_argument("--check", action="store_true", help="only measure and report; don't set the clock")
    p.add_argument("--no-ntp", action="store_true", help="use this computer's clock instead of internet time")
    p.add_argument("--battery-replaced", nargs="?", const="today", metavar="YYYY-MM-DD",
                   help="record that the coin cell was replaced (today, or on the given date)")
    p.add_argument("--list-ports", action="store_true", help="list serial ports and exit")
    p.add_argument("--verbose", action="store_true", help="show the Arduino's log lines too")
    args = p.parse_args(argv)

    if args.list_ports:
        for port in list_ports.comports():
            tag = KNOWN_VIDS.get(port.vid, "")
            print(f"{port.device:10} {port.description} {('<- ' + tag) if tag else ''}")
        return 0
    try:
        return run(args)
    except SyncError as e:
        print(f"\n[FAILED] {e}")
        return 1
    except serial.SerialException as e:
        print(f"\n[FAILED] Serial port problem: {e}\n"
              "Close the Arduino IDE Serial Monitor (only one program can use the port) and try again.")
        return 1


if __name__ == "__main__":
    sys.exit(main())
