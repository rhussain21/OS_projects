
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <getopt.h> 
#include <unistd.h> 

using namespace std; 


enum TRANSITION {
	TRANS_TO_READY = 0,
	TRANS_TO_RUN = 1,
	TRANS_TO_BLOCK = 2,
	TRANS_TO_PREEMPT = 3
};

//create enum for prev state 
enum PREV_STATE {
	CREATED = 0,
	READY = 1,
	RUNNING = 2,
	BLOCKED = 3,
	PREEMPTION = 4
};

int CURRENT_TIME = 0;
int ofs;
int quantum = 10000; //this is provided via getOpt. high number set so it wont affect FCFS, LCFS, SRTF algos
int totalCPU = 0;

double CPUutilization = 0; 
double IOutilization = 0; 
double avgTT = 0;
double avgCW = 0;
double avgThruput = 0;

bool CALL_SCHEDULER = true;
bool prio_flag = false;  // to use active and expired queues
bool preprio = false;  // to enable preemption
bool attemptPrio = false;
bool v_flag = false; //verbose output
string schedName = "" ;



/********  DEFINE PROCESS STRUCT **********/

struct PROCESS {

		int pid; //GIVEN
		int AT; //arrival time (AT)  GIVEN
		int TC; //Total CPU time (TC) GIVEN
		int CB; //CPU burst (CB) GIVEN
		int IO; //IO burst (IO) GIVEN
		int FT; //finishing time (FT)
		int TT ; //Turnaround time (TT)  FT-AT
		int IT = 0; //IT AVG I/O time
		int CW = 0; //CW AVT CPU wait time
		int staticPRIO; //ASSIGNED LATER
		int dynamicPRIO;
		int timeInPrevState = 0;  
		int timeRemaining; //  TC -  timeInPrevState
		int state_ts; // might not need
		int currentBurst;

		bool preempted = false;
		bool running = false;
		bool blocked = false; 
		bool completed = false;


	//constructor 
	PROCESS(int id, int tempAT, int tempTC, int tempCB,int tempIO) {
		pid = id;
		AT = tempAT;
		TC = tempTC;
		CB = tempCB;
		IO = tempIO;
		timeRemaining = tempTC;

	}
	//122119 added constructor with PRIO
	PROCESS(int id, int tempAT, int tempTC, int tempCB,int tempIO, int tempPRIO) {
		pid = id;
		AT = tempAT;
		TC = tempTC;
		CB = tempCB;
		IO = tempIO;
		timeRemaining = tempTC;
		staticPRIO = tempPRIO;
		dynamicPRIO = tempPRIO; 

	}

};

vector <PROCESS*> prcList; //consider making this a queue 

/******** DEFINE EVENT STRUCT **********/

struct EVENT{
		int pid; 
		int time_stamp;
		int transition;  // new state
		int prev_state; //time in previous state?
		PROCESS* evtPrc;  // Is this needed?

		//constructor w/ timestamp

	EVENT(int id, int ts, int newstate, int prevstate, PROCESS* ep) {
		pid = id;
		time_stamp = ts;
		transition = newstate;
		prev_state = prevstate;
		evtPrc = ep;
	}

	//constructor w/o timestamp

	EVENT(int id, int newstate, int prevstate, PROCESS* ep) {
		pid = id;
		transition = newstate;
		prev_state = prevstate;
		evtPrc = ep;
	}


};	

vector <EVENT*> eventList; //consider making this a queue 
vector <PROCESS*> runQueue; // aka readyQueue or active queue 
vector <PROCESS*> expiredQueue; 
vector <PROCESS*> tempQueue; //to help switch runQueue and expiredQueue

/****************** PARSE INPUT DATA  ******************/

