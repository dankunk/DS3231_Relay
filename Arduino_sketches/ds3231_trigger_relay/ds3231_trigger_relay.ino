// ds3231_trigger_relay.ino
//
// Turns a Digital Loggers IoT Relay (or any relay) on/off on a daily schedule
// using the time from a DS3231 real-time clock that stays wired to this Arduino.
//
// SETTING THE CLOCK: plug a laptop into the Arduino's USB port and run
//   laptop_sync/sync_rtc.py
// It sets the DS3231 to UTC (accurate to a few milliseconds), and records the
// sync date. No Wi-Fi or extra boards needed.
//
// DAYLIGHT SAVING: the DS3231 holds UTC. This sketch converts UTC → local time
// and applies US daylight saving time automatically, so the schedule below is
// always normal "wall clock" time. Never edit it for March/November.
//
// Status LED (the Arduino's built-in "L" LED, pin 13 on an Uno):
//   short blink every 2 s → running normally
//   fast blinking         → clock not set / RTC lost power / RTC unplugged.
//                           The relay is held in FAILSAFE_STATE until you
//                           run sync_rtc.py.
//
// Serial (57600 baud, newline line ending). Type these in the Serial Monitor,
// or let sync_rtc.py do it:
//   STATUS          show time, DST, RTC health, last sync, battery age
//   HELP            list commands
//   (PING, TICK, SET <unix>, BATTERY <unix> are used by sync_rtc.py)

#include <Wire.h>
#include <EEPROM.h>
#include "RTClib.h"

// ——— time-zone settings ———
const int8_t STD_OFFSET_HOURS = -7;   // standard-time offset from UTC. Colorado (MST) = -7
const bool   OBSERVE_DST      = true; // true = follow US daylight saving rules (MDT = -6)

// ——— your schedule (LOCAL wall-clock time — DST is handled for you) ———
// { hour, minute, second, GPIO pin, state }
// HIGH = relay control ON (Digital Loggers "normally OFF" outlets turn ON)
// LOW  = relay control OFF
// Events can be in any order. Overnight schedules work too (e.g. ON 22:00,
// OFF 06:00) because the sketch always applies the most recent event.
struct Event {
  uint8_t hour, minute, second;
  uint8_t pin;
  uint8_t state;
};

Event events[] = {
  {  0,  0,  0,  8, LOW  },   // at 00:00:00 → pin 8 LOW  (lights off)
  {  8,  0,  0,  8, HIGH },   // at 08:00:00 → pin 8 HIGH (lights on)
  // …add more if you like…
};
const uint8_t numEvents = sizeof(events) / sizeof(events[0]);

// ——— safety settings ———
// State for the relay pins if the RTC time can't be trusted.
// LOW = lights off. Change to HIGH if your plants are better off with lights on.
const uint8_t FAILSAFE_STATE = LOW;
const uint8_t STATUS_LED     = LED_BUILTIN;

// —————————————————————————————————————————————————————————————————————
#define FW_VERSION "2.0"
const uint32_t MIN_VALID_UTC = 1704067200UL;  // 2024-01-01
const uint32_t PERSIST_MAGIC = 0xD5323102UL;

// Saved in the Arduino's EEPROM so it survives power cuts and re-uploads.
struct Persist {
  uint32_t magic;
  uint32_t lastSyncUtc;     // when sync_rtc.py last set the clock (0 = never)
  uint32_t batteryUtc;      // when the coin cell was installed (0 = unknown)
};

RTC_DS3231 rtc;
Persist persist;
int16_t lastApplied[numEvents];  // last state written, per pin (-1 = never)
uint32_t lastCheckedUtc = 0;
uint32_t lastRtcReadMs = 0;
uint32_t quietUntilMs = 0;       // suppress log chatter while the laptop is syncing
bool rtcOk = false;
char cmdBuf[40];
uint8_t cmdLen = 0;

// ——— US daylight saving rules ———
// DST starts the 2nd Sunday of March at 02:00 local standard time and
// ends the 1st Sunday of November at 02:00 local daylight time.
uint8_t nthSunday(uint16_t year, uint8_t month, uint8_t n) {
  uint8_t dow = DateTime(year, month, 1).dayOfTheWeek();  // 0 = Sunday
  return 1 + (7 - dow) % 7 + 7 * (n - 1);
}

uint32_t dstStartUtc(uint16_t year) {
  return DateTime(year, 3, nthSunday(year, 3, 2), 2, 0, 0).unixtime()
         - (int32_t)STD_OFFSET_HOURS * 3600L;
}

