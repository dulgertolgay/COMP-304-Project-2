#include "queue.c"
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#define t 2
int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 40; // frequency of emergency
float p = 0.2;               // probability of a ground job (launch & assembly)

//global var
int timeZero;
int jobid = 1;
    //queues for all different jobs
    Queue* landQ;
    Queue* launchQ;
    Queue* assemblyQ;
    //mutexes for each q
    pthread_mutex_t MlandQ;
    pthread_mutex_t MlaunchQ;
    pthread_mutex_t MassemblyQ;
//global var

void* LandingJob(void *arg); 
void* LaunchJob(void *arg);
void* EmergencyJob(void *arg); 
void* AssemblyJob(void *arg); 
void* ControlTower(void *arg); 

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
    for(int i=1; i<argc; i++){
        if(!strcmp(argv[i], "-p")) {p = atof(argv[++i]);}
        else if(!strcmp(argv[i], "-t")) {simulationTime = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-s"))  {seed = atoi(argv[++i]);}
    }
    landQ = ConstructQueue(1000);
    launchQ = ConstructQueue(1000);
    assemblyQ = ConstructQueue(1000);
    //mutex init for q's
    pthread_mutex_init(&MlandQ, NULL);
    pthread_mutex_init(&MassemblyQ, NULL);
    pthread_mutex_init(&MlaunchQ, NULL);
    

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

    double r = (double) rand() / (double) RAND_MAX; //uniform random between 0-1
    double r2 = (double) rand() / (double) RAND_MAX; //uniform random between 0-1
    double r3 = (double) rand() / (double) RAND_MAX; //uniform random between 0-1
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
    
    
   
    pthread_sleep(t);
    }


  
    
    pthread_join(towerid,NULL);

    /* Queue usage example
        Queue *myQ = ConstructQueue(1000);
        Job j;
        j.ID = myID;
        j.type = 2;
        Enqueue(myQ, j);
        Job ret = Dequeue(myQ);
        DestructQueue(myQ);
    */


    return 0;
}

// the function that creates plane threads for landing
void* LandingJob(void *arg){
    printf("Spacecraft asking for landing with job id %d\n", jobid);
    fflush(stdout);
    pthread_mutex_lock(&MlandQ);
    Job j;
    j.ID = ++jobid;
    j.type = 1;
    Enqueue(landQ,j);
    printf("Tower: Spacecraft with job id %d is placed to position %d in the landing queue\n", jobid, landQ->size);
    fflush(stdout);
    pthread_mutex_unlock(&MlandQ);

}

// the function that creates plane threads for departure
void* LaunchJob(void *arg){
    printf("Spacecraft asking for launch with job id %d\n", jobid);
    fflush(stdout);
    pthread_mutex_lock(&MlaunchQ);
    Job j;
    j.ID = ++jobid;
    j.type = 2;
    Enqueue(launchQ,j);
    printf("Tower: Spacecraft with job id %d is placed to position %d in the launch queue\n", jobid, launchQ->size);
    fflush(stdout);
    pthread_mutex_unlock(&MlaunchQ);
}

// the function that creates plane threads for emergency landing
void* EmergencyJob(void *arg){ 

}

// the function that creates plane threads for emergency landing
void* AssemblyJob(void *arg){
    printf("Spacecraft asking for assembly with job id %d\n", jobid);
    fflush(stdout);
    pthread_mutex_lock(&MassemblyQ);
    Job j;
    j.ID = ++jobid;
    j.type = 3;
    Enqueue(assemblyQ,j);
        printf("Tower: Spacecraft with job id %d is placed to position %d in the assembly queue\n", jobid, assemblyQ->size);
    fflush(stdout);
    pthread_mutex_unlock(&MassemblyQ);

}

// the function that controls the air traffic
void* ControlTower(void *arg){
	int *type = (int *) arg;
	printf("Tower %d is online !\n", *type);
	fflush(stdout);
	while(time(NULL) - timeZero <= 120) {
	if (landQ->size > 0) {
		//do landing
		//since towers only share the landing q, it suffices to lock only if they are dqing from landQ
		//update: this statement is wrong since both dq and q can happen at the same time. so updating 
		pthread_mutex_lock(&MlandQ);
		Job ret = Dequeue(landQ);
		pthread_mutex_unlock(&MlandQ);
		printf("Tower: Permission granted for landing in pad %d to spacecraft with id %d\n",*type ,ret.ID);
		fflush(stdout);
		pthread_sleep(t);
	}
	else {
		//no landing rockets in queue, do one assembly and launch
		if (*type == 1) {
			//pad A do launch
			if (launchQ-> size>0) {
			pthread_mutex_lock(&MlaunchQ);
			Job ret = Dequeue(launchQ);
			pthread_mutex_unlock(&MlaunchQ);
			printf("Tower: Permission granted for launch in pad %d to spacecraft with id %d\n",*type ,ret.ID);
			fflush(stdout);
			pthread_sleep(2*t);
	

			}
			}
		else if (*type == 2) {
			//pad B do assembly
			if(assemblyQ->size >0 ) {
			pthread_mutex_lock(&MassemblyQ);
			Job ret = Dequeue(assemblyQ);
			pthread_mutex_unlock(&MassemblyQ);
			printf("Tower: Permission granted for assembly in pad %d to spacecraft with id %d\n",*type ,ret.ID);
			fflush(stdout);
			pthread_sleep(6*t);
	
	
			}
			}	
		}
	}
	}