string output;
void parse_data (FILE* file) {

	char* str2 = (char*)malloc(sizeof(char) * 200);
	char* ret2;
	//string output; 
	
	while(1) {
		ret2 = fgets(str2, 200, file);
		if (ret2 == NULL){
			//cout << "REACHED END OF FILE" << endl;
			break; 
		}
		else if (ret2[0] != '#') {
			//cout << "TEST: THIS STRING DOES NOT HAVE A #" << endl;
			output.append(ret2);
		}
	}

/******************** CREATE PROCESSES ***********************/

	stringstream buf(output);
	string temp;
	int i = 0;
	int tempAT, tempCT, tempCB, tempIB, tempPRIO;
	int prc_indicator = 0; // Indictor for keeping track of next process in FCFS algo


	PROCESS* prc ; //= new PROCESS(i, tempAT, tempCT, tempCB, tempIB);
	while (1) {

		getline(buf,temp,'\n');

		stringstream buf2(temp);
		buf2 >> tempAT >> tempCT >> tempCB >> tempIB;

		//tempPRIO = myrandom(4);
		prc = new PROCESS(i, tempAT, tempCT, tempCB, tempIB);
		prcList.push_back(prc);

		i++;

		//sloppy! Re-do this later
		if(buf.rdbuf()->in_avail() == 0) {
			break;
		} 
	}

/******************** GENERATE "CREATED" EVENTS ***********************/
	EVENT* newEvt; // =EVENT(int id, int ts, int newstate, int prevstate)
	for (int i = 0; i < prcList.size(); i++) {

		newEvt = new EVENT(prcList[i]->pid, prcList[i]->AT,0,0, prcList[i]); 
		eventList.push_back(newEvt);

		//cout << "TROUBLESHOOT: Confirm that event #" <<i << " has been created." << endl;
	}

	//cout << "EvtList has " << eventList.size() << " elements. " << endl;
}


/****************** PARSE RANDOM AND CREATE RANDOM VALUES ******************/

int randLimit;
int* randvals;

void createRandVals(FILE* rand_file) {

	char* str = (char*)malloc(sizeof(char) * 200); 
	char *ret; 

	ret = fgets(str, 200, rand_file);
	randLimit = stoi(str);

	//set first number to randLimit 
	
	randvals = new int[randLimit];
	

	for(int i = 0; i < randLimit; i++){

		ret = fgets(str, 200, rand_file);
	
			//ret = fgets(str, 200, rand_file);
		randvals[i] = stoi(ret); 
		
		//cout << "i: " << i << " " <<randvals[i] << 
		//cout << i << ": " << randvals[i]  << endl;
	}

}

//create function for randomNumber
int myrandom(int burst) { 
	//cout << "ofs: " << ofs << " burst: "<< burst;
	if (ofs > randLimit) {
		//prevent out of bounds exception
		ofs = 0; 

	}

	//cout << "myrandom has been called. ofs is " << ofs << " burst is " << burst << " randval is " << randvals[ofs] ;
	int output = (randvals[ofs] % burst)+1;  
	//cout << " randval: "<< randvals[ofs] << " burst: "<< burst <<" output: " << output << endl;
	ofs++;
	
	return output;	
}

int myPRIO(int burst) {
	if (ofs > randLimit) {
		//prevent out of bounds exception
		ofs = 0; 

	}

	//cout << "myrandom has been called. ofs is " << ofs << " burst is " << burst << " randval is " << randvals[ofs] ;
	int output = (randvals[ofs] % burst);  
	//cout << " randval: "<< randvals[ofs] << " output: " << output << endl;
	ofs++;
	
	return output;	
}


void addPRIO(){
		//TEMPORARY: 122119  populate prio for each of the processes
	for (int i = 0; i < prcList.size(); i++) {
		prcList[i]->staticPRIO = myPRIO(4);
		prcList[i]->dynamicPRIO = prcList[i]->staticPRIO ;
	}
}





EVENT* get_event() {
	EVENT* rtnPointer = NULL; 
	int tempTime = 10000000;
	int idx; 
	/*
	if (CURRENT_TIME >200) {
		cout << "Get_event has been called.  Total "<<eventList.size() << " elements." << endl;
	} */
	//cout << "Get_event has been called.  Total "<<eventList.size() << " elements." << endl;

	if (!(eventList.empty())) {
		for (int i = 0; i < eventList.size(); i++) { /*
			if (CURRENT_TIME > 200) {
				cout << "iterating thru event list (total "<<eventList.size() << " elements) and now at position... " << i << endl;
				cout << "This event timestamp is " << eventList[i]->time_stamp<< endl;
				cout << "@ event #" << i << ": Timestamp: "<< eventList[i]->time_stamp << " Transition: "<< eventList[i]->transition <<endl;
			}	*/
			rtnPointer = eventList[i]; 


			if (rtnPointer->time_stamp < tempTime ) {

				tempTime = rtnPointer->time_stamp ;
				idx = i; 


				//cout << "CURRENT_TIME is " << CURRENT_TIME << " and current event is being selected and deleted from list" << endl;
			//	eventList.erase(eventList.begin()+i);
			//	break; 
			}
			
		}

			rtnPointer = eventList[idx];
			//cout << "Get_event has selected event with position " << idx << " with timestamp " << tempTime << endl;
			eventList.erase(eventList.begin()+idx);

	}



	//cout << "get_event is now complete" << endl;
	return rtnPointer;
}


