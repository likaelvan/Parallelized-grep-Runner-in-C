#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <semaphore.h>

#define DIR_N_CHAR 1000
#define CMD_N_CHAR 1000

char search_str[100];

struct node
{
    char *dir;
    struct node *next;
};

struct node *front = NULL;
struct node *rear = NULL;

void enqueue(char *path)
{
    struct node *ptr = (struct node *)malloc(sizeof(struct node));
    ptr->dir = (char *)malloc(DIR_N_CHAR);
    strcpy(ptr->dir, path);
    ptr->next = NULL;
    if ((front == NULL) && (rear == NULL))
    {
        front = rear = ptr;
    }
    else
    {
        rear->next = ptr;
        rear = ptr;
    }
}

char *dequeue()
{
    if ((front == NULL))
    {
        return NULL;
    }
    else
    {
        char *get_dir = (char *)malloc(DIR_N_CHAR);
        strcpy(get_dir, front->dir);

        struct node *temp;
        temp = front;
        front = front->next;
        if (front == NULL)
            rear = NULL;

        free(temp->dir);
        free(temp);

        return get_dir;
    }
}

void parallel_grep(int *arg)
{
    int id = *arg;
    while (1)
    {

        char *deq;
        deq = dequeue();
        if (deq != NULL)
        {
            char abs_path[DIR_N_CHAR];
            realpath(deq, abs_path);
            printf("[%d] DIR %s\n", id, abs_path);

            DIR *dr = opendir(abs_path);
            struct dirent *child_obj;

            while ((child_obj = readdir(dr)) != NULL)
            {
                char new_path[DIR_N_CHAR];
                strcpy(new_path, abs_path);
                strcat(new_path, "/");
                strcat(new_path, child_obj->d_name);
                switch (child_obj->d_type)
                {

                case DT_DIR:
                    if (strcmp(child_obj->d_name, "..") != 0 && strcmp(child_obj->d_name, ".") != 0)
                    {
                        enqueue(new_path);
                        printf("[%d] ENQUEUE %s\n", id, new_path);
                    }
                    break;

                case DT_REG:
                    char sys_cmd[CMD_N_CHAR];

                    strcpy(sys_cmd, "grep ");
                    strcat(sys_cmd, search_str);
                    strcat(sys_cmd, " ");
                    strcat(sys_cmd, new_path);
                    strcat(sys_cmd, " >/dev/null");

                    if (system(sys_cmd) != 0)
                    {
                        printf("[%d] ABSENT %s\n", id, new_path);
                    }
                    else
                    {
                        printf("[%d] PRESENT %s\n", id, new_path);
                    }

                    break;
                }
            }
            closedir(dr);
            free(deq);
        }
        else
        {
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    enqueue(argv[2]);
    strcpy(search_str, argv[3]);

    int id = 0;
    parallel_grep(&id);

    return 0;
}
