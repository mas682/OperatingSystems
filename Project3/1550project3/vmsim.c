/****************************************************************************
* vmsim                                                                     *
* @author: Matt Stropkey                                                    *
* @username: mas682                                                         *
* This program is used to comparet the results of three different algorithms*
* on traces of memory references.  It displays the number of page faults    *
* that occur and the number of dirty frames that had to be written back to  *
* disk.  The three algorithms are optimal page replacement, FIFO, and Aging.*
* The program can be executed using:                                        *
* ./vsim -n <numframes> -a <opt|fifo|aging> [-r <refresh>] <tracefile>      *
* To compile: gcc vmsim.c -o vmsim                                          *
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *file_name;  // holds the file name
FILE *in_file;    // holds the trace file pointer

// function used to read file
int read(unsigned int *addr, char *m, unsigned int *cycles)
{
  int y = 0;              // holds return value of fscanf
  y = fscanf(in_file, "%c %x %d\n", m, addr, cycles);  // read a line from file
  return y;
}

// fifo queue
struct queue
{
  struct node *last_ptr;        // pointer to last node in queue
  struct node *first_ptr;       // pointer to first node in queue
  int size;                     // holds the size of the queue
};

// node for queue entry
struct node
{
  struct node *next_node;      // points to next node in queue
  unsigned int index;          // value of node
};

// used for aging algorithm
struct bit_counter
{
  unsigned char counter;      // holds a 8 bit counter for aging
  unsigned char ref_bit;      // holds the reference bit to say if page referenced during period
};

// this method removes the top index in the queue
int remove_node(struct queue *ptr)
{
  if(ptr->size <= 0)
  {
    printf("Cannot remove a node from an empty queue\n");
    exit(1);
  }
  struct node *next;                // will hold next value to be head of queue
  struct node *first;               // will hold the first node in the queue
  first = ptr->first_ptr;           // set first to first value in queue
  int value = first->index;         // obtain value to return
  next = first->next_node;              // set temp to the first indexes next node, whehter null or another node
  ptr->first_ptr = next;           // set the first ptr to temp
  ptr->size -= 1;                  // decrement the size of the queue
  free(first);                     // free the node that is being removed
  return value;                    // return value
}

// method used to add a node to the queue
int add_node(struct queue *ptr, int i)
{
  struct node *temp;
  struct node *last;
  if(ptr->size == 0)                   // if queue empty
  {
    temp = malloc(sizeof(struct node));      // obtain memory for node
    temp->next_node = NULL;                  // set next node to NUll
    temp->index = i;                         // set the value of the index
    ptr->last_ptr = temp;                    // set the last ptr to this node
    ptr->first_ptr = temp;                   // set the first ptr to this node
    ptr->size += 1;                          // increment size of queue
  }
  else                                 // if queue not empty
  {
    temp = malloc(sizeof(struct node));      // obtain memory for node
    temp->next_node = NULL;                  // set the next node to NULL
    temp->index = i;                         // set the value of the index
    last = ptr->last_ptr;                    // set last to queues last ptr
    last->next_node = temp;                  // set lasts next node to this node
    ptr->last_ptr = temp;                    // set the queues last node to this node
    ptr->size += 1;                          // increment the size of the queue
  }
  return i;
}

// method to read valid bit from page table(bit 8)
char read_valid(unsigned char temp)
{
  temp = temp >> 7; // shift valid/invalid bit to 1'st position
  return temp;    // should return 1 or 0
}

// set the valid bit for a page table index
unsigned char set_valid(unsigned int temp)
{
  temp = 128 | temp;
  return temp;
}

// unset the valid bit for a page table index
unsigned char unset_valid(unsigned int temp)
{
  if(read_valid(temp))
    temp = 128 ^ temp;
  return temp;
}

// used to read the dirty bit from page table (bit 7)
unsigned char read_dirty(unsigned char temp)
{
  temp = temp << 1;
  temp = temp >> 7;
  return temp;
}

// set the dirty bit for a page table index
unsigned char set_dirty(unsigned int temp)
{
  temp = 64 | temp;
  return temp;
}

// unset the dirty bit for a page table index
unsigned char unset_dirty(unsigned int temp)
{
  if(read_dirty(temp))
    temp = 64 ^ temp;
  return temp;
}

// outputs the results of an algorithm
void output(int memory_accesses, int page_faults, int writes_to_disk, int numframes, char *alg)
{
  printf("Algorithm: %s\n", alg);
  printf("Number of frames: %d\n", numframes);
  printf("Total memory accesses: %d\n", memory_accesses);
  printf("Total page faults: %d\n", page_faults);
  printf("Total writes to disk: %d\n", writes_to_disk);
}

// returns the top value in the queue without removing it
int peek(struct queue *temp)
{
  return temp->first_ptr->index;
}

// method returns the index of the frame that holds the address that will
// be used last out of all pages currently in main memory
int remove_page_opt(unsigned int *frame, int numframes, struct queue *array[])
{
  unsigned int index = 0;     // used to hold memory address that is in main memory
  int i = 0;                  // used to iterate though frame array
  int largest = 0;            // used to hold largest next instruction out of all address in frame
  int ret_index = 0;          // used to return index into frame array to remove
  int next_inst = 0;          // used to hold value of a pages next time it will be accesse
  struct queue *temp;         // just a pointer to hold each pages queue temporarily
  while(i < numframes)        // iterate through each frame
  {
    index = frame[i];         // get the address from the frame
    temp = array[index];      // get the queue for that address
    if(temp == 0)             // this memory will not get used again so just remove it
    {
      return i;
    }
    next_inst = peek(temp);   // see what the top value of the queue is
    if(next_inst > largest)   // if value larger that previous values, set it as largest
    {
      largest = next_inst;    // set largest to new value
      ret_index = i;          // set return index to this index into frame
    }
    i++;                      // increment loop counter
  }
  return ret_index;           // return index into frame to remove
}

// shifts counters for aging one bit to the right
int shift_counters(struct bit_counter **counters, int frame_counter)
{
  int i = 0;
  struct bit_counter *temp;         // holds a counter struct
  while(i < frame_counter)          // iterate through all counters
  {
    temp = counters[i];             // set temp to the current counter
    temp->counter = temp->counter >> 1;   // shift temps counter right one bit
    i++;        // increment i
  }
  return i;     // return number of counters shifted
}

// sets leftmost bit of a counter
int set_leftmost(struct bit_counter **counters, int frame_counter)
{
  int i = 0;
  struct bit_counter *temp;              // holds a counter struct
  while(i < frame_counter)               // iterate through all counters
  {
    temp = counters[i];                  // temp = current counter
    if(temp->ref_bit)                    // if reference bit set
    {
      temp->counter = temp->counter | 128;      // set leftmost bit to 1
    }
    i++;                                 // increment i
  }
  return i;                              // return number of counters checked
}

// unsets the reference bit for a bit_counter
int unset_reference_bits(struct bit_counter **counters, int frame_counter)
{
  int i = 0;
  struct bit_counter *temp;            // temp holds current bit_counter
  while(i < frame_counter)             // iterate through all bit_counters
  {
    temp = counters[i];                // temp = current bit counter
    if(temp->ref_bit)                  // check to see if reference bit set
    {
      temp->ref_bit = 0;               // if bit set, set to 0
    }
    i++;                               // increment i
  }
  return i;                            // return number of bit counters checked
}

// sets the reference bit for a specific bit counter
void set_reference_bit(struct bit_counter **counters, unsigned char index)
{
  struct bit_counter *temp = counters[index];            // temp = bit counter to set reference bit for
  temp->ref_bit = 1;                                     // set reference bit for bit counter
}

// get index into frame table by clearing valid and dirty bit
unsigned char get_index_mem(unsigned char value)
{
  value = value << 2;  // clear leftmost 2 bits
  value = value >> 2;
  return value;
}

// method used to remove a page from frame table of aging algorithm
int remove_page_age(int numframes, struct bit_counter **counters, unsigned int *frame, unsigned char *page)
{
  struct bit_counter *temp;         // used to hold a bit counter
  int removed = 0;                  // index into frame table to remove
  unsigned char lowest = 255;       // lowest = lowest counter
  int iter = 0;                     // iterate through all frames
  while(iter < numframes)
  {
    temp = counters[iter];          // temp = current counter
    if(temp->counter < lowest || iter == 0)      // if temps counter is less than lowest
    {                                            // or first counter to go
      lowest = temp->counter;                    // set lowest to temps counter
      removed = iter;                            // set removed to this index
    }
    else if(temp->counter == lowest)        // if counter equals lowest
    {
      unsigned int old = frame[removed];    // holds page table index of current removed
      unsigned int new = frame[iter];       // holds page table index of potential next to remove
      char dirty_old = 0;                   // set to 1 if old page dirty
      char dirty_new = 0;                   // set to 1 if new page (to remove) dirty
      if(read_dirty(page[old]))             // set dirty_old if old index dirty
      {
        dirty_old = 1;
      }
      if(read_dirty(page[new]))             // set dirty_new if new index dirty
      {
        dirty_new = 1;
      }
      if(dirty_old && dirty_new)  // if both dirty
      {
        if(new < old)            // if new address less than old, set removed to new
        {
           removed = iter;
        }
        // otherwise removed = old
      }
      else if(dirty_new)  // if only new dirty
      {
      }
      else if(dirty_old)  // if only old dirt, set removed to new
      {
        removed = iter;
      }
      else     // if neither dirty
      {
          if(new < old)   // if new address lower than old, set removed to new
          {
             removed = iter;
          }
          // otherwise removed = old
      }
    }
    iter++;           // increment iteration
  }
  return removed;     // return index to remove from frame table
}

void aging_alg(unsigned char *page, unsigned int *frame, int numframes, int refresh)
{
  struct bit_counter **counters = malloc(numframes * sizeof(struct bit_counter*));    // used to hold counters
  unsigned int addr;               // used to hold the address read in from the file
  char m;                          // used to hold load or save from file
  unsigned int cycles;             // used to hold cycles from file...unneeded here?
  unsigned char page_index = 0;    // used to temporarily hold the value of the page table at current index
  char valid_set = 0;              // used as a boolean for if valid bit is set
  int memory_accesses = 0;         // holds the total memory accesses
  int page_faults = 0;             // holds the total page faults
  int writes_to_disk = 0;          // holds the total writes to disk
  int frame_counter = 0;           // used to set pages to frames when all frames not full
  int current_cycles = 0;          // used to hold current cycle value in period

  int i = 0;
  while(i < numframes)
  {
    counters[i] = malloc(sizeof(struct bit_counter));
    i++;
  }
  while(read(&addr, &m, &cycles) == 3)
  {
    memory_accesses++;
    current_cycles += cycles;       // increment number of cycles
    while(current_cycles >= refresh)     // while the current number of cycles is greater than refresh value
    {
      shift_counters(counters, frame_counter);    // shift all counters right one bit
      set_leftmost(counters, frame_counter);            // set leftmost bit of counter if reference bit set
      unset_reference_bits(counters, frame_counter);       // set all reference bits to 0
      current_cycles -= refresh;    // decrement cycles
    }
    addr = addr >> 12;          // skip first 3 bytes of address
    page_index = page[addr];    // set page_index to value in page table
    valid_set = read_valid(page_index);  // see if valid bit has been set
    if(valid_set)  // page in memory, mark that it has been accessed during period
    {
      set_reference_bit(counters, get_index_mem(page_index));  // set the reference bit for this page in memory
    }
    else  // valid bit not set, so page not in main memory
    {
      page_faults++;       // increment number of page faults
      if(frame_counter < numframes)    // frame table not full yet
      {
        frame[frame_counter] = addr;  // set frame index to address into page table
        page[addr] = set_valid(frame_counter);  // set index in page table for address with valid set
        struct bit_counter *temp = counters[frame_counter];  // get the counter at frame index
        temp->ref_bit = 0;       // set reference bit to 0
        temp->counter = 128;     // set leftmost bit of counter to 1
        frame_counter++;     // increment number of frames full
      }
      else //frames full
      {
        int removed = remove_page_age(numframes, counters, frame, page);  // get a page to remove from main memory
        unsigned int old_page = frame[removed];  // get index into page table of page to remove
        page[old_page] = unset_valid(page[old_page]);   // set that page no longer in memory
        if(read_dirty(page[old_page]))     // if the page is dirty
        {
          writes_to_disk++;                // increment the writes to disk
          page[old_page] = unset_dirty(page[old_page]);   // unset dirty bit in page table of old page
        }
        frame[removed] = addr;       // set frame to new address
        page[addr] = set_valid(removed);  // set that page in memory
        struct bit_counter *temp = counters[removed];  // set temp to counter
        temp->ref_bit = 0;     // set the reference bit to 0
        temp->counter = 128;   // set leftmost bit of counter to 1
      }
    }
    if(m == 's')         // if page saving, set dirty bit
    {
      page[addr] = set_dirty(page[addr]);
    }
    current_cycles++;           // this is needed for some reason? not sure why..
  }
  output(memory_accesses, page_faults, writes_to_disk, numframes, "AGING");
  i = 0;
  while(i < numframes)   // free all malloced bit counters
  {
    free(counters[i]);
    i++;
  }
  free(counters);     // free counters itself
}

void optimal(unsigned char *page, unsigned int *frame, int numframes)
{
  struct queue **future_ref = malloc(1048576 * sizeof(struct queue*));    // used to hold counters
  int address_counter = 0;         // used to set values in queue
  unsigned int addr;               // used to hold the address read in from the file
  char m;                          // used to hold load or save from file
  unsigned int cycles;             // used to hold cycles from file...unneeded here?
  unsigned char page_index = 0;    // used to temporarily hold the value of the page table at current index
  char valid_set = 0;              // used as a boolean for if valid bit is set
  int memory_accesses = 0;         // holds the total memory accesses
  int page_faults = 0;             // holds the total page faults
  int writes_to_disk = 0;          // holds the total writes to disk
  int frame_counter = 0;           // used to set pages to frames when all frames not full

  int i = 0;
  while(i < 1048576)        // set each value to 0 in futrue_ref
  {
    future_ref[i] = 0;
    i++;
  }

  while(read(&addr, &m, &cycles) == 3)     // read a line from the file)
  {
    addr = addr >> 12;            // skip first 3 bytes of address
    if(future_ref[addr] == 0)     // if there is no queue at this index
    {
      struct queue *new_queue = malloc(sizeof(struct queue));    // create space for a new queue
      new_queue->size = 0;                // set the size of the queue to 0
      add_node(new_queue, address_counter);     // add occurence to queue
      future_ref[addr] = new_queue;
      address_counter++;                 // increment address counter
    }
    else                         // queue exists at index
    {
      struct queue *temp = future_ref[addr];     // set temp to the queue at the index
      add_node(temp, address_counter);           // add occurence to queue
      address_counter++;                   // increment address counter
    }
  }
  fseek(in_file, 0, SEEK_SET);        // reset file pointer to beginning of file

  while(read(&addr, &m, &cycles) == 3)     // read a line from the file)                       // while still addresses to read
  {
    addr = addr >> 12;            // skip first 3 bytes of address
    page_index = page[addr];      // set page index to value in page table at addr
    valid_set = read_valid(page_index);     // read the valid bit of the value from the page table
    memory_accesses++;            // increment the number of times memory has been accessed
    remove_node(future_ref[addr]);    // remove the value of this instruction from the queue
    if(future_ref[addr]->size == 0)
    {
      free(future_ref[addr]);         // if queue now empty, free the memory
      future_ref[addr] = 0;
    }
    if(!valid_set)         // if page not in main memory
    {
      page_faults++;       // incrment number of page faults
      if(frame_counter < numframes)                // if there are still frames available
      {
        frame[frame_counter] = addr;               // set the value at frame to index into page table
        page[addr] = set_valid(frame_counter);     // set the value in page table to index into frame with valid bit set
        frame_counter++;                           // incrment number of frames occupied
      }
      else
      {
        int removed = remove_page_opt(frame, numframes, future_ref);      // get index to which frame to remove
        unsigned int old_page = frame[removed];    // get the index to page table of address to remove
        page[old_page] = unset_valid(page[old_page]);   // unset valid bit of address getting removed
        if(read_dirty(page[old_page]))          // if dirty bit set
        {
          writes_to_disk++;                     // increment number of writes to disk
          page[old_page] = unset_dirty(page[old_page]);  // unset the dirty bit of the page being removed
        }
        frame[removed] = addr;    // set frame to new address
        page[addr] = set_valid(removed);  // set valid bit of page table index at new location
      }
    }
    if(m == 's')                        // if saving to memory, mark dirty bit
    {
      page[addr] = set_dirty(page[addr]);    // set dirty bit in page table
    }
  }
  output(memory_accesses, page_faults, writes_to_disk, numframes, "OPT");
}

void fifo(unsigned char *page, unsigned int *frame, int numframes)
{
  struct queue *frames = malloc(sizeof(struct queue));            // fifo queue
  frames->size = 0;                // set fifo queues size to 0
  unsigned int addr;               // used to hold the address read in from the file
  char m;                          // used to hold load or save from file
  unsigned int cycles;             // used to hold cycles from file...unneeded here?
  unsigned char page_index = 0;    // used to temporarily hold the value of the page table at current index
  char valid_set = 0;              // used as a boolean for if valid bit is set
  int memory_accesses = 0;         // holds the total memory accesses
  int page_faults = 0;             // holds the total page faults
  int writes_to_disk = 0;          // holds the total writes to disk
  int frame_counter = 0;           // holds number of frames occupied
  char large_frames = 0;           // boolean to set if using more than 64 frames

  if(numframes > 64)            // if frames > 64, ignoring dirty bit
  {
    large_frames = 1;
  }

  while(read(&addr, &m, &cycles) == 3)
  {
    addr = addr >> 12;             // shift address right 12 bits to ignore first 3 bits
    page_index = page[addr];        // set page_index to the value currently held in page table
    valid_set = read_valid(page_index);  // if 1, then page already in memory
    memory_accesses++;                   // increment the number of times memory has been accessed
    if(!valid_set)                     // if page not in memory
    {
      page_faults++;                              // increment number of page faults
      if(frames->size < numframes)     // if there are frames still available
      {
        add_node(frames, frame_counter);            // add the frame index to queue
        frame[frame_counter] = addr;                // set the value at the frame to the index of page table
        page[addr] = set_valid(frame_counter);      // set value in page table to address valid, and index to frame
        frame_counter++;                            // increment number of frames being used
      }
      else                              // no frame available so have to free one
      {
         int removed = remove_node(frames);      // get the index to the page to remove from frame table
         unsigned int old_page = frame[removed];
         page[old_page] = unset_valid(page[old_page]);      // set page we are removing to invalid
         if(read_dirty(page[old_page]) && !large_frames)      // if the page in memory was altered, write to disk
         {
           writes_to_disk++;               // increment write to disk counter
           page[old_page] = unset_dirty(page[old_page]);
         }
         frame[removed] = addr;             // set the value at the frame to the index of page_table
         page[addr] = set_valid(removed);   // set the value in page table to address valid, and index to frame
         add_node(frames, removed);           // add the frame index to the queue
      }
    }
    if(m == 's' && !large_frames)                // if altering memory, set dirty bit
    {
      page[addr] = set_dirty(page[addr]);     // set the dirty bit in page table index
    }
  }
  free(frames);        // release memory of fifo queue
  output(memory_accesses, page_faults, writes_to_disk, numframes, "FIFO");
}


int main(int argc, char *argv[])
{
  int numframes = 0;            // used to hold number of frames for address space
  int i = 1;                // used to read in command line arguments
  char *mode;               // used to choose algorithm to run program on
  int close;                // used when closing the file at the end
  int refresh;

  if(argc < 1)       //will need to alter this eventually
  {
    printf("Incorrect number of arguments given to command line\n");
    exit(1);
  }
  if(!strcmp(argv[i], "-n"))    // get the number of frames
  {
    i++;
    numframes = atoi(argv[i]);
    i++;
    if(numframes <= 0)
    {
      printf("Number of frames cannot be less than or equal to 0.\n");
      exit(1);
    }
  }

  if(!strcmp(argv[i], "-a"))      // get the algorithm to use
  {
    i++;
    mode = argv[i];
    i++;
  }

  if(!strcmp(argv[i], "-r"))    // if aging, get refresh rate
  {
    i++;
    refresh = atoi(argv[i]);
    i++;
  }
  //should be last one set
  file_name = argv[i];
  in_file = fopen(file_name, "r");
  if(in_file == NULL)          // if file null, just output 0's
  {
      if(!strcmp(mode, "fifo"))
      {
        output(0, 0, 0, numframes, "FIFO");
      }
      else if(!strcmp(mode, "opt"))
      {
        output(0, 0, 0, numframes, "OPT");
      }
      else if(!strcmp(mode, "aging"))
      {
        output(0,0,0, numframes, "AGING");
      }
      exit(0);
  }
  unsigned char *page_table = malloc(sizeof(char)*1048576);    // initialize array to all 0's
  i = 0;
  while(i < 1048576)          // initialize page table values to 0
  {
    page_table[i] = 0;
    i++;
  }
  unsigned int *frame_table = malloc(numframes * sizeof(unsigned int));   // initialize array to all 0's
  if(!strcmp(mode, "fifo"))
  {
    fifo(page_table, frame_table, numframes);
  }
  else if(!strcmp(mode, "opt"))
  {
    optimal(page_table, frame_table, numframes);
  }
  else if(!strcmp(mode, "aging"))
  {
     aging_alg(page_table, frame_table, numframes, refresh);
  }

  close = fclose(in_file);                  // close the file when finished with it
  if(close)
  {
    printf("Unable to close the file\n");
  }
  free(frame_table);
  free(page_table);
  exit(0);
}
