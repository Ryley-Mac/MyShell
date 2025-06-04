/**
 * @file myshell.c
 * @author ** Ryley MacLagan **
 * @date ** 4/28/24 **
 * @brief Acts as a simple command line interpreter.  It reads commands from
 *        standard input entered from the terminal and executes them. The
 *        shell does not include any provisions for control structures,
 *        redirection, background processes, environmental variables, pipes,
 *        or other advanced properties of a modern shell. All commands are
 *        implemented internally and do not rely on external system programs.
 *
 */

#include <pwd.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFFER_SIZE          256
#define MAX_PATH_LENGTH      256
#define MAX_FILENAME_LENGTH  256

static char buffer[BUFFER_SIZE] = {0};
static char filename[MAX_FILENAME_LENGTH] = {0};

// Implements various UNIX commands using POSIX system calls
// Each must return an integer value indicating success or failure
int do_cat(const char* filename);
int do_cd(char* dirname);
int do_ls(const char* dirname);
int do_mkdir(const char* dirname);
int do_pwd(void);
int do_rm(const char* filename);
int do_rmdir(const char* dirname);
int do_stat(char* filename);
int execute_command(char* buffer);
  
/**
 * @brief  Removes extraneous whitespace at the end of a command to avoid
 *         parsing problems
 * @param  Char array representing the command entered
 * @return None
 */
void strip_trailing_whitespace(char* string) {
  int i = strnlen(string, BUFFER_SIZE) - 1;
  
  while(isspace(string[i]))
    string[i--] = 0;
}

/**
 * @brief Displays a command prompt including the current working directory
 */
void display_prompt(void) {
  char current_dir[MAX_PATH_LENGTH];
  
  if (getcwd(current_dir, sizeof(current_dir)) != NULL)
    // Outputs the current working directory in bold green text (\033[32;1m)
    // \033 is the escape sequence for changing text, 32 is green, 1 is bold
    fprintf(stdout, "myshell:\033[32;1m%s\033[0m> ", current_dir);
}

/**
 * @brief  Main program function
 * @param  Not used
 * @return EXIT_SUCCESS is always returned
 */
int main(int argc, char** argv) {
  while (true) {
    display_prompt();
    
    // Read a line representing a command to execute from stdin into 
    // a character array
    if (fgets(buffer, BUFFER_SIZE, stdin) != 0) {
      
      // Clean up sloppy user input
      strip_trailing_whitespace(buffer);
      
      //Reset filename buffer after each command execution
      bzero(filename, MAX_FILENAME_LENGTH);     
      
      // As in most shells, "cd" and "exit" are special cases that need
      // to be handled separately
      if ((sscanf(buffer, "cd %s", filename) == 1) ||
	  (!strncmp(buffer, "cd", BUFFER_SIZE))) 
	do_cd(filename);
      else if (!strncmp(buffer, "exit", BUFFER_SIZE)) 
	exit(EXIT_SUCCESS);
      else 
	execute_command(buffer);
    }
  }
  
  return EXIT_SUCCESS;
}

/**
 * @brief  Changes the current working directory
 * @param  Char array representing the directory to change to, or if empty
 *         use the user's home directory
 * @return -1 on error, 0 on success
 */
int do_cd(char* dirname) {
  struct passwd *p = getpwuid(getuid());

  // If no argument, change to current user's home directory
  if (strnlen(dirname, MAX_PATH_LENGTH) == 0)
      strncpy(dirname, p->pw_dir, MAX_PATH_LENGTH);

  // Otherwise, change to directory specified and check for error
  if (chdir(dirname) < 0) {
    fprintf(stderr, "cd: %s\n", strerror(errno));
    return -1;
  }

  return 0;
}

/**
 * @brief  Lists the contents of a directory
 * @param  Name of directory to list, or if empty, use current working directory
 * @return -1 on error, 0 on success
 */ 
int do_ls(const char* dirname) {
  struct dirent* d;
  DIR* dir = opendir(dirname);
  if(dir == NULL){
    fprintf(stderr, "ls: %s\n", strerror(errno));
    return -1;
  }
  errno = 0;
  while((d=readdir(dir)) != NULL){
    if(errno == 0) printf("%s\n",d->d_name);
    else{
      fprintf(stderr, "ls: Cannot read entry from directory... %s\n",strerror(errno));
      closedir(dir);
      return -1;
    }
  }
  closedir(dir);
  return 0;
}

/**
 * @brief  Outputs the contents of a single ordinary file
 * @param  Name of file whose contents should be output
 * @return -1 on error, 0 on success
 */
