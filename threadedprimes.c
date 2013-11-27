/**
 * Author: Joshua Kociemba (kociembj), joshua.kociemba@oregonstate.edu
 * Created: 2013-11-24
 * Filename: threadedprimes.c
 *
 * Description: 
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <math.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>

int num_threads, max_prime;
static char *prime_storage;
clock_t start, stop;
double total_time;

/**
 * Function
 * Prototypes
 **/

void create_bitmap(char *prime_storage, int max_prime);
void distribute_primes(char *prime_storage, int max_prime);
void isnt_prime(char *prime_storage, int checked_prime);
bool is_prime(char *prime_storage, int checked_prime);
void thread_setup(int num_threads);
void *calc_primes(void *arg);
int next_prime(int current_prime);
int count_primes(char *prime_storage, int max_prime);
void print_results(char *prime_storage, int max_prime);
void usage(void);

int main(int argc, char **argv) {
	int c;
	
	while ((c = getopt(argc, argv, "ht:m:")) != -1) {
		switch(c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
			break;
		case 't':
			if (optarg == NULL || atoi(optarg) < 1 || atoi(optarg) > 100) {
				usage();
				exit(-1);
			}
			num_threads = atoi(optarg);
			break;
		case 'm':
			if (optarg == NULL || atoi(optarg) < 1 || atoi(optarg) > 4294967295) {
				usage();
				exit(-1);
			}
			max_prime = atoi(optarg);
			break;
		default:
			perror("Eject the core!");
			exit(EXIT_FAILURE);
			break;
		}
	}

	prime_storage = (char *)malloc(sizeof(char) * ((max_prime / 8) + 1));
	
	create_bitmap(prime_storage, max_prime);
	distribute_primes(prime_storage, max_prime);
	
	start = clock();
	thread_setup(num_threads);
	stop = clock();

	total_time = (double)(stop - start) / (double)CLOCKS_PER_SEC;

	print_results(prime_storage, max_prime);
	//free(prime_storage);
	return 0;
}

void create_bitmap(char *prime_storage, int max_prime) {
	int i;
	
	prime_storage[0] = 0b01010110;
	
	for(i = 1; i < (max_prime / 8); i++) {
		prime_storage[i] = 0b01010101;
	}
}

void distribute_primes(char *prime_storage, int max_prime) {
	int i, j, shortened_max_prime;
	
	shortened_max_prime = sqrt(max_prime);
	
	for (i = 3; i <= shortened_max_prime; i++) {
		for (j = i; j * i <= max_prime + 1; j++) {
			isnt_prime(prime_storage, j);
		}
		while(!is_prime(prime_storage, i)) {
			i++;
		}
	}
}

void isnt_prime(char *prime_storage, int checked_prime) {
	char current_prime;
	int prime_size, prime_bitsize;

	prime_size = checked_prime / 8;
	prime_bitsize = checked_prime % 8;
	current_prime = prime_storage[prime_size];

	prime_storage[current_prime] |= (1 << prime_bitsize);
}

bool is_prime(char *prime_storage, int checked_prime) {
	char current_prime;
	int prime_size, prime_bitsize;

	prime_size = checked_prime / 8;
	prime_bitsize = checked_prime % 8;
	current_prime = prime_storage[prime_size];

	if ((current_prime & (1 << prime_bitsize)) == 0) {
		return true;
	} else {
		return false;
	}
}

void thread_setup(int num_threads) {
	int i;
	pthread_t threads[num_threads];

	for(i = 0; i < num_threads; i++) {
		if (pthread_create(&threads[i], NULL, calc_primes, (void*)(i)) != 0) {
			printf("Abandon ship! Threads were not made.\n");
			exit(EXIT_FAILURE);
		}
	}
	for(i = 0; i < num_threads; i++) {
		pthread_join(threads[i], NULL);
	}
}

void *calc_primes(void *arg) {
	int i = 1, j, min, max;
	long id;

	id = (long)arg;
	
	min = id * (max_prime / num_threads) + 1;

	if (id == num_threads - 1) {
		max = max_prime;
	} else {
		max = min + ((max_prime / num_threads) - 1);
	}

	while((i = next_prime(i)) != 0) {
		for(j = (min / i < 3) ? 3 : (min / i); (i * j) <= max; j++) {
			isnt_prime(prime_storage, i * j);
		}
	}

	return (void *)0;
}

int next_prime(int current_prime) {
	int i;

	for(i = current_prime + 1; i <= sqrt(max_prime); i++) {
		if (is_prime(prime_storage, i)) {
			return i;
		}
	}
	return 0;
}

int count_primes(char *prime_storage, int max_prime) {
	int i, num_primes = 0;

	for(i = 3; i < max_prime + 1; i += 2) {
		if (is_prime(prime_storage, i)) {
			num_primes++;
		}
	}

	return num_primes;
}

void print_results(char *prime_storage, int max_prime) {
	printf("It took %f seconds to calculate %d primes between 1 and %d!\n", total_time, 
		count_primes(prime_storage, max_prime), max_prime);
}

void usage(void) {
	printf("Usage: ./threadedprimes -t[1-100] -m[1-4,294,967,295]\n");
	printf("OPTIONS:\n");
	printf("\t-t, --threads\t- number of threads to use\n");
	printf("\t-m, --max\t- specifies the max in the calculation range\n");
}
