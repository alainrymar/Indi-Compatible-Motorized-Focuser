// Moonlite-compatible stepper controller
//
// Uses AccelStepper (http://www.airspayce.com/mikem/arduino/AccelStepper/)
//
// Inspired by (http://orlygoingthirty.blogspot.co.nz/2014/04/arduino-based-motor-focuser-controller.html)
// orly.andico@gmail.com, 13 April 2014
//
// Modified for indilib, easydriver by Cees Lensink

// Hacked by WvB
  // Hacked by Alain Rymar (6 Feb 2021)
  //--> to be used with step motor 42SHDC3025-24B
  //--> Tested on SkyWatcher 200/1000 (200P)
  //--> Skywatcher focuser range: 3 x 360°
  //--> Tested ok on INDI server with moonloght driver (not DRO)
  //--> Default: 1600 steps per motor 360° = "Full Step", Half Step supported = 3200 steps/360° - Both supported from INDI GUI
  //--> In Full Step: Tune Indi Max.Position (GUI) to 4800 (3 x 1600 steps) -> adapt yours if focuser range different from 3 x 360°
  //--> In Half step: Tune Indi Max.Position (GUI) to 9600 (3 x 3200 steps) -> adapt yours if focuser range different from 3 x 360°
  //--> In Indi GUI, set Relative position to 0 (should be default), means, Focuser full inside telescope
  //--> Support Focus Speed from Indi GUI, from 1 to 5.  
  //--> Temp not considered yet
  //--> Arduino UNO tested ok (Alain)
  //--> Motor Driver: a4988
  //--> Check MoonLight Protocol: https://www.indilib.org/media/kunena/attachments/1/HighResSteppermotor107.pdf
  //--> do not work yet with Joystick control from INDI.
  



#include "GearedMotor.h"

/*
//Alain Setup (Dirpin = 4, StepPin = 5, SleepPin (powerPin)=7)
int stepperPin = 5;
int dirPin = 4;
int powerPin = 7;
boolean useSleep = true; // true= use sleep pin, false = use enable pin
*/
int powerPin = 7;
boolean useSleep = true; // true= use sleep pin, false = use enable pin
                         // with sleep, motor is dis-engaged after each move

#define SPEEDMULT 3

GearedMotor motor(7,4,5);

#define MAXCOMMAND 8
// Version 2.1
float VERSION = 2.1;
char inChar;
char cmd[MAXCOMMAND];
char param[MAXCOMMAND];
char line[MAXCOMMAND];
long pos;
long Step;
long MotorSpeed;
int eoc = 0;
int idx = 0;
int MAXSPEED;  // define the step intervals
               // 1 -> 1000, 2 ->1500, 3->2000, 4->2500, 5->3000
               // Check GearedMotor.cpp for details
boolean isRunning = false;
boolean isFullStep = true;

char tempString[10];



void setup()
{
	Serial.begin(9600);         // serial to INDI server. Based on RaspBerry PI4 (Alain)
	pinMode(powerPin,OUTPUT);
  MAXSPEED = 1;               //Set default motor speed at boot
  MotorSpeed = 2;             //Set Default speed to 1 on INDI GUI
	motor.setSpeed(MAXSPEED);   //Handled in GearedMotor.cpp
	//WvB & alain: Assume focuser starts at all the way in, which is position 0
	motor.setCurrentPosition(0);
	motor.setBacklashFlag(true);
	motor.setBacklashSteps(120);
	//WvB new code ends here
	turnOff();
	memset(line, 0, MAXCOMMAND);

      
  //Speed Selection Motor - MSx pins on a4988 motor driver
  pinMode(A0,OUTPUT);
  pinMode(A1,OUTPUT);
  pinMode(A2,OUTPUT);
  //Set MSx port on a4988 (MS1/MS2/MS3) to L/H/L: 1600 steps to 360 motor turn
  digitalWrite(A0,HIGH);
  digitalWrite(A1,HIGH);
  digitalWrite(A2,LOW);
}

