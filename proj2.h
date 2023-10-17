/********************************************
 * IOS project 2 - synchronizace procesu
 * FIT VUT v Brne
 * 
 * Santa Claus problem
 * 
 * autor: Pavel Hurdalek
 * xlogin: xhurda01
 * datum: 1.5.2021
 *******************************************/

/**
 * Soubor proj.h obsahuje knihovny, makra
 * a predpisy funkci.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>

// makra
#define MMAP(pointer) {(pointer) = mmap (NULL, sizeof(*(pointer)),PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof(*(pointer)));}
#define work(max_time) {if (max_time != 0) usleep((rand() % max_time)*1000);}
#define holiday(max_time) {if (max_time != 0) usleep(((rand() % (max_time/2)) + max_time/2)*1000);}

// funkce
void santa ();
void elf (int ID);
void reindeer (int ID);

int init ();
void cleanup ();

void file_print_santa (char *str);
void file_print_id (char *p_type, char *str, int ID);
void unlink_sems ();

int parse_str_to_num (int *des, char *str);
int get_valid_arg (int *des, char *arg, int min, int max);
int load_args (int argc, char **argv);

/***** konec souboru proj2.h *****/
