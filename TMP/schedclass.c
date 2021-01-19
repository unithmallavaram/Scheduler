#include<stdio.h>
#include<PA1.h>
#include<kernel.h>
#include<proc.h>
#include<q.h>

extern int sched_type;
extern int currpid;
int proccnt = NPROC;

void setschedclass(int sched_class){
	if(sched_class == 1)
		sched_type = RANDOMSCHED;
	else if(sched_class == 2){
		sched_type = LINUXSCHED;
		resetepoch();
	}
	else
		sched_type = XINUSCHED;
}


int getschedclass(){
	if (sched_type == 1)
		return RANDOMSCHED;
	else if(sched_type == 2)
		return LINUXSCHED;
	else
		return XINUSCHED;
}

void resetepoch(){
	totalquant = 0; 		//if this block is executed because every goodness is 0. So, reset totalquant
	int k;
	for(k = 0; k<proccnt; k++){
		if(proctab[k].pstate != PRFREE){	//even blocked processes are included, as they can be scheduled in the next epoch
			//reset the values of quantum, counter and goodness for all the processes
			proctab[k].quantum = proctab[k].counter/2 + proctab[k].pprio;
			proctab[k].counter = proctab[k].quantum;
			proctab[k].goodness = proctab[k].counter + proctab[k].pprio;
			totalquant += proctab[k].quantum;
		}
	}
}

void epochmanagement(){
	isgood = 1;
	int k;

	//not a new epoch
	if(totalquant > 0){
		//reducing the total quant value by the amount of quantum used up
		totalquant = totalquant - (proctab[currpid].counter - preempt); 
		//resulting value for counter is the amount of preempt left
		proctab[currpid].counter = preempt;
		if(preempt != 0)		//if there is some preempt left, adjust the value of goodness
			proctab[currpid].goodness = proctab[currpid].counter + proctab[currpid].quantum;
		else		//if there is no preempt, i.e. there is no more quantum left, so goodness is zero
			proctab[currpid].goodness = 0;

		//checking if there is any process with non-zero goodness
		for(k=0; k<proccnt; k++){
			if(proctab[k].goodness>0)
				//this is set only if there is atleast one non-zero goodness
				isgood = 0;
		}

	}

	//if it is a new epoch
	if(isgood || totalquant == 0){		//there may be no process with non-zero goodness or total quantum is zero. Start a new epoch
		resetepoch();
	}
}

int getprocesstoschedule(){
	int nextprocess;
	if(sched_type == RANDOMSCHED){
		//read priorities of all the processes eligible to be scheduled and add 

		int totalprio = 0;
		if(proctab[currpid].pstate == PRCURR)
			totalprio += proctab[currpid].pprio;
		
		//kprintf("%d\n", totalprio);
		int qptr = q[rdytail].qprev;
		int count = 1;
		while(qptr != rdyhead){
			if(qptr != 0){
				totalprio += q[qptr].qkey;
				count++;
			}
			qptr = q[qptr].qprev;
		}
		//kprintf("priocount: %d\tcount: %d\n", totalprio, count);

		//if the totalprio is zero, it means that there are no schedulable processes
		//so schedule null process
		if(totalprio == 0)
			return NULLPROC;

		//generate a random number
		int r = rand()%(totalprio);
		//kprintf("%d\n", r);


		//push the current process into the queue, and then select the next process
		if(proctab[currpid].pstate == PRCURR){
			//proctab[currpid].pstate = PRREADY;
			//enqueue(currpid, rdytail);
			insert(currpid, rdyhead, proctab[currpid].pprio);
		}

		//process selection based on the random selection algorithm
		qptr = q[rdytail].qprev;
		while(qptr != rdyhead){
			if(r < q[qptr].qkey){
				 nextprocess = qptr;
				 break;
			}
			else{
				r = r - q[qptr].qkey;
			}
			qptr = q[qptr].qprev;
		}

		//remove the current process from the ready list
		if(proctab[currpid].pstate == PRCURR)
			dequeue(currpid);

		//returning the process id that falls in the band of the random number  
		return nextprocess;


	}
	else if(sched_type == LINUXSCHED){
		//This method is called only after the proper epoch management is done

		//if there is no schedulable process with non-zero goodness, NULL is selected by default
		nextprocess = 0;
		int x = 0;

		//selecting the process with the highest goodness and that is schedulable.
		int k;
		for(k=0; k<proccnt; k++){
			if(proctab[k].pstate == PRCURR || proctab[k].pstate == PRREADY){
				if(proctab[k].goodness > x){
					x = proctab[k].goodness;
					nextprocess = k;
				}
			}
		}

		dequeue(nextprocess);

		return nextprocess;

	}
}