uint32_t dstEndUtc(uint16_t year) {
  return DateTime(year, 11, nthSunday(year, 11, 1), 2, 0, 0).unixtime()
         - (int32_t)(STD_OFFSET_HOURS + 1) * 3600L;
}

bool isDST(uint32_t utc) {
  if (!OBSERVE_DST) return false;
  uint16_t y = DateTime(utc).year();
  return utc >= dstStartUtc(y) && utc < dstEndUtc(y);
}

int8_t offsetHours(uint32_t utc) {
  return STD_OFFSET_HOURS + (isDST(utc) ? 1 : 0);
}

DateTime toLocal(uint32_t utc) {
  return DateTime(utc + (int32_t)offsetHours(utc) * 3600L);
}

uint32_t nextClockChangeUtc(uint32_t utc) {
  uint16_t y = DateTime(utc).year();
  if (utc < dstStartUtc(y)) return dstStartUtc(y);
  if (utc < dstEndUtc(y))   return dstEndUtc(y);
  return dstStartUtc(y + 1);
}

// ——— printing helpers ———
bool chatty() { return (int32_t)(millis() - quietUntilMs) >= 0; }

void print2(uint8_t v) {
  if (v < 10) Serial.print('0');
  Serial.print(v);
}

void printDateTime(const DateTime &t, char sep = ' ') {
  Serial.print(t.year()); Serial.print('-');
  print2(t.month());      Serial.print('-');
  print2(t.day());        Serial.print(sep);
  print2(t.hour());       Serial.print(':');
  print2(t.minute());     Serial.print(':');
  print2(t.second());
}

void printZone(uint32_t utc) {
  int8_t off = offsetHours(utc);
  Serial.print(F(" (UTC"));
  if (off >= 0) Serial.print('+');
  Serial.print(off);
  Serial.print(isDST(utc) ? F(", DST)") : F(", standard)"));
}

void printStatusLine(uint32_t utc) {
  Serial.print(F("UTC "));
  printDateTime(DateTime(utc));
  Serial.print(F(" | local "));
  printDateTime(toLocal(utc));
  printZone(utc);
  Serial.println();
}

// ——— saved settings ———
void loadPersist() {
  EEPROM.get(0, persist);
  if (persist.magic != PERSIST_MAGIC) {
    persist.magic = PERSIST_MAGIC;
    persist.lastSyncUtc = 0;
    persist.batteryUtc = 0;
  }
}

void savePersist() { EEPROM.put(0, persist); }  // only rewrites changed bytes

// ——— RTC health ———
// The time is trusted only if the RTC answers, its oscillator never stopped
// (lost-power flag clear), and the date is sensible.
bool rtcTimeValid(const DateTime &utc) {
  if (rtc.lostPower()) return false;
  return utc.unixtime() >= MIN_VALID_UTC && utc.year() <= 2099 &&
         utc.month() >= 1 && utc.month() <= 12 &&
         utc.day() >= 1 && utc.day() <= 31;
}

void setAllRelayPins(uint8_t state) {
  for (uint8_t i = 0; i < numEvents; i++) {
    digitalWrite(events[i].pin, state);
    lastApplied[i] = -1;  // force re-apply once the RTC is healthy again
  }
}

// ——— the schedule ———
// For every pin: find the latest event at or before the current local time of
// day. If there is none yet today, the latest event of the day (i.e. last
// night's) still applies. This makes the relay correct right after power-up,
// after a missed second, and across DST jumps — no "already fired" bookkeeping.
void applySchedule(uint32_t utc) {
  DateTime local = toLocal(utc);
  int32_t nowSec = local.hour() * 3600L + local.minute() * 60L + local.second();

  for (uint8_t i = 0; i < numEvents; i++) {
    bool seen = false;  // handle each pin once, at its first appearance
    for (uint8_t j = 0; j < i; j++) {
      if (events[j].pin == events[i].pin) { seen = true; break; }
    }
    if (seen) continue;

    int32_t bestToday = -1, bestAny = -1;
    uint8_t stateToday = FAILSAFE_STATE, stateAny = FAILSAFE_STATE;
    for (uint8_t j = i; j < numEvents; j++) {
      if (events[j].pin != events[i].pin) continue;
      int32_t evSec = events[j].hour * 3600L + events[j].minute * 60L + events[j].second;
      if (evSec <= nowSec && evSec >= bestToday) { bestToday = evSec; stateToday = events[j].state; }
      if (evSec >= bestAny)                      { bestAny = evSec;   stateAny = events[j].state; }
    }
    uint8_t desired = (bestToday >= 0) ? stateToday : stateAny;

    if (lastApplied[i] != desired) {
      digitalWrite(events[i].pin, desired);
      lastApplied[i] = desired;
      if (chatty()) {
        printStatusLine(utc);
        Serial.print(F("  -> pin "));
        Serial.print(events[i].pin);
        Serial.println(desired ? F(" HIGH (relay ON)") : F(" LOW (relay OFF)"));
      }
    }
  }
}

