/* 
		CS575
		Mike Zhong
		10/29/16
		Program 2
*/

// Header files needed
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// Define constants
#define MAX_CUSTOMERS 20
#define NUM_THREADS 40
#define MAX_ARRIVAL_TIME 30000 // 0 - 30ms
#define MAX_EATING_TIME 9000 // 1 - 10ms
#define PIZZA_MAKE_TIME 2000 // 2ms

using namespace std;

int pizza_count = 0;			// tracks number of pizzas available

sem_t seatMutex;					// prevent two customers from taking same seat
sem_t seatSem;						// prevent customer from taking a seat when there is none
sem_t pizzaMutex;					// prevent two customers taking same pizza
sem_t plateSem;						// prevent chef and customers from taking from same plate

// chef thread function
void* chef(void* num_customers)
{
	int cook_time = PIZZA_MAKE_TIME;
	int count = 0; // count of customers serviced
	int total = *(int*)num_customers; // total number of customers to service

	// as long as there are still customers to serve, make pizzas
	while(count < total){
		
		sem_wait(&plateSem); // request empty plate to put pizza in
		sem_wait(&pizzaMutex); // get pizza mutex
		
		// make pizza
		printf("Chef making a pizza...\n");
		usleep(cook_time);
		pizza_count++; // increment pizza count
		printf("Pizza made, there are %d unsold pizzas\n", pizza_count);
		
		sem_post(&pizzaMutex);

		count++;
	}
	
	pthread_exit(NULL);
}

// customer thread function
void* customer(void* threadid)
{
	int arrival_time = (int)(drand48() * MAX_ARRIVAL_TIME);
	int eat_time = (int)((drand48() * MAX_EATING_TIME) + 1);
	int seat_val; 

	usleep(arrival_time);
	printf("Customer %lld arrived...\n", (long long)(threadid));
	// customer arrived

	// check for seat, and if available, take one
	sem_wait(&seatSem);

	sem_wait(&seatMutex);
	sem_getvalue(&seatSem, &seat_val);
	printf("Customer %lld seated... there are %d seats left\n", (long long)(threadid), seat_val);
	sem_post(&seatMutex);

	// busy wait if no pizzas available
	while(pizza_count <= 0);
	
	// grab a pizza
	sem_wait(&pizzaMutex);
	pizza_count--;
	printf("Customer %lld got pizza... there are %d unsold pizzas\n", (long long)(threadid), pizza_count);
	sem_post(&plateSem); // increment plate count
	sem_post(&pizzaMutex);

	// eat pizza
	printf("Customer %lld eating...\n", (long long)(threadid));
	usleep(eat_time);

	sem_post(&seatSem); // relinquish seat
	sem_getvalue(&seatSem, &seat_val);
	printf("Customer %lld finished and left... there are %d seats left\n", (long long)(threadid), seat_val);
	pthread_exit(NULL);
}


int main(int argc, char* argv[])
{
	pthread_t customers[NUM_THREADS]; // thread array for customers
	pthread_t chefs;									// one for the chef
	int plates = 11, seats = 11;			// seats and plates
	
	srand48(time(0));
	int num_customers = (int)(drand48() * MAX_CUSTOMERS - 1) + 1;
	int status;
	
	printf("There will be %d customers today\n", num_customers);	

	while(plates > 10 || plates <= 0){
		printf("Enter the number of plates to hold unsold pizzas (max 10)\n");
		scanf("%d", &plates);
	}

	while(seats > 10 || seats <= 0){
		printf("Enter the number of seats for customers (max 10)\n");
		scanf("%d", &seats);
	}
	
	// initialize mutexes
	sem_init(&seatMutex, 0, 1);
	sem_init(&seatSem, 0, seats);
	sem_init(&pizzaMutex, 0, 1);
	sem_init(&plateSem, 0, plates); 

	printf("M: %d, N: %d\n", plates, seats);

	// create thread for chef
	printf("Creating chef thread\n");
	if(pthread_create(&chefs, NULL, chef, (void*)&num_customers) != 0){
		
		printf("pthread_create failed\n");
		exit(-1);
	}
	
	long long int i;
	// create thread pool for customers
	printf("Creating customer threads\n");
	for(i = 1; i <= num_customers; ++i){
		
		if(pthread_create(&customers[i], NULL, customer, (void*)i) != 0){
			
			printf("pthread_create failed\n");
			exit(-1);
		}
	}
	
	// join chef thread
	if(pthread_join(chefs, (void**)&status) != 0){
	
		printf("pthread join error\n");
		exit(-1);

	}
	printf("Completed thread task for chef, status = %d\n", status);

	// join all customer threads
	for(i = 1; i <= num_customers; ++i){

		if(pthread_join(customers[i], (void**)&status) != 0){
	
			printf("pthread join error\n");
			exit(-1);

		}
		printf("Completed thread task for customer %lld, status = %d\n", i, status);
	}

	// destroy mutexes
	sem_destroy(&seatMutex);
	sem_destroy(&seatSem);
	sem_destroy(&pizzaMutex);
	sem_destroy(&plateSem);

	pthread_exit(NULL);

	return 0;
}
