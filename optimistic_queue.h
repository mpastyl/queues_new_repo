#ifndef __OPTIMISTIC_H
#define __OPTIMISTIC_H

#include <pthread.h> /* pthread_spinlock */
#include "tsc.h"
#include "parallel_benchmarks.h"

struct pointer_t{
    __int128 both;
};

struct node_t{
    int value;
    struct pointer_t  next;
    struct pointer_t  prev;
};


struct queue_t{
    struct pointer_t Tail;
    //char pad1[(8-sizeof(struct pointer_t))/sizeof(char)];
    struct pointer_t Head;
    //char pad1[(8-sizeof(struct pointer_t))/sizeof(char)];
}__attribute__ ((packed));


int DUMMY_VAL = INT32_MAX;

__int128 get_count(__int128 a){

    __int128 b = a >>64;
    return b;
}

#define get_pointer(a) (a&0x0000000000000000ffffffffffffffff)
/*__int128 get_pointer(__int128 a){
    __int128 b = a << 64;
    b= b >>64;
    return b;
}
*/

__int128 set_count(__int128  a, __int128 count){
    __int128 count_temp =  count << 64;
    __int128 b = get_pointer(a);
    b = b | count_temp;
    return b;
}

#define set_pointer(a, ptr) ((a & 0xffffffffffffffff0000000000000000)| \
        (ptr & 0x0000000000000000ffffffffffffffff))
    
/*__int128 set_pointer(__int128 a, __int128 ptr){
    __int128 b = 0;
    __int128 c = get_count(a);
    b = set_count(b,c);
    ptr = get_pointer(ptr);
    b= b | ptr;
    return b;
}
*/
__int128 set_both(__int128 a, __int128 ptr, __int128 count){
    a=set_pointer(a,ptr);
    a=set_count(a,count);
    return a;
}


void initialize(struct queue_t * Q, int num_threads){
    struct node_t * node = (struct node_t * ) malloc(sizeof(struct node_t));
    //node->next =  NULL;
    //node->prev =  NULL;
    node->value =  DUMMY_VAL;
    Q->Head.both  =  set_both(Q->Head.both,node,0);
    Q->Tail.both =  set_both(Q->Tail.both,node,0);
}


void enqueue(struct queue_t * Q,int val,params_t *params){
    
    struct pointer_t  tail;
    struct node_t * nd = (struct node_t *) malloc(sizeof(struct node_t));
    //printf("%p \n",&nd);
    nd->value = val;
    while (1){
        tail = Q->Tail;
        nd->next.both = set_both(nd->next.both,tail.both,get_count(tail.both)+(__int128)1);
        __int128 new_to_set = set_both(new_to_set,nd,get_count(tail.both)+(__int128)1);
        //__int128 temp =  Q->Tail.both;
        if (__sync_bool_compare_and_swap(&(Q->Tail.both),tail.both,new_to_set)){
            struct pointer_t prev =  ((struct node_t *)get_pointer(tail.both))->prev;
            prev.both = set_both(prev.both,nd,get_count(tail.both));
            ((struct node_t * ) get_pointer(tail.both))->prev.both = prev.both;
            break;
        }
    }

}

void fixList(struct queue_t * Q,struct pointer_t tail,struct pointer_t head){
    
    struct pointer_t currNode;
    struct pointer_t currNodeNext;
    struct pointer_t nextNodePrev;

    currNode =  tail;
    while((head.both == Q->Head.both)&&(currNode.both != head.both)){
        currNodeNext = ((struct node_t *)get_pointer(currNode.both))->next;
        if( get_count(currNodeNext.both)!=get_count(currNode.both)){
            return;
        }
        nextNodePrev = ((struct node_t *)get_pointer(currNodeNext.both))->prev;
        if((get_pointer(nextNodePrev.both)!=get_pointer(currNode.both))||(get_count(nextNodePrev.both)!=(get_count(currNode.both) -(__int128)1))){
            ((struct node_t *)get_pointer(currNodeNext.both))->prev.both = set_both(((struct node_t *)get_pointer(currNodeNext.both))->prev.both,currNode.both,get_count(currNode.both)-(__int128)1);
        }
        currNode.both = set_both(currNode.both,currNodeNext.both,get_count(currNode.both) -(__int128)1);
    }
}

