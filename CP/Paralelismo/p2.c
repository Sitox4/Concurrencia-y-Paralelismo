#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

void inicializaCadena(char *cadena, int n){
    int i;
    for(i=0; i<n/2; i++){
        cadena[i] = 'A';
    }
    for(i=n/2; i<3*n/4; i++){
        cadena[i] = 'C';
    }
    for(i=3*n/4; i<9*n/10; i++){
        cadena[i] = 'G';
    }
    for(i=9*n/10; i<n; i++){
        cadena[i] = 'T';
    }
}

int MPI_FlattreeColectiva (void * buffer, void *rec_buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
        int numprocs, rank;
        int *rvbuf = rec_buffer;
        MPI_Status error_st;
        MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        int *count_sum[numprocs - 1];

        if (rank != 0) {
            MPI_Send(buffer, count, datatype, root, 100, comm);
        }
        else if (rank == 0){ 
            count_sum[0] = buffer;
            *rvbuf = *count_sum[0];
            for (int i = 1; i < numprocs; i++) {  
                MPI_Recv(buffer, count, datatype, i, 100, comm, &error_st);
                count_sum[i] = buffer;
                *rvbuf += *count_sum[i];
            }
        }
        return 0;
}

int MPI_BinomialColectiva (void * buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
        int numprocs, rank;
        MPI_Status error_st;
        MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        double lognumprocs = log2(numprocs);

            for (int i = 1; i <= ceil(lognumprocs); i++) {
                //Los procesos con rank < 2^i-1 se comunican con rank + 2^i-1
                if ((pow(2, i-1) > rank) && ((rank + pow(2, i-1)) < numprocs)) {
                    MPI_Send(buffer, count, datatype, (rank + pow(2, i-1)), 100, comm);  
                }
                else if (pow(2, i) > rank) {        
                    MPI_Recv(buffer, count, datatype, (rank - pow(2, i-1)), 100, comm, &error_st);
                }         
            }        
        return 0;
}

int main(int argc, char *argv[]){
    int rank, numprocs;
    int i, n, count=0, count_total=0;
    char *cadena;
    char L;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    if(rank == 0){
        if(argc != 3){
            printf("Numero incorrecto de parametros\nLa sintaxis debe ser: program n L\n  program es el nombre del ejecutable\n  n es el tamaño de la cadena a generar\n  L es la letra de la que se quiere contar apariciones (A, C, G o T)\n");
            exit(1);
        }

        n = atoi(argv[1]);
        L = *argv[2];

        // Distribuir n y L a todos los procesos
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&L, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
        //MPI_BinomialColectiva(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        //MPI_BinomialColectiva(&L, 1, MPI_CHAR, 0, MPI_COMM_WORLD);

        cadena = (char *) malloc(n*sizeof(char));
        inicializaCadena(cadena, n);

        // Repartir la carga de trabajo
        for(i=rank; i<n; i+=numprocs){
            if(cadena[i] == L){
                count++;
            }
        }

        // Recopilar los resultados de todos los procesos
        //MPI_Reduce(&count, &count_total, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_FlattreeColectiva(&count,&count_total, 1, MPI_INT, 0, MPI_COMM_WORLD);

        printf("El numero de apariciones de la letra %c es %d\n", L, count_total);
        free(cadena);
    } else {
        // Recibir n y L del proceso 0
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&L, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
        //MPI_BinomialColectiva(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        //MPI_BinomialColectiva(&L, 1, MPI_CHAR, 0, MPI_COMM_WORLD);

        cadena = (char *) malloc(n*sizeof(char));
        inicializaCadena(cadena, n);

        // Repartir la carga de trabajo
        for(i=rank; i<n; i+=numprocs){
            if(cadena[i] == L){
                count++;
            }
        }

        // Recopilar los resultados de todos los procesos
        //MPI_Reduce(&count, &count_total, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_FlattreeColectiva(&count,&count_total, 1, MPI_INT, 0, MPI_COMM_WORLD);

        free(cadena);
    }

    MPI_Finalize();
    exit(0);
}