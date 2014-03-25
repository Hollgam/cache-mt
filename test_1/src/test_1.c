/*
 ============================================================================================
 Name        : test_1.c
 Author      : Pavlo Bazilinskyy
 Version     : 0.1
 Copyright   : Copyright (c) 2014, Pavlo Bazilinskyy <pavlo.bazilinskyy@gmail.com>
 School of Computer Science, National University of Ireland, Maynooth

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

 Description : Test 1 (measuring cache latency)
 Target		 : MacBook Air with i7 and Xeon 5130
 ============================================================================================
 */

#include "test_1.h"

int main(void) {
	// Testing different aspects of the system/testing environment

	// Testing interrupt time. Not for Mac.
#ifndef __APPLE__
	//test_interrupt_time();
#endif

// Testing rdrsc
//	unsigned long long t[32], prev;
//	int i;
//	for (i = 0; i < 32; i++)
//		t[i] = rdtsc();
//	prev = t[0];
//	for (i = 1; i < 32; i++) {
//		printf("%llu [%llu]\n", t[i], t[i] - prev);
//		prev = t[i];
//	}
//	printf("Total=%llu\n", t[32 - 1] - t[0]);

// Set process priority to the highest possible value
#ifdef SET_HIGHEST_PRIORITY
	set_highest_process_priority();
#endif

	// TODO fix pthreads on Mac OS
#ifndef __APPLE__
	pthread_t thread1;
	int rc;

	// Run in pthread
	rc = pthread_create(&thread1, NULL, pthread_main, (void *)1);
	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
	pthread_join(thread1, NULL);
#else
	pthread_main(1);
#endif
	// Everything is good, return Success code
	return EXIT_SUCCESS;
}

