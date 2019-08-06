# Contributing

## Overview

BLE CTF Infinity was built to be very modular in nature. It allows for new challenges to be added very easily. This means you can easily write your own GATT server as a flag challenge for the project and contribute it upstream for everyone to try!

Each flag is actually a totally separate GATT server which can be compiled 100% on its own or it can be included as a modular flag module into the project.  This makes testing and trying new ideas really easy! Take a look at the current flag modules to get an idea of how they are built.

ble_ctf_infinity/gatt_servers

Even the scoreboard/dashboard is modular and can be swapped out! Feel free to write dashboards that are network enabled or hook into CTF project such as [CTFD](https://github.com/CTFd/CTFd)! 

## Simple Flag Creation Example
Lets step though an example of how to build a new flag from scratch to include in the CTF.

For this example, lets create a simple flag where the flag value is presented to the user backwards.  We will name it `reverse_flag`.  For example, if the real flag value is 12345678901234567890, we will show it to the user as 09876543210987654321 and give them a hint that they need to reverse it.

Our first step is to create the base files for the our flags GATT server.  I created a helper script to do this located at `/gatt_servers/create_gatt_server.sh`.  Its a simple script that just does a recursive copy of the TEMPLATE_table GATT server and renames everything accordingly.

```
cd gatt_servers
./create_gatt_server.sh reverse_flag
```

Now lets check out what we have:
```
tree reverse_flag
```

Output:
```
reverse_flag/
├── main
│   ├── CMakeLists.txt
│   ├── component.mk
│   ├── gatt_server_common.c
│   ├── gatt_server_common.h
│   ├── reverse_flag.c
│   └── reverse_flag.h
└── Makefile

1 directory, 7 files
```

The above command created the core .c & .h files for reverse_flag based on TEMPLATE_table.  These files are the core of your GATT server and define the majority of all of its functionality.  It also copied over some other common files and general libraries so you can build this module as a standalone GATT server if you like.  If you wanted to, you could take this whole directory and run a `make menuconfig; make; make flash` on it and flash just this GATT server to an ESP32 for testing.  For now, lets skip the make process and finish our flag. If you are interested for a deeper explanation of what the `create_gatt_server.sh` script did, just pop it open and look.  It's only about 10 lines of bash =)

For the next steps, we are going to open the `reverse_flag.c` file and code up our flag.

Lets add a hint to the users by changing this template documentation line.  We will change:
```
static const char docs_value[] = "TEMPLATE GATT READ";
```

to

```
static const char docs_value[] = "Reverse the flag value below to complete this challenge";
```

Now lets add some code logic to display the flag reversed.  For this we will want to add some custom code to the case statement that handles GATT READ events.  This is in the case statement for `ESP_GATTS_READ_EVT` shown below.
```
case ESP_GATTS_READ_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT");
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT, handle = %d", param->read.handle);

            break;
```

We will change it to look like this:
```
case ESP_GATTS_READ_EVT:
    ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT");
    ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT, handle = %d", param->read.handle);
    static char tmp[30] = "";
    for (int i = 0; i <= sizeof(flag_reverse_flag_value); i++) {
        tmp[i] = flag_reverse_flag_value[sizeof(flag_reverse_flag_value)-i];
    }
    esp_ble_gatts_set_attr_value(blectf_handle_table[REVERSE_FLAG_IDX_CHAR_READ_FLAG]+1, sizeof(tmp), (uint8_t *)tmp);
    break;
```

Boom! Donezo! Now lets add it to the CTF, compile it and test it.

To add it to the ctf, add the name of your module and a flag value to the ctf flag configuration file `code_gen/flag_config.csv`.  This is a file that houses the order of all of the flags and what their flag values are for each flag.  For this flag, lets set its flag value to 12345678901234567890.  To do this add the following line to `code_gen/flag_config.csv` 
```
reverse_flag, 12345678901234567890
```

Now lets compile.  Make sure you are in the base directory of your repo, then run the following:
```
make clean
make codegen
make
```

Then flash it to your device:
```
make flash
```

Now lets take a look at the flag.  For me it was the 10th flag in the flag config, so to get there we will pass 000a to the navigation handle of the scoreboard
```
sudo gatttool -b 11:22:33:44:55:66 --char-write-req -a 0x0030 -n 000a
```

Now lets take a look at the GATT values
```
bleah -b 11:22:33:44:55:66 -e
```

Output:

| Handle | Characteristic                       | Permissions                | Value |
| --- | --- | --- | --- |
| 0x0016 | 00002a00-0000-1000-8000-00805f9b34fb | READ                       | FLAG_10 |
| 0x0018 | 00002a01-0000-1000-8000-00805f9b34fb | READ                       |  |
| 0x001a | 00002aa6-0000-1000-8000-00805f9b34fb | READ                       |  |
| 0x002a | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | Reverse the flag value below to complete this challenge |
| 0x002c | 0000ff01-0000-1000-8000-00805f9b34fb | READ                       | 09876543210987654321 |
| 0x002e | 0000ff01-0000-1000-8000-00805f9b34fb | READ WRITE                 | write here to goto to scoreboard |

Thats it!  You'll notice the flag is reversed above based on our simple code addition.  A user can then read the hint, reverse the flag value and submit it on the scoreboard dashboard.

## Advanced Flag Creation Example
Want to add a flag that is more complex than the TEMPLATE_table GATT code?  In order for your GATT module to work with the CTF, there are a few requirements you will have to include in your code and GATT server. These requirements hook your server into the main GATT scoreboard/dashboard and allow the code generation Make scripts to add necessary code to make it compatible with the scoreboard & navigation system.  

WIP... For now take a look at what the codegen script in /code_gen/code_gen.py is doing...
