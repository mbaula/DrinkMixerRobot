/*
GENE121 Course Project: Drink mixing machine
Farris Matar
Hewitt McGaughey
Erfan Huq
Mark Baula
*/
#include "PC_FileIO.c"
#include "EV3Servo-lib-UW.c"

TFileHandle inFile;
TFileHandle outFile;

const int SEC_TO_MS = 1000;
const float C1_RATE = 33; //left container, rounded up to account for error in measurement
const float C2_RATE = 28; //right container, rounded up to account for error in measurement
const int CONTAINER1 = 1; //left container
const int CONTAINER2 = 2; //right container
const int STIRRINGROD = 3;
const int LOWERDISTANCE = 10 * 360 / (2 * PI * 1.5); //note that 2.75 should be replaced with radius of wheel we're using

// Calculates time a valve needs to be open, in ms, based on how much drink is being poured out
int calculateTime(int drinkAmt, int container) {
	if(container == 1)
		return (drinkAmt/C1_RATE) * SEC_TO_MS;
	else
		return (drinkAmt/C2_RATE) * SEC_TO_MS;
}
//Rotates motorA to the given position.  Modified for use with variables above
void rotate(int pos, int & motorPos)
{
	//motorA is the rotating motor
	//gyro plugged into port 2
	//pos -1 = left container, 0 = stirring rod, 1 = right container

	//sets var to correct value for use with `the function
	if(pos == 1)//left
		pos = 0;
	else if(pos == 3)//right
		pos = 1;

	pos *= 48;
	float rotateDist = pos - motorPos;
	if(rotateDist > 0) //rotating forward
	{
		motor[motorA] = 5;
		while(nMotorEncoder[motorA] <= pos)
		{}
	}

	else
	{
		motor[motorA] = -5;
		while(nMotorEncoder[motorA] >= pos)
		{}
	}

	motor[motorA] = 0;
	motorPos = pos;
}
//Calibrates motorA in the correct starting position (should run on startup and shutdown)
void calibrateMotorEnc(){
	motor[motorA] = -10;
	wait1Msec(1000);
	motor[motorA] = 0;
	nMotorEncoder[motorA] = 0;
	motor[motorA] = 3;
	while(nMotorEncoder[motorA] <= 37)
	{
		displayString(1, "%d", nMotorEncoder[motorA]);
	}
	motor[motorA] = 0;
	nMotorEncoder[motorA] = 0;
}
//Closes valve completely by overrotating and then resetting servo
void closeValve(int servo)
{
	setServoPosition(S1, servo, -45);
	wait1Msec(1000);
	setServoPosition(S1, servo, 0);
}

// Updates drink levels by changes entered
void writeToFile(int drink1Change, int drink2Change) {
	openReadPC(inFile, "drinkLevels_txt.txt");
	int drinkLevels[2] = {0,0};

	// Reads in current levels
	readIntPC(inFile, drinkLevels[0]);
	readIntPC(inFile, drinkLevels[1]);
	closeFilePC(inFile);

	// Re-writes values
	drinkLevels[0] += drink1Change;
	drinkLevels[1] += drink2Change;

	openWritePC(outFile, "drinkLevels_txt.txt");

	writeLongPC(outFile, drinkLevels[0]);
	writeTextPC(outFile, " ");
	writeLongPC(outFile, drinkLevels[1]);

	closeFilePC(outFile);
}

// Same function as above but rewrites requested drink to 0mL
void writeToFile(int drinkNum) {
	openReadPC(inFile, "drinkLevels_txt.txt");
	int drinkLevels[2] = {0,0};

	// Reads in current levels
	readIntPC(inFile, drinkLevels[0]);
	readIntPC(inFile, drinkLevels[1]);
	closeFilePC(inFile);

	// Re-writes values
	drinkLevels[drinkNum-1] = 0;

	openWritePC(outFile, "drinkLevels_txt.txt");

	writeLongPC(outFile, drinkLevels[0]);
	writeTextPC(outFile, " ");
	writeLongPC(outFile, drinkLevels[1]);

	closeFilePC(outFile);
}

// Gets drink level of a certain drink
int getDrinkLevel(int drinkNum) {
	openReadPC(inFile, "drinkLevels_txt.txt");
	int drinkLevels[2] = {0,0};

	// Reads in current levels
	readIntPC(inFile, drinkLevels[0]);
	readIntPC(inFile, drinkLevels[1]);
	closeFilePC(inFile);

	// Returns requested value
	return drinkLevels[drinkNum-1];
}