// Function run in the thread
int pthread_main(int thread_num) {
	unsigned long long *time = malloc(sizeof(unsigned long long) * MAX_POWER); // Record time for clean runs.
	if (time == NULL) {
		printf("Error with allocating space for the array\n");
		exit(1);
	}
	unsigned long long *timeDirty = malloc(sizeof(unsigned long long) * MAX_POWER); // Record time for dirty runs.
	if (timeDirty == NULL) {
		printf("Error with allocating space for the array\n");
		exit(1);
	}
	struct timespec start, stop;
	int i = 0;
	long n;

	// Test clock_gettime
	struct timespec start1, start2, start3;
	get_time_ns(&start1);
	get_time_ns(&start2);
	get_time_ns(&start3);

	while (calculate_time_ns(start1, start2) == 0ull) {
		get_time_ns(&start2);
	}
	get_time_ns(&start3);
	while (calculate_time_ns(start2, start3) == 0ull) {
		get_time_ns(&start3);
	}

	printf("TIME. Time: %ld %ld %ld.", BILLION * start1.tv_sec + start1.tv_nsec,
	BILLION * start2.tv_sec + start2.tv_nsec,
	BILLION * start3.tv_sec + start3.tv_nsec);
	printf(" Diff: %llu %llu\n", calculate_time_ns(start1, start2), calculate_time_ns(start2, start3));

	// Test clock_getres
	struct timespec startres1;
	get_time_res(&startres1);
	printf("RESOLUTION. time: %ld\n",
	BILLION * startres1.tv_sec + startres1.tv_nsec);

	// Sort out thread affinity
#if PROCESS_AFFINITY == PIN_TO_ONE_CPU
	pin_thread_to_core(PIN_TO_CPU); // Pin this pthread to PIN_TO_CPU
#endif

//	while (1 == 1) {
//		printf("My current process id/pid is %d\n", getpid());
//	}

	// Run experiments
	for (n = 1.0; n < pow(2.0, (double) MAX_POWER); n *= 2.0) {
//		unsigned char testAr[(int) n];
		unsigned char *testAr = malloc(sizeof(unsigned char) * n * 2);
		if (testAr == NULL) { // Array for manipulating data
			printf("Error with allocating space for the array\n");
			exit(1);
		}
		unsigned char testCh = ' '; // 1 byte of data
#ifdef WARM_CACHE
		//Warm up cache
		experiment(testAr, testCh, 0); // Call experiment function once to warm up cache
#endif

#ifdef WARM_STRINGS_WITH_FILES
		// Create two copies of each string used for storing files to fill in memory with this data.
		warm_strings_with_files();
#endif

		// Run each experiment for TIMES_RUN_EXPERIMENT times
		int expI = 0;
		unsigned long long *currentTime = malloc(sizeof(unsigned long long) * TIMES_RUN_EXPERIMENT); // Record times of experiments in the run.
		if (time == NULL) {
			printf("Error with allocating space for the array\n");
			exit(1);
		}
		unsigned long long *currentTimeDirty = malloc(sizeof(unsigned long long) * TIMES_RUN_EXPERIMENT); // Record times of experiments in the run.
		if (time == NULL) {
			printf("Error with allocating space for the array\n");
			exit(1);
		}
		int experimentsRunSuccessfully = 0; // Record how many times the experiment was run successfully
		for (expI = 0; expI < TIMES_RUN_EXPERIMENT; ++expI) {
#ifdef DETAILED_DEBUG
			printf("* Iteration: %d Bytes: %.0f Experiment: %d\n", i + 1, pow(2.0, (double) i), expI + 1);
#endif

			// Values
			unsigned long long interruptsBefore = 0;
			unsigned long long interruptsAfter = 0;
			unsigned long long pageFaultsMinorBefore = 0;
			unsigned long long pageFaultsMinorAfter = 0;
			unsigned long long pageFaultsMajorBefore = 0;
			unsigned long long pageFaultsMajorAfter = 0;
			unsigned long long contextSwitchesBefore = 0;
			unsigned long long contextSwitchesAfter = 0;

#ifndef __APPLE__
			// Strings for storing contents of the files
			char *interruptsBeforeString = malloc(1500);
			if (interruptsBeforeString == NULL) {
				printf("Error with allocating space for the string interruptsBeforeString\n");
				exit(1);
			}
			char *pageFaultsBeforeString = malloc(1500);
			if (pageFaultsBeforeString == NULL) {
				printf("Error with allocating space for the string pageFaultsBeforeString\n");
				exit(1);
			}
			char *contextSwitchesBeforeString = malloc(1500);
			if (contextSwitchesBeforeString == NULL) {
				printf("Error with allocating space for the string contextSwitchesBeforeString\n");
				exit(1);
			}

			// Get process ID
			int processId = getpid();

			// Create path to the proc/PID file
			char fileName[100];
			char buf[100];
			char buf2[100] = "/proc/";
			snprintf(buf, 100, "%d", processId);
			snprintf(fileName, 100, "%s%s", buf2, buf);

			// Get readings on interrupts, pagefaults and context switched before running the experiment
			//Info: http://www.centos.org/docs/5/html/Deployment_Guide-en-US/s1-proc-topfiles.html

			// Save files with information on interrupts, page faults, and context switches into strings
			interruptsBeforeString = file_to_string("/proc/interrupts");

			// Add stat to the name of the file
			char fileNameStat[100];
			snprintf(fileNameStat, 100, "%s%s", fileName, "/stat");
			pageFaultsBeforeString = file_to_string(fileNameStat);

			// Add status to the name of the file
			char fileNameStatus[100];
			snprintf(fileNameStatus, 100, "%s%s", fileName, "/status");
			contextSwitchesBeforeString = file_to_string(fileNameStatus);
#else
			//TODO read for Mac OS
#endif
			int j;
			// Run experiment function

#ifdef START_AFTER_TIMER_TICK
			// Start after the timer ticks
			struct timespec temp_time1, start;

			get_time_ns(&temp_time1);
			get_time_ns(&start);

			while (start.tv_sec == temp_time1.tv_sec && temp_time1.tv_nsec == start.tv_nsec) {
				get_time_ns(&start);
			}
#else
			struct timespec start,
			get_time_ns(&start); // Calculate start time
#endif

			for (j = 0; j < n; j++) { // Write and read 1 byte n times
				experiment(testAr, testCh, n);
			}
			get_time_ns(&stop); // Calculate finish time

			// Get readings on interrupts, pagefaults and context switched before running the experiment
#ifndef __APPLE__
			interruptsAfter = search_in_file("/proc/interrupts", "LOC:", 1);
			pageFaultsMinorAfter = get_page_fault(1);
			pageFaultsMajorAfter = get_page_fault(2);
			contextSwitchesAfter = search_in_file(fileNameStatus, "voluntary_ctxt_switches:", 1);

			// Retrieve information from saved into strings files.
			interruptsBefore = search_in_string(interruptsBeforeString, "LOC:", 1);
			pageFaultsMinorBefore = get_page_fault_from_string(pageFaultsBeforeString, 1);
			pageFaultsMajorBefore = get_page_fault_from_string(pageFaultsBeforeString, 2);
			contextSwitchesBefore = search_in_string(contextSwitchesBeforeString, "voluntary_ctxt_switches:", 1);

#ifdef DETAILED_DEBUG
			printf("INT B: %llu :: ", interruptsBefore);
			printf("PF Min B: %llu :: ", pageFaultsMinorBefore);
			printf("PF Maj B: %llu :: ", pageFaultsMajorBefore);
			printf("CS B: %llu\n", contextSwitchesBefore);
			printf("INT A: %llu :: ", interruptsAfter);
			printf("PF Min A: %llu :: ", pageFaultsMinorAfter);
			printf("PF Maj A: %llu :: ", pageFaultsMajorAfter);
			printf("CS A: %llu\n", contextSwitchesAfter);
#endif

#else
			//TODO read for Mac OS
#endif
			// Record difference
			unsigned long long tempTime = calculate_time_ns(start, stop); // Calculate how much time this run took
			// Record dirty time
			currentTimeDirty[expI] = tempTime; // Record how much time this iteration took

			// Disregard experiment if comparison of it with MIN and MAX makes it invalid
			//TODO verify with Stephen
			unsigned long long minTime = n * 1; // Lower bound for the duration of the run
			unsigned long long maxTime = n * 100000; // Upper bound for the duration of the run
			if (tempTime < minTime || tempTime > maxTime) { // Disregard this run if it does not meet timing requirements
				continue;
			} else if (pageFaultsMinorAfter - pageFaultsMinorBefore > ALLOWED_PAGEFAULTS_MINOR) { // Disregard this run if minor pagefaults were detected
#ifdef DEBUG
				printf("PFMIN. EXP: %ld. RUN: %d B: %llu A: %llu. LIMIT: %d\n", n, expI, pageFaultsMinorBefore, pageFaultsMinorAfter,
				ALLOWED_PAGEFAULTS_MINOR);
#endif
				continue;
			} else if (pageFaultsMajorAfter - pageFaultsMajorBefore > ALLOWED_PAGEFAULTS_MAJOR) { // Disregard this run if major pagefaults were detected
#ifdef DEBUG
				printf("PFMAJ. EXP: %ld. RUN: %d B: %llu A: %llu. LIMIT: %d\n", n, expI, pageFaultsMajorBefore, pageFaultsMajorAfter,
				ALLOWED_PAGEFAULTS_MAJOR);
#endif
				continue;
			} else if (contextSwitchesAfter - contextSwitchesBefore > ALLOWED_CONTEXT_SWITCHES) { // Disregard this run if voluntary context switches were detected
#ifdef DEBUG
				printf("CS. EXP: %ld. RUN: %d B: %llu A: %llu. LIMIT: %d\n", n, expI, contextSwitchesBefore, contextSwitchesAfter,
				ALLOWED_CONTEXT_SWITCHES);
#endif
				continue;
			} else if (interruptsAfter - interruptsBefore > ALLOWED_INTERRUPTS) { // Disregard this run if interrupts were detected
#ifdef DEBUG
				printf("INT. EXP: %ld. RUN: %d B: %llu A: %llu. LIMIT: %d\n", n, expI, interruptsBefore, interruptsAfter,
				ALLOWED_INTERRUPTS);
#endif
				continue;
			} else { // Everything it fine, record this run as successful
				currentTime[experimentsRunSuccessfully] = tempTime; // Record how much time this iteration took
				experimentsRunSuccessfully++;
			}
		}
		// Calculate average time of running the experiment
		time[(int) i] = average_time(currentTime, experimentsRunSuccessfully); // 0 denotes a failed experiment (number of successful runs = 0)
		timeDirty[(int) i] = average_time(currentTimeDirty, TIMES_RUN_EXPERIMENT); // 0 denotes a failed experiment (number of successful runs = 0)
		i++; // Iterate for easier access to array

		// Free memory
		free(currentTime);
		free(currentTimeDirty);
		free(testAr); // free memory allocated for the array
	}
	// Output results
#ifdef SHOW_RESULTS
	printf("\nRESULTS Clean - %d:\n", MAX_POWER);
	for (i = 0; i < MAX_POWER; ++i) {
		printf("%d. %.0f - %llu\n", i + 1, pow(2.0, (double) i), time[i]);
	}

	printf("\nRESULTS Dirty - %d:\n", MAX_POWER);
	for (i = 0; i < MAX_POWER; ++i) {
		printf("%d. %.0f - %llu\n", i + 1, pow(2.0, (double) i), timeDirty[i]);
	}
#endif

#ifdef OUTPUT_TO_FILE
	// Write to file
	write_to_csv(time, 1);
	write_to_csv(timeDirty, 2);
#endif

	// Free memory and exit
	free(timeDirty);
	free(time);
	return 1;
}

