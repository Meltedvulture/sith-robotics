#In your terminal run this command:
cd /etc/udev/rules.d/


#Create a rules file using this command:
sudo nano /etc/udev/rules.d/99-picotool.rules


#Inside the opened rules file, paste the following inside:

# /etc/udev/rules.d/99-pico.rules

# Make an RP2040 in BOOTSEL mode writable by all users, so you can `picotool`
# without `sudo`. 
SUBSYSTEM=="usb", ATTRS{idVendor}=="2e8a", ATTRS{idProduct}=="0003", MODE="0666"

# Symlink an RP2040 running MicroPython from /dev/pico.
#
# Then you can `mpr connect $(realpath /dev/pico)`.
SUBSYSTEM=="tty", ATTRS{idVendor}=="2e8a", ATTRS{idProduct}=="0005", SYMLINK+="pico"



#Save your file with ctrl X, hit Y to confirm and hit Enter to close the file
#Lastly run this command and restart your computer:

sudo usermod -a -G dialout yourusername