// Adds refill amount of a drink refilled to file
void refillDrink(int drinkNum) {
	// Setting up some necessary variables
	int refillAmt = 0;
	int maxRefill = 0;
	maxRefill = 500 - getDrinkLevel(drinkNum);

	// Displays refill amount prompt
	eraseDisplay();
	displayString(0,"Refill by how much?");
	displayString(1,"+1mL: Up");
	displayString(2,"+10mL: Right");
	displayString(3,"-1mL: Down");
	displayString(4,"-10mL: Left");
	displayString(5,"Proceed: Centre");
	displayString(6,"Current amount: %d mL",refillAmt);

	// Loops until centre button is pressed
	while (!getButtonPress(buttonEnter)) {
		// Waits for a button to be pushed
		while (!getButtonPress(buttonAny)) {}

		// Checks which button was pressed
		if (getButtonPress(buttonUp)) {
			while (getButtonPress(buttonAny)) {}
			refillAmt++;
			if (refillAmt > maxRefill)
				refillAmt = maxRefill;
		}

		else if (getButtonPress(buttonRight)) {
			while (getButtonPress(buttonAny)) {}
			refillAmt+= 10;
			if (refillAmt > maxRefill)
				refillAmt = maxRefill;
		}

		else if (getButtonPress(buttonDown)) {
			while (getButtonPress(buttonAny)) {}
			refillAmt--;
			if (refillAmt < 0)
				refillAmt = 0;
		}

		else if (getButtonPress(buttonLeft)) {
			while (getButtonPress(buttonAny)) {}
			refillAmt-= 10;
			if (refillAmt < 0)
				refillAmt = 0;
		}

		displayString(6,"Current amount: %d mL",refillAmt);
	}

	int refillAmts[2] = {0,0};
	// Sets the change amount of requested drink to refill amount and of other drink to 0
	refillAmts[drinkNum-1] = refillAmt;
	refillAmts[fabs(drinkNum-2)] = 0;

	writeToFile(refillAmts[0], refillAmts[1]);

	// Wait for all buttons to be released
	while (getButtonPress(buttonAny)) {}
}

// Adds refill amount to all drinks at once
void refillAll() {
	// Setting up some necessary variables
	int refillAmt = 0;
	int maxRefill = 0;
	maxRefill = 500 - getDrinkLevel(1);
	if (maxRefill > 500 - getDrinkLevel(2))
		maxRefill = 500 - getDrinkLevel(2);

	// Displays refill amount prompt
	eraseDisplay();
	displayString(0,"Refill by how much?");
	displayString(1,"+1mL: Up");
	displayString(2,"+10mL: Right");
	displayString(3,"-1mL: Down");
	displayString(4,"-10mL: Left");
	displayString(5,"Proceed: Centre");
	displayString(6,"Current amount: %d mL",refillAmt);

	// Loops until centre button is pressed
	while (!getButtonPress(buttonEnter)) {
		// Waits for a button to be pushed
		while (!getButtonPress(buttonAny)) {}

		// Checks which button was pressed
		if (getButtonPress(buttonUp)) {
			while (getButtonPress(buttonAny)) {}
			refillAmt++;
			if (refillAmt > maxRefill)
				refillAmt = maxRefill;
		}

		else if (getButtonPress(buttonRight)) {
			while (getButtonPress(buttonAny)) {}
			refillAmt+= 10;
			if (refillAmt > maxRefill)
				refillAmt = maxRefill;
		}

		else if (getButtonPress(buttonDown)) {
			while (getButtonPress(buttonAny)) {}
			refillAmt--;
			if (refillAmt < 0)
				refillAmt = 0;
		}

		else if (getButtonPress(buttonLeft)) {
			while (getButtonPress(buttonAny)) {}
			refillAmt-= 10;
			if (refillAmt < 0)
				refillAmt = 0;
		}

		displayString(6,"Current amount: %d mL",refillAmt);
	}

	writeToFile(refillAmt, refillAmt);

	// Wait for all buttons to be released
	while (getButtonPress(buttonAny)) {}
}

