# COMP-304-Project-2
COMP 304 Project 2 Report

	First of all, we believe that our code is working properly. Everything required is implemented. Secondly, our code works totally fine with UBUNTU VirtualBox, however, there happens some bugs in Tolgay’s VM. We could not figure out why. 

Part 1:
Brief Explanation of the Project:
	First, we have a lot of global variables on top. These consist of mutexes, queues, and some counters for queues and job id. Then, at the beginning of the main, initialization of the queues, mutexes, time, global variables and log file are made. Then, main initializes two tower threads, one for each pad, and the first launch job. After that, main executes the while loop for the rest of the simulation. 
After the initializations, main thread manages two tasks: 
	1- Generating threads that run the job functions by separate random generators for each job type.
	2- Printing snapshots.

Job Functions:
	All of the job functions are quite similar in terms of implementation. They first all acquire the corresponding queue’s mutex, and also a mutex for jobID global variable, then simply generate a job and enqueue it to the corresponding queue. 

Tower Threads:
	Control tower is simulated as two different threads, which only differ from each other by a local variable pad, which determines the pad that the tower is controlling. Control tower threads first checks the emergency queue, then the landing condition which will be later explained at part 2, and then launch or assembly queue depending on the tower thread. 

Dequeue part is very similar for all types of jobs: 
1- Acquire the job’s corresponding queue’s mutex.
2- Dequeue the job from the queue.
3- Release the lock.
4- Sleep. 

Part 2:
Starvation of the jobs waiting on the ground
	We added two more global variables that hold count of the waiting jobs in the launch and assembly queues. Also, we have implemented mutexes for these global counters.  In every AssemblyJob call, we increase the assembly counter and in every Dequeue we decrease the assembly counter. The same logic applies to the launch counter. This time TowerControl favors launch or assembly job if the corresponding counter is greater or equal to 3. 

Starvation of the pieces in the orbit
	To solve starvation for landing jobs; TowerControl favors landing jobs if there are 5 or more pieces waiting to land. So, the tower favors landing jobs even if the assembly or launch counter is greater or equal to 3.

Part 3:
	Additional to other queues, we implemented an emergency queue that stores emergency jobs and we enqueue two emergency jobs every 40t seconds. If the emergency queue is not empty, the tower favors jobs in the emergency queue. Thus, emergency jobs have immediate access to the pads after being added to the emergency queue.

Keeping Logs:
  For this part, we have added reqTime and endTime variables to the job struct to be able to calculate the turnaround time for each job. Also, we have implemented a simple PrintQueue function in queue.c to display queues, createLogFile function to create the log file, and logJob function to log every job when it is dequeued. Rest is straightforward, create the log file in main with createLogFile function and log every dequeued job to the log file with logJob function.
  
 Project report can be accessed via this link: https://docs.google.com/document/d/1BlxZLv2zAcd0z6aW81wYBQcM6j16S2bfNy2LQhbheCc/edit?usp=sharing
