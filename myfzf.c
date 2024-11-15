#include "myfzf.h"
#include <ncurses.h>
#include <pthread.h>
#include <string.h>

char *targetFile;
int nChoices = 0;
OrdChoice choices[MAX_CHOICES]; // Keeps the choices the program found ordered
                                // by score (levenshtein distance)

/**
 * @brief Swap two array using their pointers
 *
 * @param tab1 First array
 * @param tab2 Second array
 */
void swap_tab(int **tab1, int **tab2) {
    int *buffer = *tab1;
    *tab1 = *tab2;
    *tab2 = buffer;
}

/**
 * @brief Initialise they array of choices by setting every default score to
 * INT32_MAX
 */
void init_choices() {
    for (int i = 0; i < MAX_CHOICES; i++) {
        choices[i].score = INT32_MAX;
    }
}

/**
 * @brief Insert a new filepath and its score in the choices. Manage the array
 * as a priority queue.
 *
 * @param score The levenshtein distance
 * @param name The filepath
 */
void insert_choice(int score, char *name) {
    OrdChoice temp;
    int i;
    if (nChoices < MAX_CHOICES) {
        nChoices++;
    }
    for (i = 0; i < nChoices; i++) {
        if (choices[i].score > score) {
            temp = choices[i];
            choices[i].score = score;
            strcpy(choices[i].name, name);
            break;
        }
    }
    for (i++; i < nChoices; i++) {
        if (choices[i].score > temp.score) {
            OrdChoice buffer = temp;
            temp = choices[i];
            choices[i] = buffer;
        } else {
            break;
        }
    }
}

/**
 * @brief Starting routine for the exploration thread
 *
 * @param arg The initial path of the tree's root
 */
void *start_explore(void *arg) {
    const char *initPath = (char *)arg;

    init_choices();
    list_content(initPath);

    pthread_exit(EXIT_SUCCESS);
}

/**
 * @brief Starting routine for the renderer thread
 *
 * @param arg Don't require arguments
 */
void *start_renderer(void *arg) {
    renderer();

    pthread_exit(EXIT_SUCCESS);
}

/**
 * @brief Calculate the levenshtein's distance between the two sequences. The
 * score is depends on three constants : INSERTION_COST, SUBSITUTION_COST and
 * ERASE_COST.
 *
 * @param seq_X The first sequence of characters
 * @param n The length of sequence X
 * @param seq_Y The second sequence of characters
 * @param m The length of sequence Y
 * @return The levenshtein's distance between X and Y
 */
int levenshtein_distance(char *seq_X, int n, char *seq_Y, int m) {
    int *previous_row = (int *)calloc(m + 1, sizeof(int));
    int *current_row = (int *)calloc(m + 1, sizeof(int));

    for (int j = 1; j < m + 1; j++) {
        previous_row[j] = j;
    }

    for (int i = 1; i < n + 1; i++) {
        current_row[0] = i;
        for (int j = 1; j < m + 1; j++) {
            current_row[j] =
                (seq_X[i] == seq_Y[j])
                    ? previous_row[j - 1]
                    : min(previous_row[j] + ERASE_COST,
                          min(current_row[j - 1] + INSERTION_COST,
                              previous_row[j - 1] + SUBSITUTION_COST));
        }
        swap_tab(&current_row, &previous_row);
    }

    int res = previous_row[m];
    free(previous_row);
    free(current_row);
    return res;
}

/**
 * @brief Explore recursively the directory tree
 *
 * @param path The current path explored
 */
