#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

void* rand_num();
unsigned long long number_of_distributed;
double min = (-1);
double max = 1;

int main(int argc, char **argv)
{
    double pi_estimate;
    unsigned long long  number_of_cpu, number_of_tosses, number_in_circle, thread, toss;
    pthread_t* thread_handles;
    if ( argc < 2) {
        exit(-1);
    }
    number_of_cpu = strtoull(argv[1],NULL,10);
    number_of_tosses = strtoull(argv[2],NULL,10);
	//printf("%llu\n",number_of_cpu);
	//printf("%llu\n",number_of_tosses);
	//time_t starttime, endtime;
	number_of_distributed = number_of_tosses/number_of_cpu;
	
    if (( number_of_cpu < 1) || ( number_of_tosses < 0)) {
        exit(-1);
    }
	//starttime = time (NULL); 

    thread_handles = malloc((number_of_cpu-1)*sizeof(pthread_t));

    number_in_circle = 0;
	
	
    for (thread = 0; thread < number_of_cpu-1; thread++) {
		pthread_create(&thread_handles[thread], NULL, rand_num, NULL);

    }
	
	double distance_squared, x, y;
	number_in_circle = 0;
	unsigned int seed = time(NULL);
	for (toss = 0; toss < number_of_distributed; toss++) {
        x = (max - min) * rand_r(&seed) / (RAND_MAX + 1.0) + min;
        y = (max - min) * rand_r(&seed) / (RAND_MAX + 1.0) + min;
        distance_squared = x*x + y*y;
        if (distance_squared <= 1)
			number_in_circle++;
    }
	
	void *returnValue;
	for (thread = 0; thread < number_of_cpu-1; thread++){
		pthread_join(thread_handles[thread], &returnValue);
		number_in_circle+=(unsigned long long) returnValue;
	}
	
    pi_estimate = 4*number_in_circle/((double) number_of_tosses);
	printf("%f\n",pi_estimate);
	
	//endtime = time (NULL);
	//printf ("time = %ld\n", (endtime - starttime));
    free(thread_handles);
    return 0;
}

void* rand_num()
{
	double distance_squared, x, y;
	unsigned long long toss, my_number_in_circle;
	my_number_in_circle = 0;
	unsigned int seed = time(NULL);
	for (toss = 0; toss < number_of_distributed; toss++) {
        x = (max - min) * rand_r(&seed) / (RAND_MAX + 1.0) + min;
        y = (max - min) * rand_r(&seed) / (RAND_MAX + 1.0) + min;
        distance_squared = x*x + y*y;
        if (distance_squared <= 1)
            my_number_in_circle++;
    }
	return (void *) my_number_in_circle;
	
}