// ——— serial commands (used by laptop_sync/sync_rtc.py) ———
// Replies that a program should read start with '@'. Everything else is
// human-readable log output.

// Wait for the RTC's seconds to roll over, then report the new second at once.
// The laptop timestamps this reply to measure the RTC's error in milliseconds.
void cmdTick() {
  uint8_t start = rtc.now().second();
  uint32_t t0 = millis();
  while (millis() - t0 < 1500) {
    DateTime n = rtc.now();
    if (n.second() != start) {
      Serial.print(F("@TICK "));
      Serial.println(n.unixtime());
      return;
    }
  }
  Serial.println(F("@ERR tick timeout (RTC oscillator stopped?)"));
}

// Set the RTC. The laptop sends "SET <unix>" early and the final newline at the
// exact moment, so the clock is written the instant the newline arrives.
// Writing the seconds register also restarts the DS3231's sub-second counter,
// so the new second begins right now. rtc.adjust() also clears the lost-power flag.
void cmdSet(const char *arg) {
  uint32_t t = strtoul(arg, NULL, 10);
  if (t < MIN_VALID_UTC || t > 4102444799UL) {
    Serial.println(F("@ERR bad time"));
    return;
  }
  rtc.adjust(DateTime(t));  // do this first: timing matters
  persist.lastSyncUtc = t;
  savePersist();
  lastCheckedUtc = 0;
  for (uint8_t i = 0; i < numEvents; i++) lastApplied[i] = -1;
  Serial.print(F("@OK SET "));
  Serial.println(t);
}

void cmdBattery(const char *arg) {
  uint32_t t = strtoul(arg, NULL, 10);
  persist.batteryUtc = t;
  savePersist();
  Serial.print(F("@OK BATTERY "));
  Serial.println(t);
}

void cmdStatus() {
  DateTime now = rtc.now();
  uint32_t utc = now.unixtime();
  bool lost = rtc.lostPower();
  bool valid = rtcTimeValid(now);

  // machine-readable line for sync_rtc.py
  Serial.print(F("@STATUS fw=" FW_VERSION " utc="));  Serial.print(utc);
  Serial.print(F(" valid="));       Serial.print(valid ? 1 : 0);
  Serial.print(F(" lost_power="));  Serial.print(lost ? 1 : 0);
  Serial.print(F(" temp="));        Serial.print(rtc.getTemperature());
  Serial.print(F(" std_offset="));  Serial.print(STD_OFFSET_HOURS);
  Serial.print(F(" observe_dst=")); Serial.print(OBSERVE_DST ? 1 : 0);
  if (valid) {
    Serial.print(F(" offset="));    Serial.print(offsetHours(utc));
    Serial.print(F(" dst="));       Serial.print(isDST(utc) ? 1 : 0);
    Serial.print(F(" local="));     printDateTime(toLocal(utc), 'T');
    if (OBSERVE_DST) {
      uint32_t nc = nextClockChangeUtc(utc);
      Serial.print(F(" next_change_local=")); printDateTime(toLocal(nc), 'T');
    }
  }
  Serial.print(F(" last_sync="));   Serial.print(persist.lastSyncUtc);
  Serial.print(F(" battery="));     Serial.println(persist.batteryUtc);

  // human-readable version for the Serial Monitor
  Serial.println(F("--- status ---"));
  if (valid) {
    printStatusLine(utc);
    if (OBSERVE_DST) {
      uint32_t nc = nextClockChangeUtc(utc);
      Serial.print(F("Next clock change: "));
      printDateTime(toLocal(nc));
      printZone(nc);
      Serial.println(F(" (automatic)"));
    }
  } else {
    Serial.println(F("Clock NOT set or RTC lost power. Run laptop_sync/sync_rtc.py."));
  }
  Serial.print(F("Lost-power flag: "));
  Serial.println(lost ? F("SET (coin cell dead/missing during a power cut)") : F("clear"));
  Serial.print(F("Last sync: "));
  if (persist.lastSyncUtc) printDateTime(DateTime(persist.lastSyncUtc)); else Serial.print(F("never"));
  Serial.println(F(" UTC"));
  Serial.print(F("Battery installed: "));
  if (persist.batteryUtc) printDateTime(DateTime(persist.batteryUtc)); else Serial.print(F("unknown"));
  Serial.println(F(" UTC"));
}

