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
 * Soubor proj.c obsahuje veskerou funkcionalitu.
 * 
 * Zatim nezjisteny zadne chyby.
 */

#include "proj2.h"

FILE *file;
int NE, NR, TE, TR, r_remain;

// sdilene promenne
int *print_num = NULL;          // cislo vypisu
long *file_head = NULL;         // konec souboru

int *r_cnt = NULL;              // pocitadlo sobu
int *e_cnt = NULL;              // pocitadlo elfu

bool *is_Christmas = NULL;      // Santa zavrel dilnu, priprava na Vanoce
bool *is_sleeping = NULL;       // Santa spi

// semafory
sem_t *print = NULL;            // povoluje vypis
sem_t *santa_work = NULL;       // probouzi santu
sem_t *christmas_s = NULL;      // povoli zacatek Vanoc
sem_t *e_help = NULL;           // santa pomaha elfum
sem_t *e_got_help = NULL;       // 3 elfove dostali pomoc
sem_t *r_hitched = NULL;        // povoluje zaprazene soby

// jmena semaforu
char *sem_names[] = {"xhurda01.sem.print", "xhurda01.sem.santa_work", "xhurda01.sem.christmas_s", "xhurda01.sem.r_hitched",
                        "xhurda01.sem.e_help", "xhurda01.sem.e_got_help"};
int sem_cnt = sizeof(sem_names)/sizeof(char*); // pocet semaforu

int main (int argc, char **argv) {
    
    if (load_args(argc, argv)) return 1;
    r_remain = NR;

    if (init()) {
        cleanup();
        return 1;
    }

    // generovani podprocesu santa
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Santa fork error!\n");
        cleanup();
    }
    else if (pid == 0) santa();

    // generovani podprocesu elf
    for (int i = 1; i <= NE; i++){
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Elf %d fork error!\n", i);
            cleanup();
        }
        else if (pid == 0) elf(i);
    }

    // generovani podprocesu sob
    for (int i = 1; i <= NR; i++){
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Reindeer %d fork error!\n", i);
            cleanup();
        }
        else if (pid == 0) reindeer(i);
    }
    
    // cekani na vsechny procesy
    for(int i = 0; i < NE + NR + 1; i++) {
        wait(NULL);
    }

    cleanup();
    return 0;
}

// proces santa
void santa () {
    file_print_santa("Santa: going to sleep");
    *is_sleeping = true;
    while (1) {
        sem_wait(santa_work);
        *is_sleeping = false;

        // zacnou Vanoce
        if (*r_cnt == NR) {
            file_print_santa("Santa: closing workshop");
            *is_Christmas = true;
            // povoli elfy
            for (int i = 0; i < NE; i++){
                sem_post(e_help);
            }
            // povoli vsechny soby
            for (int i = 0; i < NR; i++){
                sem_post(r_hitched);
            }
            sem_wait(christmas_s);
            file_print_santa("Santa: Christmas started");
            *is_Christmas = true;

            break;
        }

        // nejsou Vanoce => probuzen elfy => pomuze trem, pocka na tri
        file_print_santa("Santa: helping elves");
        sem_post(e_help); sem_post(e_help); sem_post(e_help);
 
        sem_wait(e_got_help); sem_wait(e_got_help); sem_wait(e_got_help);

        file_print_santa("Santa: going to sleep");

        // zkontroluje kolik elfu je pred dilnou, jestli vazne usne nebo pokracuje dal
        if (*e_cnt >= 3) {
            sem_post(santa_work);
        } else {
            *is_sleeping = true;
        }
    }
    
    exit (0);
}

// proces elf
void elf (int ID) {
    file_print_id("Elf", "started", ID);

    while (1) {
        srand(time(0)*ID);
        work(TE);

        file_print_id("Elf", "need help", ID);
        (*e_cnt)++;

        // jsou Vanoce jdou pryc
        if (*is_Christmas){
            file_print_id("Elf", "taking holidays", ID);
            break;
        }

        // jsou tri v rade (min)
        if (*e_cnt >= 3 && *is_sleeping) {
            sem_post(santa_work);
        }

        sem_wait(e_help);
        (*e_cnt)--;

        if (!*is_Christmas){
            file_print_id("Elf", "get help", ID);
            sem_post(e_got_help);
        } else {
            file_print_id("Elf", "taking holidays", ID);
            break;
        }
    }

    exit (0);
}