int get_next_event_time() {

	//cout << "get next event time called" << endl;
	int nextTime = 2000000000;

	if(eventList.empty()) { 
		return -1;
	}
	else {
		//nextTime = eventList.front()->time_stamp;
		for (int i = 0; i < eventList.size(); i++) {
			if (nextTime > eventList[i]->time_stamp){
				nextTime = eventList[i]->time_stamp;
			}
		}
		return nextTime;
		//nextTime = eventList.front()->time_stamp;
	}
	

}

bool checkRunningProcess() {
	for (int i = 0; i < prcList.size(); i++) {
		//cout << "Process #" << i << ": " << prcList[i]->running << endl;
		if (prcList[i]->running == true) {
			return true;
		}

	}
	return false;
}

bool checkBlockedProcess() {
	for (int i = 0; i < prcList.size(); i++) {
		if (prcList[i]->blocked == true) {
			return true;
		}

	}
	return false;
}

//********************* FOR DEBUGGING PURPOSES ***********************//
bool checkAlphaNumeric(string S) {
	for (int i = 0; i < S.length(); i++) {
		if(!isalnum(S[i])) {
			return 0;
		}
	}
	return 1; 
} 


void checkRunQueue() {

	if (!runQueue.empty()) {
		for (int i = 0; i < runQueue.size(); i++) {
			cout << "process #"<<i<<": " << runQueue[i]->pid << endl;
		} 
	}
	else {
		cout << "Runqueue is empty" << endl;
	}
}


//**************** SCHEDULING ALGORITHMS HERE ************************/


class SCHEDULER { 
	public: 
	virtual PROCESS* get_next_process() =0 ;
}; 


class FCFS: public SCHEDULER { 

	//EDGE CASE: Potential fix for below algo.  The processes in input may have different arrival times.
	//May have to arrange by arrival time.  Completed bit might not be okay to have
	/*PROCESS* get_next_process() {
		cout << "get next process is called" << endl;
		PROCESS* newProc; 
		for (int i = 0; i < prcList.size(); i++){
			if (prcList[i]->completed == false) {
				newProc = prcList[i];
				break;
			}
		}	
		return newProc;
	}
 	*/
 
	PROCESS* get_next_process() {
		
		PROCESS* newProc; 

		newProc = runQueue.front();

		runQueue.erase(runQueue.begin());
		
		return newProc;
	}


};

class LCFS: public SCHEDULER { 

	//EDGE CASE: Potential fix for below algo.  The processes in input may have different arrival times.
	//May have to arrange by arrival time.  Completed bit might not be okay to have
	/*PROCESS* get_next_process() {
		cout << "get next process is called" << endl;
		PROCESS* newProc; 
		for (int i = 0; i < prcList.size(); i++){
			if (prcList[i]->completed == false) {
				newProc = prcList[i];
				break;
			}
		}	
		return newProc;
	}
 	*/
 
	PROCESS* get_next_process() {
		
		PROCESS* newProc; 

		newProc = runQueue.back();

		runQueue.pop_back();
		
		return newProc;
	}


};


class SRTF: public SCHEDULER { 

 
	PROCESS* get_next_process() {
		
		PROCESS* newProc; 

		int shortestTime = 1000000000;
		int idx = 0;

		for (int i = 0; i < runQueue.size(); i++) {

			if (shortestTime > runQueue[i]->timeRemaining){
				shortestTime = runQueue[i]->timeRemaining;
				idx = i;
			} 

		}

		newProc = runQueue[idx];
		runQueue.erase(runQueue.begin()+idx);

		
		return newProc;
	}


};


class RR: public SCHEDULER { 

 
	PROCESS* get_next_process() {

		PROCESS* newProc; 
 
		newProc = runQueue.front();

		runQueue.erase(runQueue.begin());
		
		return newProc;
	}


};


