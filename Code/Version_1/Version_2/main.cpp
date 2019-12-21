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
#include <unistd.h>
#include <pthread.h>
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
Robot *robots;

pthread_mutex_t	fileWriteLock;

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
		if(robots[i].islive){
			drawRobotAndBox(i, robots[i].roboCD[0], robots[i].roboCD[1], robots[i].boxCD[0],robots[i].boxCD[1] , robots[i].doorID);
		}
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
	sprintf(message[1], "Num threads: %d", numLiveThreads);
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
	for(int i=0; i < numBoxes; ++i) {
		outFile << "Box " << i << ": " << "row: " << robots[i].boxCD[0]<< " " << "col: " << robots[i].boxCD[1] << " | " ;
	}
	//space
	outFile << endl << ""<< endl;

	//robots
	for(int i=0; i < numBoxes; ++i) {
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
	robots = (Robot*) malloc(numBoxes * sizeof(Robot));
	for(int i = 0; i < numBoxes; i++) {
		robots[i].roboId = i;
		//gen robot coords
 		pair<int,int> roboCoords = generateFreshCoordinates(ROBOT);
		robots[i].roboCD[0] = roboCoords.first;
		robots[i].roboCD[1] = roboCoords.second;
		//gen box coords
		pair<int,int> boxCoords = generateFreshCoordinates(BOX);
		robots[i].boxCD[0] = boxCoords.first;
		robots[i].boxCD[1] = boxCoords.second;
		//assign door id 
		robots[i].doorID = rand() % numDoors;
		//get location of assigned door;
		robots[i].doorCD[0] = doorLocation[robots[i].doorID].first;
		robots[i].doorCD[1] = doorLocation[robots[i].doorID].second;

		//add robot to robot vector
		// robots.push_back(robot);
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

vector< pair<Direction,int> > computeBoxPath (int boxLoc[2], int goalLoc[2]) {

	vector< pair<Direction,int> > path;
	//compute horizontal direction and steps
	int horzDistance = goalLoc[1] - boxLoc[1];
	Direction horzDirection = horzDistance > 0 ? EAST : WEST;
	path.push_back(make_pair(horzDirection,horzDistance));

	//compute vertical direction and steps
	int vertDistance = goalLoc[0] - boxLoc[0];
	Direction vertDirection = vertDistance > 0 ? SOUTH : NORTH;
	path.push_back(make_pair(vertDirection,vertDistance));

	return path;
}

vector< pair<Direction,int> > computeRobotPathToBox (int roboLoc[2], int boxLoc[2], Direction boxMoveDirection) {

	vector< pair<Direction,int> > path;

	//compute horizontal direction and steps
	int horzDistance = boxLoc[1] - roboLoc[1];
	//account for what direction the box will be moved in
	if (boxMoveDirection == EAST) {
		horzDistance--;
	} else {
		horzDistance++;
	}
	Direction horzDirection = horzDistance > 0 ? EAST : WEST;
	path.push_back(make_pair(horzDirection,horzDistance));

	//compute vertical direction and steps
	int vertDistance = boxLoc[0] - roboLoc[0];
	Direction vertDirection = vertDistance > 0 ? SOUTH : NORTH;
	path.push_back(make_pair(vertDirection,vertDistance));

	return path;
}

vector< pair<Direction,int> > computeRobotBoxAdjust (Direction firstDir, Direction secDir) {
	vector< pair<Direction,int> > path;
	if (firstDir == EAST && secDir == NORTH) {
		path.push_back(make_pair(SOUTH,1));
		path.push_back(make_pair(EAST,1));
	}
	else if (firstDir == EAST && secDir == SOUTH) {
		path.push_back(make_pair(NORTH,1));
		path.push_back(make_pair(EAST,1));
	}
	else if (firstDir == WEST && secDir == NORTH) {
		path.push_back(make_pair(SOUTH,1));
		path.push_back(make_pair(WEST,1));
	}
	else if (firstDir == WEST && secDir == SOUTH) {
		path.push_back(make_pair(NORTH,1));
		path.push_back(make_pair(WEST,1));
	}
	return path;
}

void writeActionToFile(ActionType type,int roboId,Direction direc) {
	ofstream outFile;
	outFile.open("robotSimulOut.txt",fstream::app);

	char direction;
	if (direc == NORTH){
		direction = 'N';
	}else if (direc == SOUTH){
		direction = 'S';
	}else if (direc == EAST){
		direction = 'E';
	}else if (direc == WEST){
		direction = 'W';
	}

	outFile << "robot " << roboId << " ";
	if (type == MOVE) {
		outFile << "move " << direction << endl;
	} else if (type == PUSH) {
		outFile << "push " << direction << endl;

	} else {
		outFile << "end" << endl;
	}
	outFile.close();
}

void move(Direction direc,Robot *robot) {
	if (direc == NORTH) {
		robot->roboCD[0] -= 1;
	}
	else if (direc == SOUTH) {
		robot->roboCD[0] += 1;
	}
	else if (direc == EAST) {
		robot->roboCD[1] += 1;
	}
	else if (direc == WEST) {
		robot->roboCD[1] -= 1;
	}
	//TODO syncronize
	writeActionToFile(MOVE,robot->roboId,direc);
	return;
}

void push(Direction direc,Robot *robot) {
	if (direc == NORTH) {
		robot->roboCD[0] -= 1;
		robot->boxCD[0] -= 1;
	}
	else if (direc == SOUTH) {
		robot->roboCD[0] += 1;
		robot->boxCD[0] += 1;
	}
	else if (direc == EAST) {
		robot->roboCD[1] += 1;
		robot->boxCD[1] += 1;
	}
	else if (direc == WEST) {
		robot->roboCD[1] -= 1;
		robot->boxCD[1] -= 1;
	}
	//TODO syncronize
	writeActionToFile(PUSH,robot->roboId,direc);
	return;
}

void interpRoboInstructions(vector< pair<Direction,int> > directions, Robot *robot) {
	//move to correct spot to push box	
	for (unsigned int i = 0; i < directions.size(); i++)
	{
		Direction direc = directions[i].first;
		int steps = directions[i].second; 
		
		for (int j = 0; j < abs(steps); j++)
		{
			usleep(100000);
			move(direc,robot);
		}
		
	}
}

void interpBoxInstructions(pair<Direction,int> directions, Robot *robot) {
	Direction direc = directions.first;
	int steps = directions.second; 
	
	for (int j = 0; j < abs(steps); j++)
	{
		usleep(100000);
		push(direc,robot);
	}
}

void* threadFunc(void* param) {
	Robot* robot = (Robot*) param;
	robot->islive = true;

	numLiveThreads++;
	usleep(100000);
	//get box path
	//returns vector of pairs direction/steps ex:{{WEST,2},{SOUTH, 5}}
	vector< pair<Direction,int> > boxPath = computeBoxPath(robot->boxCD,robot->doorCD);

	// //compute robot path to box
	vector< pair<Direction,int> > roboPath = computeRobotPathToBox(robot->roboCD,robot->boxCD,boxPath[0].first);

	// //compute robo position adjust
	vector< pair<Direction,int> > adjustedRoboPath = computeRobotBoxAdjust(boxPath[0].first,boxPath[1].first);

	// //move robot to box
	interpRoboInstructions(roboPath,robot);

	// //horz push box
	interpBoxInstructions(boxPath[0],robot);
	
	// //adjust path to push
	interpRoboInstructions(adjustedRoboPath,robot);

	// //vert push box
	interpBoxInstructions(boxPath[1],robot);
	
	// //TODO syncronize
	// //write end to file (direction is arbitrary)
	pthread_mutex_lock(&fileWriteLock);
	writeActionToFile(END,robot->roboId,NORTH);
	pthread_mutex_unlock(&fileWriteLock);

	numLiveThreads--;	
	robot->islive = false;
	return NULL;
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

	//	seed the pseudo-random generator
	startTime = time(NULL);
	srand((unsigned int) startTime);

	//	normally, here I would initialize the location of my doors, boxes,
	//	and robots, and create threads (not necessarily in that order).
	//	For the handout I have nothing to do.

	//init threads
	pthread_t robotThread[numBoxes];

	//init mutexs
	pthread_mutex_init(&fileWriteLock, NULL);

	//init starting locations 
	initializeLocations();

	//print info to output
	writeInfoToFile();
	//Move each box to goal
	for(int i = 0; i < numBoxes; i++) {
		int err = pthread_create(robotThread + i, NULL, threadFunc, robots + i);
		if (err != 0)
		{
			exit(1);
		}
	}
	// printEachStruct(robots);
}


