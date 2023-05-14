/* External definitions for single-server queueing system using simlib. */

#include "simlib.h"             /* Required for use of simlib.c. */

#define EVENT_ARRIVAL1         1  /* NEW: Event type for arrival 1. */
#define EVENT_ARRIVAL2         2  /* NEW: Event type for arrival 2. */
#define EVENT_DEPARTURE        3  /* Event type for departure. */

#define LIST_QUEUE1            1  /* NEW: List number for queue 1. */
#define LIST_QUEUE2            2  /* NEW: List number for queue 1. */
#define LIST_SERVER1           3  /* NEW: List number for server 1. */
#define LIST_SERVER2           4  /* NEW: List number for server 2. */
#define LIST_NODE1_ARRIVAL     5  /* NEW: List number for arrival to node 1 */
#define LIST_NODE2_ARRIVAL     6  /* NEW: List number for arrival to node 2 */
#define LIST_SYSTEM_ARRIVAL    7  /* NEW: List number for arrival to system */

#define SAMPST_DELAYS1         1  /* NEW: sampst variable for delays in queue 1. */
#define SAMPST_DELAYS2         2  /* NEW: sampst variable for delays in queue 2. */
#define SAMPST_SYSTEM_DELAY    3  /* NEW: sampst varaible for system delay. */

#define STREAM_INTERARRIVAL    1  /* NEW: Random-number stream for interarrivals 1. */
#define STREAM_SERVICE1        2  /* NEW: Random-number stream for 1st service times. */
#define STREAM_SERVICE2        3  /* NEW: Random-number stream for 2nd service times. */

#define Q_LIMIT1 7 /* NEW: Buffer size for node 1 */
#define Q_LIMIT2 INFINITY /* NEW: Buffer size for node 2 */

/* Declare non-simlib global variables. */

int   num_custs_delayed, num_delays_required;
float mean_interarrival, mean_service;
FILE  *infile, *outfile;

/* Declare non-simlib functions. */

void init_model(void);
void arrive1(void);
void arrive2(void);
void depart(void);
void report(void);


main()  /* Main function. */
{
    /* Open input and output files. */

    infile  = fopen("mm1smlb.in",  "r");
    outfile = fopen("mm1smlb.out", "w");

    /* Read input parameters. */

    fscanf(infile, "%f %f %d", &mean_interarrival, &mean_service,
           &num_delays_required);

    /* Write report heading and input parameters. */

    fprintf(outfile, "2-node tandem queue with two exponential servers using simlib\n\n");
    fprintf(outfile, "Mean interarrival time for server%11.3f minutes\n\n",
            mean_interarrival);
    fprintf(outfile, "Mean service time for server 1 and 2%16.3f minutes\n\n", mean_service);
    fprintf(outfile, "Number of customers%14d\n\n\n", num_delays_required);

    /* Initialize simlib */

    init_simlib();

    /* Set maxatr = max(maximum number of attributes per record, 4) */

    maxatr = 4;  /* NEVER SET maxatr TO BE SMALLER THAN 4. */

    /* Initialize the model. */

    init_model();

    /* Run the simulation while more delays are still needed. */

    while (sim_time <= 100000/*num_custs_delayed < num_delays_required*/) {

        /* Determine the next event. */
 
        timing();

        /* Invoke the appropriate event function. */

        switch (next_event_type) {
            case EVENT_ARRIVAL1:
                // printf("Arrival 1 event time %.6f: \n", sim_time);
                arrive1();
                break;
            case EVENT_ARRIVAL2:
                // printf("Arrival 2 event: %.6f: \n", sim_time);
                arrive2();
                break;
            case EVENT_DEPARTURE:
                // printf("Departure event: %.6f: \n", sim_time);

                depart();
                break;
        }
    }

    /* Invoke the report generator and end the simulation. */

    report();

    fclose(infile);
    fclose(outfile);

    return 0;
}


void init_model(void)  /* Initialization function. */
{
    num_custs_delayed = 0;

    event_schedule(sim_time + expon(mean_interarrival, STREAM_INTERARRIVAL),
                   EVENT_ARRIVAL1);
}


