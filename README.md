# automatic_henhouse
Little personal project to automate a henhouse door using Arduino.


## Description

In this project I am looking to automate a trap door so that the hens in my chicken coop can go out to graze in the morning and that it closes automatically at night when they go back to sleep.
For this I want it to operate at the specified time, that I can easily change the opening and closing time and that I can actuate the trap door manually. It must also be safe for the hens, without hurting them in case of entrapment.

For this I used an Arduino that implements a state machine, an electric cylinder that opens and closes the door, driven by a double relay that performs an H-bridge. With an LCD display and some buttons an interface is implemented that allows to change the time in a simple way, and to activate or deactivate the automatic mode.

The door can be opened and closed manually by means of the open (green) or close (red) button. While the door is moving, the buzzer sounds, and the led lights indicate the status of the door.

## Components 

The hardware components I have used to carry it out are:

- Arduino UNO
- DS3231 clock
- Double relay
- Buck LM2596 converter
- Three 12 mm pushbuttons
- Two LED lights: one blue and one red
- One buzzer
- One 4 Amp fuse
- Barrel jack connectors.
- 12 V power supply
- Electric cylinder actuator

## Hardware implementation

To Do ...

## Software implementation 

To Do ...