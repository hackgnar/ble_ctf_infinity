# Contributing

## Overview

BLE CTF Infinity was built to be very modular in nature. It allows for new challenges to be added very easily. This means you can easily write your own GATT server as a flag challenge for the project and contribute it upstream for everyone to try!

Each flag is actualy a totally separate GATT server which can be compiled 100% on its own or it can be included as a modular flag module into the project.  This makes testing and trying new ideas really easy! Take a look at the current flag modules to get an idea of how they are built.

ble_ctf_infinity/gatt_servers

Even the scoreboard/dashboard is modular and can be swapped out! Feel free to write dashboards that are network enabled or hook into CTF project such as [CTFD](https://github.com/CTFd/CTFd)! 

## Simple Flag Creation Example
Lets step though an example of how to build a new flag from scratch to include in the CTF.

WIP...

## Advanced Flag Creation Example
Want to add a flag that is more complex than the TEMPLATE_table GATT code?  In order for your GATT module to work with the CTF, there are a few requirements you will have to include in your code and GATT server. These requirements hook your server into the main GATT scoreboard/dashboard and allow the code generation Make scripts to add necessary code to make it compatible with the scoreboard & navigation system.  

WIP... For now take a look at what the codegen script in /code_gen/code_gen.py is doing...