int do_cat(const char* filename) {
  ssize_t bytesRead;
  int fd = open(filename, O_RDONLY);

  if(fd == -1){
    close(fd);
    fprintf(stderr, "cat: Cannot open file. %s\n",strerror(errno));
    return fd;
  }
  while( (bytesRead = read(fd, buffer, BUFFER_SIZE)) != 0){
    if(bytesRead == -1 || write(1, buffer, bytesRead) != bytesRead) {
      close(fd);
      fprintf(stderr, "cat: cannot read file. %s\n",strerror(errno));
      return -1;
    }
    printf("\n");
  }
  close(fd);
  return 0;
}

/**
 * @brief  Creates a new directory
 * @param  Name of directory to create
 * @return -1 on error, 0 on success
 */
int do_mkdir(const char* dirname) {
  int newDir = mkdir(dirname, 0777);
  if(newDir == -1){
      fprintf(stderr, "mkdir: Cannot create directory. %s.\n",strerror(errno));
      return newDir;
  }
  else return newDir;
}

/**
 * @brief  Removes an existing directory
 * @param  Name of directory to remove
 * @return -1 on error, 0 on success
 */
int do_rmdir(const char* dirname) {
  if(rmdir(dirname) == -1){
    fprintf(stderr, "rmdir: Cannot remove directory. %s.\n",strerror(errno));
    return -1;
  }
  else return 0;
}

/**
 * @brief  Outputs the name of the current working directory
 * @return Cannot fail. Always returns 1.
 */
int do_pwd(void) {
  char current_dir[MAX_PATH_LENGTH];
  if(getcwd(current_dir, sizeof(current_dir)) == NULL){
    fprintf(stderr, "pwd: Could not find current directory. %s.\n",strerror(errno));
  }
  printf("%s\n", current_dir);
  return 1;
}

/**
 * @brief  Removes (unlinks) a file
 * @param  Name of file to delete
 * @return -1 on error, 0 on success
 */
int do_rm(const char* filename) {
  int rm = unlink(filename);
  if(rm == -1){
    fprintf(stderr, "rm: Cannot remove file. %s.\n",strerror(errno));
    return rm;
  }
  return rm;
}

/**
 * @brief  Outputs information about a file
 * @param  Name of file to stat
 * @return -1 on error, 0 on success
 */
int do_stat(char* filename) {
  struct stat stats;
  int result = stat(filename, &stats);
  if(result == -1){
    fprintf(stderr, "stat: Cannot retrieve file stats. %s.\n",strerror(errno));
    return result;
  }
  printf("\nSTATS FOR \"%s\":\n", filename);
  printf("Last access: %ld ns\n", stats.st_atime);
  printf("Last modification: %ld ns\n", stats.st_mtime);
  printf("Last change: %ld ns\n\n", stats.st_ctime);

  printf("File owner ID: %d\nFile group owner ID: %d\n", stats.st_uid, stats.st_gid);
  printf("File inode number: %ld\n", stats.st_ino);
  printf("File type & mode: %d\n", stats.st_mode);
  printf("File hard link count: %ld\n\n", stats.st_nlink);

  printf("File size: %ld byte(s)\n", stats.st_size);
  printf("File preferred block size: %ld\n", stats.st_blksize);
  printf("Allocated %ld blocks of 512 bytes\n\n", stats.st_blocks);
  
  return result;
}

//Exits the program
int do_q(){
  exit(EXIT_SUCCESS);
}

/**
 * @brief  Executes a shell command. Checks for invalid commands.
 * @param  Char array representing the command to execute
 * @return Return value of command being executed, or -1 for invalid command
 */
int execute_command(char* buffer)  {
  if (sscanf(buffer, "cat %s", filename) == 1) {
    return do_cat(filename);
  }
  
  if (sscanf(buffer, "stat %s", filename) == 1) {
    return do_stat(filename);
  }
  
  if (sscanf(buffer, "mkdir %s", filename) == 1) {
    return do_mkdir(filename);
  }
  
  if (sscanf(buffer, "rmdir %s", filename) == 1) {
    return do_rmdir(filename);
  }
  
  if (sscanf(buffer, "rm %s", filename) == 1) {
    return do_rm(filename);
  }
 
  if ((sscanf(buffer, "ls %s", filename) == 1) ||
      (!strncmp(buffer, "ls", BUFFER_SIZE))) {
    if (strnlen(filename, BUFFER_SIZE) == 0)
      sprintf(filename, ".");

    return do_ls(filename);
  }

  if (!strncmp(buffer, "pwd", BUFFER_SIZE)) {
    return do_pwd();
  }

  if ((sscanf(buffer, "q %s", filename) == 1) ||
      (!strncmp(buffer, "q", BUFFER_SIZE))) {
    return do_q();
  }

  // Invalid command
  if (strnlen(buffer, BUFFER_SIZE) != 0)
    fprintf(stderr, "myshell: %s: No such file or directory\n", buffer);

  return -1;
}
