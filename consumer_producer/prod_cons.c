#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#define MAX_NO_OF_NODES 10
#define NO_OF_PROD 20
#define NO_OF_CONS 5


struct node{
    int data;
    struct node* next;
};

struct node* head = NULL;

sem_t semFull, semEmpty;
pthread_mutex_t mutex;


struct node* create_node(int data){
    struct node* new_node = (struct node*)malloc(sizeof(struct node));
    if(!new_node){
        perror("Malloc failed");
        return new_node;
    }

    new_node->data = data;
    new_node->next = NULL;

    return new_node;
    
}


void traverse(){
    for(struct node* curr = head;
        curr != NULL;
        curr = curr->next){
            printf("%d->",curr->data);
    }
    printf("\n");
}

struct node* add_node_at_start(){
    static int i = 0;
    struct node* new_node = create_node(++i);
    if(!new_node) return head;
    new_node->next = head;
    head = new_node;
    return head;
}



void* producder(){
    while(1){
        sem_wait(&semEmpty);
        pthread_mutex_lock(&mutex);
        head = add_node_at_start(); 
        printf("Produced %d\n",head->data);
        pthread_mutex_unlock(&mutex);
        sem_post(&semFull);
    }
}

struct node* delete_node_at_start(){
    if(!head) {
            //This should never be print
            printf("Nothing to consume\n");
            return NULL;
    }
    struct node* temp = head;
    head = head->next;
    free(temp);
    return head;
}

void* consumer(){
    while(1){
        sleep(3);
        sem_wait(&semFull);
        pthread_mutex_lock(&mutex);
        int val = head->data;
        head = delete_node_at_start();
        printf("Consumed %d\n", val);
        pthread_mutex_unlock(&mutex);
        sem_post(&semEmpty);
    }

    return NULL;
}


int main(){
    int ret;
    pthread_t th_cons[NO_OF_CONS], th_prod[NO_OF_PROD];
    pthread_mutex_init(&mutex, NULL);
    sem_init(&semFull, 0, 0);
    sem_init(&semEmpty, 0, MAX_NO_OF_NODES);
    traverse();

    for(int i = 0; i < NO_OF_CONS; i++){
        ret = pthread_create(&th_cons[i], NULL, consumer, NULL);
        if(ret < 0){
            perror("Thread creation failed");
            return i + 1;
        }
    }

    

    for(int i = 0; i < NO_OF_PROD; i++){
        ret = pthread_create(&th_prod[i], NULL, producder, NULL);
        if(ret < 0){
            perror("Thread creation failed");
            return i + 1;
        }
    }

    for(int i = 0; i < NO_OF_CONS; i++){
        ret = pthread_join(th_cons[i], NULL);
        if(ret < 0){
            perror("Thread join failed");
            return i + 4;
        }
    }

    for(int i = 0; i < NO_OF_PROD; i++){
        ret = pthread_join(th_prod[i], NULL);
        if(ret < 0){
            perror("Thread join failed");
            return i + 4;
        }
    }

    pthread_mutex_destroy(&mutex);
    sem_destroy(&semFull);
    sem_destroy(&semEmpty);

    return 0;

    
}
