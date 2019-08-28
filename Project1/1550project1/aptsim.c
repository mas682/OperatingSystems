/* A program that simulates agents showing tenants a
 * apartment.
 * Matt Stropkey
 * CS1550
 * Project2
*/


#include <sys/mman.h>
#include <linux/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sem.h"


struct cs1550_sem *agent_here;              //used to mark that an agent at door
struct cs1550_sem *tenant_waiting;          //used to mark that tenant waiting at door
struct cs1550_sem *apt_open;                //used to say if door open
struct cs1550_sem *apt_occupied;            //used to make agent sleep while tenants inside
struct cs1550_sem *total_ten;               //used as a lock to keep tenants in order while executing
struct cs1550_sem *tenants_viewing;         //used to lock variable so that 10 tenants can view apartment with agent
struct cs1550_sem *num_in_apt;              //used to lock a variable to number of tenants in apartment
struct cs1550_sem *lock;                    //used to prevent agents/tenant process from running concurrently at key points
struct cs1550_sem *apt_open_boolean;        //used as lock to variable that says if door open or not
struct cs1550_sem *wait_view;               //tenants sleep on this while waiting to go into apartment
struct cs1550_sem *ten_counter_lock;        //used as a lock to variable to keep track of num tenant processes that have gone
struct cs1550_sem *mutual_lock_reader;      //used to keep two tenant processes from altering lock at same time
struct cs1550_sem *print_lock;              //used for print statements
struct cs1550_sem *entrance;                //used so agents go to door in correct order
struct cs1550_sem *previous_burst_lock;     //used to lock previous_burst_size variable
struct cs1550_sem *burst_lock;              //used to lock current bursts size variable
struct cs1550_sem *view_counter;            //used to lock a variable to see if it is a tenants turn to view apartment
struct cs1550_sem *exit_counter;            //used to lock a variable to see which tenant should exit next
struct cs1550_sem *wait_leave_lock;         //used to lock boolean variable if any tenants waiting to leave still
struct cs1550_sem *tenant_exit_lock;        //used to put tenants to sleep until their turn to exit
struct cs1550_sem *last_burst_lock;         //used to lock previous bursts size
struct cs1550_sem *last_burst_set;          //used to see if tenants from last burst still waiting to go
struct cs1550_sem *waiting_set;             //used to see if there is an agent waiting at door when another leaves
struct cs1550_sem *create_agent_lock;       //used to lock a variable keeping track of the current number of agents
struct cs1550_sem *agent_entrance_lock;     //used to keep agents in order upon calling agent arrives
struct cs1550_sem *agent_mutual_reader;     //used for agents to see if mutual lock obtained between tenants/agents
struct cs1550_sem *total_agents;            //used to lock variable counting total number of agents
struct cs1550_sem *agent_waiting_set;       //used to set boolean variable that says if agents waiting at door
struct cs1550_sem *wait_show;               //used to put agents to sleep once they have arrived at door
struct cs1550_sem *show_counter;            //used to alter variable which checks if agents opening apartment in correct order
struct cs1550_sem *agent_here_lock;         //used to set a boolean variable for if agent at door
struct cs1550_sem *apartment_reader;        //used to see if door locked
struct cs1550_sem *max_tenant_lock;         //used to lock a variable to access the max num of tenants to create
struct cs1550_sem *tenants_done_lock;       //used to access boolean that says all tenants have finished
struct cs1550_sem *start_time_lock;         //used to lock access to a start time variable
struct cs1550_sem *tenant_time_lock;        //used to lock access to the current time
struct cs1550_sem *agent_lock;              //used to lock other agents from executing while another is doing something
struct cs1550_sem *max_agent_lock;          //used to set variable for max number of agents to create
struct cs1550_sem *agents_done_lock;        //used to alter boolean that says if agents have finished

int *tenant_num;                     //used to set a tenant processes number
int *current_viewing;               //keep track of how many tenants an agent has show apartment to
int *in_apt;                        //used to keep track of num tenants in apartment
int *apt_door;                      //used as boolean to if door is open
int *ten_counter;                   //used as a counter for number of tenants
int *mutual_lock_obtained;          //used as boolean to see if mutual lock between tenants and agents obtained
int *burst_size;                    //used to keep track of current bursts size for tenants
int *previous_burst_size;           //used to keep track of previous bursts size for tenants
int *viewing_count;                 //used to see if it is tenants turn to leave apartment
int *exit_count;                    //used to see if it is tenants turn to completely exit
int *wait_leave;                    //used to see if some tenant is still waiting to leave
int *remaining;                     //used to indicate number of tenants left from previous burst
int *last_burst_boolean;            //used to indicate there are tenants from last burst that still need to go
int *waiting_ten;                   //used to indicate there is a tenant waiting at door
int *current_agents;                //used to indicate current number of agents created
int *agent_mutual_obtained;         //used as boolean to tell other agents if lock obtained
int *agent_num;                     //used to keep track of a agents number
int *waiting_agent;                 //used to say how many agents are waiting to execute
int *showing_count;                 //used to keep track of num of total tenants that have been shown apartment
int *agent_boolean;                 //used as boolean as to if an agent is already at door
int *apartment_locked;              //used as boolean as to if apartment door locked
int *max_tenants;                   //used to hold max number of tenants to go
int *tenants_done;                  //used as boolean to see if tenants have all finished
time_t *start_time;                 //used to hold start time
time_t *tenant_time;                //used to hold current time
int *max_agents;                    //used to hold the maximum number of agents
int *agents_done;                   //used as boolean to mark agents as done