// proces sob
void reindeer (int ID) {
    file_print_id("RD", "rstarted", ID);
    srand(time(0)*ID);
    
    holiday(TR);

    file_print_id("RD", "return home", ID);
    (*r_cnt)++;

    // pokud je posledni povoli Santu
    if (*r_cnt == NR) {
        sem_post(santa_work);
    }

    sem_wait(r_hitched);

    file_print_id("RD", "get hitched", ID);

    (*r_cnt)--;

    // posledni zaprazen => zacinaji Vanoce
    if (*r_cnt == 0){
        sem_post(christmas_s);
    }

    exit (0);
}

// prvotni inicializace
int init () {
    // otevreni souboru
    file = fopen("proj2.out", "w");
    if (file == NULL) {
        fprintf(stderr, "Cannot open file proj2.out!\n");
        return 1;
    }

    // mapovani sdilene pameti
    MMAP(print_num);
    *print_num = 1;
    MMAP(file_head);

    MMAP(r_cnt);
    *r_cnt = 0;
    MMAP(e_cnt);
    *e_cnt = 0;

    MMAP(is_Christmas);
    *is_Christmas = false;
    MMAP(is_sleeping);
    *is_sleeping = false;

    // jen pro pripad nejake kolize
    unlink_sems();

    // tvorba semaforu
    int err = 0;
    if ((print = sem_open("xhurda01.sem.print", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) err = 1;
    if ((santa_work = sem_open("xhurda01.sem.santa_work", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) err = 1;
    if ((christmas_s = sem_open("xhurda01.sem.christmas_s", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) err = 1;
    if ((e_help = sem_open("xhurda01.sem.e_help", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) err = 1;
    if ((e_got_help = sem_open("xhurda01.sem.e_got_help", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) err = 1;
    if ((r_hitched = sem_open("xhurda01.sem.r_hitched", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) err = 1;
    
    if (err) {
        fprintf(stderr, "Cannot open semaphores!\n");
    }

    return err;
}

// uklid
void cleanup () {
    // zavreni souboru
    fclose(file);
    
    // odmapovani pameti
    UNMAP(print_num);
    UNMAP(file_head);
    UNMAP(r_cnt);
    UNMAP(e_cnt);
    UNMAP(is_Christmas);
    UNMAP(is_sleeping);

    // zavirani semaforu
    sem_close(print);
    sem_close(santa_work);
    sem_close(christmas_s);
    sem_close(e_help);
    sem_close(e_got_help);
    sem_close(r_hitched);

    // unlink vsech semaforu
    unlink_sems();
}

// unlink vsech semaforu
void unlink_sems () {
    for (int i = 0; i < sem_cnt; i++){
        sem_unlink(sem_names[i]);
    }
}

// chraneny santuv vypis
void file_print_santa(char *str) {
    sem_wait(print);
    fseek(file, *file_head, SEEK_SET);
    fprintf(file, "%d: %s\n", *print_num, str);
    fflush(file);
    (*print_num)++;
    (*file_head) = ftell(file);
    
    sem_post(print);
}

// chraneny vypis ve formatu s ID
void file_print_id(char *p_type, char *str, int ID) {
    sem_wait(print);
    fseek(file, *file_head, SEEK_SET);
    fprintf(file, "%d: %s %d: %s\n", *print_num, p_type, ID, str);
    fflush(file);
    (*print_num)++;
    (*file_head) = ftell(file);
    sem_post(print);
}

// prevod pole charu na int, s osetrenim chyb
int parse_str_to_num (int *des, char *str){
    if (str == NULL){
        return 1;
    }
    char *ptr = NULL;
    *des = strtol(str, &ptr, 10);
    if (ptr != NULL){
        if(ptr[0] != '\0'|| strlen(str) == 0){
            // error
            return 1;
        }
    }
    return 0;
}

// prevede jeden argument na int a osetri podminky
int get_valid_arg (int *des, char *arg, int min, int max) {
    if (parse_str_to_num(des, arg)) return 1;

    if (*des < min || *des > max) return 1;

    return 0;
}

// nahraje argumenty do promennych, provede i kontrolu korektnosti
int load_args (int argc, char **argv) {
    if (argc != 5){
        fprintf(stderr, "Invalid number of arguments!\n");
        return 1;
    }

    if (get_valid_arg(&NE, argv[1], 1, 999) || get_valid_arg(&NR, argv[2], 1, 19) ||
        get_valid_arg(&TE, argv[3], 0, 1000) || get_valid_arg(&TR, argv[3], 0, 1000)) {
        
        fprintf(stderr, "Invalid arguments! Argumenst should be four whole numbers!\n");
        return 1;
    }

    return 0;
}

/***** konec souboru proj2.c *****/