void cmdHelp() {
  Serial.println(F("@HELP commands: STATUS, HELP, PING, TICK, SET <unix>, BATTERY <unix>"));
  Serial.println(F("To set the clock, run laptop_sync/sync_rtc.py on a computer plugged into this USB port."));
}

void handleCommand(char *cmd) {
  quietUntilMs = millis() + 30000UL;
  for (char *p = cmd; *p && *p != ' '; p++) *p = toupper(*p);

  if      (!strcmp(cmd, "PING"))          Serial.println(F("@PONG"));
  else if (!strcmp(cmd, "TICK"))          cmdTick();
  else if (!strncmp(cmd, "SET ", 4))      cmdSet(cmd + 4);
  else if (!strcmp(cmd, "STATUS"))        cmdStatus();
  else if (!strncmp(cmd, "BATTERY ", 8))  cmdBattery(cmd + 8);
  else if (!strcmp(cmd, "HELP"))          cmdHelp();
  else                                    Serial.println(F("@ERR unknown command (try HELP)"));
}

void pollSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      cmdBuf[cmdLen] = '\0';
      if (cmdLen) handleCommand(cmdBuf);
      cmdLen = 0;
    } else if (cmdLen < sizeof(cmdBuf) - 1) {
      cmdBuf[cmdLen++] = c;
    }
  }
}

// ——— status LED ———
void updateLed() {
  uint32_t ms = millis();
  if (rtcOk) digitalWrite(STATUS_LED, (ms % 2000) < 60);  // short heartbeat blink
  else       digitalWrite(STATUS_LED, (ms / 100) % 2);    // fast blink = problem
}

// ——— main ———
void checkClockAndSchedule() {
  DateTime utcDt = rtc.now();
  uint32_t utc = utcDt.unixtime();
  if (utc == lastCheckedUtc) return;  // once per new second
  lastCheckedUtc = utc;

  bool ok = rtcTimeValid(utcDt);
  if (!ok) {
    if (chatty() && (rtcOk || utc % 10 == 0)) {
      Serial.println(F("!! Clock not set / RTC lost power / RTC unplugged. Relay held in FAILSAFE_STATE."));
      Serial.println(F("!! Plug in a laptop and run laptop_sync/sync_rtc.py."));
    }
    setAllRelayPins(FAILSAFE_STATE);
  } else {
    applySchedule(utc);
    if (chatty() && utc % 60 == 0) printStatusLine(utc);
  }
  rtcOk = ok;
}

void setup() {
  Serial.begin(57600);
  pinMode(STATUS_LED, OUTPUT);
  for (uint8_t i = 0; i < numEvents; i++) {
    pinMode(events[i].pin, OUTPUT);
    digitalWrite(events[i].pin, FAILSAFE_STATE);
    lastApplied[i] = -1;
  }
  loadPersist();

  Serial.println(F("\n=== DS3231 relay timer v" FW_VERSION " ==="));
  if (!rtc.begin()) {
    Serial.println(F("@ERR RTC not found! Check SDA/SCL/VCC/GND wiring. Relay held in FAILSAFE_STATE."));
    while (true) {  // keep answering so sync_rtc.py can report the real problem
      rtcOk = false;
      updateLed();
      if (Serial.available() && Serial.read() == '\n')
        Serial.println(F("@ERR RTC not found! Check SDA/SCL/VCC/GND wiring."));
    }
  }
  Wire.setClock(400000);             // DS3231 supports fast I2C: quicker reads
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000, true);  // don't freeze if the RTC gets unplugged
#endif

  checkClockAndSchedule();           // set the relay right away
  if (rtcOk) {
    printStatusLine(lastCheckedUtc);
    if (OBSERVE_DST) {
      uint32_t nc = nextClockChangeUtc(lastCheckedUtc);
      Serial.print(F("Next clock change: "));
      printDateTime(toLocal(nc));
      printZone(nc);
      Serial.println(F(" (automatic)"));
    }
  }
  Serial.println(F("@READY fw=" FW_VERSION " (type HELP for commands)"));
}

void loop() {
  pollSerial();                      // checked constantly so SET is applied instantly
  uint32_t ms = millis();
  if (ms - lastRtcReadMs >= 100) {   // read the RTC 10x per second
    lastRtcReadMs = ms;
    checkClockAndSchedule();
  }
  updateLed();
}
