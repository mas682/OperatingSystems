/* A program that simulates agents showing tenants a
 * apartment.
 * accepts the following command line args: num tenants
 * num agents
 * probability of a tenant immediately following another tenants
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

struct cs1550_sem *agent_apt;
struct cs1550_sem *agent_here;
struct cs1550_sem *tenant_waiting;
struct cs1550_sem *apt_open;
struct cs1550_sem *apt_occupied;
struct cs1550_sem *total_ten;
struct cs1550_sem *tenants_viewing;
struct cs1550_sem *num_in_apt;
struct cs1550_sem *lock;        //used
struct cs1550_sem *apt_open_boolean;
struct cs1550_sem *wait_view;
struct cs1550_sem *ten_counter_lock;
struct cs1550_sem *mutual_lock_reader;
struct cs1550_sem *print_lock;
struct cs1550_sem *entrance;
struct cs1550_sem *previous_burst_lock;
struct cs1550_sem *burst_lock;
struct cs1550_sem *view_counter;
struct cs1550_sem *exit_counter;
struct cs1550_sem *wait_leave_lock;
struct cs1550_sem *tenant_exit_lock;
struct cs1550_sem *last_burst_lock;
struct cs1550_sem *last_burst_set;
struct cs1550_sem *waiting_set;
struct cs1550_sem *create_agent_lock;
struct cs1550_sem *agent_burst_lock;        //may not be needed
struct cs1550_sem *agent_entrance_lock;
struct cs1550_sem *agent_mutual_reader;
struct cs1550_sem *total_agents;
struct cs1550_sem *agent_waiting_set;
struct cs1550_sem *wait_show;
struct cs1550_sem *show_counter;
struct cs1550_sem *agent_here_lock;
struct cs1550_sem *apartment_reader;
struct cs1550_sem *max_tenant_lock;
struct cs1550_sem *tenants_done_lock;
struct cs1550_sem *start_time_lock;
struct cs1550_sem *tenant_time_lock;
struct cs1550_sem *agent_lock;
struct cs1550_sem *max_agent_lock;
struct cs1550_sem *agents_done_lock;

int *tenant_num;
int *current_viewing;
int *in_apt;
int *apt_door;
int *ten_counter;
int *mutual_lock_obtained;
int *burst_size;
int *previous_burst_size;
int *viewing_count;
int *exit_count;
int *wait_leave;
int *remaining;
int *last_burst_boolean;
int *waiting_ten;
int *current_agents;
int *agent_burst_size;      //may not be needed
int *agent_mutual_obtained;
int *agent_num;
int *waiting_agent;
int *showing_count;
int *agent_boolean;
int *apartment_locked;
int *max_tenants;
int *tenants_done;
time_t *start_time;
time_t *tenant_time;
int *max_agents;
int *agents_done;


void down(struct cs1550_sem *sem) {
  syscall(__NR_cs1550_down, sem);
}

void up(struct cs1550_sem *sem) {
  syscall(__NR_cs1550_up, sem);
}

void printOutputs()
{
    down(print_lock);
    printf("1. %d\n,", agent_apt->value);
    printf("2. %d\n,", agent_here->value);
    printf("3. %d\n,", tenant_waiting->value);
    printf("4. %d\n,", apt_open->value);
    printf("5. %d\n,", apt_occupied->value);
    printf("6. %d\n,", total_ten->value);
    printf("7. %d\n,", tenants_viewing->value);
    printf("8. %d\n,", num_in_apt->value);
    printf("9. %d\n,", lock->value);        //used
    printf("10. %d\n,", apt_open_boolean->value);
    printf("11. %d\n,", wait_view->value);
    printf("12. %d\n,", ten_counter_lock->value);
    printf("13. %d\n,",  mutual_lock_reader->value);
    printf("14. %d\n,", print_lock->value);
    printf("15. %d\n,", entrance->value);
    printf("16. %d\n,",  previous_burst_lock->value);
    printf("17. %d\n,", burst_lock->value);
    printf("18. %d\n,", view_counter->value);
    printf("19. %d\n,", exit_counter->value);
    printf("20. %d\n,", wait_leave_lock->value);
    printf("21. %d\n,", tenant_exit_lock->value);
    printf("22. %d\n,", last_burst_lock->value);
    printf("23. %d\n,", last_burst_set->value);
    printf("24. %d\n,", waiting_set->value);
    printf("25. %d\n,", create_agent_lock->value);
    printf("26. %d\n,", agent_burst_lock->value);        //may not be needed
    printf("27. %d\n,", agent_entrance_lock->value);
    printf("28. %d\n,", agent_mutual_reader->value);
    printf("29. %d\n,", total_agents->value);
    printf("30. %d\n,", agent_waiting_set->value);
    printf("31. %d\n,", wait_show->value);
    printf("32. %d\n,", show_counter->value);
    printf("33. %d\n,", agent_here_lock->value);
    printf("34. %d\n,", apartment_reader->value);
    printf("35. %d\n,", max_tenant_lock->value);
    printf("36. %d\n,", tenants_done_lock->value);
    printf("37. %d\n", max_agent_lock->value);
    printf("38. %d\n", agents_done_lock->value);

    up(print_lock);


}

void agentLeaves(int my_num)
{

    down(agent_mutual_reader);  //obtain lock so no tenant can run
    if(*agent_mutual_obtained)
    {
       up(agent_mutual_reader);
    }
   else
   {
       down(lock);         //obtain lock, sleep if not avaiable
       *agent_mutual_obtained = 1;
       up(agent_mutual_reader);
   }
    //new
   down(tenants_done_lock);
   if(*tenants_done)
   {
       up(tenants_done_lock);
       down(print_lock);
       down(start_time_lock);
       down(tenant_time_lock);
       *tenant_time = time(NULL);
       printf("Agent %d leaves the apartment at time %d.\n", my_num, ((*tenant_time) - (*start_time)));
       printf("The apartment is now empty.\n");
       up(tenant_time_lock);
       up(start_time_lock);
       up(print_lock);
       down(agent_mutual_reader);
       if(*agent_mutual_obtained)
        {
            *agent_mutual_obtained = 0;
            up(agent_mutual_reader);
            up(lock);
        }
       up(agent_mutual_reader);
       up(wait_show);
       up(agent_lock);
       return;
   }
   up(tenants_done_lock);
   ////////////////////////new
    down(print_lock);
    down(start_time_lock);
    down(tenant_time_lock);
    *tenant_time = time(NULL);
    printf("Agent %d leaves the apartment at time %d.\n", my_num, ((*tenant_time) - (*start_time)));
    printf("The apartment is now empty.\n");
    up(tenant_time_lock);
    up(start_time_lock);
    up(print_lock);
    down(agent_waiting_set);
    if(*waiting_agent)                  //if there is a agent waiting at door, wake them up
    {
        up(wait_show);
    }
    else                            //if not an agent
    {
        down(apartment_reader);
        *apartment_locked = 0;
        up(apartment_reader);
        down(max_agent_lock);           //obtain max_agent_lock
        if(my_num == *max_agents)       //see if this is the last agent
        {
            down(agents_done_lock);     //obtain agents_done_lock
            *agents_done = 1;           //set that no more agents coming
            up(agents_done_lock);       //release agents_done_lock
            down(print_lock);
            //printf("No more agents coming.\n");
            up(print_lock);
            down(waiting_set);
            if(*waiting_ten)    //if a tenant is waiting to go
            {
                up(wait_view);  //wake a tenant up
            }
            up(waiting_set);
        }
        up(max_agent_lock);             //release max_agent_lock
        down(agent_mutual_reader);      //release lock
        if(*agent_mutual_obtained);
        {
            *agent_mutual_obtained = 0;
            up(lock);
        }
        up(agent_mutual_reader);
    }
    up(agent_waiting_set);
    down(tenants_done_lock);
    if(*tenants_done)
    {
        if(*agent_mutual_obtained);
        {
            *agent_mutual_obtained = 0;
            up(lock);
        }
        up(apt_occupied);
        up(agent_mutual_reader);
        up(tenant_waiting);
    }
    up(tenants_done_lock);
    up(agent_lock);
}

void openApt(my_num)
{
    down(print_lock);
    down(start_time_lock);
    down(tenant_time_lock);
    *tenant_time = time(NULL);
    printf("Agent %d opens the apartment for inspection at time %d.\n", my_num, ((*tenant_time) - (*start_time)));
    up(tenant_time_lock);
    up(start_time_lock);
    up(print_lock);
    down(tenants_viewing);
    *current_viewing = 10;
    up(tenants_viewing);
    down(apt_open_boolean);      //set value
    *apt_door = 1;      //set to 1 when open
    up(apt_open_boolean);
    down(agent_waiting_set);
    *waiting_agent -= 1;
    up(agent_waiting_set);
    down(last_burst_set);
    if(*last_burst_boolean)
    {
        up(last_burst_set);     //release lock
        down(last_burst_lock);  //obtain lock
        if(*remaining > 1)      //if at least one from last burst still to go
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
    up(last_burst_set);
    down(agent_here_lock);
    if(!(*agent_boolean))
    {
        *agent_boolean = 1;
        up(agent_here);
    }
    up(agent_here_lock);
    down(agent_mutual_reader);
    if(*agent_mutual_obtained)
    {
        *agent_mutual_obtained = 0;
        up(lock);
    }
    up(agent_mutual_reader);
    up(apt_open);
    down(tenants_done_lock);
    if(*tenants_done)
    {
        down(apt_open_boolean);
        *apt_door = 0;
        up(apt_open_boolean);
        up(tenants_done_lock);
        up(tenants_done_lock);
        agentLeaves(my_num);
    }
    else
    {
       up(tenants_done_lock);
        up(agent_lock);
       down(apt_occupied);     //will put agent to sleep until last tenant calls up
       agentLeaves(my_num);
       down(agent_lock);
    }

}

void agentArrives(int create_new_agent, int my_num)
{
    if(create_new_agent)            //if create another, sleep
    {
        down(agent_entrance_lock);
    }
    else
    {
        up(agent_entrance_lock);
        down(agent_entrance_lock);
    }
    down(total_agents);
    while(*agent_num != my_num)
    {
        up(total_agents);
        up(agent_entrance_lock);
        down(agent_entrance_lock);
        down(total_agents);
    }
    up(total_agents);
    struct tm * timinfo;
    down(agent_lock);           //new
    down(agent_mutual_reader);
    if(*agent_mutual_obtained)
    {
        up(agent_mutual_reader);
    }
    else
    {
        down(lock);         //obtain lock, sleep if not avaiable
        *agent_mutual_obtained = 1;
        up(agent_mutual_reader);
    }
    down(total_agents);
    *agent_num+=1;
    up(total_agents);
    down(apt_open_boolean);
    down(agent_waiting_set);
    if(*apt_door == 0 && !(*waiting_agent))     //if door not open and no agent waiting
    {
        *waiting_agent += 1;////////////////need to think about this more...
        up(agent_waiting_set);
        down(agent_here_lock);
        *agent_boolean = 1;
        up(agent_here_lock);
        up(agent_here);         //when agent wakes back up when leaving, just see if an agent waiting
    }                           //if so call up on wait_show?
    else    //either door already open, or another agent already waiting, or both?
    {
        *waiting_agent += 1;        //will need to decrement when an agent no longer waiting
        up(agent_waiting_set);
    }
    up(apt_open_boolean);
    down(print_lock);
    down(start_time_lock);
    down(tenant_time_lock);
    *tenant_time = time(NULL);
    printf("Agent %d arrives at time %d.\n", my_num, ((*tenant_time) - (*start_time)));
    up(tenant_time_lock);
    up(start_time_lock);
    up(print_lock);
    up(agent_lock);     //new
    if(create_new_agent)      //sleep so next agent can arrive
    {
        up(agent_entrance_lock);
        down(wait_show);
    }
    else
    {
        down(apartment_reader);
        if(!(*apartment_locked))
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
        down(wait_show);    //go to sleep
    }
    down(show_counter);
    while(*showing_count != my_num)
    {
        up(show_counter);
        up(wait_show);
        down(wait_show);
        down(show_counter);
    }
    up(show_counter);
    //new
    down(show_counter);
    *showing_count += 1;
    up(show_counter);
    down(tenants_done_lock);
    if(*tenants_done)
    {
        up(tenants_done_lock);
        agentLeaves(my_num);
        return;
    }
    up(tenants_done_lock);
///////////////new
    down(apartment_reader);
    *apartment_locked = 1;
    up(apartment_reader);
    //down(show_counter);
    //*showing_count += 1;
    //up(show_counter);
    down(agent_lock);
    down(agent_mutual_reader);
    if(*agent_mutual_obtained)      //release lock as going to let tenant go if none have arrived
    {
        *agent_mutual_obtained = 0;
        up(lock);
    }
    up(agent_mutual_reader);
    up(agent_lock);
    down(tenant_waiting);       //check to see if there is a tenant at door, if not sleeping
    down(agent_lock);
    down(agent_mutual_reader);  //on return, hold lock until door opened
    if(*agent_mutual_obtained)
    {
        up(agent_mutual_reader);
    }
    else
    {
        down(lock);         //obtain lock, sleep if not avaiable
        *agent_mutual_obtained = 1;
        up(agent_mutual_reader);
    }
    down(waiting_set);  //set that tenants not currently waiting as you are opening door
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
    /////////new
    down(agents_done_lock);
    if(*agents_done)
    {
        up(agents_done_lock);
        down(print_lock);           //print tenant leaving
        down(start_time_lock);
        down(tenant_time_lock);
        *tenant_time = time(NULL);
        printf("Tenant %d leaves the apartment at time %d.\n", my_num, ((*tenant_time) - (*start_time)));
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
        up(wait_view);
        return;
    }
    up(agents_done_lock);
    ///////new
    down(tenants_viewing);       //see how many tenants have viewed apartment with agent
    down(num_in_apt);           //decrement num tenants in apartment
    down(burst_lock);
    *in_apt -= 1;

    down(max_tenant_lock);
    if(my_num == *max_tenants)  //if last tenant to go, set that no more tenants coming
    {
        down(tenants_done_lock);
        *tenants_done = 1;
        up(tenants_done_lock);

    }
    up(max_tenant_lock);
    if(*current_viewing == 0 && *in_apt == 0)    //if less than 10 and no people at door
    {
        up(burst_lock);
        up(tenants_viewing);        //release lock
        up(num_in_apt);             //release lock
        down(apt_open);             //if no more tenants but not at max yet, lock apartment
        down(apt_open_boolean);     //change apartment door to not open
        *apt_door = 0;              //set to 0 when closed
        up(apt_open_boolean);       //let go of lock to alter apt_door
        up(apt_occupied);           //let agent wake up
        down(print_lock);           ///////////////////////////////remove eventually
        //printf("Door locked by tenant %d\n", my_num);
        up(print_lock);             //release print lock
        down(print_lock);           //print tenant leaving
        down(start_time_lock);
        down(tenant_time_lock);
        *tenant_time = time(NULL);
        printf("Tenant %d leaves the apartment at time %d.\n", my_num, ((*tenant_time) - (*start_time)));
        up(tenant_time_lock);
        up(start_time_lock);
        up(print_lock);             //release print lock
        down(exit_counter);         //obtain exit counter
        *exit_count += 1;           //increment which tenant should exit next
        up(exit_counter);           //release lock
        down(max_tenant_lock);
        if(my_num == *max_tenants)  //if last tenant to go, set that no more tenants coming
        {
            down(print_lock);
            //printf("No more tenants coming.\n");
            up(print_lock);
        }
        up(max_tenant_lock);
        down(mutual_lock_reader);   //obtain lock for it
        if(*mutual_lock_obtained)   //if tenants have mutual lock between them and agents
        {
            *mutual_lock_obtained = 0;      //set that tenants no longer have lock
            up(lock);                       //release lock at this point as may need agent to open door
        }
        up(mutual_lock_reader);     //release lock
    }
    else if(*in_apt == 0 && *burst_size == 0)
    {
        up(burst_lock);
        up(tenants_viewing);        //release lock
        up(num_in_apt);             //release lock
        down(apt_open_boolean);     //change apartment door to not open
        *apt_door = 0;              //set to 0 when closed
        up(apt_open_boolean);       //let go of lock to alter apt_door
        down(apt_open);
        up(apt_occupied);           //let agent wake up
        down(print_lock);           ///////////////////////////////remove eventually
        //printf("Door locked by tenant %d\n", my_num);
        up(print_lock);             //release print lock
        down(print_lock);           //print tenant leaving
        down(start_time_lock);
        down(tenant_time_lock);
        *tenant_time = time(NULL);
        printf("Tenant %d leaves the apartment at time %d.\n", my_num, ((*tenant_time) - (*start_time)));
        up(tenant_time_lock);
        up(start_time_lock);
        up(print_lock);             //release print lock
        down(exit_counter);         //obtain exit counter
        *exit_count += 1;           //increment which tenant should exit next
        up(exit_counter);           //release lock
        down(max_tenant_lock);
        if(my_num == *max_tenants)  //if last tenant to go, set that no more tenants coming
        {
            down(print_lock);
            //printf("No more tenants coming.\n");
            up(print_lock);
        }
        up(max_tenant_lock);
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
        printf("Tenant %d leaves the apartment at time %d.\n", my_num, ((*tenant_time) - (*start_time)));
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
        up(tenants_viewing);       //releae lock to current viewing
        down(last_burst_lock);              //obtain lock
        down(previous_burst_lock);          //obtain lock
        *remaining = *previous_burst_size - 1;  //set remaining so they know how many times to call up on wait
        up(previous_burst_lock);            //release lock
        up(last_burst_lock);                //release lock

        down(last_burst_set);                //obtain lock
        *last_burst_boolean = 1;            //set that tenats from last burst still here
        up(last_burst_set);                 //release lock
        down(apt_open_boolean);     //get lock to see if door open
        *apt_door = 0;
        down(waiting_set);
        if(!(*waiting_ten))
        {
            *waiting_ten = 1;
            up(tenant_waiting);     //set this semaphore to tenant here
        }
        up(waiting_set);
        up(tenant_waiting);     //set this semaphore to tenant here
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
        //down(apt_open);             //lock apartment from future tenants
        down(num_in_apt);           //used to keep track of current tenants in apt
        *in_apt +=1;                //add 1 to the number of tenants in the apartment
        up(num_in_apt);             //release lock on num of tenants in apartment
        down(print_lock);           //print out tenant viewing apartment
        down(start_time_lock);
        down(tenant_time_lock);
        *tenant_time = time(NULL);
        printf("Tenant %d inspects the apartment at time %d.\n", my_num, ((*tenant_time) - (*start_time)));
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
        printf("Tenant %d inspects the apartment at time %d.\n", my_num, ((*tenant_time) - (*start_time)));
        up(tenant_time_lock);
        up(start_time_lock);
        up(print_lock);
    }
//may have to deal with this as may get changed on accident when tenant burst coming too fast
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
            {           /////////////////////////also have to deal with if agent no longer seeing tenants
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
    if(create_new_ten)
    {
        down(entrance);
    }
    else
    {
        up(entrance);
        down(entrance);
    }
    down(total_ten);
    while(my_num != *tenant_num)
    {
        up(total_ten);
        up(entrance);
        down(entrance);
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
    printf("Tenant %d arrives at time %d.\n", my_num, ((*tenant_time) - (*start_time)));
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
    while(*viewing_count != my_num)
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
    down(agents_done_lock);
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

int createNew(int prob)
{
    srand(time(NULL));
    int r = rand() % 100;
    //down(print_lock);
    //printf("%d\n", r);
    //up(print_lock);
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
    agent_apt = mmap(NULL,sizeof(struct cs1550_sem),  //
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);

    agent_here = mmap(NULL,sizeof(struct cs1550_sem),    //used as a binary semaphore for if there is at least one tenant waiting at closed door
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
    agent_burst_lock = mmap(NULL,sizeof(struct cs1550_sem),
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


    tenant_num = mmap(NULL,sizeof(int),                               //used to hold if door open
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    current_viewing = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    in_apt = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    apt_door = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    ten_counter = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
        PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    mutual_lock_obtained = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    burst_size = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    previous_burst_size = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    viewing_count = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    exit_count = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    wait_leave = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    remaining = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    last_burst_boolean = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    waiting_ten = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    current_agents = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agent_burst_size = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agent_mutual_obtained = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agent_num = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    waiting_agent = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    showing_count = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agent_boolean = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    apartment_locked = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    max_tenants = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    tenants_done = mmap(NULL,sizeof(int),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    start_time = mmap(NULL,sizeof(time_t),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    tenant_time = mmap(NULL,sizeof(time_t),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    max_agents = mmap(NULL,sizeof(time_t),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    agents_done = mmap(NULL,sizeof(time_t),                          //used to hold if there is a tenant waiting at closed door
            PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);



    //Initialize the semaphore to 0
    agent_apt->value = 1;
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
    agent_burst_lock->value = 1;
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

    *tenant_num = 1;    //changed to 1
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
    *agent_burst_size = 0;
    *agent_mutual_obtained = 0;
    *agent_num = 1; //changed to 1
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


    //may need -m 8 -k 9 -pt 20
    if(argc != 13)
    {
/////////////////////////////////////fix
        printf("Pass in exactly 6 non-negative integers: \n");
        printf("The first one is for the number of tenants\n");
        printf("The second one is for the number of agents\n");
        printf("The third one is for the probability of creating new tenants\n");
        printf("The fourth one is for the probability of creating new agents\n");
        printf("The fifth one is for the delay of tenants\n");
        printf("The sixth one is for the delay of agents\n");
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
        }
        else if(!strcmp(argv[i],"-k"))
        {
            i++;
            agents = atoi(argv[i]);
            i++;
        }
        else if(!strcmp(argv[i],"-pt"))
        {
            i++;
            prob_ten = atoi(argv[i]);
            i++;
        }
        else if(!strcmp(argv[i],"-dt"))
        {
            i++;
            delay_ten = atoi(argv[i]);
            i++;
        }
        else if (!strcmp(argv[i],"-pa"))
        {
            i++;
            prob_agent = atoi(argv[i]);
            i++;
        }
        else if (!strcmp(argv[i],"-da"))
        {
            i++;
            delay_agent = atoi(argv[i]);
            i++; 
        }
        else
        {
            printf("Invalid arguments passed\n");
            exit(1);
        }
    }
    printf(" %d %d %d %d %d %d\n", tenants, agents, prob_ten, delay_ten, prob_agent, delay_agent);
    down(max_tenant_lock);
    *max_tenants = tenants;
    up(max_tenant_lock);
    down(max_agent_lock);
    *max_agents = agents;
    up(max_agent_lock);

    down(start_time_lock);
    *start_time = time(NULL);
    up(start_time_lock);
    down(print_lock);
    printf("The apartment is now empty.\n");
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
      down(print_lock);
      //printf("Exiting\n");
      up(print_lock);
    }//end of else
    wait(NULL);//wait until Tenant creation process terminates
    down(print_lock);
    //printf("Exiting for good\n");
    up(print_lock);
    return 0;

}//end of main
