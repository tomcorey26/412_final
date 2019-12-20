//
//  main.c
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2019-12-12
//	This is public domain code.  By all means appropriate it and change is to your
//	heart's content.
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <map>
#include <vector>
#include <utility>   
#include <iostream>
#include <fstream>
//
#include "gl_frontEnd.h"

using namespace std;

//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane(void);
void displayStatePane(void);
void initializeApplication(void);
void cleanupGridAndLists(void);

//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch
extern int	GRID_PANE, STATE_PANE;
extern int 	GRID_PANE_WIDTH, GRID_PANE_HEIGHT;
extern int	gMainWindow, gSubwindow[2];

//	Don't rename any of these variables
//-------------------------------------
//	The state grid and its dimensions (arguments to the program)
int** grid;
int numRows = -1;	//	height of the grid
int numCols = -1;	//	width
int numBoxes = -1;	//	also the number of robots
int numDoors = -1;	//	The number of doors.

int numLiveThreads = 0;		//	the number of live robot threads

//	robot sleep time between moves (in microseconds)
const int MIN_SLEEP_TIME = 1000;
int robotSleepTime = 100000;

//	An array of C-string where you can store things you want displayed
//	in the state pane to display (for debugging purposes?)
//	Dont change the dimensions as this may break the front end
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char** message;
time_t startTime;

// Holds all currently reserved squares
map<int,int> takenCoords;

//vector of robot structs
vector<Robot> robots;

//vector of door locations
vector< pair<int,int> > doorLocation;




//==================================================================================
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================


void displayGridPane(void)
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[GRID_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0, GRID_PANE_HEIGHT, 0);
	glScalef(1.f, -1.f, 1.f);
	
	//-----------------------------
	//	CHANGE THIS
	//-----------------------------
	//	Here I hard-code myself some data for robots and doors.  Obviously this code
	//	this code must go away.  I just want to show you how to display the information
	//	about a robot-box pair or a door.
	//	Important here:  I don't think of the locations (robot/box/door) as x and y, but
	//	as row and column.  So, the first index is a row (y) coordinate, and the second
	//	index is a column (x) coordinate.
	// int robotLoc[][2];
// int boxLoc[][2];
// int doorAssign[];	//	door id assigned to each robot-box pair
// int doorLoc[][2];
	
	for (int i=0; i<numBoxes; i++)
	{
		//	here I would test if the robot thread is still live
		//						row				column			row			column
		drawRobotAndBox(i, robots[i].roboCD[0], robots[i].roboCD[1], robots[i].boxCD[0],robots[i].boxCD[1] , robots[i].doorID);
	}

	for (int i=0; i<numDoors; i++)
	{
		//	here I would test if the robot thread is still alive
		//				row				column	
		drawDoor(i, doorLocation[i].first,doorLocation[i].second);
	}

	//	This call does nothing important. It only draws lines
	//	There is nothing to synchronize here
	drawGrid();

	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	
	glutSetWindow(gMainWindow);
}

void displayStatePane(void)
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[STATE_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//	Here I hard-code a few messages that I want to see displayed
	//	in my state pane.  The number of live robot threads will
	//	always get displayed.  No need to pass a message about it.
	time_t currentTime = time(NULL);
	double deltaT = difftime(currentTime, startTime);

	int numMessages = 3;
	sprintf(message[0], "We have %d doors", numDoors);
	sprintf(message[1], "I like cheese");
	sprintf(message[2], "Run time is %4.0f", deltaT);

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//
	//	You *must* synchronize this call.
	//
	//---------------------------------------------------------
	drawState(numMessages, message);
	
	
	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	
	glutSetWindow(gMainWindow);
}

//------------------------------------------------------------------------
//	You shouldn't have to touch this one.  Definitely if you don't
//	add the "producer" threads, and probably not even if you do.
//------------------------------------------------------------------------
void speedupRobots(void)
{
	//	decrease sleep time by 20%, but don't get too small
	int newSleepTime = (8 * robotSleepTime) / 10;
	
	if (newSleepTime > MIN_SLEEP_TIME)
	{
		robotSleepTime = newSleepTime;
	}
}

void slowdownRobots(void)
{
	//	increase sleep time by 20%
	robotSleepTime = (12 * robotSleepTime) / 10;
}


void writeInfoToFile() {
	ofstream outFile;
	outFile.open("robotSimulOut.txt");
	// write input params
	outFile << "Size: " << to_string(numCols*numRows) << " " << "Boxes: " << to_string(numBoxes)<< " " << "Doors: " <<to_string(numDoors) << endl;
	outFile << "" << endl;

	//print door coordinates
	for (unsigned int j = 0; j < doorLocation.size(); j++) {
		outFile << "Door " << j << ": " << "Row: " << doorLocation[j].first << " " << "Col: " << doorLocation[j].second << " | ";
	}
	//space
	outFile << endl << ""<< endl;

	//box
	for(unsigned int i=0; i < robots.size(); ++i) {
		outFile << "Box " << i << ": " << "row: " << robots[i].boxCD[0]<< " " << "col: " << robots[i].boxCD[1] << " | " ;
	}
	//space
	outFile << endl << ""<< endl;

	//robots
	for(unsigned int i=0; i < robots.size(); ++i) {
		outFile << "Robot " << i << ": " << "row: " << robots[i].roboCD[0]<< " " << "col: " << robots[i].roboCD[1] << " Door: " << robots[i].doorID << " | "; 
	}
	//space
	outFile << endl << ""<< endl;

	outFile.close();

	return;
}