void down(struct cs1550_sem *sem) {         //method to call to lock semaphore
  syscall(__NR_cs1550_down, sem);
}

void up(struct cs1550_sem *sem) {           //method to call to release sempahore
  syscall(__NR_cs1550_up, sem);
}

void agentLeaves(int my_num)
{
    down(agent_lock);
    down(agent_mutual_reader);  //obtain lock so no tenant process can run at this time
    if(*agent_mutual_obtained)  //if lock already obtained
    {
       up(agent_mutual_reader);     //relase agent_mutual_reader
    }
   else                         //if lock not already obtained
   {
       down(lock);         //obtain lock, sleep if not avaiable
       *agent_mutual_obtained = 1;  //set that lock was obtained
       up(agent_mutual_reader);     //release agent_mutual_reader
   }
   down(tenants_done_lock);         //see if all tenants are finished(no more coming)
   if(*tenants_done)                //if no more tenants
   {
       up(tenants_done_lock);       //release tenants_done_lock
       down(print_lock);            //obtain print lock
       down(start_time_lock);       //obtain lock to access start time of program
       down(tenant_time_lock);      //obtain lock to access current time
       *tenant_time = time(NULL);   //set current time
       printf("Agent %d leaves the apartment at time %d.\n", my_num, (int)((*tenant_time) - (*start_time)));
       fflush(stdout);;
       printf("The apartment is now empty.\n"); //if no more tenants, apartment is empty
       fflush(stdout);;
       up(tenant_time_lock);       //release current time lock
       up(start_time_lock);        //release start time lock
       up(print_lock);             //release print lock
       down(agent_mutual_reader);   //obtain mutual reader lock
       if(*agent_mutual_obtained)   //if lock obtained
        {
            *agent_mutual_obtained = 0; //set that agents do not have lock
            up(lock);                   //release mutual lock between agents/tenants
        }
       up(agent_mutual_reader);         //release the agent mutual reader lock
       up(wait_show);                   //wake up a waiting agent process
       up(agent_lock);                  //release lock to agents
       return;
   }
    up(tenants_done_lock);          //release tenants_done_lock if tenants not done
    down(print_lock);               //obtain lock to print
    down(start_time_lock);          //obtain lock to start time
    down(tenant_time_lock);         //obtain lcok to current time
    *tenant_time = time(NULL);      //set current time
    printf("Agent %d leaves the apartment at time %d.\n", my_num, (int)((*tenant_time) - (*start_time)));
    fflush(stdout);;
    printf("The apartment is now empty.\n");
    fflush(stdout);;
    up(tenant_time_lock);       //release lock to current time
    up(start_time_lock);        //release lock to start time
    up(print_lock);             //release lock to print
    down(agent_waiting_set);    //obtain lock to see if agent waiting at door
    if(*waiting_agent)          //if there is a agent waiting at door, wake them up
    {
        up(wait_show);
    }
    else                            //if not an agent at door
    {
        down(apartment_reader);         //set that apartment is locked
        *apartment_locked = 0;          //set boolean to 0
        up(apartment_reader);           //release apartment_reader lock
        down(max_agent_lock);           //obtain max_agent_lock
        if(my_num == *max_agents)       //see if this is the last agent to execute
        {
            down(agents_done_lock);     //obtain agents_done_lock
            *agents_done = 1;           //set that no more agents coming
            up(agents_done_lock);       //release agents_done_lock
            down(waiting_set);          //obtain lock to see if a tenant is waiting
            if(*waiting_ten)            //if a tenant is waiting to go, wake them as no other process will
            {
                up(wait_view);          //wake a tenant up
            }
            up(waiting_set);            //release tenant waiting lock
        }
        up(max_agent_lock);             //release max_agent_lock
        down(agent_mutual_reader);      //release lock
        if(*agent_mutual_obtained);     //if mutual lock obtained
        {
            *agent_mutual_obtained = 0;     //release the lock
            up(lock);
        }
        up(agent_mutual_reader);       //releae lock to see if mutual lock obtained
    }
    up(agent_waiting_set);             //release lock to see if an agent is waiting
    up(agent_lock);
}

