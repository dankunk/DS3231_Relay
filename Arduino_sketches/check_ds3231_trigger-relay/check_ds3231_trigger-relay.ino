#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

struct Event {
  uint8_t hour, minute, second;
  uint8_t pin;
  bool    state;
  bool    fired;
};

// ——— your schedule ———
// Note: all fired flags start false
Event events[] = {
  {  0,  0,  0,  8, LOW,  false },   // at 00:00:00 → pin8 LOW
  {  8,  0,  0,  8, HIGH, false },   // at 08:00:00 → pin8 HIGH
  // …add more if you like…
};
const int numEvents = sizeof(events) / sizeof(events[0]);

uint8_t lastSecond = 255;

// Finds the most‐recent event (by time) that’s ≤ now, applies its state,
// and marks all those ≤ now as fired so they won’t run again until tomorrow.
void applyInitialState() {
  DateTime now = rtc.now();
  bool pinState = LOW;  // default if no event has yet occurred today

  // Walk through every event
  for (int i = 0; i < numEvents; i++) {
    // build a DateTime for today at the event’s time
    DateTime evTime(now.year(), now.month(), now.day(),
                    events[i].hour,
                    events[i].minute,
                    events[i].second);

    // if evTime ≤ now, this event *has* already happened today
    if (evTime.unixtime() <= now.unixtime()) {
      pinState = events[i].state;    // remember its state
      events[i].fired = true;        // don’t re‐fire until tomorrow
    }
  }

  // apply the last applicable state
  // (if none ≤ now, this leaves pin LOW)
  for (int i = 0; i < numEvents; i++) {
    pinMode(events[i].pin, OUTPUT);
    digitalWrite(events[i].pin, pinState);
  }
}

void setup() {
  Serial.begin(57600);
  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    while (1);
  }

  // initialize pins (we’ll overwrite in applyInitialState)
  for (int i = 0; i < numEvents; i++) {
    pinMode(events[i].pin, OUTPUT);
  }

  // set them to the correct “current” state & mark past events fired
  applyInitialState();
}

void loop() {
  DateTime now = rtc.now();

  // only check once per new second
  if (now.second() != lastSecond) {
    lastSecond = now.second();

    for (int i = 0; i < numEvents; i++) {
      Event &e = events[i];
      if (!e.fired
          && now.hour()   == e.hour
          && now.minute() == e.minute
          && now.second() == e.second) {
        digitalWrite(e.pin, e.state);
        e.fired = true;
        Serial.print("Event fired on pin ");
        Serial.print(e.pin);
        Serial.print(" → state=");
        Serial.println(e.state ? "HIGH" : "LOW");
      }
      // reset for tomorrow once we’ve moved past its minute
      if (e.fired
          && (now.hour()   != e.hour
           || now.minute() != e.minute)) {
        e.fired = false;
      }
    }
  }

  delay(50);
}
