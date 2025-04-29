# code.py — Pico W → DS3231 via NTP sync

# Allows for the system to syc to an NTP server for UTC time.

import time
import board
import busio
import wifi
import socketpool
import adafruit_ntp
import adafruit_ds3231
import digitalio
import rtc as onboard_rtc
import secrets  # your Wi-Fi creds

# 1) I²C bus to the DS3231
i2c = busio.I2C(board.GP17, board.GP16)
ds3231 = adafruit_ds3231.DS3231(i2c)

# 2) Connect to Wi-Fi
print("Connecting to Wi-Fi…")
wifi.radio.connect(secrets.SSID, secrets.PASSWORD)
print("Connected!")

# 3) Fetch UTC from NTP
pool = socketpool.SocketPool(wifi.radio)
ntp = adafruit_ntp.NTP(pool, tz_offset=-6)  # tz_offset=0 for UTC
print("Getting time from NTP…")
ntp_time = ntp.datetime  # a time.struct_time: (Y, M, D, h, m, s, w, yday)
print("NTP time:", ntp_time)

# 4) Write to DS3231
ds3231.datetime = ntp_time
print("DS3231 now set to:", ds3231.datetime)

# 5) (Optional) also set the Pico’s onboard RTC so you can print/log later
onboard = onboard_rtc.RTC()
onboard.datetime = ntp_time

# Blink LED to show we’re done
led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT
for _ in range(6):
    led.value = not led.value
    time.sleep(0.2)

print("Sync complete — DS3231 locked to UTC from NTP.")