void openApt(my_num)
{
    down(print_lock);           //obtain print lock
    down(start_time_lock);      //obtain start time lock
    down(tenant_time_lock);     //obtain current time lock
    *tenant_time = time(NULL);
    printf("Agent %d opens the apartment for inspection at time %d.\n", my_num, (int)((*tenant_time) - (*start_time)));
    fflush(stdout);;
    up(tenant_time_lock);
    up(start_time_lock);
    up(print_lock);
    down(tenants_viewing);          //set that this agent can see 10 tenants
    *current_viewing = 10;
    up(tenants_viewing);
    down(apt_open_boolean);      //set that door is open for tenants
    *apt_door = 1;              //set to 1 when open
    up(apt_open_boolean);
    down(agent_waiting_set);    //decrement number of agents waiting at door
    *waiting_agent -= 1;
    up(agent_waiting_set);
    down(last_burst_set);       //see if the last burst of tenants finished
    if(*last_burst_boolean)
    {
        //up(last_burst_set);         //release lock
        down(last_burst_lock);      //obtain lock
        if(*remaining > 1)          //if at least one from last burst still to go
        {
            (*remaining)--;          //decrement num of tenants from last burst
            up(last_burst_lock);     //release lock
            up(wait_view);          //release lock

        }
        else                //last tenant from last burst
        {
            up(last_burst_lock);        //release lock
            down(last_burst_set);       //obtian lock
            *last_burst_boolean = 0;    //set boolean to false
            up(last_burst_set);          //release lock
            up(wait_view);
        }
    }
    up(last_burst_set);         //release lock seeing if tenants from last burst have finsihed
    down(agent_here_lock);      //mark that an agent is here
    if(!(*agent_boolean))
    {
        *agent_boolean = 1;
        up(agent_here);
    }
    up(agent_here_lock);
    down(agent_mutual_reader);      //release the mutual lock so tenants can go
    if(*agent_mutual_obtained)
    {
        *agent_mutual_obtained = 0;
        up(lock);
    }
    up(agent_mutual_reader);
    up(apt_open);                   //call up on apt_open so tenants can enter

    up(agent_lock);         //release lock so other agents can execute
    down(apt_occupied);     //will put agent to sleep until last tenant calls up
    agentLeaves(my_num);    //go to agent leaves
}

void agentArrives(int create_new_agent, int my_num)
{
    if(create_new_agent)            //if creating another agent immediately, sleep
    {
        down(agent_entrance_lock);
    }
    else                            //if last agent of burst
    {
        up(agent_entrance_lock);
        down(agent_entrance_lock);
    }
    down(total_agents);
    while(*agent_num != my_num)         //do not exit loop until correct agent woken up
    {
        up(total_agents);
        up(agent_entrance_lock);
        down(agent_entrance_lock);      //sleep if not correct agent to go
        down(total_agents);
    }
    up(total_agents);
    down(agent_lock);               //do not let any other agents execute right now
    down(agent_mutual_reader);      //see if mutual lock obtained
    if(*agent_mutual_obtained)
    {
        up(agent_mutual_reader);
    }
    else
    {
        down(lock);                 //obtain lock, sleep if not avaiable
        *agent_mutual_obtained = 1;
        up(agent_mutual_reader);
    }
    down(total_agents);             //increment number of agents
    *agent_num+=1;
    up(total_agents);
    down(apt_open_boolean);
    down(agent_waiting_set);
    if(*apt_door == 0 && !(*waiting_agent))     //if door not open and no agent waiting
    {
        *waiting_agent += 1;                    //mark that there is an agent waiting at the door
        up(agent_waiting_set);
        down(agent_here_lock);
        *agent_boolean = 1;
        up(agent_here_lock);
        up(agent_here);         //when agent wakes back up when leaving, just see if an agent waiting
    }                           //if so call up on wait_show?
    else                        //either door already open, or another agent already waiting, or both?
    {
        *waiting_agent += 1;        //will need to decrement when an agent no longer waiting
        up(agent_waiting_set);
    }
    up(apt_open_boolean);
    down(print_lock);               //obtain lock to print
    down(start_time_lock);          //obtain lock to start time
    down(tenant_time_lock);         //obtain lock to current time
    *tenant_time = time(NULL);
    printf("Agent %d arrives at time %d.\n", my_num, (int)((*tenant_time) - (*start_time)));
    fflush(stdout);;
    up(tenant_time_lock);           //release locks obtained
    up(start_time_lock);
    up(print_lock);
    up(agent_lock);                 //release agent lock
    if(create_new_agent)            //sleep so next agent can arrive of burst
    {
        up(agent_entrance_lock);
        down(wait_show);
    }
    else                           //if last agent of burst
    {
        down(apartment_reader);   // if an agent is in apartment, do not wake up an agent as
        if(!(*apartment_locked))  // they will when they leave
        {
            up(wait_show);      //last agent of burst, wake a agent up
        }
        else    //if not waking someone up, release the lock
        {
            down(agent_mutual_reader);
            if(*agent_mutual_obtained)      //release lock as going to let tenant go if none have arrived
            {
                *agent_mutual_obtained = 0;
                up(lock);
            }
            up(agent_mutual_reader);
        }
        up(apartment_reader);
        down(wait_show);                //go to sleep
    }
    down(show_counter);
    while(*showing_count != my_num)     //do not continue until correct agent is awoken
    {
        up(show_counter);
        up(wait_show);
        down(wait_show);                //sleep if you are not the correct agent
        down(show_counter);
    }
    up(show_counter);
    down(show_counter);
    *showing_count += 1;                //increment number of agent to go next
    up(show_counter);
    down(tenants_done_lock);
    if(*tenants_done)                   //if all tenants are done, go directly to leaving
    {
        up(tenants_done_lock);
        agentLeaves(my_num);
        return;
    }
    up(tenants_done_lock);
    down(apartment_reader);             //otherwise, set that agent is at apartment waiting
    *apartment_locked = 1;
    up(apartment_reader);
    down(agent_lock);                   //obtain agent lock
    down(agent_mutual_reader);
    if(*agent_mutual_obtained)          //release lock as going to let tenant go if none have arrived
    {
        *agent_mutual_obtained = 0;
        up(lock);
    }
    up(agent_mutual_reader);
    up(agent_lock);             //release agent lock in case agent sleeps for good
    down(tenant_waiting);       //check to see if there is a tenant at door, if not sleeping
    down(agent_lock);           //obtain agent lock
    down(agent_mutual_reader);  //on return, hold lock until door opened
    if(*agent_mutual_obtained)
    {
        up(agent_mutual_reader);
    }
    else
    {
        down(lock);              //obtain lock, sleep if not avaiable
        *agent_mutual_obtained = 1;
        up(agent_mutual_reader);
    }
    down(waiting_set);          //set that tenants not currently waiting as you are opening door
    {
        *waiting_ten = 0;
    }
    up(waiting_set);

    openApt(my_num);
}

