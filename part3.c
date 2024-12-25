// Maayan Ifergan 212437453
#include "copytree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s [-l] [-p] <source_directory> <destination_directory>\n", prog_name);
    fprintf(stderr, "  -l: Copy symbolic links as links\n");
    fprintf(stderr, "  -p: Copy file permissions\n");
}

int main(int argc, char *argv[]) {
    int copy_symlinks = 0;
    int copy_permissions = 0;
    const char *src_dir = NULL;
    const char *dest_dir = NULL;

    // Parse command-line arguments manually
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            copy_symlinks = 1;
        } else if (strcmp(argv[i], "-p") == 0) {
            copy_permissions = 1;
        } else if (src_dir == NULL) {
            src_dir = argv[i];
        } else if (dest_dir == NULL) {
            dest_dir = argv[i];
        } else {
            // Too many arguments
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Check that both source and destination directories are specified
    if (src_dir == NULL || dest_dir == NULL) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Copy the directory tree
    copy_directory(src_dir, dest_dir, copy_symlinks, copy_permissions);

    return 0;
}