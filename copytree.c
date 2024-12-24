#include "copytree.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#define PATH_MAX 4096

void copy_file(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    struct stat stat_buf;

    if (lstat(src, &stat_buf) == -1) {
        perror("lstat failed");
        return;
    }

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

    int src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        perror("open source failed");
        return;
    }

    int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, copy_permissions ? stat_buf.st_mode : 0644);
    if (dest_fd == -1) {
        perror("open destination failed");
        close(src_fd);
        return;
    }

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

    close(src_fd);
    close(dest_fd);
}

void create_directory(const char *path, int copy_permissions, mode_t mode) {
    if (mkdir(path, mode) == -1 && errno != EEXIST) {
        perror("mkdir failed");
    }
}

void copy_directory(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    DIR *dir = opendir(src);
    if (!dir) {
        perror("opendir failed");
        return;
    }

    struct stat src_stat;
    if (stat(src, &src_stat) == -1) {
        perror("stat failed");
        closedir(dir);
        return;
    }

    create_directory(dest, copy_permissions, copy_permissions ? src_stat.st_mode : 0755);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[PATH_MAX], dest_path[PATH_MAX];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, entry->d_name);

        struct stat stat_buf;
        if (lstat(src_path, &stat_buf) == -1) {
            perror("lstat failed");
            continue;
        }

        if (S_ISDIR(stat_buf.st_mode)) {
            copy_directory(src_path, dest_path, copy_symlinks, copy_permissions);
        } else {
            copy_file(src_path, dest_path, copy_symlinks, copy_permissions);
        }
    }

    closedir(dir);

    if (copy_permissions && chmod(dest, src_stat.st_mode) == -1) {
        perror("chmod failed");
    }
}
