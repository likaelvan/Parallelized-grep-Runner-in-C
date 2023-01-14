#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <semaphore.h>

#define DIR_N_CHAR 1000
#define CMD_N_CHAR 1000

int dir_num = 0;
int dir_traversed = 0;
char search_str[100];

sem_t mutex;

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
        sem_wait(&mutex);
        deq = dequeue();
        sem_post(&mutex);
        if (deq != NULL)
        {
            sem_wait(&mutex);
            dir_traversed++;
            sem_post(&mutex);
            
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
                        sem_wait(&mutex);
                        enqueue(new_path);
                        sem_post(&mutex);

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
                default:
                    printf("Other: %s\n", new_path);
                }
            }
            closedir(dr);
            free(deq);
        }

        if (dir_traversed >= dir_num)
            break;
    }
}

void get_dir_num(char *root)
{
    char abs_path[DIR_N_CHAR];
    realpath(root, abs_path);
    DIR *dr = opendir(abs_path);
    struct dirent *child_obj;
    char new_path[DIR_N_CHAR];

    while ((child_obj = readdir(dr)) != NULL)
    {

        if (child_obj->d_type == DT_DIR)
        {
            if (strcmp(child_obj->d_name, "..") != 0 && strcmp(child_obj->d_name, ".") != 0)
            {
                dir_num++;
                strcpy(new_path, abs_path);
                strcat(new_path, "/");
                strcat(new_path, child_obj->d_name);
                get_dir_num(new_path);
            }
        }
    }
    closedir(dr);
    return;
}

int main(int argc, char *argv[])
{

    int NTHREADS = atoi(argv[1]);
    enqueue(argv[2]);
    strcpy(search_str, argv[3]);

    get_dir_num(argv[2]);

    int *t_id = (int *)malloc(DIR_N_CHAR);

    for (int i = 0; i < NTHREADS; i++)
    {
        t_id[i] = i;
        
    }

    sem_init(&mutex, 0, 1);
    pthread_t thread[NTHREADS];

    for (int i = 0; i < NTHREADS; i++)
    {
        pthread_create(&thread[i], NULL, (void *)parallel_grep, &t_id[i]);
    }

    for (int i = 0; i < NTHREADS; i++)
    {
        pthread_join(thread[i], NULL);
    }

    sem_destroy(&mutex);
    free(t_id);
    return 0;
}

// REFERENCES:
// https://www.scaler.com/topics/c/implementation-of-queue-using-linked-list/
// https://linuxhint.com/opendir-3-c-function/
// https://linuxhint.com/readdir-3-c-function/
// https://linuxhint.com/realpath-3-c-function/
// https://stackoverflow.com/questions/23958040/checking-if-a-dir-entry-returned-by-readdir-is-a-directory-link-or-file-dent

// https://www.ibm.com/docs/en/zos/2.4.0?topic=functions-system-execute-command
// CS 140 Lab Exercise 8+9
