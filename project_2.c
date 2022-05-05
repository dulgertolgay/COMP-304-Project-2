#include "queue.c"
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#define t 2
int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 40*t; // frequency of emergency
float p = 0.2;   	// probability of a ground job (launch & assembly)
int snapshotTime = 0;    //snapshot time        

//global var
int timeZero;
int jobid = 1;
//queues for all different jobs
Queue* landQ;
Queue* launchQ;
Queue* assemblyQ;
Queue* emergencyQ;
//mutex for jobid
pthread_mutex_t Mjobid;
//mutexes for each q
pthread_mutex_t MlandQ;
pthread_mutex_t MlaunchQ;
pthread_mutex_t MassemblyQ;
pthread_mutex_t MemergencyQ;
//mutexes for counters
pthread_mutex_t MlaunchCounter;
pthread_mutex_t MassemblyCounter;
//counters
int launchCounter = 0;
int assemblyCounter = 0;
//global var

void* LandingJob(void *arg); 
void* LaunchJob(void *arg);
void* EmergencyJob(void *arg); 
void* AssemblyJob(void *arg); 
void* ControlTower(void *arg); 
void createLogFile();
void logJob(Job job, int padID);

// pthread sleeper function
int pthread_sleep (int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if(pthread_mutex_init(&mutex,NULL))
    {
        return -1;
    }
    if(pthread_cond_init(&conditionvar,NULL))
    {
        return -1;
    }
    struct timeval tp;
    //When to expire is an absolute time, so get the current time and add it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;
    
    pthread_mutex_lock (&mutex);
    int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock (&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);
    
    //Upon successful completion, a value of zero shall be returned
    return res;
}

int main(int argc,char **argv){
    // -p (float) => sets p
    // -t (int) => simulation time in seconds
    // -s (int) => change the random seed
    // -n (int) => snapshot time in seconds
    for(int i=1; i<argc; i++){
        if(!strcmp(argv[i], "-p")) {p = atof(argv[++i]);}
        else if(!strcmp(argv[i], "-t")) {simulationTime = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-s"))  {seed = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-n"))  {snapshotTime = atoi(argv[++i]);}
    }
   
    //construct queues
    landQ = ConstructQueue(1000);
    launchQ = ConstructQueue(1000);
    assemblyQ = ConstructQueue(1000);
    emergencyQ = ConstructQueue(1000);
    //create log file
    createLogFile();
    //mutex init for jobid
    pthread_mutex_init(&Mjobid, NULL);
    //mutex init for q's
    pthread_mutex_init(&MlandQ, NULL);
    pthread_mutex_init(&MassemblyQ, NULL);
    pthread_mutex_init(&MlaunchQ, NULL);
    pthread_mutex_init(&MemergencyQ, NULL);
    //mutex init for counters
    pthread_mutex_init(&MassemblyCounter, NULL);
    pthread_mutex_init(&MlaunchCounter, NULL);
    
    srand(seed); // feed the seed
    timeZero = time(NULL);
    //first tower thread
    pthread_t towerid;
    int padID = 1;
    pthread_create(&towerid, NULL, ControlTower,&padID);
    //second tower thread
    pthread_t towerid2;
    int padID2 = 2;
    pthread_create(&towerid2, NULL, ControlTower,&padID2);
    
    //rocket waiting to take off
    pthread_t lathread0;
    pthread_create(&lathread0,NULL,LaunchJob,NULL);
    
    while(time(NULL) - timeZero <= 120) {
    int currTime = time(NULL) - timeZero;
    double r = (double) rand() / (double) RAND_MAX; //uniform random between 0-1
    double r2 = (double) rand() / (double) RAND_MAX; //uniform random between 0-1
    double r3 = (double) rand() / (double) RAND_MAX; //uniform random between 0-1
    if(currTime > 0 && (currTime % emergencyFrequency) == 0) {
    	for(int i = 0; i < 2; i++) {
    		pthread_t emergencythread;
  		pthread_create(&emergencythread,NULL,EmergencyJob,NULL);
    	}
    }
    if (r <= p/2) {
    	//launch
    	pthread_t lathread;
    	pthread_create(&lathread,NULL,LaunchJob,NULL);
    }
    if (r2 <= p/2) {
    	//assembly
    	pthread_t asthread;
    	pthread_create(&asthread,NULL,AssemblyJob,NULL);
    }
    if (r3 <= 1-p) {
    	//landing
    	pthread_t lthread;
    	pthread_create(&lthread,NULL,LandingJob,NULL);
    }
    
    for (int i = 0; i<t; i++) {
    if (snapshotTime != 0  && currTime >= snapshotTime) {
     	printf("\n\n");
      	printf("At %ld sec landing  : ", time(NULL) - timeZero);
      	PrintQueue(landQ);
      	printf("\n");
      	printf("At %ld sec launch   : ", time(NULL) - timeZero);
      	PrintQueue(launchQ);
      	printf("\n");
      	printf("At %ld sec assembly : ", time(NULL) - timeZero);
      	PrintQueue(assemblyQ);
      	printf("\n\n");
    }
    
    pthread_sleep(1);
    }
    
    }
    
    pthread_join(towerid,NULL);
    return 0;
}