void tenantLeaves(int my_num)
{
    down(mutual_lock_reader);       //see if mutual lock obtained for tenants
    if(*mutual_lock_obtained)       //if set to 1
    {
        up(mutual_lock_reader);     //if lock already obtained, do nothing
    }
    else                            //lock not obtained for tenants
    {
        down(lock);                 //obtain lock, sleep if not available
        *mutual_lock_obtained = 1;
        up(mutual_lock_reader);     //release lock
    }
    down(agents_done_lock);
    if(*agents_done)                //if all agents are done, leave
    {
        up(agents_done_lock);
        down(print_lock);           //print tenant leaving
        down(start_time_lock);
        down(tenant_time_lock);
        *tenant_time = time(NULL);
        printf("Tenant %d leaves the apartment at time %d.\n", my_num, (int)((*tenant_time) - (*start_time)));
        fflush(stdout);;
        up(tenant_time_lock);
        up(start_time_lock);
        up(print_lock);             //release print lock
        down(mutual_lock_reader);   //obtain lock for it
        if(*mutual_lock_obtained)   //if tenants have mutual lock between them and agents
        {
            *mutual_lock_obtained = 0;      //set that tenants no longer have lock
            up(lock);                       //release lock at this point as may need agent to open door
        }
        up(mutual_lock_reader);     //release lock
        up(wait_view);              //wake up the next tenant that has not gone yet
        return;
    }
    up(agents_done_lock);         //otherwise, exit as if tenants still to come
    down(tenants_viewing);       //see how many tenants have viewed apartment with agent
    down(num_in_apt);            //decrement num tenants in apartment
    down(burst_lock);
    *in_apt -= 1;                   //decrement number in apartment
    down(max_tenant_lock);
    if(my_num == *max_tenants)  //if last tenant to go, set that no more tenants coming
    {
        down(tenants_done_lock);
        *tenants_done = 1;
        up(tenants_done_lock);

    }
    up(max_tenant_lock);
    if(*current_viewing == 0 && *in_apt == 0)    //if 10 tenants have viewed apartment and no tenants at door
    {
        up(burst_lock);
        up(tenants_viewing);        //release lock
        up(num_in_apt);             //release lock
        down(apt_open);             //if no more tenants but not at max yet, lock apartment
        down(apt_open_boolean);     //change apartment door to not open
        *apt_door = 0;              //set to 0 when closed
        up(apt_open_boolean);       //let go of lock to alter apt_door
        up(apt_occupied);           //let agent wake up
        down(print_lock);           //print tenant leaving
        down(start_time_lock);
        down(tenant_time_lock);
        *tenant_time = time(NULL);
        printf("Tenant %d leaves the apartment at time %d.\n", my_num, (int)((*tenant_time) - (*start_time)));
        fflush(stdout);;
        up(tenant_time_lock);
        up(start_time_lock);
        up(print_lock);             //release print lock
        down(exit_counter);         //obtain exit counter
        *exit_count += 1;           //increment which tenant should exit next
        up(exit_counter);           //release lock
        down(mutual_lock_reader);   //obtain lock for it
        if(*mutual_lock_obtained)   //if tenants have mutual lock between them and agents
        {
            *mutual_lock_obtained = 0;      //set that tenants no longer have lock
            up(lock);                       //release lock at this point as may need agent to open door
        }
        up(mutual_lock_reader);     //release lock
    }
    else if(*in_apt == 0 && *burst_size == 0)               //if number in apartment is 0 and no more tenants at door
    {
        up(burst_lock);
        up(tenants_viewing);        //release lock
        up(num_in_apt);             //release lock
        down(apt_open_boolean);     //change apartment door to not open
        *apt_door = 0;              //set to 0 when closed
        up(apt_open_boolean);       //let go of lock to alter apt_door
        down(apt_open);
        up(apt_occupied);           //let agent wake up
        down(print_lock);           //print tenant leaving
        down(start_time_lock);
        down(tenant_time_lock);
        *tenant_time = time(NULL);
        printf("Tenant %d leaves the apartment at time %d.\n", my_num, (int)((*tenant_time) - (*start_time)));
        fflush(stdout);;
        up(tenant_time_lock);
        up(start_time_lock);
        up(print_lock);             //release print lock
        down(exit_counter);         //obtain exit counter
        *exit_count += 1;           //increment which tenant should exit next
        up(exit_counter);           //release lock
        down(mutual_lock_reader);   //obtain lock for it
        if(*mutual_lock_obtained)   //if tenants have mutual lock between them and agents
        {
            *mutual_lock_obtained = 0;      //set that tenants no longer have lock
            up(lock);                       //release lock at this point as may need agent to open door
        }
        up(mutual_lock_reader);     //release lock
    }
    else        //apartment not empty
    {
        up(burst_lock);
        up(tenants_viewing);        //release lock from if
        up(num_in_apt);             //release lock from if
        down(print_lock);           //obtain lock to print
        down(start_time_lock);
        down(tenant_time_lock);
        *tenant_time = time(NULL);
        printf("Tenant %d leaves the apartment at time %d.\n", my_num, (int)((*tenant_time) - (*start_time)));
        fflush(stdout);;
        up(tenant_time_lock);
        up(start_time_lock);
        up(print_lock);             //release print lock
        down(exit_counter);         //obtain lock
        *exit_count += 1;           //update whom next tenant to exit will be
        up(exit_counter);           //release lock
        down(wait_leave_lock);      //see if any tenants waiting to leave
        if(*wait_leave)             //if in queue waiting to leave
        {
            up(tenant_exit_lock);   //wake up a tenant waiting to see if they can go
        }
        up(wait_leave_lock);        //release lock
    }
}

