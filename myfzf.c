#include "myfzf.h"

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

        char complete_path[MAX_LENGTH_PATH];
        snprintf(complete_path, sizeof(complete_path), "%s/%s", path,
                 entry->d_name);

        // Check if it's a directory or a file
        struct stat infos;
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

int main(int argc, char *argv[]) {
    list_content(argv[1]);
    return EXIT_SUCCESS;
}
