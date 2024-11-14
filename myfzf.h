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
#define SUBSITUTION_COST 3
#define ERASE_COST 1
#define INSERTION_COST 1

#define ACCEPT_SCORE 15

#define MAX_CHOICES 15

#define min(a, b) (((a) < (b)) ? (a) : (b))

/**
 * @class OrderedChoices
 * @brief Structure to implement an array ordered by score for the choices we
 * have
 *
 */
typedef struct OrderedChoices {
    int score;
    char name[MAX_LENGTH_PATH];
} OrdChoice;

/**
 * @brief Swap the start pointer of the 2 tabs
 *
 * @param tab1 Start pointer of the first tab
 * @param tab2 Start pointer of the second tab
 */
void swap_tab(int **tab1, int **tab2);

/**
 * @brief Process the levenshtein's distance with constants defined in header
 *
 * @param seq_X The sequence X with no empty caracters
 * @param n Length of the seq_X
 * @param seq_Y The sequence Y with no empty caracters
 * @param m Length of the seq_Y
 * @return The distance between the two sequences
 */
int levenshtein_distance(char *seq_X, int n, char *seq_Y, int m);

/**
 * @brief Recursively go through all the sub-dir
 *
 * @param path The path of the tree's root
 */
void list_content(const char *path);

/**
 * @brief Manage the selector in the terminal
 */
void renderer();

#endif // !MY_FZF_H
