[![Follow Hackgnar](static/twitter_hackgnar.png)](https://twitter.com/hackgnar)

## Index
- [Building and installing the CTF](docs/setup.md)
- [CTF architecture](docs/architecture.md)
- [Contributing to the project](docs/contributing.md)

## BLE Capture the Flag v2.0
The purpose of BLE CTF is to teach the core concepts of Bluetooth Low Energy client and server interactions.  While it has also been built to be fun, it was built with the intent to teach and reinforce core concepts that are needed to plunge into the world of Bluetooth hacking.  After completing this CTF, you should have everything you need to start fiddling with any BLE GATT device you can find.

## Getting Started
Assuming you have the [CTF flashed to an ESP32](docs/setup.md) and are ready to go, read on.

It is recommended that you have a Linux box (OSX/Win + Linux VM works) with a Bluetooth controller or a Bluetooth USB dongle to do the CTF. On your Linux machine you should install Bluetooth tools such as Bluez tools (hcitool, gatttool, bluetoothctl etc).  For some flags, it is also useful to install wireshark/tshark, bleah, bettercap, etc.

Once your ESP32 is powered up, it will automatically start you off on the CTFs main dashboard.  This dashboard is a GATT server that shows you your current status on all flags.  It is also the location where you will submit all of your flag values once you solve the challenges.  Finally, it also functions as navigation system to go to flags you want to work on.  Lets give it a try.

Find your CTFs Bluetooth MAC address if you don't already have it.  Look for a GATT server named "BLE_CTF_SCORE"
```
hcitool lescan
```
Output:
```
LE Scan ...
11:22:33:44:55:66 BLE_CTF_SCORE
11:22:33:44:55:66 (unknown)

```

Take a look at your dashboard with the tool of your choice (ie gatttool, bleah, bettercap, etc).  The following is generated with an ugly gist I use:
```
./ghetto_bleah.sh 11:22:33:44:55:66
```
Output:

| Handle | Characteristic                       | Permissions                | Value |
| 0x0016 | 00002a00-0000-1000-8000-00805f9b34fb | READ                       | 04dc54d9053b4307680a |
| 0x0018 | 00002a01-0000-1000-8000-00805f9b34fb | READ                       |  |
| 0x001a | 00002aa6-0000-1000-8000-00805f9b34fb | READ                       |  |
| 0x002a | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | docs: https://github.com/hackgnar/ble_ctf |
| 0x002c | 0000ff02-0000-1000-8000-00805f9b34fb | READ                       | Flags complete: 0 /10 |
| 0x002e | 0000ff02-0000-1000-8000-00805f9b34fb | READ WRITE                 | Submit flags here |
| 0x0030 | 0000ff02-0000-1000-8000-00805f9b34fb | READ WRITE                 | Write 0x0000 to 0x00FF to goto flag |
| 0x0032 | 0000ff02-0000-1000-8000-00805f9b34fb | READ WRITE                 | Write 0xC1EA12 to reset all flags |
| 0x0034 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 0: Incomplete |
| 0x0036 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 1: Incomplete |
| 0x0038 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 2: Incomplete |
| 0x003a | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 3: Incomplete |
| 0x003c | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 4: Incomplete |
| 0x003e | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 5: Incomplete |
| 0x0040 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 6: Incomplete |
| 0x0042 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 7: Incomplete |
| 0x0044 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 8: Incomplete |
| 0x0046 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 9: Incomplete |

Above you can see:
- Handles that show the status of each flag in the CTF
- A handle for submitting CFT flag values
- A handle for navigating to each flag
- A handle for clearing your score on the CTF

Lets navigate to Flag 1 and take a look:
```
gatttool -b 11:22:33:44:55:66 --char-write-req -a 0x0030 -n 0001
```
Output:
```
Characteristic Write Request failed: Request attribute has encountered an unlikely error
```
Don't worry about the above error.  You will get a similar error each time you navigate to a different flag or scoreboard.  Its unavoidable because of the way the CTF was architected.

Now, lets take a look at the flag 1 GATT server
```
./ghetto_bleah.sh 11:22:33:44:55:66
```

Output:
| Handle | Characteristic                       | Permissions                | Value |
| 0x0003 | 00002a05-0000-1000-8000-00805f9b34fb |      WRITE NOTIFY          | Characteristic value/descriptor read failed: Attribute can't be read|
| 0x0016 | 00002a00-0000-1000-8000-00805f9b34fb | READ                       | FLAG_1 |
| 0x0018 | 00002a01-0000-1000-8000-00805f9b34fb | READ                       |  |
| 0x001a | 00002aa6-0000-1000-8000-00805f9b34fb | READ                       |  |
| 0x002a | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | goodbye 👋 |
| 0x002c | 0000ff02-0000-1000-8000-00805f9b34fb | READ                       | Characteristic value/descriptor read failed: Request attribute has encountered an unlikely error|
| 0x002e | 0000ff03-0000-1000-8000-00805f9b34fb | READ WRITE                 | write here to goto to scoreboard |

Whoh!  That's a whole different GATT server!  How did you do that?  I thought you could only host a single GATT server on one Bluetooth device!

Now lets return back to the scoreboard:
```
gatttool -b 11:22:33:44:55:66 --char-write-req -a 0x002e -n 0001
```

Assuming we found a flag value of 12345678901234567890, this is how you would submit it:
```
gatttool -b 11:22:33:44:55:66 --char-write-req -a 0x002e -n $(echo -n "12345678901234567890"|xxd -ps)
```

Then if we look at the dashboard again, you can see it shows we completed a flag
```
./ghetto_bleah.sh 11:22:33:44:55:66
```

| Handle | Characteristic                       | Permissions                | Value |
| 0x0018 | 00002a01-0000-1000-8000-00805f9b34fb | READ                       |  |
| 0x001a | 00002aa6-0000-1000-8000-00805f9b34fb | READ                       |  |
| 0x002a | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | docs: https://github.com/hackgnar/ble_ctf |
| 0x002c | 0000ff02-0000-1000-8000-00805f9b34fb | READ                       | Flags complete: 1 /10 |
| 0x002e | 0000ff02-0000-1000-8000-00805f9b34fb | READ WRITE                 | Submit flags here |
| 0x0030 | 0000ff02-0000-1000-8000-00805f9b34fb | READ WRITE                 | Write 0x0000 to 0x00FF to goto flag |
| 0x0032 | 0000ff02-0000-1000-8000-00805f9b34fb | READ WRITE                 | Write 0xC1EA12 to reset all flags |
| 0x0034 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 0: Incomplete |
| 0x0036 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 1: Complete   |
| 0x0038 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 2: Incomplete |
| 0x003a | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 3: Incomplete |
| 0x003c | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 4: Incomplete |
| 0x003e | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 5: Incomplete |
| 0x0040 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 6: Incomplete |
| 0x0042 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 7: Incomplete |
| 0x0044 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 8: Incomplete |
| 0x0046 | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Flag 9: Incomplete |
