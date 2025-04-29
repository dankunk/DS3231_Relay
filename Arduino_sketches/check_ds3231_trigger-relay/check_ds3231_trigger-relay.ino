// This script will check the time on the DS3231 every defined interval.
// if the time matches a set of input conditions it will trigger the IOT relay via GPIO pins.

#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

// A single “event” (turn a pin HIGH or LOW at a given h:m:s)
struct Event {
  uint8_t hour, minute, second;  // trigger time (24 h)
  uint8_t pin;                   // Arduino pin to drive
  bool    state;                 // HIGH = on, LOW = off
  bool    fired;                 // has today’s trigger already run?
};

// ——— Configure your pattern here ———
Event events[] = {
  // { hh, mm, ss, pin#, state, fired=false }
  // turn off at midnight
  {  0,  0,  0,  8, LOW, false },   // at 00:00:00 → pin 8 LOW
  // turn on at 8 am
  {  8,  0,  0,  8,  HIGH, true },   // at 08:00:00 → pin 8 HIGH
  // add more events as needed:
  // { 10, 30, 15,  9, HIGH, false },
  // { 10, 30, 45,  9,  LOW, false },

  //Testing events; Picking a random time to check and see if the light turns on/off at the appropriate time.
  //{  16,  52,  45,  8, LOW, false },   // at 00:00:00 → pin 8 LOW
  // turn on at 8 am
  //{ 16,  53,  0,  8,  HIGH, false },   // at 08:00:00 → pin 8 HIGH
};

const int numEvents = sizeof(events) / sizeof(events[0]);
uint8_t lastSecond = 255;

void setup() {
  Serial.begin(57600);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Initialize all event pins
  for (int i = 0; i < numEvents; i++) {
    pinMode(events[i].pin, OUTPUT);
    // set to the opposite state so the first on/off always kicks in:
    digitalWrite(events[i].pin, !events[i].state);
  }
}

void loop() {
  DateTime now = rtc.now();

  // Only run our checks once per new second:
  if (now.second() != lastSecond) {
    lastSecond = now.second();

    for (int i = 0; i < numEvents; i++) {
      Event &e = events[i];

      // If it’s exactly the trigger time AND we haven't fired yet today:
      if (now.hour()   == e.hour &&
          now.minute() == e.minute &&
          now.second() == e.second &&
          !e.fired) {
        digitalWrite(e.pin, e.state);
        e.fired = true;
        Serial.print("Event fired on pin ");
        Serial.print(e.pin);
        Serial.print(" → state=");
        Serial.println(e.state ? "HIGH" : "LOW");
      }

      // Once the clock has moved past this minute, reset for tomorrow:
      // (so the event can fire again next day)
      if (e.fired &&
         (now.hour()   != e.hour ||
          now.minute() != e.minute)) {
        e.fired = false;
      }
    }
  }

  // wait a little so we don’t hammer I²C
  delay(50);
}