class PRIO: public SCHEDULER { 

 	
	PROCESS* get_next_process() {

		PROCESS* newProc; 
		int tempPRIO = -1; 
		int idx; 


		if (runQueue.empty()) {
			tempQueue = expiredQueue ;
			expiredQueue = runQueue;
			runQueue = tempQueue;

		}
		
		for (int i = 0; i < runQueue.size(); i++) {
			//cout << "At idx " << i << " the dynamicPRIO is " << (runQueue[i]->dynamicPRIO) << " and staticPRIO is " << (runQueue[i]->staticPRIO) << endl;
			if (runQueue[i]->dynamicPRIO > tempPRIO) {
				idx = i; 
				tempPRIO = runQueue[i]->dynamicPRIO;
				
			}
			
			//cout << "runQueue.begin()+idx: " << runQueue[runQueue.begin()+idx]->pid << endl;
			//cout << "idx chosen: " << idx << endl;
			

		}

		newProc = runQueue[idx];
		
		runQueue.erase(runQueue.begin()+idx);

		return newProc;
	}


};

class PREPRIO: public SCHEDULER {
	PROCESS* get_next_process() {

		PROCESS* newProc; 
		int tempPRIO = -1; 
		int idx; 


		if (runQueue.empty()) {
			tempQueue = expiredQueue ;
			expiredQueue = runQueue;
			runQueue = tempQueue;

		}
		
		for (int i = 0; i < runQueue.size(); i++) {
			//cout << "At idx " << i << " the dynamicPRIO is " << (runQueue[i]->dynamicPRIO) << " and staticPRIO is " << (runQueue[i]->staticPRIO) << endl;
			if (runQueue[i]->dynamicPRIO > tempPRIO) {
				idx = i; 
				tempPRIO = runQueue[i]->dynamicPRIO;
				
			}
			
			//cout << "runQueue.begin()+idx: " << runQueue[runQueue.begin()+idx]->pid << endl;
			//cout << "idx chosen: " << idx << endl;
			

		}

		newProc = runQueue[idx];
		
		runQueue.erase(runQueue.begin()+idx);

		return newProc;
	}




};


bool comparePRIO(PROCESS* prc) {

	int readyPrio = prc->dynamicPRIO; 
	int readyPID = prc->pid;
	int runningPrio;
	int ts ;//= (prc->state_ts)+quantum;

	int runningPID;

	for (int i = 0; i < prcList.size(); i++){
		if (prcList[i]->running) {
			runningPrio = prcList[i]->dynamicPRIO; 
			runningPID = prcList[i]->pid;
			ts =  (prcList[i]->state_ts)+ min(quantum,prcList[i]->currentBurst);
			break;
		}
	}

	//cout << "readyPrio is " << readyPrio << " and running Prio is "<< runningPrio << endl;

	if (readyPrio <= runningPrio) {  //does it have to be higher?  double-check
		if (v_flag) {
			if (ts == CURRENT_TIME) {
				printf("---> PRIO preemption %d by %d ? 1 TS=%d now=%d) --> NO\n", runningPID, readyPID,CURRENT_TIME,CURRENT_TIME);
			}
			else{
				printf("---> PRIO preemption %d by %d ? 0 TS=%d now=%d) --> NO\n", runningPID, readyPID,ts,CURRENT_TIME);
			}
		}

		return false;
	}
	else {  //start preemption
		if (v_flag)
			printf("---> PRIO preemption %d by %d ? 1 TS=%d now=%d) --> YES\n", runningPID, readyPID,ts,CURRENT_TIME);
		return true;
	}

}

SCHEDULER *sched = NULL; 


