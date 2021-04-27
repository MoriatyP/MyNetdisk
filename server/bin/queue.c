#include "../include/server.h"

//初始化
void init_queue(queue_t *q, int quesize){
    q->BUF = (ptask_t)calloc(quesize + 1, sizeof(task_t));
    q->front = q->rear = 0;
    q->quesize = quesize + 1;
    pthread_mutex_init(&q->queMutex, NULL);
}

//清理栈
void free_queue(queue_t *q){
    q->front = q->rear = q->quesize = 0;
    free(q->BUF);
}

//判空
int is_empty_queue(queue_t *q){
    if (q->front == q->rear){
        return 1;
    }
    else{
        return 0;
    }
}

//判满
int is_full_queue(queue_t *q){
    if ((q->rear + 1) % q->quesize == q->front){
        return 1;
    }
    else{
        return 0;
    }
}
//入队
void in_queue(queue_t *q, task_t value){
    if (!is_full_queue(q)){//队列未满
        q->BUF[q->rear] = value;
        q->rear = (q->rear + 1) % q->quesize; //尾指针偏移
    }
}

//出队
void de_queue(queue_t *q, ptask_t value){
    if (!is_empty_queue(q)){//队列未空
        *value = q->BUF[q->front];
        q->front = (q->front + 1) % q->quesize;
    }
}