void viewApt(int my_num)
{
    down(tenants_viewing);          //used access the number of tenants a agent has let in
    if(*current_viewing > 1)        //if agent still accepting tenants
    {
        (*current_viewing)--;     //subtract 1 from number of tenants allowed to still enter
        up(tenants_viewing);       //release lock to current_vieiwing
    }
    else                           //agent no longer letting tenants in
    {
        (*current_viewing)--;
        up(tenants_viewing);                 //releae lock to current viewing
        down(last_burst_lock);              //obtain lock
        down(previous_burst_lock);          //obtain lock
        *remaining = *previous_burst_size - 1;  //set remaining so they know how many times to call up on wait
        up(previous_burst_lock);            //release lock
        up(last_burst_lock);                //release lock

        down(last_burst_set);               //obtain lock
        *last_burst_boolean = 1;            //set that tenats from last burst still here
        up(last_burst_set);                 //release lock
        down(apt_open_boolean);             //get lock to see if door open
        *apt_door = 0;
        down(waiting_set);                  //set that there is a tenant waiting at door
        if(!(*waiting_ten))
        {
            *waiting_ten = 1;
            up(tenant_waiting);             //set this semaphore to tenant here
        }
        up(waiting_set);
        up(tenant_waiting);                 //set this semaphore to tenant here
        up(apt_open_boolean);
        down(agent_here);
        down(agent_here_lock);
        *agent_boolean = 0;
        up(agent_here_lock);
    }
    down(tenants_viewing);          //obtain lock to current_viewing
    if(*current_viewing == 0)       //if no more tenants can view with agent
    {
        //this is the last tenant allowed in
        up(tenants_viewing);        //release lock
        down(num_in_apt);           //used to keep track of current tenants in apt
        *in_apt +=1;                //add 1 to the number of tenants in the apartment
        up(num_in_apt);             //release lock on num of tenants in apartment
        down(print_lock);           //print out tenant viewing apartment
        down(start_time_lock);
        down(tenant_time_lock);
        *tenant_time = time(NULL);
        printf("Tenant %d inspects the apartment at time %d.\n", my_num, (int)((*tenant_time) - (*start_time)));
        fflush(stdout);;
        up(tenant_time_lock);
        up(start_time_lock);
        up(print_lock);
    }
    else                            //not last tenant in
    {

        up(tenants_viewing);        //release the lock to current_viewing
        down(apt_open);             //if not open, sleep until open, not sure why needed?
        up(apt_open);               //wake any waiting processes
        down(num_in_apt);           //used to keep track of current tenants in apt
        *in_apt +=1;                //add 1 to num of tenants in apartment
        up(num_in_apt);             //release lock on num tenants in apartment
        down(print_lock);           //print out tenant viewing apartment
        down(start_time_lock);
        down(tenant_time_lock);
        *tenant_time = time(NULL);
        printf("Tenant %d inspects the apartment at time %d.\n", my_num, (int)((*tenant_time) - (*start_time)));
        fflush(stdout);;
        up(tenant_time_lock);
        up(start_time_lock);
        up(print_lock);
    }
    down(tenants_viewing);
    if(*current_viewing == 0)
    {
        down(previous_burst_lock);      //obtain lock
        *previous_burst_size -= 1;       //decrement previous locks burst
        up(previous_burst_lock);        //release lock
        up(tenants_viewing);
        //skip doing this if last tenant in
    }
    else
    {
        up(tenants_viewing);
        down(last_burst_set);
        if(*last_burst_boolean)     //if from last burst
        {
            up(last_burst_set);     //release lock
            down(last_burst_lock);  //obtain lock
            if(*remaining > 0)      //if at least one from last burst still to go
            {
                *remaining -=1;          //decrement num of tenants from last burst
                up(last_burst_lock);     //release lock
                up(wait_view);          //release lock

            }
            else                //last tenant from last burst
            {
                up(last_burst_lock);        //release lock
                down(last_burst_set);       //obtian lock
                *last_burst_boolean = 0;    //set boolean to false
                up(last_burst_set);          //release lock
            }
        }
        else            //not altering last burst
        {
            up(last_burst_set);             //release lock
            down(previous_burst_lock);      //obtain lock
            *previous_burst_size -=1;       //decrement previous locks burst
            if(*previous_burst_size > 0)     //if all tenants from burst have not yet gone, wake next up
            {
                up(wait_view);              //wake up next tenant waiting to view apartment
            }
            up(previous_burst_lock);        //release lock
        }
    }
    sleep(2);                       //once in apartment, sleep for 2 seconds
    down(exit_counter);             //obtain lock
    if(*exit_count < my_num)       //if current tenant process leaving in order
    {
        while(*exit_count < my_num)    //put process to sleep until it is it's turn
        {
            down(wait_leave_lock);      //obtain lock
            *wait_leave += 1;           //set that some tenant is waiting in list
            up(wait_leave_lock);        //release lock
            up(exit_counter);
            down(tenant_exit_lock);     //go to sleep
            down(wait_leave_lock);      //upon waking up, decrement num tenants waiting
            *wait_leave -=1;
            up(wait_leave_lock);        //release lock
            down(exit_counter);
            if(*exit_count != my_num)
            {
                up(tenant_exit_lock);
            }
            up(exit_counter);
            down(exit_counter);         //obtain lock for while loop
        }
    }
    up(exit_counter);                   //release lock for if
    tenantLeaves(my_num);                //then go to leaving the apartment
}