void list_content(const char *path) {
    struct dirent *entry;
    DIR *directory = opendir(path);
    char complete_path[MAX_LENGTH_PATH];
    struct stat infos;

    if (!directory) {
        perror("Error during the opening of the initial directory");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(directory))) {
        // Ignore '.' and '..' folder
        if (entry->d_name[0] == '.' &&
            (entry->d_name[1] == '\0' ||
             (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
            continue;
        }

        snprintf(complete_path, sizeof(complete_path), "%s/%s", path,
                 entry->d_name);

        // Check if it's a directory or a file
        if (stat(complete_path, &infos) == 0) {
            if (S_ISDIR(infos.st_mode)) {
                list_content(complete_path);
            } else if (S_ISREG(infos.st_mode)) {
                int score =
                    levenshtein_distance(targetFile, strlen(targetFile),
                                         entry->d_name, strlen(entry->d_name));
                if (score <= ACCEPT_SCORE) {
                    insert_choice(score, complete_path);
                }
            } else {
                continue;
            }
        }
    }

    closedir(directory);
}

/**
 * @brief Set the cell at index idx to true and set all the others to false
 *
 * @param selections The array keeping the current selection
 * @param idx The index to put on true value
 */
void selector(int *selections, int idx) {
    for (int i = 0; i < MAX_CHOICES; i++)
        selections[i] = (i == idx) ? 1 : 0;
}

/**
 * @brief Render the selection menu in the terminal
 *
 * @param menuWin The current window
 * @param highlight The line selected to highlight
 * @param selections The array of selection
 */
void print_menu(WINDOW *menuWin, int highlight, int selections[]) {
    int x, y, i;
    x = 2;
    y = 2;
    box(menuWin, 0, 0);
    for (i = 0; i < nChoices; ++i) {
        if (highlight == i + 1)
            wattron(menuWin,
                    A_REVERSE); // Inverse la couleur pour l'élément sélectionné
        if (selections[i])
            mvwprintw(menuWin, y, x, "[X] %s",
                      choices[i].name); // Indique que l'élément est sélectionné
        else
            mvwprintw(menuWin, y, x, "[ ] %s", choices[i].name);
        wattroff(menuWin, A_REVERSE);
        y++;
    }
    wrefresh(menuWin);
}

/**
 * @brief Initialise the window in the terminal and manage user's key input
 */
void renderer() {
    WINDOW *menuWin;
    int highlight = 1;
    bool userSelected = false;
    int keyInput;
    int selections[MAX_CHOICES] = {0}; // Garde la trace des sélections
    int startx = 0;
    int starty = 0;

    initscr();
    clear();
    noecho();
    cbreak();    // Désactiver le buffering pour une réponse instantanée
    curs_set(0); // Masquer le curseur

    menuWin = newwin(40, 80, starty, startx);
    keypad(menuWin, TRUE);
    nodelay(menuWin, TRUE); // Active le mode non-bloquant
    mvprintw(0, 81,
             "Use ARROWS to navigate, Use SPACE to select,"
             "Use ENTER to confirm");
    refresh();
    print_menu(menuWin, highlight, selections);

    // Navigation in the menu
    while (1) {
        keyInput = wgetch(menuWin);
        switch (keyInput) {
        case KEY_UP:
            if (highlight == 1)
                highlight = nChoices;
            else
                --highlight;
            break;
        case KEY_DOWN:
            if (highlight == nChoices)
                highlight = 1;
            else
                ++highlight;
            break;
        case ' ': // Toggle la sélection avec la barre d'espace
            selector(selections, highlight - 1);
            break;
        case 10: // Touche ENTREE
            userSelected = true;
            break;
        default:
            break;
        }
        print_menu(menuWin, highlight, selections);
        if (userSelected) // Si l'utilisateur a appuyé sur ENTREE
            break;
    }

    // Afficher les sélections
    clear();
    mvprintw(0, 0, "Vous avez sélectionné :");
    for (int i = 0; i < nChoices; i++) {
        if (selections[i])
            mvprintw(i + 1, 0, "%s", choices[i].name);
    }
    refresh();
    getch();

    endwin();
}

int main(int argc, char *argv[]) {
    pthread_t thread_explore;
    pthread_t thread_renderer;
    char *arg;

    if (argc != 3) {
        perror("Nombre d'arguments invalide. Format : ./myfzf <filename> "
               "<root_dir>");
        return EXIT_FAILURE;
    }

    targetFile = argv[1];
    arg = argv[2];

    pthread_create(&thread_explore, NULL, start_explore, (void *)arg);
    pthread_create(&thread_renderer, NULL, start_renderer, NULL);

    pthread_join(thread_explore, NULL);
    pthread_join(thread_renderer, NULL);

    return EXIT_SUCCESS;
}