//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function besides
//	the initialization of numRows, numCos, numDoors, numBoxes.
//------------------------------------------------------------------------
int main(int argc, char** argv)
{
	//	We know that the arguments  of the program  are going
	//	to be the width (number of columns) and height (number of rows) of the
	//	grid, the number of boxes (and robots), and the number of doors.
	//	You are going to have to extract these.  For the time being,
	//	I hard code-some values
	numCols = atoi(argv[1]);
	numRows = atoi(argv[2]);
	numBoxes = atoi(argv[3]);
	numDoors = atoi(argv[4]);
	

	//	Even though we extracted the relevant information from the argument
	//	list, I still need to pass argc and argv to the front-end init
	//	function because that function passes them to glutInit, the required call
	//	to the initialization of the glut library.
	initializeFrontEnd(argc, argv, displayGridPane, displayStatePane);
	
	//	Now we can do application-level initialization
	initializeApplication();

	//print info to output
	writeInfoToFile();

	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that 
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
	
	cleanupGridAndLists();
	
	//	This will probably never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}

//---------------------------------------------------------------------
//	Free allocated resource before leaving (not absolutely needed, but
//	just nicer.  Also, if you crash there, you know something is wrong
//	in your code.
//---------------------------------------------------------------------
void cleanupGridAndLists(void)
{
	for (int i=0; i< numRows; i++)
		free(grid[i]);
	free(grid);
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		free(message[k]);
	free(message);
}


void allocateGrid() {
	grid = (int**) malloc(numRows * sizeof(int*));
	for (int i=0; i<numRows; i++)
		grid[i] = (int*) malloc(numCols * sizeof(int));
	
	message = (char**) malloc(MAX_NUM_MESSAGES*sizeof(char*));
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		message[k] = (char*) malloc((MAX_LENGTH_MESSAGE+1)*sizeof(char));
}

bool isTouchingBorder(int row, int col) {

	if ( (row == 0) || (row == numRows-1) || (col == 0) || (col == numCols-1) ) {
		return true;
	}
	return false;
}

pair<int,int> generateFreshCoordinates (Type type) {
	// x is row y is col
	pair<int,int> coords;
	bool emptySpotFound = false;
	while (emptySpotFound == false) {

		int row = rand() % numRows;
		int col = rand() % numCols;

		if ((takenCoords.find(row) == takenCoords.end()) && (type == ROBOT || type == DOOR)) {
			takenCoords.insert({row,col});
			coords = make_pair(row,col);
			emptySpotFound = true;
		}
		else if (( takenCoords.find(row) == takenCoords.end()) && (!isTouchingBorder(row,col)) && (type == BOX)) {
			takenCoords.insert({row,col});
			coords = make_pair(row,col);
			emptySpotFound = true;
		}
	}
	return coords;
}

void initializeLocations() {
	//gen doors
	for (int j = 0; j < numDoors; j++) {
		doorLocation.push_back(generateFreshCoordinates(DOOR));
	}
	//gen robot/boxes
	for(int i = 0; i < numBoxes; i++) {
		struct Robot robot;
		//gen robot coords
 		pair<int,int> roboCoords = generateFreshCoordinates(ROBOT);
		robot.roboCD[0] = roboCoords.first;
		robot.roboCD[1] = roboCoords.second;
		//gen box coords
		pair<int,int> boxCoords = generateFreshCoordinates(BOX);
		robot.boxCD[0] = boxCoords.first;
		robot.boxCD[1] = boxCoords.second;
		//assign door id 
		robot.doorID = rand() % numDoors;
		//get location of assigned door;
		robot.doorCD[0] = doorLocation[robot.doorID].first;
		robot.doorCD[1] = doorLocation[robot.doorID].second;

		//add robot to robot vector
		robots.push_back(robot);
	}

	return;
}


void printEachStruct(vector<Robot> vec){
	for(unsigned int i=0; i < vec.size(); ++i) {
		cout << "Robot Coords: " << vec[i].roboCD[0]<< "," << vec[i].roboCD[1] << endl;
		cout << "Box Coords: " << vec[i].boxCD[0]<< "," << vec[i].boxCD[1] << endl ;
		cout << "Door Coords: " << vec[i].doorCD[0]<< "," << vec[i].doorCD[1] << endl ;
		cout << "Door ID: " << vec[i].doorID << endl;
	}
}
//==================================================================================
//
//	This is a part that you have to edit and add to.
//
//==================================================================================
void initializeApplication(void)
{
	//	Allocate the grid
	allocateGrid();
	//---------------------------------------------------------------
	//	All the code below to be replaced/removed
	//	I initialize the grid's pixels to have something to look at
	//---------------------------------------------------------------
	//	Yes, I am using the C random generator after ranting in class that the C random
	//	generator was junk.  Here I am not using it to produce "serious" data (as in a
	//	simulation), only some color, in meant-to-be-thrown-away code
	
	//	seed the pseudo-random generator
	startTime = time(NULL);
	srand((unsigned int) startTime);

	//	normally, here I would initialize the location of my doors, boxes,
	//	and robots, and create threads (not necessarily in that order).
	//	For the handout I have nothing to do.

	initializeLocations();

	// printEachStruct(robots);
}