// Experiment itself, aslo used for warming up cache
void experiment(unsigned char *testAr, unsigned char testCh, int n) {
	testAr[(int) n] = CHAR_TO_ADD; // Write 1 byte
	testCh = testAr[(int) n]; // Read 1 byte
}

// Calculate average time of running the experiment
unsigned long long average_time(unsigned long long *time, int timesRun) {
	if (timesRun == 0) // avoid division
		return 0;
	// Loop through times of all runs of the experiment
	int i;
	unsigned long long avTime = 0;
	for (i = 0; i < timesRun; ++i) {
		avTime += time[i];
	}
	return avTime / timesRun;
}

// Pin thread to a particular core
int pin_thread_to_core(int coreId) {
#ifndef __APPLE__
	int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
	if (coreId < 0 || coreId >= num_cores)
	return EINVAL;

	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(coreId, &cpuset);

	pthread_t current_thread = pthread_self();
	return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
#else
	return -1;
#endif
}

// Set priority of the current to be the highest
// From: http://stackoverflow.com/questions/29621/change-own-process-priority-in-c
int set_highest_process_priority(void) {
	setpriority(PRIO_PROCESS, 0, -20); // TODO set to FIFO real time process
	return 1;
}

// Record how much time one interrupt takes on the testing system
void test_interrupt_time(void) {
	struct timespec start, stop;
	int run = 10;
	int numBytes = 40;

	// Record time run times
	unsigned long long *time = malloc(sizeof(unsigned long long) * run); // Record times of experiments in the run.
	if (time == NULL) {
		printf("Error with allocating space for the array\n");
		exit(1);
	}

	int i = 0;
	for (i = 0; i < run; ++i) {
		unsigned long long interruptsBefore;
		unsigned long long interruptsAfter;

		// Warmup
		interruptsAfter = search_in_file("/proc/interrupts", "LOC:", 1);
		interruptsBefore = interruptsAfter;

		while (interruptsBefore == interruptsAfter)
			interruptsBefore = search_in_file("/proc/interrupts", "LOC:", 1);

		get_time_ns(&start); // Record time before causing the interrupt

		// Create interrupts
		do {
			interruptsAfter = search_in_file("/proc/interrupts", "LOC:", 1);
		} while (interruptsAfter - interruptsBefore == 0);

		get_time_ns(&stop); // Record time after causing the interrupt

		int numInterrupts = interruptsAfter - interruptsBefore; // How many interrupts occurred
		unsigned long long timeWithInterupts = calculate_time_ns(start, stop); // Record time with interrupts

		printf("Time %llu Num %d\n", timeWithInterupts, numInterrupts);
		time[i] = timeWithInterupts / numInterrupts; // Record time difference over a number of interrupts
	}
	printf("1 interrupt takes (average from %d runs): %llu\n", run, average_time(time, run));

	free(time);
}