void printSummary(){

	/*
	Summary Information - Finally print a summary for the simulation:
	Finishing time of the last event (i.e. the last process finished execution)
	CPU utilization (i.e. percentage (0.0 – 100.0) of time at least one process is running
	IO utilization (i.e. percentage (0.0 – 100.0) of time at least one process is performing IO
	Average turnaround time among processes
	Average cpu waiting time among processes
	Throughput of number processes per 100 time units 
	*/

	cout << schedName << endl;


	for (int i = 0; i < prcList.size(); i++) {
		// pid: AT TC CB IO PRIO | FT TT IT CW

		avgTT = avgTT+ (prcList[i]->TT);
		avgCW = avgCW + (prcList[i]->CW);

		/*
		cout << prcList[i]->pid <<":    " << prcList[i]->AT << "   "<< prcList[i]->TC << "   "<< prcList[i]->CB << "   "<< prcList[i]->IO << "   "<< (prcList[i]->staticPRIO)+1;
		cout << "   |   " << prcList[i]->FT << "   "<< prcList[i]->TT << "   "<< prcList[i]->IT << "   "<< prcList[i]->CW << endl;
		*/

		printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n", prcList[i]->pid, prcList[i]->AT, prcList[i]->TC, prcList[i]->CB, prcList[i]->IO, (prcList[i]->staticPRIO)+1, prcList[i]->FT, prcList[i]->TT, prcList[i]->IT, prcList[i]->CW);

	}

	avgTT = avgTT/(prcList.size());
	avgCW = avgCW/(prcList.size());
	//cout << "total CPU: " << totalCPU << endl;
	avgThruput = ((prcList.size())/ (double)CURRENT_TIME)*100 ; 
	CPUutilization = (CPUutilization / (double)CURRENT_TIME) * 100;  
	IOutilization = (IOutilization / (double)CURRENT_TIME) * 100;  
	//cout << " total CPU is " << totalCPU << " and current time is " << CURRENT_TIME << " and avg thruput is " << avgThruput << endl;

	printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", CURRENT_TIME, CPUutilization, IOutilization, avgTT, avgCW, avgThruput);  

}

void printEventList() {
	
	for (int j = 0; j < eventList.size(); j++) {
		cout << "Event for PID #" <<eventList[j]->pid << " at timestamp" << eventList[j]->time_stamp << endl;
	} 
} 

void printCPUbursts(){
	for (int j = 0; j < prcList.size(); j++){
		cout << "At PID: "<< prcList[j]->pid << " the CPU burst is curently: " << prcList[j]->currentBurst << endl;
	}
}

/********************************  SIMULATOR ***************************************/ 



