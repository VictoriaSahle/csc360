/*
***************************************************************************************************
*** ID                  : xxxxxxxxx
*** Name                : Victoria Sahle
*** Date                : February 10, 2016
*** Program Name        : assign2.c
*** Program Description : Assignment 2: A simulation of an automated control system for a single lane 
***                       bridge. Threads are used to simulate trains approaching the bridge from two
***                       different directions. This program ensures that there is never more than 
***                       one train on the bridge at any given time.
***************************************************************************************************
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> 
#include <pthread.h>
#include "train.h"

/*
 * If you uncomment the following line, some debugging
 * output will be produced.
 *
 * Be sure to comment this line out again before you submit 
 */

int priorityCountEast = 0;
//#define DEBUG	1
#define true 1
#define false 0

int CurrentDirection; //current direction of train
int tCount; //# of trains on the bridge
int TrainsWaiting[3]; //# east/west bound waiting

pthread_mutex_t lock; //lock
pthread_cond_t EastWest[3]; //blocking east/west trains

void Init(void);
void ArriveBridge (TrainInfo *train);
void CrossBridge (TrainInfo *train);
void LeaveBridge (TrainInfo *train);

/*
 * This function is started for each thread created by the
 * main thread.  Each thread is given a TrainInfo structure
 * that specifies information about the train the individual 
 * thread is supposed to simulate.
 */
void * Train ( void *arguments )
{
	TrainInfo	*train = (TrainInfo *)arguments;

	/* Sleep to simulate different arrival times */
	usleep (train->length*SLEEP_MULTIPLE);
	
	ArriveBridge (train);
	CrossBridge  (train);
	LeaveBridge  (train); 

	/* I decided that the paramter structure would be malloc'd 
	 * in the main thread, but the individual threads are responsible
	 * for freeing the memory.
	 *
	 * This way I didn't have to keep an array of parameter pointers
	 * in the main thread.
	 */
	free (train);
	return NULL;
}
/* 
*FUNCTION Init():
*	This function initializes the bridge monitoring(lock).
*/
void Init(){

	tCount = 0;
	TrainsWaiting[0] = TrainsWaiting[1] = TrainsWaiting[2] = 0;
	pthread_mutex_init(&lock, (void*) NULL);
	pthread_cond_init(&(EastWest[1]), (void*) NULL);
	pthread_cond_init(&(EastWest[2]), (void*) NULL);

}

/*
* FUNCTION BridgeIsSafe():
*	This function tests if a train can proceed (if bridge is empty).
*/
int BridgeIsSafe(){
	if(tCount == 0)
		return true;
	else
		return false;
}

/*
 * FUNCTION ArriveBridge():
 *	This  function simulates the arrival of trains at the bridge.
 */
void ArriveBridge ( TrainInfo *train )
{
//	printf ("Train %2d arrives going %s\n", train->trainId, 
//			(train->direction == DIRECTION_WEST ? "West" : "East"));
	
	pthread_mutex_lock(&lock); //lock to ensure mutual exclusion
		if(!BridgeIsSafe()){ //Check if the brigde is safe (empty)
			TrainsWaiting[train->direction]++; //no, wait at the bridge
			while(!BridgeIsSafe()){ //safe to cross?
				pthread_cond_wait(&(EastWest[train->direction]), &lock);
			}
			TrainsWaiting[train->direction]--;
		}
		tCount++; //can proceed
		CurrentDirection = train->direction; //set direction
	pthread_mutex_unlock(&lock); //unlock


}

/*
 * FUNCTION CrossBridge():
 *	This function simulates the crossing of the bridge.
 */
void CrossBridge ( TrainInfo *train )
{
//	printf ("Train %2d is ON the bridge (%s)\n", train->trainId,
//			(train->direction == DIRECTION_WEST ? "West" : "East"));
//	fflush(stdout);
	//If an eastbound train is on the bridge, and no trains are currently waiting. Add to the proirity Count for the eastbound trains		
	if((TrainsWaiting[3 - train->direction] == 0) & (TrainsWaiting[train->direction] == 0) & (train->direction == 2)) {
				priorityCountEast++;
	}	
	
	/*
	 * This sleep statement simulates the time it takes to 
	 * cross the bridge.  Longer trains take more time.
	 */
	usleep (train->length*SLEEP_MULTIPLE);
	
	printf ("Train %2d is OFF the bridge(%s)\n", train->trainId, 
			(train->direction == DIRECTION_WEST ? "West" : "East"));
		
	fflush(stdout);
}

/*
 * FUNCTION LeaveBridge(): 
 *	This function makes the bridge available to the waiting train (in the correct order/priority).
 */
void LeaveBridge ( TrainInfo *train )
{
	pthread_mutex_lock(&lock); //lock 
		tCount--;   //one train exits
		if((TrainsWaiting[3 - train->direction] != 0) & (TrainsWaiting[train->direction] != 0)) { //bridge is empty, check for east wait priority
			priorityCountEast++;
			//If two eastern trains have left, one western train may cross
			if(priorityCountEast == 3){
				pthread_cond_signal(&(EastWest[1]));
				priorityCountEast = 0;
			}
			else
				pthread_cond_signal(&(EastWest[2]));
			
			

		}else if((TrainsWaiting[3 - train->direction] != 0)) //bridge is empty, check for opposite
			pthread_cond_signal(&(EastWest[3 - train->direction]));  //yes
		else
			pthread_cond_signal(&(EastWest[train->direction]));
	pthread_mutex_unlock(&lock);  //release the lock


}

int main ( int argc, char *argv[] )
{
	int		trainCount = 0;
	char 		*filename = NULL;
	pthread_t	*tids;
	int		i;

		
	/* Parse the arguments */
	if ( argc < 2 )
	{
		printf ("Usage: part1 n {filename}\n\t\tn is number of trains\n");
		printf ("\t\tfilename is input file to use (optional)\n");
		exit(0);
	}
	
	if ( argc >= 2 )
	{
		trainCount = atoi(argv[1]);
	
	}
	if ( argc == 3 )
	{
		filename = argv[2];
	}	
	
	initTrain(filename);
	
	/*
	 * Since the number of trains to simulate is specified on the command
	 * line, we need to malloc space to store the thread ids of each train
	 * thread.
	 */
	tids = (pthread_t *) malloc(sizeof(pthread_t)*trainCount);
	/*
	 * Create all the train threads pass them the information about
	 * length and direction as a TrainInfo structure
	 */
	Init();	
	for (i=0;i<trainCount;i++)
	{
		TrainInfo *info = createTrain();
		
//		printf ("Train %2d headed %s length is %d\n", info->trainId,
//			(info->direction == DIRECTION_WEST ? "West" : "East"),
//			info->length );

		if ( pthread_create (&tids[i],0, Train, (void *)info) != 0 )
		{
			printf ("Failed creation of Train.\n");
			exit(0);
		}
	}

	/*
	 * This code waits for all train threads to terminate
	 */
	for (i=0;i<trainCount;i++)
	{
		pthread_join (tids[i], NULL);
	}
	
	free(tids);
	return 0;
}

