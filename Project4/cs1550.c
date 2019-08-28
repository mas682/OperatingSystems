/*
	FUSE: Filesystem in Userspace
	Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

	This program can be distributed under the terms of the GNU GPL.
	See the file COPYING.
*/

#define	FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

//size of a disk block
#define	BLOCK_SIZE 512

//we'll use 8.3 filenames
#define	MAX_FILENAME 8
#define	MAX_EXTENSION 3

//How many files can there be in one directory?
#define MAX_FILES_IN_DIR (BLOCK_SIZE - sizeof(int)) / ((MAX_FILENAME + 1) + (MAX_EXTENSION + 1) + sizeof(size_t) + sizeof(long))

//The attribute packed means to not align these things
struct cs1550_directory_entry
{
	int nFiles;	//How many files are in this directory.
				//Needs to be less than MAX_FILES_IN_DIR

	struct cs1550_file_directory
	{
		char fname[MAX_FILENAME + 1];	//filename (plus space for nul)
		char fext[MAX_EXTENSION + 1];	//extension (plus space for nul)
		size_t fsize;					//file size
		long nStartBlock;				//where the first block is on disk
	} __attribute__((packed)) files[MAX_FILES_IN_DIR];	//There is an array of these

	//This is some space to get this to be exactly the size of the disk block.
	//Don't use it for anything.
	char padding[BLOCK_SIZE - MAX_FILES_IN_DIR * sizeof(struct cs1550_file_directory) - sizeof(int)];
} ;

typedef struct cs1550_root_directory cs1550_root_directory;

#define MAX_DIRS_IN_ROOT (BLOCK_SIZE - sizeof(int)) / ((MAX_FILENAME + 1) + sizeof(long))

struct cs1550_root_directory
{
	int nDirectories;	//How many subdirectories are in the root
						//Needs to be less than MAX_DIRS_IN_ROOT
	struct cs1550_directory
	{
		char dname[MAX_FILENAME + 1];	//directory name (plus space for nul)
		long nStartBlock;				//where the directory block is on disk
	} __attribute__((packed)) directories[MAX_DIRS_IN_ROOT];	//There is an array of these

	//This is some space to get this to be exactly the size of the disk block.
	//Don't use it for anything.
	char padding[BLOCK_SIZE - MAX_DIRS_IN_ROOT * sizeof(struct cs1550_directory) - sizeof(int)];
} ;


typedef struct cs1550_directory_entry cs1550_directory_entry;

//How much data can one block hold?
#define	MAX_DATA_IN_BLOCK (BLOCK_SIZE)

struct cs1550_disk_block
{
	//All of the space in the block can be used for actual data
	//storage.
	char data[MAX_DATA_IN_BLOCK];
};

struct cs1550_bitmap
{
   // Hold information about whether a block is avaiable or not
   unsigned char blocks[1280];
};

typedef struct cs1550_disk_block cs1550_disk_block;


