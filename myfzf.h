#ifndef MY_FZF_H

#define MY_FZF_H

#include <dirent.h>
#include <ncurses.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_LENGTH_PATH 1024
#define SUBSITUTION_COST 2
#define ERASE_COST 3
#define INSERTION_COST 3

#define ACCEPT_SCORE 5

#define MAX_CHOICES 15

#define min(a, b) (((a) < (b)) ? (a) : (b))

typedef struct OrderedChoices {
    int score;
    char name[MAX_LENGTH_PATH];
} OrdChoice;

void swap_tab(int **tab1, int **tab2);
void init_choices();
void insert_choice(int score, char *name);

void *start_explore(void *arg);
void *start_renderer(void *arg);

int levenshtein_distance(char *seq_X, int n, char *seq_Y, int m);
void list_content(const char *path);
void selector(int *selections, int idx);

void print_menu(WINDOW *menuWin, int highlight, int selections[]);
void renderer();

#endif // !MY_FZF_H