// the function that creates plane threads for landing
void* LandingJob(void *arg){
    pthread_mutex_lock(&MlandQ);
    pthread_mutex_lock(&Mjobid);
    jobid += 1;
    pthread_mutex_unlock(&Mjobid);
    Job j;
    j.ID = jobid;
    j.type = 1;
    j.reqTime = time(NULL) - timeZero;
    Enqueue(landQ,j);
    pthread_mutex_unlock(&MlandQ);

}

// the function that creates plane threads for departure
void* LaunchJob(void *arg){
    pthread_mutex_lock(&MlaunchQ);
    pthread_mutex_lock(&Mjobid);
    jobid += 1;
    pthread_mutex_unlock(&Mjobid);
    Job j;
    j.ID = jobid;
    j.type = 2;
    j.reqTime = time(NULL) - timeZero;
    Enqueue(launchQ,j);
    pthread_mutex_lock(&MlaunchCounter);
    launchCounter++;
    pthread_mutex_unlock(&MlaunchCounter);
    pthread_mutex_unlock(&MlaunchQ);
}

// the function that creates plane threads for emergency landing
void* EmergencyJob(void *arg){ 
    pthread_mutex_lock(&MemergencyQ);
    pthread_mutex_lock(&Mjobid);
    jobid += 1;
    pthread_mutex_unlock(&Mjobid);
    Job j;
    j.ID = jobid;
    j.type = 4;
    j.reqTime = time(NULL) - timeZero;
    Enqueue(emergencyQ,j);
    pthread_mutex_unlock(&MemergencyQ);
}

// the function that creates plane threads for emergency landing
void* AssemblyJob(void *arg){
    pthread_mutex_lock(&MassemblyQ);
    pthread_mutex_lock(&Mjobid);
    jobid += 1;
    pthread_mutex_unlock(&Mjobid);
    Job j;
    j.ID = jobid;
    j.type = 3;
    j.reqTime = time(NULL) - timeZero;
    Enqueue(assemblyQ,j);
    pthread_mutex_lock(&MassemblyCounter);
    assemblyCounter++;
    pthread_mutex_unlock(&MassemblyCounter);
    pthread_mutex_unlock(&MassemblyQ);

}

// the function that controls the air traffic
void* ControlTower(void *arg){
	int *pad = (int *) arg;
	printf("Tower %d is online!\n", *pad);
	fflush(stdout);
	while(time(NULL) - timeZero <= 120) {
	if(!isEmpty(emergencyQ)) {
		pthread_mutex_lock(&MemergencyQ);
		Job ret = Dequeue(emergencyQ);
		pthread_mutex_unlock(&MemergencyQ);
		pthread_sleep(t);
		logJob(ret, *pad);
		
	} else if ((!isEmpty(landQ) && launchCounter < 3 && assemblyCounter < 3) || landQ->size >= 5) {
		//do landing 
		pthread_mutex_lock(&MlandQ);
		Job ret = Dequeue(landQ);
		pthread_mutex_unlock(&MlandQ);
		pthread_sleep(t);
		logJob(ret, *pad);
		
	} else {
		//no landing rockets in queue, do one assembly and launch
		if (!isEmpty(launchQ) && *pad == 1) {
			//pad A do launch
			pthread_mutex_lock(&MlaunchQ);
			Job ret = Dequeue(launchQ);
			pthread_mutex_lock(&MlaunchCounter);
    			launchCounter--;
    			pthread_mutex_unlock(&MlaunchCounter);
			pthread_mutex_unlock(&MlaunchQ);
			pthread_sleep(2*t);
			logJob(ret, *pad);
		}
		else if (!isEmpty(assemblyQ) && *pad == 2) {
			//pad B do assembly
			pthread_mutex_lock(&MassemblyQ);
			Job ret = Dequeue(assemblyQ);
			pthread_mutex_lock(&MassemblyCounter);
    			assemblyCounter--;
    			pthread_mutex_unlock(&MassemblyCounter);
			pthread_mutex_unlock(&MassemblyQ);
			pthread_sleep(6*t);
			logJob(ret, *pad);
		}	
	}
}
}

void createLogFile() {
	FILE *fp = fopen("events.log", "w");
  	fprintf(fp, "\tEventID\tStatus\t\tRequest Time\tEnd Time\tTurnaround Time\tPad\n");
  	fprintf(fp, "    -------------------------------------------------------------------------------------------------\n");
  	fclose(fp);
}

void logJob(Job job, int padID) {
	job.endTime = time(NULL) - timeZero;
	char status;
	switch(job.type){  
		case 1:  
			status = 'L';
			break;
		case 2:  
			status = 'D';
			break;
		case 3:  
			status = 'A';
			break;
		case 4:  
			status = 'E';
			break; 
		default:  
			printf("Error! Job has no type.\n"); 
	}  
	FILE *fp = fopen("events.log", "a");
  	fprintf(fp, "\t%d\t\t%c\t\t%d\t\t%d\t\t%d\t\t\t%c\n", job.ID, status, job.reqTime, job.endTime, job.endTime - job.reqTime, padID == 1 ? 'A' : 'B');
  	fclose(fp);
}