static int find_contains_char(const char *string, int ascii_val, int num)
{
  int i = 0;
  char current_char = 0;
  int found = 0;
  if(string != NULL)
  {
    printf("Contains called with %s\n", string);
    while(i < strlen(string))
	{
	  current_char = string[i];
	  if(current_char == ascii_val)
	  {
		found += 1;
	  }
	  i+=1;
	}
  }
  if(found >= num)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// method only called by split_path
// returns a file name, and extension via pointers
// returns 0 on success, anything else otherwise
static int split_file_name(const char *path,char *file, char **file_name, char **ext)
{
  printf("SPLIT FILE\n");
  char *token = NULL;                                   // used to hold strings when tokenized
 // int token_count = 0;                                  // used for counter of tokens
  if(!find_contains_char(file, 46, 1))
  {
	printf("Cannot call strtok\n");
	return -EPERM;
  }
  token = strtok(file, ".");             // get file name, without extension
  *file_name = malloc(strlen(token) + 20);                     // first token should be file name
  *ext = malloc(strlen(token) + 20);                        // set ext to extension
  strcpy(*file_name, token);
  if(token != NULL)                   // keep reading tokens until no more
  {
	  if(strlen(*file_name) > 8)
	  {
		printf("File name is too long %s\n", token);
		free(*file_name);
        free(*ext);
		return -ENAMETOOLONG;
	  }
	  if(!find_contains_char(path, 46, 1))
	  {
		printf("Cannot call strtok\n");
		free(*file_name);
        free(*ext);
		return -ENOENT;
	  }
	  token = strtok(NULL, ".");
	  if(token != NULL)
	  {
		strcpy(*ext, token);
		if(strlen(token) > 3)
		{
		  printf("Extension is too long %s\n", token);
		  free(*file_name);
          free(*ext);
		  return -ENAMETOOLONG;
		}
		int i = 0;
		char current_char = 0;
		while(i < 3)
		{
		  current_char = *ext[i];
		  if(current_char == 46)
		  {
			printf("Extension contains more than one period: %s\n", *ext);
			free(*file_name);
            free(*ext);
			return -ENAMETOOLONG;
		  }
		  i+=1;
		}
	  }
	  else
	  {
		printf("No extension for file\n");
		free(*file_name);
        free(*ext);
		return -ENOENT;
	  }
  }
  else
  {
    printf("No extension for file\n");
    free(*file_name);
    free(*ext);
    return -ENOENT;
  }
  return 0;
}

// split the path up for directory/filename.ext
// return error value on error, 0 otherwise
static int split_path(const char *path, char **directory, char **file_name, char **ext)
{
  printf("SPLIT PATH\n");
  int string_len_orig = strlen(path);                   // used to hold length of path
  char *original = malloc(sizeof(string_len_orig) + 20);     // malloc a region for a string
  strcpy(original, path);
  char *token = NULL;                                   // used to hold strings when tokenized
  int token_count = 0;                                  // used for counter of tokens
  char *file = NULL;                                    // used to hold file name with extension
  int file_ret = 0;                                     // used to hold return value of split_file_name

  if(!find_contains_char(original, 47, 1))
  {
	printf("Cannot call strtok\n");
	free(original);
	return -ENOENT;
  }
  token = strtok(original, "/");                        // get the first token of path, should be a directory
  *directory = malloc(strlen(token) + 20);
  strcpy(*directory,token);                                // set directory to first token
  if(token != NULL)                                  // loop to see how maky tokens in path
  {
    token_count += 1;
	// if the file name or directory name is bigger than 8 characters, should not be in system
	if(strlen(token) > 8)
	{
      free(original);
      free(*directory);
	  return -ENOENT;
	}
	if(!find_contains_char(path, 47, 2))
	{
	  printf("Cannot call strtok\n");
    }
    else
    {
	  token = strtok(NULL, "/");                    // get next token, which should be a file name
	  if(token != NULL)
      {
	    file = token;                               // set token to file name
	    token_count += 1;                             // increment tokens encountered
	    if(strlen(token) > 12)
	    {
		  printf("File name is too long %s\n", token);
          free(*directory);
		  free(original);
		  return -ENAMETOOLONG;
	    }
	    int i = 0;
	    char current_char = 0;
	    while(i < strlen(file))
	    {
		  current_char = 0;
		  if(current_char == 47)
		  {
		    printf("Too many / in path: %s\n", file);
            free(*directory);
		    free(original);
		    return -ENOENT;
		  }
		  i += 1;
	    }
      }
    }
  }

  if(*directory == NULL || file == NULL)       // if trying to create file in directory
  {
	//printf("Cannot create file in root directory\n");
    free(original);
    free(*directory);
	return -EPERM;
  }
  file_ret = split_file_name(path,file, file_name, ext);  // split file name and extension
  if(file_ret != 0)                                     // if anything but 0 returned, return failure
  {
    //printf("Split_file_name failed\n");            // for testing
    free(original);
    free(*directory);
    return file_ret;
  }
  free(original);
  return 0;
}
// sets a bit in a given byte
// returns the byte
static unsigned char set_bit(unsigned char value, int bit)
{
  unsigned char added_val = 1;
  added_val = added_val << bit;
  return value ^ added_val;
}


// returns index of next free block from a given start location
static int find_empty_block_index(struct cs1550_bitmap *bitmap, int location)
{
  int index = location;
  int i = location % 8;
  int bitmap_index = location / 8;
  unsigned char current = 0;
  while(index < 10240 && bitmap_index < 1280)
  {
    current = bitmap->blocks[bitmap_index];                   // get a byte from bit map
    current = current << (7-i);                        // clear leftmost bits
    current = current >> 7;                            // get bit value
    if(current == 0)                                   // if block free
      return index;
    i += 1;                                            // increment bit index
    index += 1;                                        // increment number of blocks checked
    if(i == 8)                                         // if at last byte in block
    {
      i = 0;                                           // reset i
      bitmap_index += 1;                                   // get next byte in bit map
    }
  }
  return -1;                                          // return -1 if no free block found
}


// returns 1 if a contiguous regions is found of a certain size from a specific location
// returns -1 if not found
static int find_contiguous_blocks(struct cs1550_bitmap *bitmap, int location, int size)
{
  int index = location;
  int i = index % 8;
  int bitmap_index = index / 8;
  unsigned char current = 0;
  int num_contiguous = 0;
  while(index < 10240 && bitmap_index < 1280)
  {
    current = bitmap->blocks[bitmap_index];                   // get a byte from bit map
    current = current << (7-i);                        // clear leftmost bits
    current = current >> 7;                            // get bit value
    if(current == 0)                                   // if block free
    {
      num_contiguous += 1;
      if(num_contiguous == size)
      {
        return 1;
      }
    }
    else
    {
      return -1;
    }
    i += 1;                                            // increment bit index
    index += 1;                                        // increment number of blocks checked
    if(i == 8)                                         // if at last byte in block
    {
      i = 0;                                           // reset i
      bitmap_index += 1;                                   // get next byte in bit map
    }
  }
  return -1;                                          // return -1 if no free block found
}

// returns block index if a contiguous regions is found of a certain size from anywhere
// returns -1 if not found
static int find_contiguous_blocks_anywhere(struct cs1550_bitmap *bitmap, int size)
{
  int index = 0;
  int i = 0;
  int bitmap_index = 0;
  unsigned char current = 0;
  int num_contiguous = 0;
  int start_index = 0;
  while(index < 10240 && bitmap_index < 1280)
  {
    current = bitmap->blocks[bitmap_index];                   // get a byte from bit map
    current = current << (7-i);                        // clear leftmost bits
    current = current >> 7;                            // get bit value
    if(current == 0)                                   // if block free
    {
      if(num_contiguous == 0)
        start_index = index;
      num_contiguous += 1;
      if(num_contiguous == size)
      {
        return start_index;
      }
    }
    else
    {
      num_contiguous = 0;
    }
    i += 1;                                            // increment bit index
    index += 1;                                        // increment number of blocks checked
    if(i == 8)                                         // if at last byte in block
    {
      i = 0;                                           // reset i
      bitmap_index += 1;                                   // get next byte in bit map
    }
  }
  return -1;                                          // return -1 if no free block found
}


// method used to get the number of directories for the disk
static int get_root_directory(cs1550_root_directory *root)
{
    FILE *disk_data;                               // variable used to hold pointer to file
    disk_data = fopen(".disk", "rb");              // open the file in read binary
    if(disk_data == NULL)                          // make sure the file was opened
    {
      printf("Could not open .disk file\n");
      return -1;
    }
    if(fread(root, sizeof(struct cs1550_root_directory),1, disk_data) != 1)  // read the number of directories
    {
      printf("DISK READ ERROR ROOT\n");
      fclose(disk_data);
      return -1;
    }
    if(fclose(disk_data))                        // close the file
    {
      printf(".disk file not closed\n");
    }
    return 0;
}


// method just for error checking
static int check_index(long disk_index)
{
  if(disk_index % 512 != 0)
    return 1;
  return 0;
}

// method used to get a directory
static int get_directory(cs1550_directory_entry *dir, long disk_ind)
{
    FILE *disk_data;                               // variable used to hold pointer to file
	if(check_index(disk_ind))
	{
	  printf("DISK INDEX IS NOT A MULTIPLE OF 512\n");
	  return -1;
	}

    disk_data = fopen(".disk", "rb");              // open the file in read binary
    if(disk_data == NULL)                          // make sure the file was opened
    {
      printf("Could not open .disk file\n");
      return -1;
    }
    fseek(disk_data, disk_ind, SEEK_SET);                           // go to location on disk where directory found
    if(fread(dir, sizeof(struct cs1550_directory_entry), 1, disk_data) != 1)
    {
      printf("DISK READ ERROR DIRECTORY\n");
      fclose(disk_data);
      return -1;
    }

    if(fclose(disk_data))                        // close the file
    {
      printf(".disk file not closed\n");
    }
    return 0;
}

// method used to get the bit map
static int get_bitmap(struct cs1550_bitmap *bitmap)
{
	FILE *disk_data;                               // variable used to hold pointer to file

    disk_data = fopen(".disk", "rb");              // open the file in read binary
    if(disk_data == NULL)                          // make sure the file was opened
    {
      printf("Could not open .disk file\n");
      return -1;
    }
    //printf("size of bitmap %d but storing in 3 whole blocks\n", sizeof(struct cs1550_bitmap));
    fseek(disk_data, 5241344, SEEK_SET);                           // go to location on disk where directory found
    if(fread(bitmap, sizeof(struct cs1550_bitmap), 1, disk_data) != 1)
    {
      printf("DISK READ ERROR BITMAP\n");
      fclose(disk_data);
      return -1;
    }

    if(fclose(disk_data))                        // close the file
    {
      printf(".disk file not closed\n");
    }
    return 0;
}

// method used to get a file block at the specified index
static int get_file_block(struct cs1550_disk_block *file_block, long disk_index)
{
	FILE *disk_data;                               // variable used to hold pointer to file
    if(check_index(disk_index))
    {
      printf("DISK INDEX IS NOT A MULTIPLE OF 512\n");
      return -1;
    }

    disk_data = fopen(".disk", "rb");              // open the file in read binary
    if(disk_data == NULL)                          // make sure the file was opened
    {
      printf("Could not open .disk file\n");
      return -1;
    }
    fseek(disk_data, disk_index, SEEK_SET);                           // go to location on disk where directory found
    if(fread(file_block, sizeof(struct cs1550_disk_block), 1, disk_data) != 1)
    {
      printf("DISK READ ERROR FILE BLOCK\n");
      fclose(disk_data);
      return -1;
    }

    if(fclose(disk_data))                        // close the file
    {
      printf(".disk file not closed\n");
    }
    return 0;
}


// method used to see if a file exists within the root
// returns location on disk if found
// returns 0 if not found
static long contains_file_root(char *name)
{
    int num_directories = 0;                             // counter for number of directories
    int i = 0;                                           // index into root
    long found = 0;                                      // holds directories location on disk
    cs1550_root_directory *root = malloc(sizeof(struct cs1550_root_directory));                          // holds root block
    int root_read = get_root_directory(root);  // get the root block
	if(root_read == -1)                        // unable to get root if -1
	{
      printf("Root not found\n");
      free(root);                              // free root
	  return 0;                                // return 0
	}
	num_directories = root->nDirectories;     // get number of directories in root
    while( i < num_directories)               // iterate through all directories
    {
      if(strcmp(name, root->directories[i].dname) == 0)     // if directory found in root
      {
        found = root->directories[i].nStartBlock;          // set found to directories index in disk
        break;                                             // break out of loop
      }
      i = i + 1;                                           // increment counter
    }
    free(root);                                            // free root
    return found;                                          // return found or not
}


// method checks to see if a file exists within the file system
// takes a directory name, file name, and extension name
// returns 1 if found, 0 if not found, and -1 on error
static int contains_file_directory(char *directory, char *file_name, char *ext, int *file_location)
{
	int num_directories = 0;                 // counter for number of directories
	int i = 0;                               // index for while loops
	int found_dir = 0;                       // boolean for if directory found
    int found_file = 0;                      // boolean for if file found
    long disk_ind = 0;                       // holds index to directory on disk
	cs1550_root_directory *root = malloc(sizeof(struct cs1550_root_directory));          // holds root block
    cs1550_directory_entry *dir = malloc(sizeof(struct cs1550_directory_entry));         // hold directory block
	int root_read = get_root_directory(root);  // get the root block

	if(root_read == -1)                        // unable to get root if -1
	{
	  free(root);                              // free root
      free(dir);                               // free directory
	  return -1;                               // return -1
	}
	num_directories = root->nDirectories;      // get number of directories
	while( i < num_directories)                // iterate through directories of root
	{
	  if(strcmp(directory, root->directories[i].dname) == 0)    // if directory matches one we are looking for
	  {
        disk_ind = root->directories[i].nStartBlock;            // set the disk_index to value
		found_dir = 1;                                          // set that directory found
		break;                                                  // break out of loop
	  }
	  i = i + 1;                                                // increment counter
	}
    if(found_dir)                                              // if directory has been found, look for file
    {
      if(get_directory(dir, disk_ind) != 0)                    // if unable to get directory
      {
        printf("ERROR getting directory\n");
        free(root);
        free(dir);
        return -1;
      }
      i = 0;                                                   // reset value of i
      while(i < dir->nFiles && !found_file)                    // iterate through directories files
      {
        if(strcmp(file_name, dir->files[i].fname) == 0)        // if file name matches file we are looking for
        {
          if(strcmp(ext, dir->files[i].fext) == 0)             // if extension matches extension we are looking for
          {
            printf("File found: %s.%s\n", file_name, ext);     // for testing
            found_file = 1;                                    // set file found
            *file_location = i;
            break;                                             // break out of loop
          }
        }
        i++;                                                   // increment counter
      }
    }
    free(dir);                            // free dir
	free(root);                           // free root
    return found_file;                    // return if file found or not
}

/*
 * Called whenever the system wants to know the file attributes, including
 * simply whether the file exists or not.
 *
 * man -s 2 stat will show the fields of a stat structure
 */


/////////////
static int cs1550_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
    char *directory = NULL;
    char *file = NULL;
    char *file_name = NULL;
    char *ext = NULL;
    char *token = NULL;
    int token_count = 0;
	int string_len_orig = strlen(path);
	char *original = malloc(sizeof(string_len_orig) + 20);
    int file_location = 0;
	strcpy(original, path);

	memset(stbuf, 0, sizeof(struct stat));

	//is path the root dir?
	if (strcmp(original, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {

	    //Check if name is subdirectory
        if(!find_contains_char(path, 47, 1))
        {
          printf("Cannot call strtok\n");
          free(original);
          return -ENOENT;
        }
        token = strtok(original, "/");              // get first token of path, which should be a directory
        directory = token;
        if(token != NULL)                        // loop to see how maky tokens in path
        {
          printf("DIRECTORY NAME: %s\n", directory);
          // if the file name or directory name is bigger than 8 characters, should not be in system
          token_count += 1;
		  if(strlen(token) > 8)
		  {
            printf("Directory %s too long\n", token);
            free(original);
		    return -ENOENT;
		  }
		  if(!find_contains_char(path, 47, 2))
		  {
			printf("Cannot call strtok\n");
		  }
          else
          {
            token = strtok(NULL, "/");                // get next token, which should be a file if exists
            if(token != NULL)
            {
              file = token;
              printf("FILE NAME: %s\n", file);
              token_count += 1;                        // increment tokens encountered
              if(strlen(token) > 12)
              {
                printf("File name is too long %s\n", token);
                free(original);
                return -ENAMETOOLONG;
              }
              int i = 0;
              char current_char = 0;
              while(i < strlen(file))
              {
                current_char = 0;
                if(current_char == 47)
                {
                  printf("Too many / in path: %s\n", file);
                  free(original);
                  return -ENOENT;
                }
                i += 1;
              }
            }
          }
        }
        else
        {
          printf("Error reading directory name\n");
          free(original);
          return -ENOENT;
        }
        if(file != NULL)
        {
	      if(!find_contains_char(file, 46, 1))
	      {
	        printf("Cannot call strtok\n");
	        free(original);
	        return -ENOENT;
	      }
          token = strtok(file, ".");
          file_name = token;
          if(token != NULL)
          {
            printf("FILENAME: %s\n", file_name);
            if(strlen(token) > 8)
            {
              printf("File name is too long %s\n", token);
              free(original);
              return -ENAMETOOLONG;
            }
		    if(file_name != NULL)
            {
			if(!find_contains_char(path, 47, 1))
	        {
	          printf("Cannot call strtok\n");
	          free(original);
	          return -ENAMETOOLONG;
	        }
            token = strtok(NULL, ".");
            if(token != NULL)
            {
              ext = token;
              if(strlen(token) > 3)
              {
                printf("Extension is too long %s\n", token);
                free(original);
                return -ENAMETOOLONG;
              }
              int i = 0;
              char current_char = 0;
              while(i < 3)
              {
                current_char = ext[i];
                if(current_char == 46)
                {
                  printf("Extension contains more than one period: %s\n", ext);
                  free(original);
                  return -ENAMETOOLONG;
                }
                i+=1;
              }
            }
            else
            {
			  printf("No extension for file\n");
              free(original);
              return -ENAMETOOLONG;
            }
            }
          }
          else
          {
              printf("No extension for file\n");
              free(original);
              return -ENAMETOOLONG;
          }
        }
        // will need to alter this because root may be able to contain files?
        if(token_count == 1)
        {
          if( contains_file_root(directory) )
          {
		    //Might want to return a structure with these fields
            printf("GETATTR: File found in: %s\n", directory);
		    stbuf->st_mode = S_IFDIR | 0755;
		    stbuf->st_nlink = 2;
		    res = 0; //no error
          }
          else
          {
            res = -ENOENT;
          }
        }
        else if(token_count == 2)     // this means the file is in the form directory/filename
        {
           if(!contains_file_directory(directory, file_name, ext, &file_location))
           {
            res = -ENOENT;
            printf("File does not exist!\n");
           }
           else
           {
		     //regular file, probably want to be read and write
             struct cs1550_directory_entry *dir = malloc(sizeof(struct cs1550_directory_entry));
             get_directory(dir, contains_file_root(directory));
             long file_size = dir->files[file_location].fsize;
		     stbuf->st_mode = S_IFREG | 0666;
		     stbuf->st_nlink = 1; //file links
		     stbuf->st_size = file_size; //file size - make sure you replace with real size!
		     res = 0; // no error
             free(dir);
             printf("GETATTR: File found %s.%s\n", file_name, ext);
           }
           // then need to gain access to direcotry and see if file exists
        }
        else
        {
          res = -ENOENT;
        }
	}
    free(original);
	return res;
}

// method writes root back to disk
// MAKE SURE WHEN WRITING USE rb+
static void write_disk_root(cs1550_root_directory *root)
{
	FILE *disk_data;                               // variable used to hold pointer to file
	disk_data = fopen(".disk", "rb+");              // open the file in write binary
	if(fseek(disk_data, 0, SEEK_SET))
	{
	  printf("Seek failed for writing root to disk");
	}
    fwrite(root, sizeof(struct cs1550_root_directory), 1, disk_data);
	if(fclose(disk_data))
    {
	  printf("File failed to close in write_disk_root\n");
    }
}

static void write_disk_bitmap(struct cs1550_bitmap *bitmap)
{
	FILE *disk_data;                               // variable used to hold pointer to file
  	disk_data = fopen(".disk", "rb+");              // open the file in write binary
  	if(fseek(disk_data, 5241344, SEEK_SET))
  	{
  	  printf("Seek failed for writing bitmap to disk");
  	}
    fwrite(bitmap, sizeof(struct cs1550_bitmap), 1, disk_data);
  	if(fclose(disk_data))
    {
  	  printf("File failed to close in write_disk_bitmap\n");
    }
}

// method writes a block back to disk
// MAKE SURE WHEN WRITING USE rb+
static void write_disk_directory(cs1550_directory_entry *dir, long index)
{
    FILE *disk_data;                        // variable used to hold pointer to file
    disk_data = fopen(".disk", "rb+");       // open the file to write binary
	if(check_index(index))
    {
      printf("DISK INDEX IS NOT A MULTIPLE OF 512\n");
    }
    if(fseek(disk_data, index, SEEK_SET))
    {
      printf("Seek failed for writing directory to disk");
    }
    fwrite(dir, sizeof(struct cs1550_directory_entry), 1, disk_data);
    if(fclose(disk_data))
    {
      printf("File failed to close in write_disk_directory\n");
    }
}

// method writes a block back to disk
// MAKE SURE WHEN WRITING USE rb+
static void write_disk_file_block(cs1550_disk_block *block, long index)
{
    FILE *disk_data;                        // variable used to hold pointer to file
    disk_data = fopen(".disk", "rb+");       // open the file to write binary
	if(check_index(index))
    {
      printf("DISK INDEX IS NOT A MULTIPLE OF 512\n");
    }
    if(fseek(disk_data, index, SEEK_SET))
    {
      printf("Seek failed for writing file block to disk");
    }
    fwrite(block, sizeof(struct cs1550_disk_block), 1, disk_data);
    if(fclose(disk_data))
    {
      printf("File failed to close in write_disk_file_block\n");
    }
}

/*
 * Called whenever the contents of a directory are desired. Could be from an 'ls'
 * or could even be when a user hits TAB to do autocompletion
 */
static int cs1550_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	//Since we're building with -Wall (all warnings reported) we need
	//to "use" every parameter, so let's just cast them to void to
	//satisfy the compiler
	(void) offset;
	(void) fi;
	int string_len_orig = strlen(path);
	char *original = malloc(sizeof(string_len_orig) + 20);
	strcpy(original, path);

    if(strcmp(original, "/") == 0)
    {
	  //the filler function allows us to add entries to the listing
	  //read the fuse.h file for a description (in the ../include dir)
	  filler(buf, ".", NULL, 0);
	  filler(buf, "..", NULL, 0);


      int num_directories = 0;
	  int i = 0;
      int boolean = 0;
	  cs1550_root_directory *root = malloc(sizeof(struct cs1550_root_directory));                          // holds root block
	  int root_read = get_root_directory(root);  // get the root block
	  if(root_read == -1)                                     // unable to get root if null
	  {
        free(root);
        free(original);
	    return -ENOENT;
	  }
	  num_directories = root->nDirectories; // get number of directories
	  while( i < num_directories && !boolean)
	  {
	    boolean = filler(buf, root->directories[i].dname, NULL, 0);
	    i = i + 1;
	  }
      free(root);
     }
     else
     {
	   filler(buf, ".", NULL, 0);
	   filler(buf, "..", NULL, 0);
       char *directory;
       char *file = NULL;
       char *file_name = NULL;
       char *ext;
       char *token;
       int num_tokens = 0;
       int num_files = 0;
       int i = 0;
       int boolean = 0;
	   if(!find_contains_char(original, 47, 1))
	   {
		 printf("Cannot call strtok\n");
		 free(original);
		 return -ENOENT;
	   }
       directory = strtok(original, "/");
       if(directory != NULL)
       {
         num_tokens += 1;
         if(strlen(directory) > 8)
         {
           printf("Directory name is too long\n");
           free(original);
           return -ENOENT;
         }
		 if(!find_contains_char(path, 47, 2))
		 {
		   printf("Cannot call strtok\n");
		 }
         else
         {
           token = strtok(NULL, "/");
           if(token != NULL)
           {
             file = token;
             num_tokens += 1;
             if(strlen(token) > 12)
             {
               printf("File name is too long %s\n", token);
               free(original);
               return -ENAMETOOLONG;
             }
		     int i = 0;
             char current_char = 0;
             while(i < strlen(file))
             {
               current_char = 0;
               if(current_char == 47)
               {
	             printf("Too many / in path: %s\n", file);
	             free(original);
	             return -ENOENT;
               }
               i += 1;
             }
           }

         }
       }
       else
       {
         printf("Error reading directory name\n");
         free(original);
         return -ENOENT;
       }
       if(file != NULL)
       {
		   if(!find_contains_char(file, 46, 1))
		   {
			 printf("Cannot call strtok\n");
			 free(original);
			 return -ENOENT;
		   }
		   token = strtok(file, ".");
	       file_name = token;
	       if(token != NULL)
	       {
	         if(strlen(token) > 8)
	         {
		       printf("File name is too long %s\n", token);
		       free(original);
		       return -ENAMETOOLONG;
	         }
			 if(!find_contains_char(path, 46, 1))
			 {
			   printf("Cannot call strtok\n");
			   free(original);
			   return -ENAMETOOLONG;
			 }
	         token = strtok(NULL, ".");
	         if(token != NULL)
	         {
		       ext = token;
		       if(strlen(token) > 3)
		       {
		         printf("Extension is too long %s\n", token);
		         free(original);
		         return -ENAMETOOLONG;
		       }
		       int i = 0;
		       char current_char = 0;
		       while(i < 3)
		       {
		         current_char = ext[i];
		         if(current_char == 46)
		         {
			       printf("Extension contains more than one period: %s\n", ext);
			       free(original);
			       return -ENAMETOOLONG;
		         }
		         i+=1;
		       }
	         }
			 else
			 {
			   printf("No extension for file\n");
			   free(original);
			   return -ENAMETOOLONG;
			 }
	       }
		   else
		   {
			 printf("No extension for file\n");
			 free(original);
			 return -ENAMETOOLONG;
		   }
           int file_location = 0;
           if(contains_file_directory(directory, file_name, ext, &file_location))
           {
             printf("Writing to buffer %s\n", file);
             filler(buf, file , NULL, 0);
             free(original);
             return 0;
           }
           else
           {
             printf("File does not exist\n");
             free(original);
             return -ENOENT;
           }
         }
       cs1550_directory_entry *dir_ptr = malloc(sizeof(struct cs1550_root_directory));
       long dir_ind= contains_file_root(directory);
       if(dir_ind == 0)
       {
         printf("Could not find directory\n");
         free(dir_ptr);
         free(original);
         return -ENOENT;
       }
       int dir_read = get_directory(dir_ptr, dir_ind);
       if(dir_read == -1)
       {
         free(dir_ptr);
         free(original);
         return -ENOENT;
       }

       char file_name_dir[20];
       char ext_dir[10];
       char period[] = ".";
       num_files = dir_ptr->nFiles;
       while( i < num_files && !boolean)
       {
         char *to_buf = NULL;
         int size_file;
         int size_ext;
         strncpy(file_name_dir, dir_ptr->files[i].fname, 9);
         size_file = strlen(file_name_dir);
         strncpy(ext_dir, dir_ptr->files[i].fext, 4);  // changed from 4
         size_ext = strlen(ext_dir);
         to_buf = malloc(size_file + size_ext + 20);
         strcat(file_name_dir, period);
         strcat(file_name_dir, ext_dir);
         strcpy(to_buf, file_name_dir);
         boolean = filler(buf, to_buf , NULL, 0);
         free(to_buf);
         i = i + 1;
       }
       free(dir_ptr);
     }
    free(original);
	return 0;
}


/*
 * Creates a directory. We can ignore mode since we're not dealing with
 * permissions, as long as getattr returns appropriate ones for us.
 */
static int cs1550_mkdir(const char *path, mode_t mode)
{
	int string_len_orig = strlen(path);                      // get lenght of path
	char *original = malloc(sizeof(string_len_orig) + 20);        // create a pointer to a string of paths size
	strcpy(original, path);                                  // copy path to original
	(void) path;
	(void) mode;
    char *token;
    int token_count = 0;                // used for counter of tokens
    char nul_dir[MAX_FILENAME + 10];     // used to null terminate string
    int new_block = 0;
    int bitmap_index = 0;
    unsigned char bitmap_val = 0;

	long new_index = 0;                 // used to hold where to place new directory in disk
    char *dir_name;                     // used to hold directory name
    cs1550_directory_entry *new_dir = malloc(sizeof(struct cs1550_directory_entry));                      // used to hold information for new directory
    cs1550_root_directory *root = malloc(sizeof(struct cs1550_root_directory));                          // holds root block
    struct cs1550_bitmap *bitmap = malloc(sizeof(struct cs1550_bitmap));                            // holds the bit map
    int root_read = get_root_directory(root);  // get the root block
    if(root_read == -1)                                     // unable to get root if null
    {
	  free(root);                                          // free the malloced root region
  	  free(new_dir);                                       // free the malloced new_dir region
  	  free(bitmap);
      free(original);
      return -1;
    }

    if(root->nDirectories == MAX_DIRS_IN_ROOT)           // if the number of directories in root at max, return -1
    {
	  free(root);                                          // free the malloced root region
  	  free(new_dir);                                       // free the malloced new_dir region
  	  free(bitmap);
      free(original);
      return -1;
    }

    // the following section checks to see if trying to create directory not under root
    // may have to check to make sure first token actually root?
	if(!find_contains_char(original, 47, 1))
	{
	  printf("Cannot call strtok\n");
	  free(original);
      free(new_dir);
      free(bitmap);
	  return -EPERM;
	}
	token = strtok(original, "/");                            // get the first token
    dir_name = token;									  // first token should be directory name
	while(token != NULL)                                  // keep scanning until no more tokens
	{
	  token = strtok(NULL, "/");                          // get the next token, NULL if not one
	  token_count += 1;                                   // increment number of tokens found
      if(token_count >= 2)                                // no need to keep scanning past 2
      {
        break;
      }
	}
    if(token_count != 1)                                  // if more than one token discovered, file not only in root
    {
	  free(root);                                          // free the malloced root region
	  free(new_dir);                                       // free the malloced new_dir region
	  free(bitmap);
      free(original);
      return -EPERM;                                      // return error code for not in root
    }

    // if here, trying to create directory in root
    // checking lenght of name for new directory
    if(strlen(dir_name) > 8)                                    // check the strings lenght
    {
	  free(root);                                          // free the malloced root region
	  free(new_dir);                                       // free the malloced new_dir region
	  free(bitmap);
      free(original);
      return -ENAMETOOLONG;                               // return error code for string too long
    }

    strncpy(nul_dir, dir_name, 8);               // to null terminate the string
    nul_dir[8] = '\0';                          // set last character to null terminator

    // now checking to see if directory name already exists
    if(contains_file_root(nul_dir))                    // checks to see if file name exists
    {
      printf("Cannot create directory %s as it already exists\n", nul_dir);
	  free(root);                                          // free the malloced root region
	  free(new_dir);                                       // free the malloced new_dir region
	  free(bitmap);
      free(original);
      return -EEXIST;
    }

    get_bitmap(bitmap);
	if(root->nDirectories == 0)                        // initialize bitmap
	{
	  unsigned char value = bitmap->blocks[0];                  // get the first block
      unsigned char new_value = set_bit(value, 0);
      bitmap->blocks[0] = new_value;                 // place char back in bit map
      value = bitmap->blocks[1279];                  // get the last byte
      new_value = set_bit(value, 5);                  // set bits for bit map
      new_value = set_bit(new_value, 6);
      new_value = set_bit(new_value, 7);
      bitmap->blocks[1279] = new_value;              // place char back in bitmap
	}

    new_block = find_empty_block_index(bitmap, 0);
    printf("MKDIR: Placing new directory in block: %d\n", new_block);
    if(new_block == -1)
    {
      free(root);
      free(new_dir);
      free(bitmap);
      free(original);
      return -ENOMEM;
    }
	new_index = 512 * new_block;         // get where to place directory on disk
    printf("MKDIR: Starting index of new directory on disk: %ld\n", new_index);
    bitmap_index = new_block / 8;                                       // get which byte to read from bitmap
    bitmap_val = bitmap->blocks[bitmap_index];                          // get the byte from bitmap
    printf("MKDIR: Bitmap byte: %d Byte value: %x Bit to set: %d\n", bitmap_index, bitmap_val, new_block % 8 );
    bitmap_val = set_bit(bitmap_val, new_block % 8);                        // set bit in bitmap for allocated block
    bitmap->blocks[bitmap_index] = bitmap_val;                          // set new bitmap value in bitmap
    strncpy(root->directories[root->nDirectories].dname, nul_dir, 9);   // set directory name in root directory
    root->directories[root->nDirectories].nStartBlock = new_index;       // set where directory is in .disk
    root->nDirectories = root->nDirectories + 1;                  // increment number of directories in root
    new_dir->nFiles = 0;                                         // set number of files in new directory to 0
    write_disk_root(root);                                       // write updated root to disk
    write_disk_directory(new_dir, new_index);                    // write a directory to disk
    write_disk_bitmap(bitmap);
    free(root);                                          // free the malloced root region
    free(new_dir);                                       // free the malloced new_dir region
    free(bitmap);
    free(original);

	return 0;
}

/*
 * Removes a directory.
 */
static int cs1550_rmdir(const char *path)
{
	(void) path;
    return 0;
}


/*
 * Does the actual creation of a file. Mode and dev can be ignored.
 *
 */
static int cs1550_mknod(const char *path, mode_t mode, dev_t dev)
{
    printf("MKNOD\n");
    int string_len_orig = strlen(path);         // get lenght of path
    char *original = malloc(sizeof(string_len_orig) + 20);  // malloc a region for a string
    strcpy(original, path);               // copy the path to original
    // not sure why I had to do this but path was getting messed up by strtok
	(void) mode;
	(void) dev;
    char *token = NULL;               // used to hold strings when broken up
    char *dir_name = NULL;            // used to hold directory name
    char *file = NULL;                // used to hold file when tokenizing
    char *ext = NULL;                 // used to hold extension
    char *file_name = NULL;           // used to hold file name without extension
    int token_count = 0;              // used for counter of tokens
    long dir_index = 0;               // holds index for directory on disk
    char new_file[MAX_FILENAME + 1];  // used to hold new file name
    char new_ext[MAX_EXTENSION + 1];  // used to hold extension of file

    cs1550_directory_entry *file_dir = malloc(sizeof(struct cs1550_directory_entry));       // used to place directory here
    cs1550_root_directory *root = malloc(sizeof(struct cs1550_root_directory));             // used to hold root directory
    int file_location = 0;

    int root_read = get_root_directory(root);  // get the root block
    if(root_read == -1)                  // if unable to get root
    {
      free(file_dir);
      free(root);
      free(original);
      return ENOENT;
    }

    // check the path to make sure it contains a directory/file
	if(!find_contains_char(original, 47, 1))
	{
	  printf("Cannot call strtok\n");
	  free(original);
      free(file_dir);
      free(original);
	  return -ENOENT;
	}
	token = strtok(original, "/");           // get first token of path, which should be a directory
	dir_name = token;                        // first token should be a directory
	if(token != NULL)                     // loop to see how maky tokens in path
	{
	  // if the file name or directory name is bigger than 8 characters, should not be in system
      // may not have to check this here?
      token_count += 1;
	  if(strlen(token) > 8)
	  {
        free(original);
        free(file_dir);
        free(root);
		return -ENOENT;
	  }
	  if(!find_contains_char(path, 47, 2))
	  {
		printf("Cannot call strtok\n");
	  }
      else
      {
	    token = strtok(NULL, "/");                // get next token, which should be a file name
        if(token != NULL)
        {
	      file = token;                             // set token to file name
	      token_count += 1;                         // increment tokens encountered
		  if(strlen(token) > 12)
          {
            printf("File name is too long %s\n", token);
            free(original);
            free(file_dir);
            free(root);
            return -ENAMETOOLONG;
          }
          int i = 0;
          char current_char = 0;
          while(i < strlen(file))
          {
            current_char = 0;
            if(current_char == 47)
            {
	          printf("Too many / in path: %s\n", file);
	          free(original);
              free(file_dir);
              free(root);
	          return -ENOENT;
            }
            i += 1;
          }
        }
      }
	  if(token_count != 2)                       // if token_count incremented more than once
	  {                                         // it means there were too many / in the path
        free(original);
		free(file_dir);
		free(root);
		return -EPERM;
	  }
	}

    if(dir_name == NULL)       // if trying to create file in directory
    {
      free(original);
	  free(file_dir);
	  free(root);
      return -EPERM;
    }

    // this part checks the file name to make sure in correct format
	if(!find_contains_char(file, 46, 1))
	{
	  printf("Cannot call strtok\n");
	  free(original);
      free(file_dir);
      free(root);
	  return -ENOENT;
	}
    token = strtok(file, ".");             // get file name, without extension
    file_name = token;                     // first token should be file name
    if(token != NULL)                   // keep reading tokens until no more
    {
      if(strlen(token) > 8)  // checking lenght for file name
      {
		free(file_dir);
		free(root);
        free(original);
        return -ENAMETOOLONG;
      }
	  if(!find_contains_char(path, 46, 1))
	  {
		printf("Cannot call strtok\n");
		free(original);
		return -ENAMETOOLONG;
	  }
      token = strtok(NULL, ".");            // read the next token
      if(token != NULL)
      {
		ext = token;
		if(strlen(token) > 3)
		{
		  printf("Extension is too long %s\n", token);
          free(file_dir);
          free(root);
		  free(original);
		  return -ENAMETOOLONG;
		}
		int i = 0;
		char current_char = 0;
		while(i < 3)
		{
		  current_char = ext[i];
		  if(current_char == 46)
		  {
		    printf("Extension contains more than one period: %s\n", ext);
		    free(original);
            free(root);
            free(file_dir);
		    return -ENAMETOOLONG;
		  }
		  i+=1;
		}
      }
      else
      {
		free(file_dir);
		free(root);
        free(original);
        return -ENAMETOOLONG;
      }
    }

    if(file_name == NULL)                   // if null returned by first read
    {
	  free(file_dir);
	  free(root);
      free(original);
      return -ENOENT;
    }

    // passed error checking for path names
    dir_index = contains_file_root(dir_name);     // get the index of the directory on the disk
    if(!dir_index)                                // if directory does not exist
    {
	  free(file_dir);
	  free(root);
      free(original);
      return -ENOENT;
    }
    get_directory(file_dir, dir_index);           // set file_dir to the directory
    if(contains_file_directory(dir_name, file_name, ext, &file_location))  // see if file already exists
    {
      free(original);
	  free(file_dir);
	  free(root);
      return -EEXIST;
    }

    if(file_dir->nFiles == MAX_FILES_IN_DIR)            // check to see if directory can add new file
    {
      printf("No more room for another file in directory\n");
      free(original);
	  free(file_dir);
	  free(root);
      return -ENOMEM;
    }

    strncpy(new_file, file_name, 8);                   // copy the file name into new_file variable
    new_file[8] = '\0';                                // null terminate the string
    strncpy(new_ext, ext, 3);                          // copy the new extension into new_ext variable
    new_ext[3] = '\0';                                 // null terminate the extension
    strncpy(file_dir->files[file_dir->nFiles].fname, new_file, 9);         // set file name
    strncpy(file_dir->files[file_dir->nFiles].fext, new_ext, 4);           // set file extension
    file_dir->files[file_dir->nFiles].nStartBlock = 0;                     // set where first file block is
    file_dir->files[file_dir->nFiles].fsize = 0;                           // set the files size
    file_dir->nFiles = file_dir->nFiles + 1;                               // increment number of files in directory
    write_disk_directory(file_dir, dir_index);                             // write updated directory to disk

    free(file_dir);                   // free malloced region
    free(root);                       // free malloced region
    free(original);                   // free the copied path
	return 0;
}

/*
 * Deletes a file
 */
static int cs1550_unlink(const char *path)
{
    (void) path;

    return 0;
}


/*
 * Read size bytes from file into buf starting from offset
 *
 */
static int cs1550_read(const char *path, char *buf, size_t size, off_t offset,
			  struct fuse_file_info *fi)
{
    printf("IN READ\n");
	(void) buf;
	(void) offset;
	(void) fi;
	(void) path;

	//check to make sure path exists
	char *directory = NULL;                      // holds directories name
    char *file = NULL;                           // holds files name
    char *ext = NULL;                            // holds extensions name
    int split_ret = 0;                           // holds return value of split_path
    long dir_index = 0;                          // holds directory index on disk
    cs1550_directory_entry *file_dir = NULL;           // used to hold directory struct
    int file_index = 0;                       // used to hold file index into direcotry if exists


    //check to make sure path exists
    split_ret = split_path(path, &directory, &file, &ext);   // split up path
    if(split_ret)                             // if anything but 0
    {
      printf("Split path returned a non-zero value\n");      // for testing
      return 0;
    }
    file_dir = malloc(sizeof(struct cs1550_directory_entry));       // allocate space for directory struct
    // passed error checking for path names
    //printf("Here4 %s\n", directory);
    dir_index = contains_file_root(directory);     // get the index of the directory on the disk
    if(dir_index == 0)                                // if directory does not exist
    {
      printf("Directory not found in write\n");  // for testing
      free(file_dir);
	  free(directory);
	  free(file);
	  free(ext);
      return 0;
    }

    get_directory(file_dir, dir_index);           // set file_dir to the directory
    if(!contains_file_directory(directory, file, ext, &file_index))  // see if file already exists
    {
      printf("File does not exist!\n");
      free(file_dir);
	  free(directory);
	  free(file);
	  free(ext);
      return 0;
    }
	//check that size is > 0
	if(size <= 0)
    {
      printf("Write size is less than or equal to 0\n");
      free(file_dir);
	  free(directory);
	  free(file);
	  free(ext);
      return 0;                         // not sure what to return here..
    }
	//check that offset is < to the file size
	if(offset > file_dir->files[file_index].fsize)
    {
       free(file_dir);
       free(directory);
       free(file);
       free(ext);
       return 0;
    }
	//read in data
    if(file_dir->files[file_index].fsize == 0)
    {
      free (file_dir);
	  free(directory);
	  free(file);
	  free(ext);
      return 0;
    }

	FILE *disk_data;                               // variable used to hold pointer to file
	disk_data = fopen(".disk", "rb");              // open the file in read binary
	if(disk_data == NULL)                          // make sure the file was opened
	{
      free(file_dir);
	  free(directory);
	  free(file);
	  free(ext);
      return 0;
	}
    fseek(disk_data, file_dir->files[file_index].nStartBlock + offset, SEEK_SET);
    if(offset + size > file_dir->files[file_index].fsize)
      size = file_dir->files[file_index].fsize - offset;
	fread(buf, 1,size, disk_data);  // read the number of directories
    fseek(disk_data, 0, SEEK_SET);

	fclose(disk_data);                       // close the file
	free(directory);
	free(file);
	free(ext);
	free(file_dir);

	return size;  // should be size
}



// method will set several bits when at a starting location and given size
static void set_multiple_bits(struct cs1550_bitmap *bitmap, int location, int size)
{
	int index = 0;
    int i = location % 8;
    int bitmap_index = location / 8;
    unsigned char current = 0;
    while(index < size)
    {
      printf("In set_multiple_bits: byte index from bitmap to get %d number of bits altered so far: %d\n", bitmap_index, index);
      current = bitmap->blocks[bitmap_index];            // get a byte from bit map
       printf("In set_multiple_bits: setting bit %d of the byte %x\n", i, current);
      current = set_bit(current, i);                     // set the bit
      printf("In set_multiple_bits: new byte value %x\n", current);
      bitmap->blocks[bitmap_index] = current;             // place byte back in bitmap
      i += 1;                                            // increment bit index
      index += 1;                                        // increment number of bits updated
      if(i == 8)                                         // if at last byte in block
      {
        i = 0;                                           // reset i
        bitmap_index += 1;                               // get next byte in bit map
      }
    }
}

static void copy_old_blocks(long old_index, long old_size, long new_index)
{
  struct cs1550_disk_block *block = malloc(sizeof(struct cs1550_disk_block));
  long iterations = old_size / 512;
  int i = 0;
  while(i < iterations)
  {
    printf("COPY OLD BLOCKS: number of blocks written: %d writing block: %ld to block: %ld\n", i, old_index, new_index);
    get_file_block(block, old_index);
    write_disk_file_block(block, new_index);
    i++;
    old_index += 512;
    new_index += 512;
  }
  free(block);
}

/*
 * Write size bytes from buf into file starting from offset
 *
 */
static int cs1550_write(const char *path, const char *buf, size_t size,
			  off_t offset, struct fuse_file_info *fi)
{
  //printf("WRITE\n");
  (void) buf;
  (void) offset;
  (void) fi;
  (void) path;
  // buf is what to write
  // size is size of what to write
  // offset is where in file to write, offset from beginning
  //

  char *directory = NULL;                      // holds directories name
  char *file = NULL;                           // holds files name
  char *ext = NULL;                            // holds extensions name
  int split_ret = 0;                           // holds return value of split_path
  long dir_index = 0;                          // holds directory index on disk
  cs1550_directory_entry *file_dir = NULL;           // used to hold directory struct
  int file_index = 0;                       // used to hold file index into direcotry if exists
  long file_size = 0;


  //check to make sure path exists
  split_ret = split_path(path, &directory, &file, &ext);   // split up path
  if(split_ret)                             // if anything but 0
  {
    if(directory != NULL)
      free(directory);
    if(file != NULL)
      free(file);
    if(ext != NULL)
      free(ext);
    return split_ret;
  }
  file_dir = malloc(sizeof(struct cs1550_directory_entry));       // allocate space for directory struct
  // passed error checking for path names
  dir_index = contains_file_root(directory);     // get the index of the directory on the disk
  if(dir_index == 0)                                // if directory does not exist
  {
    printf("WRITE: Directory not found in write\n");  // for testing
	free(directory);
	free(file);
	free(ext);
    free(file_dir);
    return -ENOENT;
  }

  get_directory(file_dir, dir_index);           // set file_dir to the directory
  if(!contains_file_directory(directory, file, ext, &file_index))  // see if file already exists
  {
    printf("WRITE: File does not exist!\n");
    free(file_dir);
	free(directory);
	free(file);
	free(ext);
    return -ENOENT;
  }
  //check that size is > 0
  if(size <= 0)
  {
    printf("Write size is less than or equal to 0\n");
    free(file_dir);                         // free directory before returning
	free(directory);
	free(file);
	free(ext);
    return -EINVAL;                         // not sure what to return here..
  }
  //check that offset is <= to the file size
  // need to handle this more....
  file_size = file_dir->files[file_index].fsize / 512;
  if(file_dir->files[file_index].fsize % 512 != 0)
    file_size += 1;
  if((file_size * 512) <= offset + size) // may need to fix here..
  {
    struct cs1550_bitmap *bitmap = malloc(sizeof(struct cs1550_bitmap));
    get_bitmap(bitmap);                                 // get the bitmap
    if(file_size == 0)         // if no blocks allocated for file
    {
      // printf("In write: file size is 0, inside if\n");
       int new_start_block = 0;                         // used to hold index of block on disk
       int num_blocks = 0;                              // used to hold number of blocks needed
       long disk_index = 0;                             // index on disk
       num_blocks = (offset + size) / 512;              // divide size by 512
       if((offset+size) % 512 != 0)
         num_blocks += 1;                               // add 1 to size if not perfect fit
       printf("In zero file size: number of blocks to allocate: %d\n", num_blocks);
       new_start_block =  find_contiguous_blocks_anywhere(bitmap,num_blocks);
       printf("In zero file size: block number to place file: %d\n", new_start_block);
       if(new_start_block == -1)                             // if unable to find space for file
       {
         printf("Unable to find contiguous region to place file\n");        // for testing
         free(file_dir);                               // free the directory
         free(bitmap);                                 // free the bit map
		 free(directory);
		 free(file);
		 free(ext);
         return -EFBIG;
       }
       disk_index = new_start_block * 512;                        // get actual index on disk
       printf("In zero file size: actual disk location to place block %ld\n", disk_index);
       file_dir->files[file_index].fsize = offset + size;      // change size to number of blocks allocated
       file_dir->files[file_index].nStartBlock = disk_index;      // set where file starts on disk
       set_multiple_bits(bitmap, new_start_block, num_blocks);    // set bitmap bits
       write_disk_bitmap(bitmap);
       write_disk_directory(file_dir, dir_index);
       free(bitmap);                                              // free the bitmap
    }
    else
    {
      long current_size = file_dir->files[file_index].fsize;    // get current size of file
      printf("Inside write: file size is %ld\n", current_size);
      long new_size = (offset + size) - (file_size * 512);                    // get home much space is needed
      printf("Inside write: space needed in bytes %ld\n", new_size);
      int new_num_blocks = new_size / 512;                      // get how many new blocks needed
      if(new_size % 512 != 0)                                   // if not divisible by 512 add 1
        new_num_blocks += 1;
      printf("Inside write: number of blocks that need allocated: %d\n", new_num_blocks);
      long last_old_block = file_dir->files[file_index].nStartBlock;  // get location of old block start
      printf("Inside write: index on disk of where file currently begins: %ld\n", last_old_block);
      last_old_block = last_old_block + (file_size * 512);                 // go to last block of old location
      printf("Inside write: index on disk of end of file currently: %ld\n", last_old_block);
      last_old_block = last_old_block / 512;                          // get index for bitmap
      printf("Inside write: index of last block for bitmap: %ld\n", last_old_block);
      int contiguous_block = find_contiguous_blocks(bitmap, last_old_block, new_num_blocks);  // look for contiguous from current
      if(contiguous_block == 1)
      {
        printf("Inside write: contiguous blocks found\n");
		file_dir->files[file_index].fsize = size + offset;      // change size to number of blocks allocated
		set_multiple_bits(bitmap, last_old_block, new_num_blocks);    // set bitmap bits
		write_disk_bitmap(bitmap);
		write_disk_directory(file_dir, dir_index);
		free(bitmap);                                              // free the bitmap
      }
      else if(contiguous_block != 1)
      {
        printf("Inside write: contiguous blocks not found\n");
        int new_start_block = 0;
        int num_blocks = 0;
        long disk_index = 0;
        num_blocks = (offset + size) / 512;
        if((offset+size) % 512 != 0)
          num_blocks += 1;
        printf("Inside write: number of blocks that need to be allocated: %d\n", num_blocks);
	    new_start_block =  find_contiguous_blocks_anywhere(bitmap,num_blocks);
        printf("Inside write: block index to place file: %d\n", new_start_block);
	    if(new_start_block == -1)                             // if unable to find space for file
	    {
		  printf("Unable to find contiguous region to place file\n");        // for testing
		  free(file_dir);                               // free the directory
		  free(bitmap);                                 // free the bit map
		  free(directory);
		  free(file);
		  free(ext);
		  return -EFBIG;
	    }
	    disk_index = new_start_block * 512;                        // get actual index on disk
        printf("Inside write: new index on disk: %ld\n", disk_index);
        long old_index = file_dir->files[file_index].nStartBlock;
        printf("Inside write: old index on disk: %ld\n", old_index);
        long old_size = file_dir->files[file_index].fsize;
        printf("Inside write: old file size in directory in bytes: %ld\n", old_size);
        copy_old_blocks(old_index, (file_size * 512), disk_index);         // copy old blocks to new space
        long old_start_block = old_index / 512;                   // get old block number
        printf("Inside write: old bitmap beginning index for file: %ld\n", old_start_block);
        long old_num_blocks = file_size;                     // get old number of blocks
        printf("Inside write: calling unset bits\n");
        set_multiple_bits(bitmap, old_start_block, old_num_blocks);  // unset old bits
	    file_dir->files[file_index].fsize = size + offset;      // change size to number of blocks allocated
	    file_dir->files[file_index].nStartBlock = disk_index;      // set where file starts on disk
        printf("Inside write: setting new bits for file\n");
	    set_multiple_bits(bitmap, new_start_block, num_blocks);    // set bitmap bits
	    write_disk_bitmap(bitmap);
	    write_disk_directory(file_dir, dir_index);
	    free(bitmap);                                              // free the bitmap
      }
    }
  }
  //write data
  printf("WRITE: going to write file to disk now\n");
  struct cs1550_disk_block *block = malloc(sizeof(struct cs1550_disk_block));
  long start_index = 0;                                     // used to hold index of block on disk
  int file_block = 0;                                       // used to hold which file block to access
  long bytes_written = 0;                                  // used to keep track of how many blocks have been written
  int block_index = 0;                                      // which byte to write to in the block
  int file_read = 0;                                        // used to see if invalid file block read
  char byte_write = 0;                                           // used to hold byte to write

  start_index = file_dir->files[file_index].nStartBlock;    // get the position of the start block
  file_block = offset / 512;                                // divide offset by 512 to get which block to start at
  start_index = start_index + (512 * file_block);          // change start index to correct block
  block_index = offset % 512;                               // set block_index
  file_read = get_file_block(block, start_index);
  if(file_read)                                    // see if block read correctly
  {
	printf("Unable to access block to write\n");          // for testing
	free(file_dir);
    free(block);
	free(directory);
	free(file);
	free(ext);
	return -EFBIG;                                 // needs changed
  }
  while(bytes_written < size)                              // loop until all bytes written
  {
    byte_write = buf[bytes_written];                // get the byte to write
    block->data[block_index] = byte_write;          // set byte in block
    bytes_written += 1;                            // increment number of bytes written
    block_index  += 1;                              // increment index in block
    if(block_index == 512)  // if at end of block and not done
    {
      write_disk_file_block(block, start_index);    // write block back to disk
      block_index = 0;                              // reset block index
      if(bytes_written < size)
      {
        start_index += 512;                           // change index to next block
	    file_read = get_file_block(block, start_index);  // get next block
	    if(file_read)                                    // see if block read correctly
	    {
		  //printf("Unable to access block to write\n");          // for testing
		  free(file_dir);
		  free(block);
		  free(directory);
		  free(file);
		  free(ext);
		  return -EFBIG;                                 // needs changed
	    }
      }
    }
    if(bytes_written == size)
    {
      write_disk_file_block(block, start_index);    // write block back to disk
    }

  }
	//set size (should be same as input) and return, or error
    file_dir->files[file_index].fsize = offset + bytes_written;
    write_disk_directory(file_dir, dir_index);
    size = bytes_written;

	free(file_dir);
	free(block);
    //printf("FREE DIRECORY\n");
	free(directory);
	free(file);
    //printf("FREE EXT\n");
	free(ext);
	return size;
}

/******************************************************************************
 *
 *  DO NOT MODIFY ANYTHING BELOW THIS LINE
 *
 *****************************************************************************/

/*
 * truncate is called when a new file is created (with a 0 size) or when an
 * existing file is made shorter. We're not handling deleting files or
 * truncating existing ones, so all we need to do here is to initialize
 * the appropriate directory entry.
 *
 */
static int cs1550_truncate(const char *path, off_t size)
{
	(void) path;
	(void) size;

    return 0;
}


/*
 * Called when we open a file
 *
 */
static int cs1550_open(const char *path, struct fuse_file_info *fi)
{
	(void) path;
	(void) fi;
    /*
        //if we can't find the desired file, return an error
        return -ENOENT;
    */

    //It's not really necessary for this project to anything in open

    /* We're not going to worry about permissions for this project, but
	   if we were and we don't have them to the file we should return an error

        return -EACCES;
    */

    return 0; //success!
}

/*
 * Called when close is called on a file descriptor, but because it might
 * have been dup'ed, this isn't a guarantee we won't ever need the file
 * again. For us, return success simply to avoid the unimplemented error
 * in the debug log.
 */
static int cs1550_flush (const char *path , struct fuse_file_info *fi)
{
	(void) path;
	(void) fi;

	return 0; //success!
}


//register our new functions as the implementations of the syscalls
static struct fuse_operations hello_oper = {
    .getattr	= cs1550_getattr,
    .readdir	= cs1550_readdir,
    .mkdir	= cs1550_mkdir,
	.rmdir = cs1550_rmdir,
    .read	= cs1550_read,
    .write	= cs1550_write,
	.mknod	= cs1550_mknod,
	.unlink = cs1550_unlink,
	.truncate = cs1550_truncate,
	.flush = cs1550_flush,
	.open	= cs1550_open,
};

//Don't change this.
int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &hello_oper, NULL);
}