void loop(){
	if (isRunning) { // only have to do this is stepper is on
		motor.run();
		if (motor.stepsToGo() == 0) {
			//  we have arrived, remove power from motor
			turnOff();
		}
	}

	// read the command until the terminating # character
	//WvB each command starts with : and ends with #
	//WvB the following seems strange, as idx is never set to 0
	//WvB this code will put all characters received between : and # in the string line
	while (Serial.available() && !eoc) {
		inChar = Serial.read();
		if (inChar != '#' && inChar != ':') {
			line[idx++] = inChar;
			if (idx >= MAXCOMMAND) {
				idx = MAXCOMMAND - 1;
			}
		}
		else {
			if (inChar == '#') {
				eoc = 1;
			}
		}
	} // end while Serial.available()
	// we may not have a complete command yet but there is no character coming in for now and might as well loop in case stepper needs updating
	// eoc will flag if a full command is there to act upon

	// process the command we got
	if (eoc) {
		memset(cmd, 0, MAXCOMMAND);
		memset(param, 0, MAXCOMMAND);
		int len = strlen(line);
		if (len >= 2) {
			strncpy(cmd, line, 2);
		}

		if (len > 2) {
			strncpy(param, line + 2, len - 2);
      //Serial.print('param=');
      //Serial.println(param);
		}
		memset(line, 0, MAXCOMMAND);  //WvB sets the string line to 0
		eoc = 0;
		idx = 0;

		//now execute the command

		//Immediately stop any focus motor movement. returns nothing
		//code from Quickstop example. This is blocking
		if (!strcasecmp(cmd, "FQ")) {  // WvB: "FQ" = Halt motor, position is retained
			//WvB turnOn();
			motor.disableOutputs(); // Stop as fast as possible: sets new target
			pos = motor.currentPosition();
			//WvB motor.moveTo(0);  // WvB: is this needed for dc?
			// Now stopped after quickstop
		}

		//Go to the new position as set by the ":SNYYYY#" command. returns nothing    // initiate a move
		//turn stepper on and flag it is running
		// is this the only command that should actually make the stepper run ?
		if (!strcasecmp(cmd, "FG")) {  //WvB: start motor, to NewPosition
			turnOn();
      //motor.moveTo(pos + 2);
		}

		//Returns the temperature coefficient where XX is a two-digit signed (2�s complement) hex number.
		//hardcoded
		if (!strcasecmp(cmd, "GC")) {
			Serial.print("02#");
		}

		//Returns the current stepping delay where XX is a two-digit unsigned hex number. See the :SD# command for a list of possible return values.
		//hardcoded for now
		// might turn this into AccelStepper acceleration at some point
		if (!strcasecmp(cmd, "GD")) {
      Serial.print(MotorSpeed);
			Serial.print("#");
		}

		//Returns "FF#" if the focus motor is half-stepped otherwise return "00#"
		//hardcoded
		if (!strcasecmp(cmd, "GH")) {
      if (isFullStep == true){
        Serial.print("00#");
       }
      else {
        Serial.print("FF#"); 
      }
		}

		//Returns "00#" if the focus motor is not moving, otherwise return "01#",
		//AccelStepper returns Positive as clockwise
		if (!strcasecmp(cmd, "GI")) {
			if (motor.stepsToGo() == 0) {
				Serial.print("00#");
			}
			else {
				Serial.print("01#");
			}
		}

		//Returns the new position previously set by a ":SNYYYY" command where YYYY is a four-digit unsigned hex number.
		if (!strcasecmp(cmd, "GN")) {
			pos = motor.targetPosition();
			sprintf(tempString, "%04X", pos);
			Serial.print(tempString);
			Serial.print("#");
		}

		//Returns the current position where YYYY is a four-digit unsigned hex number.
		if (!strcasecmp(cmd, "GP")) {
			pos = motor.currentPosition();
			sprintf(tempString, "%04X", pos);
			Serial.print(tempString);
			Serial.print("#");
		}

		//Returns the current temperature where YYYY is a four-digit signed (2�s complement) hex number.
		if (!strcasecmp(cmd, "GT")) {
			Serial.print("0020#");
		}

		//Get the version of the firmware as a two-digit decimal number where the first digit is the major version number, and the second digit is the minor version number.
		//hardcoded
		if (!strcasecmp(cmd, "GV")) {
      //Serial.print(VERSION);
			Serial.print("10#");
		}

		//Set the new temperature coefficient where XX is a two-digit, signed (2�s complement) hex number.
		if (!strcasecmp(cmd, "SC")) {
       //Step = hexstr2long(param);
       
			//do nothing yet
		}

		//Set the new stepping delay where XX is a two-digit,unsigned hex number.
    // define the step intervals
    // 1 -> 1000, 2 ->1500, 3->2000, 4->2500, 5->3000
    // Check GearedMotor.cpp for details
		if (!strcasecmp(cmd, "SD")) {
           //Serial.println("SD received"); //To be removed
       //MotorSpeed = param;
       MotorSpeed = hexstr2long(param);
       if (MotorSpeed == 2){
        //Speed 1 - Interstep = 1000
        MAXSPEED = 1;
        motor.setSpeed(MAXSPEED);
       }
       if (MotorSpeed == 4){
        //Speed 2 - Interstep = 1500
        MAXSPEED = 2;
        motor.setSpeed(MAXSPEED);
       }
       if (MotorSpeed == 8){      
        //Speed 3 - Interstep = 2000
        MAXSPEED = 3;
        motor.setSpeed(MAXSPEED);
       }
       if (MotorSpeed == 16){
        //Speed 4 - Interstep = 2500
        MAXSPEED = 4;
        motor.setSpeed(MAXSPEED);
       }
       if (MotorSpeed == 32){
        //Speed 5 - Interstep = 3000
        MAXSPEED = 5;
        motor.setSpeed(MAXSPEED);
       }
			//do nothing yet
		}

		//Set full-step mode. 1600 steps per 360°
		if (!strcasecmp(cmd, "SF")) {
    digitalWrite(A0,HIGH);
    digitalWrite(A1,HIGH);
    digitalWrite(A2,LOW);
    isFullStep = true;
			//do nothing yet
		}

		//Set half-step mode.  3200 steps per 360°
		if (!strcasecmp(cmd, "SH")) {
      digitalWrite(A0,HIGH);
      digitalWrite(A1,HIGH);
      digitalWrite(A2,HIGH);
      isFullStep = false;
			//do nothing yet
		}

		//Set the new position where YYYY is a four-digit
		if (!strcasecmp(cmd, "SN")) {
			pos = hexstr2long(param);
			// stepper.enableOutputs(); // turn the motor on here ??
			//WvB: this seems strange, motor is turned on here. There should be another command to do this
			turnOn();
			motor.moveTo(pos);
		}

		//Set the current position where YYYY is a four-digit unsigned hex number.
		if (!strcasecmp(cmd, "SP")) {
			pos = hexstr2long(param);
			motor.setCurrentPosition(pos);
		}

	}// end if(eoc)


} // end loop

long hexstr2long(char *line) {
	long ret = 0;

	ret = strtol(line, NULL, 16);
	return (ret);
}

void turnOn() {
	if (useSleep) {
		digitalWrite(powerPin, HIGH);
		} else {
		digitalWrite(powerPin, LOW);
	}
	isRunning = true;
}
void turnOff() {
	if (useSleep) {
		digitalWrite(powerPin, LOW);
		} else {
		digitalWrite(powerPin, HIGH);
	}
	isRunning = false;
}
