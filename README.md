# Remote Control for Sony A7IV based on ESP32

This is pre-alpha (demo) version of my Remote Control for Sony A7IV implemented on ESP32.

The demo is based on https://github.com/coral/freemote

My setup:
* Environment: Visual Studio Code with PlatormIO
* Device: ESP32 Dev Module
* Camera: Sony A7IV

How this demo works:
1. Boot.
2. Search for "ILCE-7M4" bluetooth device.
3. If found it pairs with it.
4. After successful pairing, it takes a photo every 10 seconds.

It should works with other Sony Alpha cameras but you must enter `bleServerName`.
If you don't known you can launch and looking for `BLE: something found: xxx` line in Serial Monitor.

If I helped you and saved you a few hours, please buy me a coffee :)
https://www.paypal.com/paypalme/nkgmn/10
