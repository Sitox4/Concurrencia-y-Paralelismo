#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <mpi.h>

#define DEBUG 2

/* Translation of the DNA bases
   A -> 0
   C -> 1
   G -> 2
   T -> 3
   N -> 4*/

#define M  31 // Number of sequences
#define N  20  // Number of bases per sequence

unsigned int g_seed = 0;

int fast_rand(void) {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16) % 5;
}

// The distance between two bases
int base_distance(int base1, int base2){

  if((base1 == 4) || (base2 == 4)){
    return 3;
  }

  if(base1 == base2) {
    return 0;
  }

  if((base1 == 0) && (base2 == 3)) {
    return 1;
  }

  if((base2 == 0) && (base1 == 3)) {
    return 1;
  }

  if((base1 == 1) && (base2 == 2)) {
    return 1;
  }

  if((base2 == 2) && (base1 == 1)) {
    return 1;
  }

  return 2;
}

int main(int argc, char *argv[] ) {

  int numprocs, rank;
  int *rows1, *rows2, *data1, *data2;
  int i, j, n;
  int *root_result, *result;
  int t_com, t_op;
  
  struct timeval tv1, tv2, tv3, tv4; 
  
  MPI_Status status;
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  n = ceil(M / numprocs);  // Padding redondea hacia arriba<-----
 
  if(rank == 0){  
    /* Initialize Matrices */
    data1 = (int *) malloc(M* N * sizeof(int));
    data2 = (int *) malloc(M* N * sizeof(int));
    root_result = (int *) malloc(M* sizeof(int));
   
    for(i=0;i<M;i++) {//<--------
         for(j=0;j<N;j++) {
          /* random with 20% gap proportion */
         data1[i*N+j] = fast_rand();
         data2[i*N+j] = fast_rand();
         }
     }   
  }   
  
  rows1 = (int *) malloc(n * N * sizeof(int));
  rows2 = (int *) malloc(n * N * sizeof(int));
  result = (int *) malloc(n * sizeof(int));
  
  gettimeofday(&tv1, NULL);
  // envio data1
  MPI_Scatter(data1, n*N, MPI_INT, rows1, n*N, MPI_INT, 0, MPI_COMM_WORLD); 
  // envio data2 
  MPI_Scatter(data2, n*N, MPI_INT, rows2, n*N, MPI_INT, 0, MPI_COMM_WORLD);  
  
  gettimeofday(&tv2, NULL);

  for(i=0;i<n;i++) {
    result[i]=0;
    for(j=0;j<N;j++) {
      result[i] += base_distance(rows1[i*N+j], rows2[i*N+j]);
    }
  }

  gettimeofday(&tv3, NULL);
  
  MPI_Gather(result, n, MPI_INT, root_result, n, MPI_INT, 0, MPI_COMM_WORLD);  
  gettimeofday(&tv4, NULL);
  
  t_com = ((tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec)) + ((tv4.tv_usec - tv3.tv_usec) + 1000000 * (tv4.tv_sec - tv3.tv_sec));
  
  t_op = (tv3.tv_usec - tv2.tv_usec) + 1000000 * (tv3.tv_sec - tv2.tv_sec);
    
  /* Display result */
  if(rank == 0){
    if (DEBUG == 1) {
        int checksum = 0;
        for(i=0;i<M;i++) {//<------
            checksum += root_result[i];
        }
        printf("Checksum: %d\n ", checksum);
    } else if (DEBUG == 2) {
        for(i=0;i<M;i++) {//<----
            printf(" %d \t\n", root_result[i]);
        }
  } else 
      printf ("\n");
  }
  
  if(rank != 0){
    // Enviar tiempo comumicacion
    MPI_Send(&t_com, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    // Enviar tiempo conmutacion
    MPI_Send(&t_op, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);  
  }
  else {
    printf("Proceso %d - Tiempo comunicación: %lf\n", rank, (double) t_com/1E6);
    printf("Proceso %d - Tiempo conmutacion: %lf\n", rank, (double) t_op/1E6);
    for(i = 1; i < numprocs; i++){
        // Recibir tiempos
        MPI_Recv(&t_com, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status); 
        MPI_Recv(&t_op, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
        
        printf("Proceso %d - Tiempo comunicación: %lf\n", i, (double) t_com/1E6);
        printf("Proceso %d - Tiempo conmutacion: %lf\n", i, (double) t_op/1E6);
    }
  }

  if(rank == 0){
    free(data1); free(data2); free(root_result);
  }
  
  free(rows1); free(rows2); free(result);
  MPI_Finalize();

  return 0;
}