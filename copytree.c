// Maayan Ifergan 212437453
#include "copytree.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <libgen.h> 

#define PATH_MAX 4096

// Function to copy a single file
void copy_file(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    // Check if the source and destination paths are the same
    if (strcmp(src, dest) == 0) {
        fprintf(stderr, "Source and destination paths are the same.\n");
        return;
    }
    struct stat stat_buf;

    // Get metadata about the source file
    if (lstat(src, &stat_buf) == -1) {
        perror("lstat failed");
        return;
    }

    // Handle symbolic links if the flag is set
    if (S_ISLNK(stat_buf.st_mode) && copy_symlinks) {
        char link_target[PATH_MAX];
        ssize_t len = readlink(src, link_target, sizeof(link_target) - 1);
        if (len == -1) {
            perror("readlink failed");
            return;
        }
        link_target[len] = '\0';
        if (symlink(link_target, dest) == -1) {
            perror("symlink failed");
        }
        return;
    }

    // Open the source file
    int src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        perror("open source failed");
        return;
    }

    // Open or create the destination file
    int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, copy_permissions ? stat_buf.st_mode : 0644);
    if (dest_fd == -1) {
        perror("open destination failed");
        close(src_fd);
        return;
    }

    // Copy file contents
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        if (write(dest_fd, buffer, bytes_read) != bytes_read) {
            perror("write failed");
            break;
        }
    }

    if (bytes_read == -1) {
        perror("read failed");
    }

    // Close file descriptors
    close(src_fd);
    close(dest_fd);

    // Copy permissions if the flag is set
    if (copy_permissions && chmod(dest, stat_buf.st_mode) == -1) {
        perror("chmod failed");
    }
}

// Function to create directories recursively
int create_directory_recursive(const char *path, mode_t mode) {
    if (!path || strlen(path) == 0) {
        fprintf(stderr, "Invalid path provided to create_directory_recursive.\n");
        return -1;
    }

    // Convert relative path to absolute path
    char abs_path[PATH_MAX];
    if (realpath(path, abs_path) == NULL) {
        // If the path doesn't exist yet, use the original path
        strncpy(abs_path, path, PATH_MAX - 1);
        abs_path[PATH_MAX - 1] = '\0';
    }

    char temp_path[PATH_MAX];
    strncpy(temp_path, abs_path, PATH_MAX - 1);
    temp_path[PATH_MAX - 1] = '\0';

    // Get the parent directory
    char *parent_dir = dirname(temp_path);

    // Base case: stop recursion if we reach the root or the parent directory exists
    if (strcmp(parent_dir, abs_path) == 0 || access(parent_dir, F_OK) == 0) {
        // Try to create the directory if it doesn't exist yet
        if (mkdir(abs_path, mode) == -1 && errno != EEXIST) {
            perror("mkdir failed");
            return -1;
        }
        return 0;
    }

    // Recursively create the parent directory
    if (create_directory_recursive(parent_dir, mode) == -1) {
        return -1;
    }

    // Create the current directory
    if (mkdir(abs_path, mode) == -1 && errno != EEXIST) {
        perror("mkdir failed");
        return -1;
    }

    return 0;
}

// Function to copy an entire directory
void copy_directory(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    // Check if source and destination are the same
    if (strcmp(src, dest) == 0) {
        fprintf(stderr, "Source and destination directories are the same.\n");
        return;
    }

    // Check if destination is a subdirectory of the source
    if (strstr(dest, src) == dest) {
        fprintf(stderr, "Destination directory is a subdirectory of the source.\n");
        return;
    }

    // Check if the destination directory already exists and is not empty
    DIR *test_dir = opendir(dest);
    if (test_dir) {
        struct dirent *test_entry;
        while ((test_entry = readdir(test_dir)) != NULL) {
            if (strcmp(test_entry->d_name, ".") != 0 && strcmp(test_entry->d_name, "..") != 0) {
                fprintf(stderr, "Destination directory is not empty: %s\n", dest);
                closedir(test_dir);
                return;
            }
        }
        closedir(test_dir);
    } else if (errno != ENOENT) { // If the error is not "No such file or directory"
        perror("Failed to access destination directory");
        return;
    }

    // Open the source directory
    DIR *dir = opendir(src);
    if (!dir) {
        perror("opendir failed");
        return;
    }

    // Get metadata about the source directory
    struct stat src_stat;
    if (stat(src, &src_stat) == -1) {
        perror("stat failed");
        closedir(dir);
        return;
    }

    // Create the destination directory
    if (create_directory_recursive(dest, copy_permissions ? src_stat.st_mode : 0755) == -1) {
        fprintf(stderr, "Failed to create destination directory: %s\n", dest);
        closedir(dir);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Build full paths for source and destination
        char src_path[PATH_MAX], dest_path[PATH_MAX];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, entry->d_name);

        // Get metadata about the current entry
        struct stat stat_buf;
        if (lstat(src_path, &stat_buf) == -1) {
            perror("lstat failed");
            continue;
        }

        // Recursively copy directories or copy files
        if (S_ISDIR(stat_buf.st_mode)) {
            copy_directory(src_path, dest_path, copy_symlinks, copy_permissions);
        } else {
            copy_file(src_path, dest_path, copy_symlinks, copy_permissions);
        }
    }

    // Close the directory stream
    closedir(dir);
}