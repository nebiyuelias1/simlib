/* External definitions for single-server queueing system using simlib. */

#include "simlib.h"             /* Required for use of simlib.c. */

#define EVENT_CLASS1_ARRIVAL        1  /* NEW: Event type for arrival for class I. */
#define EVENT_CLASS2_ARRIVAL        2  /* NEW: Event type for arrival for class II. */
#define EVENT_CLASS1_DEPARTURE      3  /* NEW: Event type for departure for class I. */
#define EVENT_CLASS2_DEPARTURE      4  /* NEW: Event type for departure for class II. */
#define LIST_CLASS1_QUEUE           1  /* NEW: List number for class I queue. */
#define LIST_CLASS2_QUEUE           2  /* NEW List number for class II queue. */
#define LIST_SERVER                 3  /* List number for server. */
#define SAMPST_CLASS1_DELAYS        1  /* NEW: sampst variable for delays in class I queue. */
#define SAMPST_CLASS2_DELAYS        2  /* NEW: sampst variable for delays in class II queue. */
#define STREAM_CLASS1_INTERARRIVAL  1  /* NEW: Random-number stream for interarrivals for class I. */
#define STREAM_CLASS2_INTERARRIVAL  2  /* NEW: Random-number stream for interarrivals for class II. */
#define STREAM_CLASS1_SERVICE       3  /* NEW: Random-number stream for class I service times. */
#define STREAM_CLASS2_SERVICE       4  /* NEW: Random-number stream for class II service times. */
#define CLASS1 1 /* NEW: Type Class I */
#define CLASS2 2 /* NEW: Type Class II */

/* Declare non-simlib global variables. */

int   num_custs_delayed, num_delays_required;
float mean_interarrival_class1, mean_interarrival_class2, mean_service_class1, mean_service_class2; /* NEW */
FILE  *infile, *outfile;

/* Declare non-simlib functions. */

void init_model(void);
void arrive(int);
void depart(int);
void report(void);


