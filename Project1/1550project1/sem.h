#include <linux/sched.h>

struct cs1550_sem
{
        int value;                          //holds the current semaphore value
        struct queue *list;                 //holds all the processes waiting for the semaphore
};

struct queue {
        struct task_struct *task;              //holds a process
        struct queue *next_ptr;                //used to hold next process in queue
};
