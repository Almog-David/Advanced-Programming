
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void create_permission_string(mode_t mode, char *str)
{
    str[0] = S_ISDIR(mode) ? 'd' : '-';
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
}

void print_indentation(int depth, int is_last, int is_first_depth)
{
    if (depth == 0)
        return;

    if (is_first_depth)
    {
        printf("%*s", depth * 4, "");
        return;
    }

    for (int i = 1; i < depth; i++)
    {
        printf(is_last ? "    " : "|   ");
    }
    printf(is_last ? "└── " : "├── ");
}

void print_tree(const char *path, int depth, int *total_files, int *total_dirs)
{
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        perror("Failed to open directory");
        return;
    }

    struct dirent *entry;
    struct dirent dir_entries[1024];
    struct dirent file_entries[1024];
    int dir_count = 0;
    int file_count = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        struct stat st;
        if (stat(full_path, &st) == -1)
        {
            perror("Failed to get file information");
            continue;
        }

        if (S_ISDIR(st.st_mode))
        {
            dir_entries[dir_count++] = *entry;
            (*total_dirs)++;
        }
        else
        {
            file_entries[file_count++] = *entry;
            (*total_files)++;
        }
    }

    for (int i = 0; i < dir_count + file_count; i++)
    {
        int is_last = i == dir_count + file_count - 1;

        if (i < dir_count)
        {
            entry = &dir_entries[i];
        }
        else
        {
            entry = &file_entries[i - dir_count];
        }

        print_indentation(depth, is_last, depth == 0);

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        struct stat st;
        if (stat(full_path, &st) == -1)
        {
            perror("Failed to get file information");
            continue;
        }

        struct passwd *user = getpwuid(st.st_uid);
        struct group *group = getgrgid(st.st_gid);
        char perm_string[11];
        create_permission_string(st.st_mode, perm_string);

        printf("[%s %s %s %10lld]  %s\n", perm_string, user->pw_name, group->gr_name, (long long)st.st_size, entry->d_name);

        if (S_ISDIR(st.st_mode))
        {
            print_tree(full_path, depth + 1, total_files, total_dirs);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[])
{
    int total_files = 0;
    int total_dirs = 0;
    if (argc < 2)
    {
        // print from current directory
        print_tree(".", 0, &total_files, &total_dirs);
    }
    else
    {
        print_tree(argv[1], 0, &total_files, &total_dirs);
    }
    printf("\n%d directories, %d files\n", total_dirs, total_files);
    return EXIT_SUCCESS;
}