void tenantArrives(int create_new_ten,int my_num)
{
    if(create_new_ten)                  //this is used so tenants do not continue until
    {                                   //all from burst are at this point
        down(entrance);
    }
    else
    {
        up(entrance);
        down(entrance);
    }
    down(total_ten);
    while(my_num != *tenant_num)            //wake up another tenant until correct one going
    {
        up(total_ten);
        up(entrance);
        down(entrance);                     //sleep if not your turn
        down(total_ten);
    }
    up(total_ten);
    struct tm * timeinfo;
    down(mutual_lock_reader);       //see if mutual lock obtained for tenants
    if(*mutual_lock_obtained)       //if set to 1
    {
        up(mutual_lock_reader);     //if lock already obtained, do nothing
    }
    else                            //lock not obtained for tenants
    {
        down(lock);                 //obtain lock, sleep if not available
        *mutual_lock_obtained = 1;
        up(mutual_lock_reader);     //release lock
    }
    down(total_ten);            //used to give access to total tenants
    *tenant_num += 1;            //increment tenant processes
    up(total_ten);              //release lock to access tenant num
    down(apt_open_boolean);     //get lock to see if door open
    down(waiting_set);          //get lock
    if(*apt_door == 0 && !(*waiting_ten))          //check to see if door currently closed
    {
        *waiting_ten = 1;
        up(tenant_waiting);     //set this semaphore to tenant here
        up(waiting_set);
    }
    else
    {
        up(waiting_set);
    }
    up(apt_open_boolean);
    down(print_lock);
    down(start_time_lock);
    down(tenant_time_lock);
    *tenant_time = time(NULL);
    printf("Tenant %d arrives at time %d.\n", my_num, (int)((*tenant_time) - (*start_time)));
    fflush(stdout);;
    up(tenant_time_lock);
    up(start_time_lock);
    up(print_lock);
    if(create_new_ten)  //sleep to allow next tenant to go
    {
        down(burst_lock);
        *burst_size +=1;
        up(burst_lock);
        up(entrance);
        down(wait_view);
    }
    else                    //no tenant next, so see if any waiting to move on
    {                       //this is the last tenant to arrive in current burst
        down(burst_lock);   //need this down here or messed up
        *burst_size += 1;
        down(previous_burst_lock);
        *previous_burst_size = *burst_size;
        up(previous_burst_lock);
        up(burst_lock);
        down(burst_lock);
        *burst_size = 0;
        up(burst_lock);
        up(wait_view);   //if no other tenant ready, will not do anything
        down(wait_view); //if tenant process ready to run, will run to here
    }
    down(view_counter);
    while(*viewing_count != my_num)     //sleep until your turn to view apartment
    {
        up(view_counter);
        up(wait_view);
        down(wait_view);
        down(view_counter);
    }
    *viewing_count += 1;
    up(view_counter);
    down(mutual_lock_reader);
    if(*mutual_lock_obtained)
    {
        *mutual_lock_obtained = 0;      //set that tenants no longer have lock
        up(lock);                //release lock at this point as may need agent to open door
    }
    up(mutual_lock_reader);
    down(agents_done_lock);         //if all agents are done, go directly to leaving
    if(*agents_done)
    {
        up(agents_done_lock);
        tenantLeaves(my_num);
        return; //exit
    }
    up(agents_done_lock);
    down(agent_here);        //if no agent, sleep
    down(apt_open);
    up(apt_open);
    down(mutual_lock_reader);
    if(*mutual_lock_obtained)       //if set to 1
    {
        up(mutual_lock_reader);     //if lock already obtained, do nothing
    }
    else                            //lock not obtained for tenants
    {
        down(lock);                 //obtain lock, sleep if not available
        *mutual_lock_obtained = 1;
        up(mutual_lock_reader);     //release lock
    }
    down(mutual_lock_reader);
    if(*mutual_lock_obtained)
    {
        *mutual_lock_obtained = 0;      //set that tenants no longer have lock
        up(lock);                //release lock at this point as may need agent to open door
    }
    up(mutual_lock_reader);
    up(agent_here);          //wake up next waiting process
    viewApt(my_num);         //go to view apartment if agent arrived
}

int createNew(int prob)             //method used to see if a new tenant should be created immediately
{
    srand(time(NULL));
    int r = rand() % 100;
    if(r <= prob)
    {
        return 1;
    }
    else
    {
        return 0;
    }

}

