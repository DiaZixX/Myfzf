#include "myfzf.h"
#include <ncurses.h>

void swap_tab(int **tab1, int **tab2) {
    int *buffer = *tab1;
    *tab1 = *tab2;
    *tab2 = buffer;
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
                printf("File : %s\n", complete_path);
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

void print_menu(WINDOW *menu_win, int highlight, int selections[],
                char *choices[], int n_choices) {
    int x, y, i;
    x = 2;
    y = 2;
    box(menu_win, 0, 0);
    for (i = 0; i < n_choices; ++i) {
        if (highlight == i + 1)
            wattron(menu_win,
                    A_REVERSE); // Inverse la couleur pour l'élément sélectionné
        if (selections[i])
            mvwprintw(menu_win, y, x, "[X] %s",
                      choices[i]); // Indique que l'élément est sélectionné
        else
            mvwprintw(menu_win, y, x, "[ ] %s", choices[i]);
        wattroff(menu_win, A_REVERSE);
        y++;
    }
    wrefresh(menu_win);
}

int main() {
    WINDOW *menu_win;
    int highlight = 1;
    int choice = 0;
    int c;
    int selections[MAX_CHOICES] = {0}; // Garde la trace des sélections

    char *choices[] = {
        "Option", "Option", "Option", "Option", "Option",
        "Option", "Option", "Option", "Option", "Option",
        "Option", "Option", "Option", "Option", "Option",
    };
    int n_choices = sizeof(choices) / sizeof(char *);

    initscr();
    clear();
    noecho();
    cbreak();    // Désactiver le buffering pour une réponse instantanée
    curs_set(0); // Masquer le curseur

    int startx = 0;
    int starty = 0;
    menu_win = newwin(40, 80, starty, startx);
    keypad(menu_win, TRUE);
    mvprintw(0, 81,
             "Utilisez les fleches pour naviguer, ESPACE pour selectionner, "
             "ENTREE pour valider.");
    refresh();
    print_menu(menu_win, highlight, selections, choices, n_choices);

    while (1) {
        c = wgetch(menu_win);
        switch (c) {
        case KEY_UP:
            if (highlight == 1)
                highlight = n_choices;
            else
                --highlight;
            break;
        case KEY_DOWN:
            if (highlight == n_choices)
                highlight = 1;
            else
                ++highlight;
            break;
        case ' ': // Toggle la sélection avec la barre d'espace
            selector(selections, highlight - 1);
            break;
        case 10: // Touche ENTREE
            choice = 1;
            break;
        default:
            break;
        }
        print_menu(menu_win, highlight, selections, choices, n_choices);
        if (choice == 1) // Si l'utilisateur a appuyé sur ENTREE
            break;
    }

    // Afficher les sélections
    clear();
    mvprintw(0, 0, "Vous avez sélectionné :");
    for (int i = 0; i < n_choices; i++) {
        if (selections[i])
            mvprintw(i + 1, 0, "%s", choices[i]);
    }
    refresh();
    getch();

    endwin();

    printf("%d\n", n_choices);
    return 0;
}