void Simulation() {
	int cpuBurst, ioBurst ;
 	EVENT* evt;
 	EVENT* tempEvt; 
 	int tempTrans ; 
 	int tempTime; 
 	int tempPRIO;
 	int prevTime; 
 	string trans_output ; 


 	//cout << "Starting simulator" <<endl;
 //	cout << "EvtList has " << eventList.size() << " elements. " << endl;

 	while( (evt = get_event()) ) {
 		//	cout << "Event called" << endl;
 			//cout << "SIMULATION TIME: " << simTimer<< endl;
 		
 			//cout << "EvtList has " << eventList.size() << " elements. " << endl;

  			
			PROCESS *proc = evt->evtPrc; // this is the process the event works on
			//cout << "Testing.  current process is :" << evt->evtPrc->pid << endl;
			prevTime = CURRENT_TIME;
			CURRENT_TIME = evt->time_stamp;
		//	cout << "CURRENT_TIME: " << CURRENT_TIME << endl;
			//cout << "CURRENT_TIME is: " << CURRENT_TIME << endl;
			evt->evtPrc->timeInPrevState = CURRENT_TIME - (proc->state_ts);

			switch(evt->transition) { // which state to transition to?
				case TRANS_TO_READY:
				 	// must come from BLOCKED or from PREEMPTION
					// must add to run queue


					if(checkRunningProcess()) {
						CPUutilization = CPUutilization + (CURRENT_TIME-prevTime);

						//cout << "Processes are running: " << CPUutilization << endl;
					}	

					if(checkBlockedProcess()) {
						IOutilization = IOutilization + (CURRENT_TIME-prevTime);
					}		

					if (evt->prev_state == 0 ){ //from created

						runQueue.push_back(evt->evtPrc);

						proc->state_ts = CURRENT_TIME;
						proc->timeInPrevState = CURRENT_TIME - (proc->state_ts);
						trans_output = "CREATED -> READY";
						tempTrans = 0;
					}	
					else if (evt->prev_state == 3) {  //from blocked
						runQueue.push_back(evt->evtPrc);

						proc->timeInPrevState = CURRENT_TIME - proc->state_ts;
						proc->state_ts = CURRENT_TIME;
						trans_output = "BLOCK -> READY";
						tempTrans = 0;
						attemptPrio = true;
						
					}	

					else if (evt->prev_state == 2) { // from running
						
							proc->timeInPrevState = CURRENT_TIME - proc->state_ts;
							proc->state_ts = CURRENT_TIME;
							proc->currentBurst = (proc->currentBurst)-quantum;
							//printCPUbursts();
							tempPRIO = evt->evtPrc->dynamicPRIO;
							evt->evtPrc->dynamicPRIO --;  
							//cout << "dynamicPRIO has ben decremented " << endl;
							if (evt->evtPrc->dynamicPRIO == -1 && prio_flag) {
								//cout << "prio below -1" << endl;
								evt->evtPrc->dynamicPRIO = evt->evtPrc->staticPRIO; 
								
								expiredQueue.push_back(evt->evtPrc);
								
							}
							else {
				
								runQueue.push_back(evt->evtPrc);
							}

						

							proc->running = false;
							tempTime = proc->timeRemaining ;
							trans_output = "RUNNG -> READY";
							tempTrans = 1; 
					}

					proc->blocked = false;
					CALL_SCHEDULER = true; // conditional on whether something is run
				
					break;


				case TRANS_TO_RUN:
				 	// create event for either preemption or blocking
					//event for block

					if(checkRunningProcess()) {
						CPUutilization = CPUutilization + (CURRENT_TIME-prevTime);
						//cout << "Processes are running: " << CPUutilization << endl;
					}	

					if(checkBlockedProcess()) {
						IOutilization = IOutilization + (CURRENT_TIME-prevTime);
					}

					proc->running = true; 
					proc->blocked = false;  

					proc->CW = (proc->CW) + proc->timeInPrevState;

					tempPRIO = proc->dynamicPRIO;

					if (!proc->preempted) {   //calculate new cpu burst for processes that were previously blocked, not preempted
						cpuBurst = myrandom(proc->CB);
						proc->currentBurst = cpuBurst;
						

						if (cpuBurst > proc->timeRemaining) {
							cpuBurst = proc->timeRemaining;
							proc->currentBurst = cpuBurst;
						}	

					}
					else {
						//proc->currentBurst = (proc->currentBurst)-quantum;
						if (proc->currentBurst > proc->timeRemaining) {
							proc->currentBurst = proc->timeRemaining;
							proc->preempted = false;
						}

					}
					

					if ( (proc->currentBurst <= quantum) ){  //proc->timeRemaining <= quantum

						tempTime = proc->timeRemaining;
						proc->timeRemaining = (proc->timeRemaining) - (proc->currentBurst); //change this calculation
						//cout << "Temp time is " << tempTime << " & process time remaining is now " << proc->timeRemaining << endl;
						proc->timeInPrevState = CURRENT_TIME - proc->state_ts;
						proc->state_ts = CURRENT_TIME; // new burst time
						
						tempEvt = new EVENT(proc->pid, CURRENT_TIME+(proc->currentBurst),2,2, proc);
						eventList.push_back(tempEvt);
						proc->preempted = false;

					}
					else {
						tempTime = proc->timeRemaining;
						proc->timeRemaining = (proc->timeRemaining) - quantum;
						//cout << "TEMP time is " << tempTime << " & process time remaining is now " << proc->timeRemaining << endl;
						proc->timeInPrevState = CURRENT_TIME - proc->state_ts;
						proc->state_ts = CURRENT_TIME;

						tempEvt = new EVENT(proc->pid, CURRENT_TIME+quantum,0,2, proc);
						eventList.push_back(tempEvt);
						proc->preempted = true; 
					}
					
		

					trans_output = "READY -> RUNNG"; 
					tempTrans = 1;
					//cout << "TRANS TO RUN COMPLETE" << endl;
				 	break;

				case TRANS_TO_BLOCK:
					//create an event for when process becomes READY again
					//cout << "Proc: " << proc->pid << " Time Remaining: " << proc->timeRemaining << " Last state ts: " << proc->state_ts << endl;
					
					if(checkRunningProcess()) {
						CPUutilization = CPUutilization + (CURRENT_TIME-prevTime);
						//cout << "Processes are running: " << CPUutilization << endl;

					}	

					if(checkBlockedProcess()) {
						IOutilization = IOutilization + (CURRENT_TIME-prevTime);
					}

					proc->running = false;
					proc->timeInPrevState = CURRENT_TIME - proc->state_ts;
					

					if (proc->timeRemaining == 0) {
						proc->completed = true; 
						proc->blocked = false; 
						trans_output =  "Done";
						proc->FT = CURRENT_TIME;
						proc->TT = (proc->FT) - (proc->AT);
						tempTrans = 0; 
						CALL_SCHEDULER = true;
					}
					else {

						ioBurst = myrandom(proc->IO);
						proc->IT = (proc->IT) + ioBurst;
						proc->state_ts = CURRENT_TIME;
						tempEvt = new EVENT(proc->pid, CURRENT_TIME+ioBurst,0,3, proc);
						eventList.push_back(tempEvt);
						proc->dynamicPRIO = proc->staticPRIO; //reset priority
						proc->running = false;
						proc->blocked = true; 
						CALL_SCHEDULER = true;

						trans_output =  "RUNNG -> BLOCK";
						tempTrans = 2; 
					}

			

					break;

				case TRANS_TO_PREEMPT:
					// add to runqueue (no event is generated)
					//cout << "preemption is occurring " << endl;
					if(checkRunningProcess()) {
						CPUutilization = CPUutilization + (CURRENT_TIME-prevTime);
						//cout << "Processes are running: " << CPUutilization << endl;

					}	

					if(checkBlockedProcess()) {
						IOutilization = IOutilization + (CURRENT_TIME-prevTime);
					}

					proc->timeInPrevState = CURRENT_TIME - proc->state_ts;
					proc->state_ts = CURRENT_TIME;
					proc->currentBurst = (proc->currentBurst)-quantum;
					//printCPUbursts();
					tempPRIO = evt->evtPrc->dynamicPRIO;
					evt->evtPrc->dynamicPRIO --;  
					//cout << "dynamicPRIO has ben decremented " << endl;
					if (evt->evtPrc->dynamicPRIO == -1 && prio_flag) {
						//cout << "prio below -1" << endl;
						evt->evtPrc->dynamicPRIO = evt->evtPrc->staticPRIO; 
						
						expiredQueue.push_back(evt->evtPrc);
						
					}
					else {
					//	cout <<"pushed to runqeue" << endl;
						runQueue.push_back(evt->evtPrc);
					}

					/*
					if(proc->currentBurst <= quantum) {
						proc->preempted = false;
					}*/
					
			

					proc->running = false;
					proc->blocked = false;
					CALL_SCHEDULER = true;
					tempTime = proc->timeRemaining ;
					trans_output = "RUNNG -> READY";
					tempTrans = 1; 
					
					break;
			}


			//INSERT HERE: check if process is running

			//remove current event object from Memory
			delete evt;
			evt = nullptr; //maybe delete tempEvt as well?
			//Consider having bottom before Scheduler is called, in case process is switched

			if (tempTrans == 1) {
				totalCPU = totalCPU + (proc->currentBurst);
				//cout << " Current burst is: " << proc->currentBurst << " and total CPU is " << totalCPU << endl;
			} 
			//cout << " Current burst is: " << proc->currentBurst << " and total CPU is " << totalCPU << endl;

			if (v_flag) {
				cout << CURRENT_TIME << " " << proc->pid << " " <<proc->timeInPrevState<< ": "<< trans_output   ; //write code for time spent
				if (tempTrans == 1 && prio_flag) {
					cout << " cb=" << proc->currentBurst << " rem="<< tempTime << " prio=" << tempPRIO << endl; //" and static: " << proc->staticPRIO<< endl;
				}  
				else if (tempTrans ==1 ) {
					cout << " cb=" << proc->currentBurst << " rem="<< tempTime << " prio="<<proc->staticPRIO << endl;
				} 
				else if (tempTrans == 2) {
					cout << " ib=" << ioBurst << " rem="<< proc->timeRemaining << endl;
				}
				else {
					cout << endl;
				} 
			}
			



			if(CALL_SCHEDULER) {
				//cout << "Scheduler called" << endl;


				if (attemptPrio & checkRunningProcess()) {
					//cout << "trying prio" << endl;
					if(comparePRIO(proc)){
						//create prempted event
						tempEvt = new EVENT(proc->pid, CURRENT_TIME,3,2, proc);
					}
					attemptPrio = false;
			 	}
				if (get_next_event_time() == CURRENT_TIME) { 
			 		continue;//process next event from Event queue
				}	

				CALL_SCHEDULER = false; // reset global flag
			
				if (!(checkRunningProcess()) ) {

					if (!runQueue.empty() || !expiredQueue.empty()) {
						//cout <<"IF loop condition met" << endl;
				 		proc = sched->get_next_process();

				 		tempEvt = new EVENT(proc->pid, CURRENT_TIME, 1,0, proc);
						eventList.push_back(tempEvt);
						//proc->running = true;
					}

			 		if (proc == nullptr)
			 			continue;
			 		// create event to make this process runnable for same time.
			 	}

			 	
			}

			
			
			
		
	}

	 
}



