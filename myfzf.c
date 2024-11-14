#include "myfzf.h"
#include <pthread.h>

char *targetFile;
int nChoices = 0;
OrdChoice choices[MAX_CHOICES]; // Garde les différents choix qu'on trouve

void swap_tab(int **tab1, int **tab2) {
    int *buffer = *tab1;
    *tab1 = *tab2;
    *tab2 = buffer;
}

void insert_choice(int score, char *name) { return; }

void *start_explore(void *arg) {
    const char *initPath = (char *)arg;

    list_content(initPath);

    pthread_exit(EXIT_SUCCESS);
}

void *start_renderer(void *arg) {
    renderer();

    pthread_exit(EXIT_SUCCESS);
}

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
                perror("Not a directory or a file");
                exit(EXIT_FAILURE);
            }
        }
    }

    closedir(directory);
}

void selector(int *selections, int idx) {
    for (int i = 0; i < MAX_CHOICES; i++)
        selections[i] = (i == idx) ? 1 : 0;
}

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
        perror("Nombre d'arguments invalide. Format : ./myfzf <root_dir> "
               "<filename>");
        return EXIT_FAILURE;
    }

    targetFile = argv[1];
    arg = argv[2];

    pthread_create(&thread_explore, NULL, start_explore, (void *)arg);
    pthread_create(&thread_renderer, NULL, start_renderer, NULL);

    return EXIT_SUCCESS;
}
