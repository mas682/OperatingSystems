


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char set_bit(char value, char bit)
{
  unsigned char added_val = 1;
  added_val = added_val << bit;
  return value ^ added_val;
}

// don't even think this is needed for this implementation
static unsigned char unset_bit(char value, int bit)
{
  unsigned char unset = 1;
  unset = unset << bit;
  return value ^ unset;
}

int find_empty(char value)
{
  char current = NULL;
  int i = 0;
  int char_count = 0;
  int index = 0;
  while(index < 8)
  {
    current = value;
    current = current << (7-i);
    current = current >> 7;
    if(current == 0)
      return index;
    i += 1;
    index += 1;
    //if(i == 8)
    //{
     // i = 0;
      //char_count += 1;
    //}
  }
  return -1;                                            // if at this point, empty spot not found
}

int main(int argc, char *argv[])
{
  char value = 5;
  char ret_val = 0;

  ret_val = set_bit(value, 3);
  printf("Returned after set: %x\n", ret_val);
  ret_val = set_bit(ret_val, 4);
  printf("Returned after set: %x\n", ret_val);
  ret_val = unset_bit(ret_val, 2);
  printf("Returned after unset: %x\n", ret_val);
  ret_val = set_bit(ret_val, 1);
  ret_val = set_bit(ret_val, 2);
  ret_val = set_bit(ret_val, 5);
  ret_val = set_bit(ret_val, 6);
  ret_val = set_bit(ret_val, 7);
  printf("VALUE PASSED TO EMTPY: %x\n", (unsigned char)ret_val);
  int empty = 0;
  empty = find_empty(ret_val);
  printf("First empty bit: %d\n", empty);
  exit(0);
}


// method returns -1 if it does not find a empty bit in a blcok from the given start location
// returns the index of the byte if it is free on first free byte encountered
//static int find_empty_byte_block_index(struct cs1550_disk_block *block_one, int location)
//{
//  unsigned char current = NULL;                       // holds a byte for the block
//  int i = 0;                                          // index into byte
  //int char_count = 0;                                 // which byte in the block
//  int index = location;                                      // number of bytes encountered
  //char_count = location / 8;                          // get current byte
  //i = location % 8;                                   // get index of current byte
//  while(index < MAX_DATA_IN_BLOCK)
//  {
//    current = cs1550_disk_block->data[index];    // set current to the current byte
    //current = current << (7-i);                       // clear leftmost bits
    //current = current >> 7;                           // get bit value
//    if(current == 0)                                  // if free, return index
//      return index;
    //i += 1;                                           // indrement bit index
//    index += 1;                                       // increment bytes encountered
    //if(i == 8)                                        // if at last bit of byte
    //{
    //  i = 0;                                          // reset bit counter
    //  char_count += 1;                                // increment to next byte of block
//    }
//  }
//  return -1;                                          // if at this point, empty spot not found
//}

// sees if a certain number of bytes are avaiable from a start location
/*static int find_empty_byte_from(struct cs1550_bit_map *bitmap, int num_blocks, int byte_start, int startBlock)
{
 int char_count = 0;
  int block = 0;
  int i = 0;
  int index = 0;
  int finished = 0;
  int num_empty = 0;
  unsigned char current = 0;            // used to hold a byte from bitmap
  int bit_map_index = 0;


  //remainder = byte_start;               // gets byte for particular block
  //block = start / 512;                   // get the starting block
  struct cs1550_disk_block *cur_block = NULL;    // get current block
  //i = remainder % 8;                     // get index into byte
  index = start;                         // used to keep track of how many bits encountered
  bit_map_index = startBlock;


  while(bit_map_index < 10240 && !finished  && num_empty < num_blocks)
  {
    cur_block = bitmap->blocks[bit_map_index];                 // get current block

    while(index < MAX_DATA_IN_BLOCK && !finished && num_empty < num_blocks)
    {
      current = cur_block->data[index];         // set current to block
      if(current == 0)                              // if block emtpy
      {
        num_empty += 1;                             // increment number of contiguous emtpy blocks found
      }
      else
      {
        finished = 1;
      }
      index += 1;                                  // increment byte index of block
    }
    index = 0;
    bit_map_index += 1;
    // may want to add check to see if next spot in bit map open..
  }

  if(num_empty == num_blocks)                 // if total blocks needed found, retur 1
  {
    return 1;
  }
  else                                       // if total block needed not found, return 0
  {
    return 0;
  }
}
*/