// Drains a selected container of fluid
void drain(int container, int & motorPos){
	writeToFile(container);
	rotate(container, motorPos);

	// Get confirmation from user
	eraseDisplay();
	displayString(0,"Drain container %d?",container);
	displayString(1,"Yes: centre button");
	displayString(2,"No: any other button");

	while (!getButtonPress(buttonAny)) {}

	if (getButtonPress(buttonEnter)) {
		// Waits for buttons to be released
		while (getButtonPress(buttonAny)) {}

		// Opens container valve for required amount of time plus one second to ensure complete drainage
		clearTimer(T1);
		setServoPosition(S1, container, 90);
		int drainTime = calculateTime(getDrinkLevel(container), container)+1000;

		while (time1[T1] < drainTime) {}

		// Closes container valve and rotates it back to the stirring rod.
		closeValve(container);
		rotate(3, motorPos);
	}
	// Waits for buttons to be released
	while (getButtonPress(buttonAny)) {}
}

// Drains every container of fluid
void drainAll(int & motorPos){
	for (int container = 1; container < 3; container++){
		drain(container, motorPos);
	}
}

// Stirs the drink after being poured
void stirDrink(int & motorPos)
{
	rotate(STIRRINGROD, motorPos);

	//assume need to lower 5cm
	//assume using motorA

	nMotorEncoder[motorB] = 0;
	motor[motorB] = 25;

	while(nMotorEncoder[motorB] < LOWERDISTANCE)
	{}

	motor[motorB] = 0;

	//assume motorB for stirrer
	clearTimer(T1);
	while(time1[T1] < 10000)
	{
		motor[motorC] = 25;
	}

	motor[motorC] = 0;

	motor[motorB] = -25;

	while(nMotorEncoder[motorB] > 0) {}

	motor[motorB] = 0;

}

void pourDrinks(int drinkSize, int drink1Amt, int drink2Amt, int & motorPos) {
	eraseDisplay();
	int drink1Level = getDrinkLevel(1);
	int drink2Level = getDrinkLevel(2);

	if(drink1Level < drink1Amt) {
		displayString(0,"Drink 1 too low");
		wait1Msec(5000);
	}
	else if(drink2Level < drink2Amt) {
		displayString(0,"Drink 2 too low");
		wait1Msec(5000);
	}
	else {
		//pouring from container1 if it was requested
		if (drink1Amt > 0) {
			rotate(CONTAINER1, motorPos);
			int timeDrink1 = calculateTime(drink1Amt, CONTAINER1);
			clearTimer(T1);

			setServoPosition(S1,CONTAINER1,90);

			while(time1[T1] < timeDrink1) {}

			closeValve(CONTAINER1);
		}

		//pouring from container2 if it was requested
		if (drink2Amt > 0) {
			rotate(CONTAINER2, motorPos);
			int timeDrink2 = calculateTime(drink2Amt, CONTAINER2);
			clearTimer(T1);

			setServoPosition(S1,CONTAINER2,90);

			while(time1[T1] < timeDrink2) {}

			closeValve(CONTAINER2);
		}
		stirDrink(motorPos); // Stirs drink
		writeToFile(-drink1Amt, -drink2Amt);
	}
}