int dequeue(struct queue_t * Q,int * p_val, params_t * params){
    
    struct pointer_t  tail;
    struct pointer_t  head;
    struct pointer_t  firstNodePrev;
    int val;
    __int128 new_to_set;
    while (1){

        head = Q->Head;
        tail = Q->Tail;
        firstNodePrev = ((struct node_t *)get_pointer(head.both))->prev;
        val =  ((struct node_t * )get_pointer(head.both))-> value;
        if (head.both == Q->Head.both){
            if (val!=DUMMY_VAL){
                if (tail.both!=head.both){
                    if (get_count(firstNodePrev.both)!=get_count(head.both)){
                        fixList(Q,tail,head);
                        continue;
                    }
                }

                else{
                    struct node_t * nd_dummy = (struct node_t * ) malloc(sizeof(struct node_t));
                    nd_dummy->value =  DUMMY_VAL;
                    nd_dummy->next.both = set_both(nd_dummy->next.both,tail.both,get_count(tail.both)+(__int128)1);
                    new_to_set = set_both(new_to_set,nd_dummy,get_count(tail.both)+(__int128)1);
                    if (__sync_bool_compare_and_swap(&(Q->Tail.both),tail.both,new_to_set)){
                        ((struct node_t *)get_pointer(head.both))->prev.both =set_both(((struct node_t *)get_pointer(head.both))->prev.both,nd_dummy,get_count(tail.both));
                    }
                    else free(nd_dummy);
                    continue;
                }
                new_to_set =  set_both(new_to_set,firstNodePrev.both,get_count(head.both)+(__int128)1);
                if(__sync_bool_compare_and_swap(&(Q->Head.both),head.both,new_to_set)){
                    free((struct node_t *)get_pointer(head.both));
                    *p_val = val;
                    return 1;
                }
            }
            else{
                if (get_pointer(tail.both) == get_pointer(head.both)) return 0;
                else {
                    //TODO: add tag check and call fixlist
                    if(get_count(firstNodePrev.both) != get_count(head.both)){
                        fixList(Q,tail,head);
                        continue;
                    }
                    new_to_set = set_both(new_to_set,firstNodePrev.both,get_count(head.both)+(__int128)1);
                    int temp = __sync_bool_compare_and_swap(&(Q->Head.both),head.both,new_to_set);
                }
            }
        }
    }


            
}

long long int find_element_sum(struct queue_t * Q){
    
    struct pointer_t  curr ;
    struct pointer_t  prev;
    long long res=0;
    int val;
    curr = Q->Head;
    prev = ((struct node_t *)get_pointer(Q->Head.both))->prev;
    while (get_pointer(curr.both) != get_pointer(Q->Tail.both)){
        val =((struct node_t *)get_pointer(curr.both))->value;
        if(val!=DUMMY_VAL) res+=val;
        curr = prev;
        prev = ((struct node_t *)get_pointer(curr.both)) ->prev;
    }
    val =((struct node_t *)get_pointer(curr.both))->value;
    if(val!=DUMMY_VAL) res+=val;
    //res-=((struct node_t *)get_pointer(Q->Head.both))->value;
    //printf("%\n%d\n",((struct node_t *)get_pointer(Q->Tail.both))->value);
    //printf("%\n%d\n",((struct node_t *)get_pointer(Q->Head.both))->value);
    return res;
}

void printqueue(struct queue_t * Q){

    struct pointer_t  curr ;
    struct pointer_t  prev;
    curr = Q->Head;
    prev = ((struct node_t *)get_pointer(Q->Head.both))->prev;
    while (get_pointer(curr.both) != get_pointer(Q->Tail.both)){
        printf("%d \n",((struct node_t *)get_pointer(curr.both))->value);
        curr = prev;
        prev = ((struct node_t *)get_pointer(curr.both)) ->prev;
    }
    printf("%d ",((struct node_t *)get_pointer(curr.both))->value);
    printf("\n");

}

#endif /* __OPTIMITIC_H */
