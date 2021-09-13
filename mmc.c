#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.h"

#define BUSY 1
#define IDLE 0
#define Q_LIMIT 1000000

int next_event_type, num_events, num_in_q;
int num_of_server_idle, server_number, total_num_of_server;
int num_delays_required, num_cus_delayed;
long double mean_inter_arr_time, mean_service_time, area_num_in_q, area_server_status, sim_time, time_arrival[Q_LIMIT + 1], time_last_event, next_arrival_time, next_departure_time[10000], min_next_departure_time, total_of_delays;

FILE *infile, *outfile;

void initialize(void);
void timing(void);
void update_time_avg_stats(void);
void arrival(void);
void departure(void);
void report(void);

long double exponential(long double mean);
long double min_departure_time(long double next_departure_time[]); // detect the smallest departure time

int main() {
    infile = fopen("mmc.in", "r");
    outfile = fopen("mmc.out", "w");
    num_events = 2; // arrive 1 and depart 2
    
    fscanf(infile, "%d %Lf %Lf %d", &num_delays_required, &mean_inter_arr_time, &mean_service_time, &total_num_of_server);
    printf("num of delay required = %d\n", num_delays_required);
    printf("mean interval = %Lf\n", mean_inter_arr_time);
    printf("mean service = %Lf\n", mean_service_time);
    printf("total num of server = %d\n",total_num_of_server);
    initialize();
    
    while(num_cus_delayed < num_delays_required){
        //printf("num in queue = %d\n",num_in_q);
        timing(); // Determine the next event
        update_time_avg_stats(); // Update time_average area_num_in_q area_server_status

        switch (next_event_type)
        {
        case 1:
            arrival();
            break;
        case 2:
            departure();
            break;
        }
    }
    
    report();
    
    fclose(infile);
    fclose(outfile);
    
    printf("Simulation Done !");
    return 0;
}

void initialize(void){
    sim_time = 0.0;

    next_event_type = 0;
    num_cus_delayed = 0;
    num_in_q = 0;
    num_of_server_idle = total_num_of_server;
    area_num_in_q = 0.0;
    area_server_status = 0.0;
    server_number = 0;
    
    next_arrival_time = sim_time + exponential(mean_inter_arr_time);
    for(int i=1; i<=total_num_of_server; i++){
        //server_status[i] = IDLE;
        next_departure_time[i] = 1.0e+30;
    }
}

void timing(void){
    long double min_time_next_event;
    next_event_type = 0;
    //compare and choose the smallest time to occur
    min_next_departure_time = min_departure_time(next_departure_time);
    if(next_arrival_time <= min_next_departure_time){
        min_time_next_event = next_arrival_time;
        next_event_type = 1;
    }
    else{
        min_time_next_event = min_next_departure_time;
        next_event_type = 2;
    }
    if(next_event_type == 0){
        fprintf(outfile,"\nEvent list is empty at time %Lf",sim_time);
        exit(1);
    }
    sim_time = min_time_next_event;
}

long double min_departure_time(long double next_departure_time[]){
    long double min_departure_time = 1.0e+30;
    
    for (int i = 1; i <= total_num_of_server; i++){
        if (next_departure_time[i] <= min_departure_time){
          min_departure_time = next_departure_time[i];
          server_number=i; //server of this number will departure
        }
    }
    return min_departure_time;
}

long double exponential(long double mean){
    return -mean * log(lcgrand(1));
}

void update_time_avg_stats(void){
    long double time_since_last_event;
    time_since_last_event = sim_time - time_last_event;
    time_last_event = sim_time;
    
    area_num_in_q += num_in_q * time_since_last_event;
    area_server_status += (total_num_of_server - num_of_server_idle) * time_since_last_event;
}

void arrival(void){
    long double delay;
    
    next_arrival_time = sim_time + exponential(mean_inter_arr_time);
    if(num_of_server_idle == 0){
        ++num_in_q;
        if (num_in_q > Q_LIMIT){
            fprintf(outfile, "\nOverflow of the array time_arrival at");
            fprintf(outfile, " time %Lf", sim_time);
            exit(2);
        }
        time_arrival[num_in_q] = sim_time; //remember time arrival in array
    }
    else{ //there has an idle server
        delay = 0;
        total_of_delays += delay;
        ++num_cus_delayed;
        //server_status = BUSY;
        --num_of_server_idle; //server number decrement
        for(int i=1; i<=total_num_of_server;i++){
            if(next_departure_time[i] == 1.0e+30){
                next_departure_time[i] = sim_time + exponential(mean_service_time); //setting the departure time for this server
                break;
            }
        }
        time_arrival[num_in_q] = sim_time;
    }
}

void departure(void){
    long double delay;
    if(num_in_q == 0){ //if there is no any customer in queue
        if(num_of_server_idle < total_num_of_server){ //server number initialize
            ++num_of_server_idle;
        } 
        next_departure_time[server_number] = 1.0e+30;
    }
    else{ //there still has customers in queue and go into server
        --num_in_q;
        delay = sim_time - time_arrival[1];
        total_of_delays += delay;
        ++num_cus_delayed;
        next_departure_time[server_number] = sim_time + exponential(mean_service_time);
        for(int i = 1; i<=num_in_q; i++){
            time_arrival[i] = time_arrival[i+1];
        }
    }
}

void report(void){
    long double mean_inter_arr_rate, mean_service_rate, theo_avg_delay, theo_avg_num_in_q, theo_server_util;
    mean_inter_arr_rate = pow(mean_inter_arr_time,-1);
    mean_service_rate = pow(mean_service_time,-1);
    theo_avg_delay = mean_inter_arr_rate/mean_service_rate/(mean_service_rate-mean_inter_arr_rate);
    theo_avg_num_in_q = pow(mean_inter_arr_rate,2)/(mean_service_rate*(mean_service_rate-mean_inter_arr_rate));
    theo_server_util = mean_inter_arr_rate/mean_service_rate;
    fprintf(outfile, "Mean interarrival time%11.3Lf minutes\n", mean_inter_arr_time);
    fprintf(outfile, "Mean service time%16.3Lf minutes\n", mean_service_time);
    fprintf(outfile, "Number of customers%16.3d\n", num_delays_required);
    fprintf(outfile, "Number of servers%16.3d\n", total_num_of_server);
    fprintf(outfile, "-----------------------------------------\n");
    fprintf(outfile, "[Theo] Avg delay in queue%8.4f minutes\n",0.023561);
    fprintf(outfile, "[Expe] Avg delay in queue%8.4Lf minutes\n\n",total_of_delays/num_delays_required);
    fprintf(outfile, "[Theo] Avg number in queue%7.4f \n",0.018921);
    fprintf(outfile, "[Expe] Avg number in queue%7.4Lf \n\n",area_num_in_q/sim_time);
    fprintf(outfile, "[Theo] Server utilization%8.3f \n",0.2667);
    fprintf(outfile, "[Expe] Server utilization%8.3Lf \n",area_server_status/sim_time/total_num_of_server);
    fprintf(outfile, "-----------------------------------------\n");
    fprintf(outfile, "Time simulation ended %12.3Lf minutes", sim_time);
}