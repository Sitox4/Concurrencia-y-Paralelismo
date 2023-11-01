#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <threads.h>
#include <pthread.h>
#include "options.h"

#define DELAY_SCALE 1000

//Ejercicio 4

struct array {
    int size;
    int *arr;
};
typedef struct str_increment {
    
    int id;
    int iterations;
    int delay;
    struct array *arr;
    struct thr_args *arg;
    int *total;	
}str;

struct thr_args{
    mtx_t mutex;
};


void apply_delay(int delay) {
    for(int i = 0; i < delay * DELAY_SCALE; i++); // waste time
}


int increment(void *ptr)
{
    int pos, val;
    struct str_increment str1 = *(struct str_increment *)ptr;
    int *total = str1.total;
    	
    for(int i = 0; i < str1.iterations; i++) {
        
        pos = rand() % str1.arr->size;
        mtx_lock(&str1.arg[pos].mutex);
        printf("%d increasing position %d\n",str1.id, pos);

        val = str1.arr->arr[pos];
        apply_delay(str1.delay);
	
        val ++;
        apply_delay(str1.delay);
	
        str1.arr->arr[pos] = val;
        apply_delay(str1.delay);
        mtx_unlock(&str1.arg[pos].mutex);
        
        (*total)++;
    }
    

    return 0;
}

int move(void *ptr){

    int pos1,pos2, val;
    struct str_increment str1 = *(struct str_increment *)ptr;
    int *total = str1.total;
    
    for(int i = 0; i < str1.iterations; i++) {
    
        do{
          pos1 = rand() % str1.arr->size;
          pos2 = rand() % str1.arr->size;
        }while(pos1==pos2);
        
        
        if(pos1<pos2){
          mtx_lock(&str1.arg[pos1].mutex);
          mtx_lock(&str1.arg[pos2].mutex);
        }else{
          mtx_lock(&str1.arg[pos2].mutex);
          mtx_lock(&str1.arg[pos1].mutex);
        }
        printf("%d restando 1 en la posicion %d y sumando 1 en la posicion %d \n",str1.id, pos1,pos2);

        val = str1.arr->arr[pos1];
        //apply_delay(str1.delay);
        val --;
        //apply_delay(str1.delay);
        str1.arr->arr[pos1] = val;
        //apply_delay(str1.delay);
        
        val = str1.arr->arr[pos2];
        //apply_delay(str1.delay);
        val ++;
        //apply_delay(str1.delay);
        str1.arr->arr[pos2] = val;
        //apply_delay(str1.delay);
        
        if(pos1<pos2){
           mtx_unlock(&str1.arg[pos1].mutex);
           mtx_unlock(&str1.arg[pos2].mutex);
        }else{
           mtx_unlock(&str1.arg[pos2].mutex);
           mtx_unlock(&str1.arg[pos1].mutex);
        }
        (*total)++;
         
    }
    

    return 0;
    
}


void print_array(struct array arr) {
    int total = 0;

    for(int i = 0; i < arr.size; i++) {
        total += arr.arr[i];
        printf("%d ", arr.arr[i]);
    }

    printf("\nTotal: %d\n", total);
}


int main (int argc, char **argv)
{   
    
    
    int i;
    int total =0;
    struct options       opt;
    struct array         arr;
    
    
    srand(time(NULL));

    // Default values for the options
    opt.num_threads  = 5;
    opt.size         = 10;
    opt.iterations   = 100;
    opt.delay        = 1000;
    
    
    read_options(argc, argv, &opt);
    
    str str1[opt.num_threads],str2[opt.num_threads];
    thrd_t thr1[opt.num_threads],thr2[opt.num_threads];
    struct thr_args arg[opt.size];
    
    arr.size = opt.size;
    arr.arr  = malloc(arr.size * sizeof(int));

    memset(arr.arr, 0, arr.size * sizeof(int));
    for(i=0;i<opt.size;i++){
       mtx_init(&arg[i].mutex,mtx_plain);
    }
    for(i=0;i<opt.num_threads;i++){
       str1[i].id=i;
       str1[i].iterations=opt.iterations/opt.num_threads;
       str1[i].delay=opt.delay;
       str1[i].arr = &arr;
       str1[i].arg = arg;
       str1[i].total = &total;
       
       str2[i].id=i;
       str2[i].iterations=opt.iterations/opt.num_threads;
       str2[i].delay=opt.delay;
       str2[i].arr = &arr;
       str2[i].arg = arg;
       str2[i].total = &total;
       
       thrd_create(&thr1[i], increment, &str1[i] );
       thrd_create(&thr2[i], move, &str2[i] );
    }
    for(i=0;i<opt.num_threads;i++){
       thrd_join(thr1[i], NULL);
       thrd_join(thr2[i], NULL);
    }
    for(i=0;i<opt.size;i++){
       mtx_destroy(&arg[i].mutex);
    }
    print_array(arr);
    printf("Total iterations: %d\n", total);
    free(arr.arr);
    


    return 0;
}
