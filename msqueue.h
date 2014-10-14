#ifndef __MSQUEUE_H
#define __MSQUEUE_H

#include <pthread.h> /* pthread_spinlock */
#include "tsc.h"
#include "parallel_benchmarks.h"

struct pointer_t{
    __int128 both;
};

__int128 get_count(__int128 a){

    __int128 b = a >>64;
    return b;
}

#define get_pointer(a) (a&0x0000000000000000ffffffffffffffff)
/*
__int128 get_pointer(__int128 a){
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

struct node_t{
	int value;//TODO: maybe change this
	struct pointer_t  next;
};

struct queue_t{
	struct pointer_t  Head;
    int64_t pad;
	struct pointer_t  Tail;
};

void initialize(struct queue_t * Q,int num_threads){
	struct node_t * node = (struct node_t *) malloc(sizeof(struct node_t));
	node->next.both = 0;
	Q->Head.both= set_both(Q->Head.both,node,0); 
	Q->Tail.both= set_both(Q->Tail.both,node,0);
}

void enqueue(struct queue_t * Q, int val,params_t * params){
	
	struct pointer_t tail;
    __int128 new_to_set;
	int temp;	
	struct node_t * node = (struct node_t *) malloc(sizeof(struct node_t));
	node->value = val;
	node->next.both = 0;
	while (1){
		tail = Q->Tail;
		struct pointer_t next_p = ((struct node_t *)get_pointer(tail.both))->next;
		if (tail.both == Q->Tail.both){
			if (get_pointer(next_p.both) == 0){
                new_to_set =set_both(new_to_set,node,get_count(next_p.both) +(__int128)1);
				if (__sync_bool_compare_and_swap(&(((struct node_t * )get_pointer(tail.both))->next.both),next_p.both,new_to_set)){
					
                    if(params->last_cas==1)
                            params->curr_streak++;
                    else {
                        params->curr_streak=1;
                        params->last_cas=1;
                    }
                    break;
                 }
                 else{
                    if (params->curr_streak > params->max_streak){
                        params->max_streak = params->curr_streak;
                    }
                    params->curr_streak =0;
                    params->last_cas=0;
                }
			}
			else{
                new_to_set=set_both(new_to_set,next_p.both,get_count(tail.both)+(__int128)1);
				temp = __sync_bool_compare_and_swap(&Q->Tail.both,tail.both,new_to_set);
                if (temp){
                    if(params->last_cas==1)
                            params->curr_streak++;
                    else {
                        params->curr_streak=1;
                        params->last_cas=1;
                    }
                }
                else{
                    if (params->curr_streak > params->max_streak){
                        params->max_streak = params->curr_streak;
                    }
                    params->curr_streak =0;
                    params->last_cas=0;
                }
			}
		}
	}
    new_to_set=set_both(new_to_set,node,get_count(tail.both)+(__int128)1);
	temp = __sync_bool_compare_and_swap(&Q->Tail.both,tail.both,new_to_set);
    if (temp){
        if(params->last_cas==1)
                params->curr_streak++;
        else {
            params->curr_streak=1;
            params->last_cas=1;
        }
    }
    else{
        if (params->curr_streak > params->max_streak){
            params->max_streak = params->curr_streak;
        }
        params->curr_streak =0;
        params->last_cas=0;
    }
}

int dequeue(struct queue_t * Q,int * pvalue,params_t * params){

	struct pointer_t head;
	struct pointer_t tail;
	struct pointer_t next;
	int  temp;
	int first_val;
    __int128 new_to_set;
	while(1){
		head =  Q->Head;
        first_val=((struct node_t *)get_pointer(head.both))->value;
		tail =  Q->Tail;
		next =  ((struct node_t *)get_pointer(head.both))->next;
		if ( head.both == Q->Head.both){
			if (head.both == tail.both){
				if ( get_pointer(next.both) == 0) 
					return 0;
                new_to_set =  set_both(new_to_set,next.both,get_count(tail.both) +(__int128)1);
				temp = __sync_bool_compare_and_swap(&Q->Tail.both,tail.both,new_to_set);
			}
			else{
                //if ((head==Q->Head) &&(first_val!=((struct node_t *)get_pointer(head))->value)) printf("change detected!\n");
				*pvalue =((struct node_t *)get_pointer(next.both))->value;
                new_to_set = set_both(new_to_set,next.both,get_count(head.both)+(__int128)1);
				if( __sync_bool_compare_and_swap(&Q->Head.both,head.both,new_to_set))
					break;
			}
		}
	}
    //printf(" about to free %p \n",head);
	free(get_pointer(head.both));
	return 1;
}

long long int find_element_sum(struct queue_t * Q){
    
    struct pointer_t  curr ;
    struct pointer_t  next;
    long long res=0;
    int val;
    curr = Q->Head;
	next = ((struct node_t * )get_pointer(Q->Head.both))->next;
	while ((get_pointer(curr.both) != get_pointer(Q->Tail.both))&&(get_pointer(curr.both)!=0)){
        val =((struct node_t *)get_pointer(curr.both))->value;
        res+=val;
		curr = next;
		if (get_pointer(next.both)) next = ((struct node_t * )get_pointer(curr.both))->next;
    }
    val =((struct node_t *)get_pointer(curr.both))->value;
    res+=val;
    res-=((struct node_t *)get_pointer(Q->Head.both))->value;
    //printf("%\n%d\n",((struct node_t *)get_pointer(Q->Tail.both))->value);
    //printf("%\n%d\n",((struct node_t *)get_pointer(Q->Head.both))->value);
    return res;
}
void printqueue(struct queue_t * Q){
	
	struct pointer_t curr ;
	struct pointer_t next ;
	
	curr = Q->Head;
    //printf(" in printqueue curr = %p\n",curr);
	next = ((struct node_t * )get_pointer(Q->Head.both))->next;
    //printf(" in printqueue next = %p\n",next);
	while ((get_pointer(curr.both) != get_pointer(Q->Tail.both))&&(get_pointer(curr.both)!=0)){
    //printf(" in printqueue curr = %p\n",curr);
    //printf(" in printqueue next = %p\n",next);
		printf("%d ",((struct node_t * )get_pointer(curr.both))->value);
		curr = next;
		if (get_pointer(next.both)) next = ((struct node_t * )get_pointer(curr.both))->next;
	}
	printf("%d ",((struct node_t * )get_pointer(curr.both))->value);
	printf("\n");
	
}

#endif /* __MSQUEUE_H */
