# wifi-ntp-pps

Produce a pps signal reasonably well synchronized to NTP.  The duty cycle is 100ms on (starting at the top of the second), 900ms off.  Jitter is very high relative to GPS or even WWVB, but long term accuracy should be extremely good thanks to NTP.

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

* Select an ntp server. Ideally, a local server. Otherwise, `pool.ntp.org` can be used. The default **will not work**.

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

# PPS Output

The pin with silk "A0" gets a 1PPS output with the rising edge placed near the top of the second.

# Status indicator

The on-board neopixel gives the board status:

* Black: power off or crashed
* Solid red: never connected to wifi or crashed
* Blinking red: never got NTP sync since power on
* Blinking green: connected & got NTP sync since power on
