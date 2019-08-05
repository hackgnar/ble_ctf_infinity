#FAQ

## I can't get my Bluetooth to work to do the CTF
Try reading [this documentation](workshop_setup.md).  It has tips on setting up Bluetooh and operating systems for doing the CTF. 

## How do you host multiple GATT servers on a single Bluetooth device?
It's a trick.  I'm not really hosting multiple servers.  I designed the architecture where the code for multiple GATT servers is stored on the ESP, but depending on data persistent in the chips memory, it will start up with code for a particular GATT server.  When you choose to navigate to a new flag, the chip writes to persistent memory what flag you want to go to, then reboots.  On reboot, it will spin up GATT server code based on that persistent value.

## Can I do the CTF with my phone?
Ya, sure!  I've had someone complete all of BLE CTF V1 on an Android phone using the NFR Connect app.  For BLE CTF infinity you will likely not be able to complete all of the flags with the apps available to you.  If you want to complete the whole CTF, you'll likely have to cut some code yourself.

## Can I do the CTF with OSX or Windows?
Ya, sure!  Just keep in mind that Windows and OSX are lacking out of the box tools for interacting with Bluetooth.  You'll likely have to cut some code yourself. 

## I found a flag value but the CTF doesn't accept it
Did you truncate your flag value to 20 characters?  When you convert it to hex, are you using `echo -n` to remove the newline character?  Are you sure you found the right value?  I continuously test each flag value so they should all be legit.  I do however make mistakes sometimes, so feel free to ask me or submit an issue to the project.

## Can I do a writeup on your CTF?
Sure!  But keep in mind your writeup will become outdated very fast with this CTF.  This version has dynamically ordered flags and they can change at any release along with the values.  The flag architecture is also very modular so I can add new flags all the time.  For infinity!