/***************** DEBUGGING PURPOSES ONLY **************************/

// check contents of process list

void printProcess(){
	for (int i = 0; i < prcList.size(); i++) {

		cout << "Pid: " << prcList[i]->pid << endl;
		cout << "AT: " << prcList[i]->AT << endl;
		cout << "TC: " << prcList[i]->TC << endl;
		cout << "CB: " << prcList[i]->CB << endl;
		cout << "IO: " << prcList[i]->IO << endl;

		cout << endl;

	}
}



/*
void printRunQueue() {
	for (int i = 0; i < runQueue.size(); i++) { 
		cout << "Event #: " << i << endl;
		cout << "Event pid: " << runQueue[i]->pid << endl;
		cout << "Timestamp:  " << runQueue[i]->time_stamp << endl;
		cout << "Transition:  " << runQueue[i]->transition << endl;
		cout << endl;
	}
} */

/********************************  START MAIN PROGRAM HERE ***************************************/ 

int main(int argc, char** argv) {

	//cout << "argv[1]: " << argv[1] << endl;

	extern char* optarg; 
	char *algoType = NULL;
	int c;
	char in;
	char in2;   
	int k =1;
	string getQuantum = "";



	while ((c = getopt (argc, argv, "vs:")) != -1) { 
		switch(c) {
			case 's': {
				algoType = optarg;
				in = algoType[0];

				/*
				while(meow != '\0') {
					meow = algoType[k+1] ; 
					testing = testing + meow; 
					cout << "optarg #" << k << ": " << meow << endl;
					cout << "algoType is " << algoType << endl;
					cout << "Testing: " << testing << endl;

					k++;		
					cout << stoi(testing)+1 << endl;
				} */

		

				if (in == 'F') {
					sched = new FCFS();
					schedName = "FCFS";
				}
				else if (in == 'L') {
					sched = new LCFS();
					schedName = "LCFS";
				}
				else if (in == 'S') {
					sched = new SRTF();
					schedName = "SRTF";
				}
				else if (in == 'R') {
					sched = new RR();
					in2 = algoType[k];
					while(in2 != '\0') {
						getQuantum = getQuantum + in2;
						k++;
						in2 = algoType[k];
					}
					quantum = stoi(getQuantum);
					schedName = "RR "+to_string(quantum);
				}
				else if (in == 'P') {
					sched = new PRIO();
					in2 = algoType[k];
					while(in2 != '\0') {
						getQuantum = getQuantum + in2;
						k++;
						in2 = algoType[k];
					}
					quantum = stoi(getQuantum);
					schedName = "PRIO "+to_string(quantum);
					prio_flag = true;
					preprio = true;
				}
				else if (in == 'E') {
					sched = new PREPRIO();
					in2 = algoType[k];
					while(in2 != '\0') {
						getQuantum = getQuantum + in2;
						k++;
						in2 = algoType[k];
					}
					quantum = stoi(getQuantum);
					schedName = "PREPRIO "+to_string(quantum);
					prio_flag = true;
					preprio = true;
				}


				else {
					  
				}
				break;
			}
			case 'v': {
				v_flag = true;
				break;
			}
			case '?': {
				if (optopt == 's'){
					cerr<< "Option requires an argument" << endl;
				}
				else if (isprint(optopt)) {
					cerr << "Unknown option! " << endl ; 
				}
				else {
					cerr << "Unknown option character! " << endl;
				}
				return 1; 
			}
			default:
				abort();
		}

	}
	
	FILE *input_file = fopen(argv[optind], "r");
	if (input_file == NULL) {
		cout << "Cannot read the file" << endl;
	}

	optind++ ; 

	FILE *rand_file = fopen(argv[optind], "r");
	if (rand_file == NULL) {
		cout << "Cannot read the random file" << endl; 
	}

	parse_data(input_file);
	createRandVals(rand_file);
	addPRIO();

	Simulation();
	printSummary();
	

	return 0;
}