main()  /* Main function. */
{
    /* Open input and output files. */

    infile  = fopen("mm1smlb.in",  "r");
    outfile = fopen("mm1smlb.out", "w");

    /* Read input parameters. */

    fscanf(infile, "%f %f %f %f %d", &mean_interarrival_class1, 
                                    &mean_interarrival_class2, 
                                    &mean_service_class1, 
                                    &mean_service_class2,
                                    &num_delays_required);

    /* Write report heading and input parameters. */

    fprintf(outfile, "Single-server queueing system using simlib\n\n");
    fprintf(outfile, "Mean interarrival time for class I%11.3f minutes\n\n",
            mean_interarrival_class1);
    fprintf(outfile, "Mean interarrival time for class II%11.3f minutes\n\n",
            mean_interarrival_class2);
    fprintf(outfile, "Mean service time class I%16.3f minutes\n\n", mean_service_class1);
    fprintf(outfile, "Mean service time class II%16.3f minutes\n\n", mean_service_class2);
    fprintf(outfile, "Number of customers%14d\n\n\n", num_delays_required);

    /* Initialize simlib */

    init_simlib();

    /* Set maxatr = max(maximum number of attributes per record, 4) */

    maxatr = 4;  /* NEVER SET maxatr TO BE SMALLER THAN 4. */

    /* Initialize the model. */

    init_model();

    /* Run the simulation while more delays are still needed. */

    while (num_custs_delayed < num_delays_required) {

        /* Determine the next event. */

        timing();

        /* Invoke the appropriate event function. */

        switch (next_event_type) {
            case EVENT_CLASS1_ARRIVAL:
                arrive(CLASS1);
                break;
            case EVENT_CLASS2_ARRIVAL:
                arrive(CLASS2);
                break;
            case EVENT_CLASS1_DEPARTURE:
                depart(CLASS1);
                break;
            case EVENT_CLASS2_DEPARTURE:
                depart(CLASS2);
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

    event_schedule(sim_time + expon(mean_interarrival_class1, STREAM_CLASS1_INTERARRIVAL),
                   EVENT_CLASS1_ARRIVAL);

    event_schedule(sim_time + expon(mean_interarrival_class2, STREAM_CLASS2_INTERARRIVAL),
                   EVENT_CLASS2_ARRIVAL);
}


void arrive(int class_type)  /* Arrival event function. */
{
    /* Schedule next arrival based on class type. */
    switch (class_type)
    {
        case CLASS1:
            event_schedule(sim_time + expon(mean_interarrival_class1, STREAM_CLASS1_INTERARRIVAL),
                        EVENT_CLASS1_ARRIVAL);
            break;

        case CLASS2:
            event_schedule(sim_time + expon(mean_interarrival_class2, STREAM_CLASS2_INTERARRIVAL),
                        EVENT_CLASS2_ARRIVAL);
            break;
    }

    /* Check to see whether server is busy (i.e., list SERVER contains a
       record). */

    if (list_size[LIST_SERVER] == 1) {

        /* Server is busy, so store time of arrival of arriving customer at end
           of list LIST_QUEUE. */

        transfer[1] = sim_time;
        switch (class_type)
        {
            case CLASS1:
                list_file(LAST, LIST_CLASS1_QUEUE);
                break;
            
            case CLASS2:
                list_file(LAST, LIST_CLASS2_QUEUE);
                break;
        }

    }

    else {

        

        /* Increment the number of customers delayed. */

        ++num_custs_delayed;

        /* Make server busy by filing a dummy record in list LIST_SERVER. */

        list_file(FIRST, LIST_SERVER);

        switch (class_type)
        {
            case CLASS1:
                /* Server is idle, so start service on arriving customer, who has a
                delay of zero.  (The following statement IS necessary here.) */
            
                sampst(0.0, SAMPST_CLASS1_DELAYS);

                /* Schedule a departure (service completion). */

                event_schedule(sim_time + expon(mean_service_class1, STREAM_CLASS1_SERVICE),
                            EVENT_CLASS1_DEPARTURE);
                break;
            
            case CLASS2:
                /* Server is idle, so start service on arriving customer, who has a
                delay of zero.  (The following statement IS necessary here.) */
            
                sampst(0.0, SAMPST_CLASS2_DELAYS);

                /* Schedule a departure (service completion). */

                event_schedule(sim_time + expon(mean_service_class2, STREAM_CLASS2_SERVICE),
                            EVENT_CLASS2_DEPARTURE);
                break;
        }
    }
}


void depart(int class_type)  /* Departure event function. */
{
    printf("current class: %d\n", class_type);
    printf("class 1 size: %d\n", list_size[LIST_CLASS1_QUEUE]);
    printf("class 2 size: %d\n", list_size[LIST_CLASS2_QUEUE]);
    int queue_list_size = class_type == CLASS1 ? list_size[LIST_CLASS1_QUEUE] : list_size[LIST_CLASS2_QUEUE];

    /* Check to see whether queue is empty. */

    if (list_size[LIST_CLASS1_QUEUE] == 0 && list_size[LIST_CLASS2_QUEUE] == 0) 
    {

            /* The queue is empty, so make the server idle and leave the departure
            (service completion) event out of the event list. (It is currently
            not in the event list, having just been removed by timing before
            coming here.) */

            list_remove(FIRST, LIST_SERVER);
    }

    else {

        /* The queue is nonempty, so remove the first customer from the queue,
           register delay, increment the number of customers delayed, and
           schedule departure. */
        switch (class_type)
        {
            case CLASS1:
                if (list_size[LIST_CLASS1_QUEUE] > 0) 
                {
                    list_remove(FIRST, LIST_CLASS1_QUEUE);
                    sampst(sim_time - transfer[1], SAMPST_CLASS1_DELAYS);
                    event_schedule(sim_time + expon(mean_service_class1, STREAM_CLASS1_SERVICE),
                                EVENT_CLASS1_DEPARTURE);
                } else {
                     list_remove(FIRST, LIST_CLASS2_QUEUE);
                    sampst(sim_time - transfer[1], SAMPST_CLASS2_DELAYS);
                    event_schedule(sim_time + expon(mean_service_class2, STREAM_CLASS2_SERVICE),
                                EVENT_CLASS2_DEPARTURE);
                }
                break;
               
            
            case CLASS2:
                if (list_size[LIST_CLASS2_QUEUE] > 0) 
                {
                    list_remove(FIRST, LIST_CLASS2_QUEUE);
                    sampst(sim_time - transfer[1], SAMPST_CLASS2_DELAYS);
                    event_schedule(sim_time + expon(mean_service_class2, STREAM_CLASS2_SERVICE),
                                EVENT_CLASS2_DEPARTURE);
                } else 
                {
                    list_remove(FIRST, LIST_CLASS1_QUEUE);
                    sampst(sim_time - transfer[1], SAMPST_CLASS1_DELAYS);
                    event_schedule(sim_time + expon(mean_service_class1, STREAM_CLASS1_SERVICE),
                                EVENT_CLASS1_DEPARTURE);
                }
                break;

        }
        ++num_custs_delayed;
    }
}


void report(void)  /* Report generator function. */
{
    /* Get and write out estimates of desired measures of performance. */

    fprintf(outfile, "\nDelays in queue for class I (1) and class II (2), in minutes:\n");
    out_sampst(outfile, SAMPST_CLASS1_DELAYS, SAMPST_CLASS2_DELAYS);
    fprintf(outfile, "\nQueue length for class I (1), class II (2) and server utilization (3):\n");
    out_filest(outfile, LIST_CLASS1_QUEUE, LIST_SERVER);
    fprintf(outfile, "\nTime simulation ended:%12.3f minutes\n", sim_time);
}

