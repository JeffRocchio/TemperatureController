# Temperature Controller

## Purpose

A relatively simple controller that can be used for both a small wood dryer and a bread proofer. Suitable for cases where temperature can vary by 1-2 degrees from the set point and where 100-200 watts of heating element power is sufficient. For my uses one to two incandescent lights bulbs will be the heating element.

## Physical Architecture

Device is based on an ATTiny 84a MCU chip and a Solid State Relay for controlling the heater element load.

The user sets the desired temperature by use of a potentiometer. There are three LEDs that provide very basic status information on the current temperature relative to the set-point.

## Software Architecture

Probably the only significant thing to note here is that the software is structured as a mini real-time-OS (RTOS). The main program loop calls the necessary 'tasks' continuously and with no hard-coded delays. Each task is assigned a time, in milliseconds, that defines the tempo at which it should actually execute. When the task is called (from the main loop) it checks to see if it is time for it to run itself. If not, it simply returns without running any of it's logic.

The other thing to be aware of is that this application does consist of multiple files. The status LEDs are managed by their own object class, which is contained in it's own .h and .cpp file. If the software is enhanced additional class may be developed and they will also be placed in their own class files.

## Status

As of December 6, 2025 I have the controller built on only a breadboard and am testing it's operation by using a cardboard box with the light bulb heat source under it.