// Create two copies of each string used for storing files to fill in memory with this data.
int warm_strings_with_files(void) {
	// Strings for storing contents of the files
	char *interruptsBeforeStringWarm1 = malloc(1500);
	if (interruptsBeforeStringWarm1 == NULL) {
		printf("Error with allocating space for the string interruptsBeforeString\n");
		exit(1);
	}
	char *pageFaultsBeforeStringWarm1 = malloc(1500);
	if (pageFaultsBeforeStringWarm1 == NULL) {
		printf("Error with allocating space for the string pageFaultsBeforeString\n");
		exit(1);
	}
	char *contextSwitchesBeforeStringWarm1 = malloc(1500);
	if (contextSwitchesBeforeStringWarm1 == NULL) {
		printf("Error with allocating space for the string contextSwitchesBeforeString\n");
		exit(1);
	}
	char *interruptsBeforeStringWarm2 = malloc(1500);
	if (interruptsBeforeStringWarm2 == NULL) {
		printf("Error with allocating space for the string interruptsBeforeString\n");
		exit(1);
	}
	char *pageFaultsBeforeStringWarm2 = malloc(1500);
	if (pageFaultsBeforeStringWarm2 == NULL) {
		printf("Error with allocating space for the string pageFaultsBeforeString\n");
		exit(1);
	}
	char *contextSwitchesBeforeStringWarm2 = malloc(1500);
	if (contextSwitchesBeforeStringWarm2 == NULL) {
		printf("Error with allocating space for the string contextSwitchesBeforeString\n");
		exit(1);
	}

	// Get process ID
	int processIdTemp = getpid();

	// Create path to the proc/PID file
	char fileNameTemp[100];
	char bufTemp[100];
	char buf2Temp[100] = "/proc/";
	snprintf(bufTemp, 100, "%d", processIdTemp);
	snprintf(fileNameTemp, 100, "%s%s", buf2Temp, bufTemp);

	// Get readings on interrupts, pagefaults and context switched before running the experiment
	//Info: http://www.centos.org/docs/5/html/Deployment_Guide-en-US/s1-proc-topfiles.html

	// Save files with information on interrupts, page faults, and context switches into strings
	interruptsBeforeStringWarm1 = file_to_string("/proc/interrupts");
	interruptsBeforeStringWarm2 = file_to_string("/proc/interrupts");

	// Add stat to the name of the file
	char fileNameStatTemp[100];
	snprintf(fileNameStatTemp, 100, "%s%s", fileNameStatTemp, "/stat");
	pageFaultsBeforeStringWarm1 = file_to_string(fileNameStatTemp);
	pageFaultsBeforeStringWarm2 = file_to_string(fileNameStatTemp);

	// Add status to the name of the file
	char fileNameStatusTemp[100];
	snprintf(fileNameStatTemp, 100, "%s%s", fileNameStatTemp, "/status");
	contextSwitchesBeforeStringWarm1 = file_to_string(fileNameStatTemp);
	contextSwitchesBeforeStringWarm2 = file_to_string(fileNameStatTemp);

	return 1;
}