void arrive1(void)  /* Arrival event function. */
{
    /* Schedule next arrival. */

    event_schedule(sim_time + expon(mean_interarrival, STREAM_INTERARRIVAL),
                   EVENT_ARRIVAL1);

    /* Check to see whether server is busy (i.e., list SERVER contains a
       record). */

    if (list_size[LIST_SERVER1] == 1) {
        /* Check to see if the queue is full. If that's the case we do nothing. */
        if (list_size[LIST_QUEUE1] > Q_LIMIT1) {
            printf("Customer blocked because queue 1 size: %d\n", list_size[LIST_QUEUE1]);
            sampst(0, SAMPST_DELAYS1);

            return;
        }

        /* Server is busy, so store time of arrival of arriving customer at end
           of list LIST_QUEUE1. */

        transfer[1] = sim_time;
        list_file(LAST, LIST_QUEUE1);
    }

    else {

        /* Server is idle, so start service on arriving customer, who has a
           delay of zero.  (The following statement IS necessary here.) */

        // sampst(0.0, SAMPST_DELAYS1);

        /* Increment the number of customers delayed. */

        ++num_custs_delayed;

        /* Make server busy by filing a dummy record in list LIST_SERVER. */

        list_file(FIRST, LIST_SERVER1);

        /* Schedule a departure (service completion). */
        event_schedule(sim_time + expon(mean_service, STREAM_SERVICE1),
                       EVENT_ARRIVAL2);
    }

    transfer[1] = sim_time;
    list_file(LAST, LIST_NODE1_ARRIVAL);
    list_file(LAST, LIST_SYSTEM_ARRIVAL);
}

void arrive2(void)  /* Arrival event function. */
{

    /* Check to see whether queue is empty. */

    if (list_size[LIST_QUEUE1] == 0) {
        /* The queue is empty, so make the server idle and leave the departure
           (service completion) event out of the event list. (It is currently
           not in the event list, having just been removed by timing before
           coming here.) */

        list_remove(FIRST, LIST_SERVER1);
    } else {
        list_remove(FIRST, LIST_QUEUE1);
        event_schedule(sim_time + expon(mean_service, STREAM_SERVICE1),
                EVENT_ARRIVAL2);
    }


    /* Check to see whether server is busy (i.e., list SERVER contains a
       record). */

    if (list_size[LIST_SERVER2] == 1) {
        /* Check to see if the queue is full. If that's the case ignore the arriving customer. */
        if (list_size[LIST_QUEUE2] > Q_LIMIT2) {
            printf("Customer blocked because queue 2 size: %d\n", list_size[LIST_QUEUE2]);
            list_remove(FIRST, LIST_SYSTEM_ARRIVAL);
            list_remove(FIRST, LIST_NODE1_ARRIVAL);
            return;
        }

        /* Server is busy, so store time of arrival of arriving customer at end
           of list LIST_QUEUE. */

        transfer[1] = sim_time;
        list_file(LAST, LIST_QUEUE2);
    }

    else {

        /* Server is idle, so start service on arriving customer, who has a
           delay of zero.  (The following statement IS necessary here.) */

        /* Increment the number of customers delayed. */

        // ++num_custs_delayed;

        /* Make server busy by filing a dummy record in list LIST_SERVER. */

        list_file(FIRST, LIST_SERVER2);

        /* Schedule a departure (service completion). */

        event_schedule(sim_time + expon(mean_service, STREAM_SERVICE2),
                       EVENT_DEPARTURE);
    }

    list_remove(FIRST, LIST_NODE1_ARRIVAL);
    sampst(sim_time - transfer[1], SAMPST_DELAYS1);
    transfer[1] = sim_time;
    list_file(LAST, LIST_NODE2_ARRIVAL);
}


void depart(void)  /* Departure event function. */
{
    /* Check to see whether queue is empty. */

    if (list_size[LIST_QUEUE2] == 0)

        /* The queue is empty, so make the server idle and leave the departure
           (service completion) event out of the event list. (It is currently
           not in the event list, having just been removed by timing before
           coming here.) */

        list_remove(FIRST, LIST_SERVER2);

    else {

        /* The queue is nonempty, so remove the first customer from the queue,
           register delay, increment the number of customers delayed, and
           schedule departure. */

        list_remove(FIRST, LIST_QUEUE2);
        // sampst(sim_time - transfer[1], SAMPST_DELAYS);
        ++num_custs_delayed;
        event_schedule(sim_time + expon(mean_service, STREAM_SERVICE2),
                       EVENT_DEPARTURE);
    }

    list_remove(FIRST, LIST_NODE2_ARRIVAL);
    sampst(sim_time - transfer[1], SAMPST_DELAYS2);
    list_remove(FIRST, LIST_SYSTEM_ARRIVAL);
    sampst(sim_time - transfer[1], SAMPST_SYSTEM_DELAY);
}


void report(void)  /* Report generator function. */
{
    /* Get and write out estimates of desired measures of performance. */

    fprintf(outfile, "\nDelays in queue, in minutes:\n");
    out_sampst(outfile, SAMPST_DELAYS1, SAMPST_SYSTEM_DELAY);
    fprintf(outfile, "\nQueue length (1) and server utilization (2):\n");
    out_filest(outfile, LIST_NODE1_ARRIVAL, LIST_NODE2_ARRIVAL);
    fprintf(outfile, "\nTime simulation ended:%12.3f minutes\n", sim_time);
}