task main() {
	// Initializing sensors
	SensorType[S1] = sensorI2CCustom;
	int motorPos = 0;
	calibrateMotorEnc(); //resetting motor encoder

	// Have user select menu
	displayString(0, "Pick user or maintenance menu");
	displayString(1, "User: right button");
	displayString(2, "Maintenance: left button");

	// Loops till one of the requested buttons pressed
	while (!getButtonPress(buttonLeft) && !getButtonPress(buttonRight)) {}

	// If user chooses maintenance menu
	if (getButtonPress(buttonLeft)) {
		// Waits for button to be released
		while (getButtonPress(buttonAny)) {}
		eraseDisplay();

		bool shutdown = false;
		int taskSelected = 0;
		const int MAINT_TASKS = 5;
		string maintTasks[MAINT_TASKS] = {"Refill 1 drink", "Refill all drinks", "Drain 1 drink", "Drain all drinks", "Shutdown"};

		// Looping until robot is shutdown.
		while (!shutdown) {
			// Rewriting menu again
			displayString(0,"Use up/down to look at tasks");
			displayString(1,"Use center to select a task");
			displayString(2, maintTasks[taskSelected]);
			// Looping until center button pressed
			while (!getButtonPress(buttonEnter)) {
				// Waits for a button to be pressed
				while(!getButtonPress(buttonAny)) {}

				// Checks which button was pushed
				// Increment task # by 1
				if (getButtonPress(buttonUp)) {
					while (getButtonPress(buttonAny)) {}
					taskSelected++;
					if (taskSelected >= MAINT_TASKS)
						taskSelected = 0;
				}
				// Decrement task # by 1
				else if (getButtonPress(buttonDown)) {
					while (getButtonPress(buttonAny)) {}
					taskSelected--;
					if (taskSelected < 0)
						taskSelected = MAINT_TASKS-1;
				}

				// Rewrite task selected
				displayString(2, maintTasks[taskSelected]);
			}
			// Seeing which task was selected and continuing based on that
			// Refill one drink
			if (taskSelected == 0) {
				// Displays drink selection prompt
				eraseDisplay();
				displayString(0,"Refill which drink?");
				displayString(1,"Drink 1: Left");
				displayString(2,"Drink 2: Right");

				int drinkNum = 0;
				// Waits for a prompted button to be pressed
				while (!getButtonPress(buttonLeft) && !getButtonPress(buttonRight)) {}

				if (getButtonPress(buttonLeft))
					drinkNum = 1;
				else
					drinkNum = 2;

				while (getButtonPress(buttonAny)) {}

				refillDrink(drinkNum);
			}

			// Refill all drinks
			else if (taskSelected == 1) {
				// Waits for buttons to be released
				while (getButtonPress(buttonAny)) {}
				refillAll();
			}

			// Drain one drink
			else if (taskSelected == 2) {
				// Displays drink selection prompt
				eraseDisplay();
				displayString(0,"Drain which drink?");
				displayString(1,"Drink 1: Left");
				displayString(2,"Drink 2: Right");

				int drinkNum = 0;
				// Waits for a prompted button to be pressed
				while (!getButtonPress(buttonLeft) && !getButtonPress(buttonRight)) {}

				if (getButtonPress(buttonLeft))
					drinkNum = 1;
				else
					drinkNum = 2;

				while (getButtonPress(buttonAny)) {}

				eraseDisplay();
				drain(drinkNum, motorPos);
			}
			// Drain all
			else if (taskSelected == 3) {
				eraseDisplay();
				drainAll(motorPos);
			}
			// Shutdown
			else if (taskSelected == 4) {
				// Wait for centre button to be released to avoid accidental double input
				while (getButtonPress(buttonAny)) {}
				// Displays confirmation prompt
				eraseDisplay();
				displayString(0,"Shutdown program?");
				displayString(1,"Yes: centre button");
				displayString(2,"No: any other button");

				while (!getButtonPress(buttonAny)) {}

				// Sees if shut down was confirmed
				if (getButtonPress(buttonEnter)) {
					// Indicates shutdown request to loop
					shutdown = true;
					rotate(CONTAINER1, motorPos); //rotates back to position 1 to recalibrate better
				}
				// Waits for buttons to be released
				while (getButtonPress(buttonAny)) {}
			}
			eraseDisplay();
		}
	}

	// If user chooses user menu
	else if (getButtonPress(buttonRight)) {
		// Waits for button to be released
		while (getButtonPress(buttonAny)) {}
		eraseDisplay();

		bool shutdown = false;
		int sizeSelected = 0;
		const int SIZES = 4;
		string userSelect[SIZES] = {"100mL cup","150mL cup","200mL cup","Shutdown"};
		int cupSizes[SIZES-1] = {100,150,200};

		// Looping until shutdown option selected.
		while (!shutdown) {
			// Rewriting menu again
			displayString(0,"Use up/down to look at sizes");
			displayString(1,"Use center to select a size");
			displayString(2, userSelect[sizeSelected]);
			// Looping until center button pressed
			while (!getButtonPress(buttonEnter)) {
				// Waits for a button to be pressed
				while(!getButtonPress(buttonAny)) {}

				// Checks which button was pushed
				// Increment size # by 1
				if (getButtonPress(buttonUp)) {
					while (getButtonPress(buttonAny)) {}
					sizeSelected++;
					if (sizeSelected > SIZES-1)
						sizeSelected = 0;
				}
				// Decrement size # by 1
				else if (getButtonPress(buttonDown)) {
					while (getButtonPress(buttonAny)) {}
					sizeSelected--;
					if (sizeSelected < 0)
						sizeSelected = SIZES-1;
				}

				// Rewrite task selected
				displayString(2, userSelect[sizeSelected]);
			}
			// Checking if shutdown button was selected
			if (sizeSelected == 3) {
				// Waits for center button to be released to avoid accidental double input
				while (getButtonPress(buttonAny)) {}
				// Displays confirmation prompt
				eraseDisplay();
				displayString(0,"Shutdown program?");
				displayString(1,"Yes: centre button");
				displayString(2,"No: any other button");

				while (!getButtonPress(buttonAny)) {}

				// Sees if shut down was confirmed
				if (getButtonPress(buttonEnter)) {
					// Indicates shutdown request to loop
					shutdown = true;
					rotate(CONTAINER1, motorPos);
				}
				// Waits for buttons to be released
				while (getButtonPress(buttonAny)) {}
			}
			else {
				// Waits for buttons to be released
				while (getButtonPress(buttonAny)) {}

				// Display drink selection prompt
				eraseDisplay();
				displayString(0,"Pour which drinks?");
				displayString(1,"Drink 1: Left");
				displayString(2,"Drink 2: Right");
				displayString(3,"Both drinks: Centre");

				// Setting up array of drink percentages
				float drinkPercents[2] = {0,0};

				// Waits for a prompted button to be pressed
				while (!getButtonPress(buttonLeft) && !getButtonPress(buttonRight) &&  !getButtonPress(buttonEnter)) {}

				if (getButtonPress(buttonLeft)) {
					drinkPercents[0] = 1;
				}
				else if (getButtonPress(buttonRight)) {
					drinkPercents[1] = 1;
				}
				else {
					// Displays refill amount prompt
					eraseDisplay();
					displayString(0,"Pour how much of drink 1?");
					displayString(1,"+1%%: Up");
					displayString(2,"+10%%: Right");
					displayString(3,"-1%%: Down");
					displayString(4,"-10%%: Left");
					displayString(5,"Proceed: Centre");
					displayString(6,"Current amount: %d percent",drinkPercents[0]*100);
					float maxPour = 1-drinkPercents[0]-drinkPercents[1];

					// Waits for buttons to be released
					while (getButtonPress(buttonAny)) {}

					// Loops until centre button is pressed
					while (!getButtonPress(buttonEnter)) {
						// Waits for a button to be pushed
						while (!getButtonPress(buttonAny)) {}

						// Checks which button was pressed
						if (getButtonPress(buttonUp)) {
							while (getButtonPress(buttonAny)) {}
							drinkPercents[0]+= 0.01;
							if (drinkPercents[0] > maxPour)
								drinkPercents[0] = maxPour;
						}

						else if (getButtonPress(buttonRight)) {
							while (getButtonPress(buttonAny)) {}
							drinkPercents[0]+= 0.1;
							if (drinkPercents[0] > maxPour)
								drinkPercents[0] = maxPour;
						}

						else if (getButtonPress(buttonDown)) {
							while (getButtonPress(buttonAny)) {}
							drinkPercents[0]-= 0.01;
							if (drinkPercents[0] < 0)
								drinkPercents[0] = 0;
						}

						else if (getButtonPress(buttonLeft)) {
							while (getButtonPress(buttonAny)) {}
							drinkPercents[0]-= 0.1;
							if (drinkPercents[0] < 0)
								drinkPercents[0] = 0;
						}

						displayString(6,"Current amount: %d percent",drinkPercents[0]*100);
					}
					// Sets second percentage to be 100 minus the first
					drinkPercents[1] = 1-drinkPercents[0];

					// Waits for buttons to be released
					while (getButtonPress(buttonAny)) {}
				}
				// Waits for buttons to be released
				while (getButtonPress(buttonAny)) {}

				// Displays confirmation prompt
				eraseDisplay();
				displayString(0,"Drink 1: %d mL",cupSizes[sizeSelected]*drinkPercents[0]);
				displayString(1,"Drink 2: %d mL",cupSizes[sizeSelected]*drinkPercents[1]);
				displayString(2,"Confirm amounts?");
				displayString(3,"Yes: centre button");
				displayString(4,"No: any other button");

				while (!getButtonPress(buttonAny)) {}

				// Sees if amounts were confirmed
				if (getButtonPress(buttonEnter)) {
					// Waits for buttons to be released
					while (getButtonPress(buttonAny)) {}

					// Pours & stirs drink
					pourDrinks(cupSizes[sizeSelected],cupSizes[sizeSelected]*drinkPercents[0],cupSizes[sizeSelected]*drinkPercents[1], motorPos);
					eraseDisplay();
				}
				// Waits for buttons to be released
				while (getButtonPress(buttonAny)) {}
			}
			eraseDisplay();
		}
	}
}