int main(int argc, char *argv[])
{
    agent_here = mmap(NULL,sizeof(struct cs1550_sem),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    tenant_waiting = mmap(NULL,sizeof(struct cs1550_sem),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    apt_open = mmap(NULL,sizeof(struct cs1550_sem),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    apt_occupied = mmap(NULL,sizeof(struct cs1550_sem),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    total_ten = mmap(NULL,sizeof(struct cs1550_sem),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    tenants_viewing = mmap(NULL,sizeof(struct cs1550_sem),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    num_in_apt = mmap(NULL,sizeof(struct cs1550_sem),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    lock = mmap(NULL,sizeof(struct cs1550_sem),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    apt_open_boolean = mmap(NULL,sizeof(struct cs1550_sem),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    wait_view = mmap(NULL,sizeof(struct cs1550_sem),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    ten_counter_lock = mmap(NULL,sizeof(struct cs1550_sem),
                    PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    mutual_lock_reader = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    print_lock = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    create_agent_lock = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    entrance= mmap(NULL,sizeof(struct cs1550_sem),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    previous_burst_lock = mmap(NULL,sizeof(struct cs1550_sem),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    burst_lock = mmap(NULL,sizeof(struct cs1550_sem),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    view_counter = mmap(NULL,sizeof(struct cs1550_sem),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    exit_counter = mmap(NULL,sizeof(struct cs1550_sem),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    wait_leave_lock = mmap(NULL,sizeof(struct cs1550_sem),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    tenant_exit_lock = mmap(NULL,sizeof(struct cs1550_sem),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    last_burst_lock = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    last_burst_set = mmap(NULL,sizeof(struct cs1550_sem),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    waiting_set = mmap(NULL,sizeof(struct cs1550_sem),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agent_entrance_lock = mmap(NULL,sizeof(struct cs1550_sem),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agent_mutual_reader = mmap(NULL,sizeof(struct cs1550_sem),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    total_agents =  mmap(NULL,sizeof(struct cs1550_sem),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agent_waiting_set = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    wait_show = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    show_counter = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agent_here_lock = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    apartment_reader = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    max_tenant_lock = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    tenants_done_lock = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    start_time_lock = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    tenant_time_lock = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agent_lock = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    max_agent_lock = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agents_done_lock = mmap(NULL,sizeof(struct cs1550_sem),
                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);


    tenant_num = mmap(NULL,sizeof(int),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    current_viewing = mmap(NULL,sizeof(int),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    in_apt = mmap(NULL,sizeof(int),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    apt_door = mmap(NULL,sizeof(int),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    ten_counter = mmap(NULL,sizeof(int),
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    mutual_lock_obtained = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    burst_size = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    previous_burst_size = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    viewing_count = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    exit_count = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    wait_leave = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    remaining = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    last_burst_boolean = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    waiting_ten = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    current_agents = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agent_mutual_obtained = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agent_num = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    waiting_agent = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    showing_count = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agent_boolean = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    apartment_locked = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    max_tenants = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    tenants_done = mmap(NULL,sizeof(int),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    start_time = mmap(NULL,sizeof(time_t),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    tenant_time = mmap(NULL,sizeof(time_t),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    max_agents = mmap(NULL,sizeof(time_t),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agents_done = mmap(NULL,sizeof(time_t),
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);

    //initialize semaphores
    agent_here->value = 0;
    tenant_waiting->value = 0;
    apt_open->value = 0;
    apt_occupied->value = 0;
    total_ten->value = 1;
    tenants_viewing->value=1;
    num_in_apt->value = 1;
    lock->value = 1;
    apt_open_boolean->value = 1;
    wait_view->value = 0;
    ten_counter_lock->value = 1;
    mutual_lock_reader -> value = 1;
    print_lock->value = 1;
    entrance->value = 0;
    previous_burst_lock->value = 1;
    burst_lock->value = 1;
    view_counter->value = 1;
    tenant_exit_lock->value = 0;
    exit_counter->value = 1;
    wait_leave_lock->value = 1;
    last_burst_lock->value = 1;
    last_burst_set->value = 1;
    waiting_set->value = 1;
    create_agent_lock->value = 1;
    agent_entrance_lock->value = 0;
    agent_mutual_reader->value = 1;
    total_agents->value = 1;
    agent_waiting_set->value = 1;
    wait_show->value = 0;
    show_counter->value = 1;
    agent_here_lock->value = 1;
    apartment_reader->value = 1;
    max_tenant_lock->value = 1;
    tenants_done_lock->value = 1;
    start_time_lock->value = 1;
    tenant_time_lock->value = 1;
    agent_lock->value = 1;
    max_agent_lock->value = 1;
    agents_done_lock->value = 1;

    //initialize shared variables
    *tenant_num = 1;
    *current_viewing = 10;
    *in_apt = 0;
    *apt_door = 0;
    *ten_counter = 0;
    *mutual_lock_obtained = 0;
    *previous_burst_size = 0;
    *burst_size = 0;
    *viewing_count = 1;
    *wait_leave = 0;
    *exit_count = 1;
    *remaining = 0;
    *last_burst_boolean = 0;
    *waiting_ten = 0;
    *current_agents = 0;
    *agent_mutual_obtained = 0;
    *agent_num = 1;
    *waiting_agent = 0;
    *showing_count = 1;
    *agent_boolean = 0;
    *apartment_locked = 0;
    *max_tenants = 0;
    *tenants_done = 0;
    *start_time = 0;
    *tenant_time = 0;
    *max_agents = 0;
    *agents_done = 0;


    if(argc != 13)
    {
        printf("Incorrect number of arguments given to command line\n");
        fflush(stdout);;
        exit(1);
    }

    int tenants;
    int agents;
    int prob_ten;
    int delay_ten;
    int prob_agent;
    int delay_agent;
    int i = 1;
    while(i <= 12)
    {
        if(!strcmp(argv[i],"-m"))
        {
            i++;
            tenants = atoi(argv[i]);
            i++;
            if(tenants < 0)
            {
                printf("Tenants cannot be less than or equal to 0.\n");
                fflush(stdout);
                exit(1);
            }
        }
        else if(!strcmp(argv[i],"-k"))
        {
            i++;
            agents = atoi(argv[i]);
            i++;
            if(agents < 0)
            {
                printf("Agents cannot be less than or equal to 0.\n");
                fflush(stdout);
                exit(1);
            }
        }
        else if(!strcmp(argv[i],"-pt"))
        {
            i++;
            prob_ten = atoi(argv[i]);
            i++;
            if(prob_ten <= 0)
            {
                printf("Cannot have the probability of the tenant be less than or equal to 0.\n");
                fflush(stdout);
                exit(1);
            }
        }
        else if(!strcmp(argv[i],"-dt"))
        {
            i++;
            delay_ten = atoi(argv[i]);
            i++;
            if(delay_ten < 0)
            {
                printf("Cannot have the tenant delay less than 0.\n");
                fflush(stdout);
                exit(1);
            }
        }
        else if (!strcmp(argv[i],"-pa"))
        {
            i++;
            prob_agent = atoi(argv[i]);
            i++;
            if(prob_agent <= 0)
            {
                printf("Cannot have the probability of the agent be less than or equal to 0.\n");
                fflush(stdout);
                exit(1);
            }
        }
        else if (!strcmp(argv[i],"-da"))
        {
            i++;
            delay_agent = atoi(argv[i]);
            i++;
            if(delay_agent < 0)
            {
                printf("Cannot have the agent delay be less than 0.\n");
                fflush(stdout);
                exit(1);
            }
        }
        else
        {
            printf("Invalid argument(s) passed\n");
            fflush(stdout);;
            exit(1);
        }
    }
    down(max_tenant_lock);
    *max_tenants = tenants;         //set maximum number of tenants
    up(max_tenant_lock);
    down(max_agent_lock);
    *max_agents = agents;           //set maximum number of agents
    up(max_agent_lock);

    down(start_time_lock);
    *start_time = time(NULL);
    up(start_time_lock);
    down(print_lock);
    printf("The apartment is now empty.\n");
    fflush(stdout);
    up(print_lock);
    int pid = fork(); // Create the tenant arrival process
    if (pid==0)
    { // I am the tenant arrival process
        int my_num = 0;
        down(ten_counter_lock);
        *ten_counter = 0;       // total tenants created
        while(*ten_counter < tenants)
        {
            int create_new_ten = 0;
            *ten_counter += 1;
            my_num = *ten_counter;
            if(*ten_counter < tenants)
            {
                create_new_ten = createNew(prob_ten);
            }
            up(ten_counter_lock);
            pid = fork();
            if(pid == 0)
            {
                tenantArrives(create_new_ten, my_num);
                exit(0);
            }
            else
            {   //parent
                down(ten_counter_lock);
                if(!create_new_ten && (*ten_counter == tenants))
                {

                    up(ten_counter_lock);
                    int counter1 = 1;
                    while(counter1 <= tenants)
                    {
                        counter1++;
                        wait(NULL);
                    }
                }
                else if(!create_new_ten)
                {
                    up(ten_counter_lock);
                    sleep(delay_ten);//will have to specify exact time to sleep
                }
                else
                {
                    up(ten_counter_lock);
                }
            }
            down(ten_counter_lock);
        }
        up(ten_counter_lock);
        exit(0);
    } else { // I am the parent
      pid = fork(); // Create a agent arrival process
      if(pid == 0)
      { // I am a agent arrival processes
        int my_num = 0;
        down(create_agent_lock);
        *current_agents = 0;
        while(*current_agents < agents)
        {
            int create_new_agent = 0;
            my_num = 0;
            *current_agents += 1;
            my_num = *current_agents;
            if(*current_agents < agents)    //if still more agents to create
            {
                create_new_agent = createNew(prob_agent);
            }
            up(create_agent_lock);
            pid = fork();
            if(pid == 0)        //tenant process
            {
                agentArrives(create_new_agent, my_num);
                exit(0);        //kill child upon return
            }
            else            //parent process for agent creation
            {
                down(create_agent_lock);        //acquire lock
                if(!create_new_agent && (*current_agents == agents))
                {
                    up(create_agent_lock);      //release lock
                    int counter2 = 1;
                    while(counter2 <= agents)
                    {
                        counter2++;
                        wait(NULL);
                    }
                }//end if
                else if(!create_new_agent)      //if not creating new agent immediately
                {
                    up(create_agent_lock);      //release lock
                    sleep(delay_agent);                 //need to set this to correct argument
                }
                else            //create new agent immediately
                {
                    up(create_agent_lock);      //just release lock
                }
            }//end of else
            down(create_agent_lock);    //obtain lock
        }//end while loop
      up(create_agent_lock);
      exit(0);
      }//end of agent arrival process
      wait(NULL);
    }//end of else
    wait(NULL);//wait until Tenant creation process terminates
    return 0;

}//end of main
