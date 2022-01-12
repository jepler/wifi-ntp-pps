# wifi-ntp-pps

Produce a pps signal reasonably well synchronized to NTP.  The duty cycle is 100ms on (starting at the top of the second), 900ms off.  Jitter is very high relative to GPS or even WWVB, but long term accuracy should be extremely good thanks to NTP.

(In practice, we measured the thing against a GPS-derived PPS signal for awhile, and saw the signal moving around +-20us compared to GPS, which is really pretty amazing.)

# Installing and setting up esp-idf

One time set-up:
```shell
git submodule update --init
esp-idf/install.sh
```

(Note that this will fail if you've previously done `. esp-idf/export.sh`; use a fresh terminal if you have to (re)install esp-idf)

Set environment variables in current shell until exit:
```shell
. esp-idf/export.sh
```

# Configuring wifi-ntp-pps

Open the project configuration menu (`idf.py menuconfig`):

* Configure Wi-Fi under "Example Connection Configuration" menu. See "Establishing Wi-Fi or Ethernet Connection" section in [examples/protocols/README.md](https://github.com/espressif/esp-idf/blob/HEAD/examples/protocols/README.md) for more details.

* Select the board type (Adafruit QT Py or Adafruit Feather)

* Select an ntp server. Ideally, a local server. Otherwise, `pool.ntp.org` can be used. The default **will not work**.

* Optionally, turn down `LWIP_SNTP_UPDATE_DELAY`; the default is 1 hour. For a local NTP server something smaller like 30000 (30 seconds) is probably fine.

# Compiling

```shell
idf.py all
```

# Flashing

Plug in the device while holding the BOOT button. Alternately hold BOOT and press RESET once. Determine where the device has appeared, such as `/dev/ttyACM1`.

```shell
idf.py flash --after no_reset --port=/dev/ttyACM???
```

Once flash has finished, click the reset button once.  (Technical limitations seem to prevent automatically resetting after loading the code, and it prints a scary failure message every time!)

# Output pins

The "A0" pin gets a 1PPS output with the rising edge placed near the top of the second.

"A1" gets a pulse-per-minute, "A2" gets pulse-per-hour, and "A3" is "exclusive PPS": True once per second but only if it is not the hour or the minute, i.e., `A3 = A0 & ~(A1 | A2)`.

# Status indicator

The on-board neopixel gives the board status:

* Black: power off or crashed
* Solid red: never connected to wifi or crashed
* Blinking red: never got NTP sync since power on
* Blinking cyan: connected & got NTP sync in last 10s (sync is nominally every 30s, free-running timer)
* Blinking green: connected & got NTP sync since power on
