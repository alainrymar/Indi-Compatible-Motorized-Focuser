# Arduino_Moonlite_Focuser
Arduino Focuser for a telescope, based on the Moonlite protocol, INDI compatible.

This software will control a home made motorfocuser that uses a Step motor.

Hardware:
- Home Made Motorized Focuser, just the motor and the cable are needed. There is no handbox in this concept. Motor is controlled by the INDI interface.
- Arduino UNO (will probably also work on other Arduino boards)
- step motor 42SHDC3025-24B.
- Motor driver a4988 wired to the Arduino
    pinMode(A0,OUTPUT);MS1
    pinMode(A1,OUTPUT);MS2
    pinMode(A2,OUTPUT);MS3
- Power source of your choice. The focuser can't run off the Arduino's power if it's powered from USB. You need a source that can power the DC motor. 

Software:
Install the GearedMotor files as a library in the Arduino IDE.
Upload the ino file to the Arduino UNO

The motor is connected to ports 7 (enable), 5 (step) and 4 (direction) of the Arduino. 
  //Speed Selection Motor - MSx pins on a4988 motor driver
  pinMode(A0,OUTPUT);
  pinMode(A1,OUTPUT);
  pinMode(A2,OUTPUT);
  //Set MSx port on a4988 (MS1/MS2/MS3) to L/H/L: 1600 steps to 360 motor turn
  digitalWrite(A0,HIGH);
  digitalWrite(A1,HIGH);
  digitalWrite(A2,LOW);
  
Start INDIserver with the Moonlite focuser (indi_moonlite_focus)
The focuser has no speed control. The focuser can't tell at which position the software starts, so it supposes that position '0' is all the way in. 
Increase position by focusing outward. (Depends on how you wire the motor, of course.)
  //--> to be used with step motor 42SHDC3025-24B
  //--> Tested on SkyWatcher 200/1000 (200P)
  //--> Skywatcher focuser range: 3 x 360°
  //--> Tested ok on INDI server with moonloght driver (not DRO)
  //--> Default: 1600 steps per motor 360° = "Full Step", Half Step supported = 3200 steps/360° - Both supported from INDI GUI
  //--> In Full Step: Tune Indi Max.Position (GUI) to 4800 (3 x 1600 steps) -> adapt yours if focuser range different from 3 x 360°
  //--> In Half step: Tune Indi Max.Position (GUI) to 9600 (3 x 3200 steps) -> adapt yours if focuser range different from 3 x 360°
  //--> In Indi GUI, set Relative position to 0 (should be default), means, Focuser full inside telescope
  //--> Support Focus Speed from Indi GUI, from 1 to 5.  
  //--> Temperature compensation not considered yet
  //--> Arduino UNO tested ok
  //--> Motor Driver: a4988
  //--> Check MoonLight Protocol: https://www.indilib.org/media/kunena/attachments/1/HighResSteppermotor107.pdf
  //--> do not work yet with Joystick control from INDI.
There's no temperature compensation.

The software was tested with INDI installed on a Raspberry Pi, focuser software on an Arduino UNO. 
Motor shield: Home made to support a4988 driver for Step Motor 42SHDC3025-24B. Telescope: SkyWatcher 200P 200/100. 
The autofocus routine of INDI was not tested yet (August 2017), due to lack of clear skies. 
On this setup, the maximum focus travel is at most 9600 steps.
