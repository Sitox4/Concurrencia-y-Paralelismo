#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

//int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
//int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)

int MPI_FlattreeColectiva(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
    //Comprobar el envío y recibimiento de los procesos, igual q en la p1, el root recibe del resto de procesos
    
    if(op != MPI_SUM)        //Comprobación para saber q es una suma (en el enunciado pone asumir q siempre es suma)
        return MPI_ERR_OP;    
    else if(datatype != MPI_DOUBLE)
        return MPI_ERR_TYPE;
    else if(root != 0)
        return MPI_ERR_ROOT;

    int j, rank, numprocs;
    double total = *(double*)sendbuf;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &numprocs);

    if(rank == root) {
        for(j=1; j<numprocs; j++) {  
            if (MPI_Recv(recvbuf, count, datatype, MPI_ANY_SOURCE, 0, comm, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
                return MPI_ERR_OP;
            } else {
                double *recepcion = (double*)recvbuf;
                total += *recepcion;
            }
        }
        *(double*)recvbuf = total;
    } else {
        if(MPI_Send(sendbuf, count, datatype, root, 0, comm) != MPI_SUCCESS)
            return MPI_ERR_OP;
    }
    return MPI_SUCCESS;
}

int MPI_BinomialColectiva(void * buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm){
    int i, numprocs, rank;
    int tag = 0;
    MPI_Comm_size(comm, &numprocs);
    MPI_Comm_rank(comm, &rank);
    
    if(rank != root){
        if (MPI_Recv(buf, count, datatype, MPI_ANY_SOURCE, tag, comm, MPI_STATUS_IGNORE) != MPI_SUCCESS)
            return MPI_ERR_OP;
    }
    for(i = 0; rank + pow(2,i) < numprocs; i++){
        if(rank < pow(2,i)) {
            if (MPI_Send(buf, count, datatype, rank+pow(2,i), tag, comm) != MPI_SUCCESS)
                return MPI_ERR_OP;
        }
    }
    return MPI_SUCCESS;
}

int main(int argc, char *argv[])
{
    int i, done = 0, n;
    int rango; //nº del proceso, identificador, proceso individual, ej.proceso0, proceso1, etc.
    int numprocs; //nº total de procesos
    int j;
    double PI25DT = 3.141592653589793238462643;
    double pi, h, sum, x;
    double pitotal; //sumatorio de pi de todos los procesos

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rango);

    while (!done)
    {
        if(rango == 0) {
            printf("Enter the number of intervals: (0 quits) \n");
            scanf("%d",&n);
        } 

        //el proceso 0 (root) envía el parámetro n al resto de procesos
        MPI_BinomialColectiva(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (n == 0) break; //n = nº intervalos

        h   = 1.0 / (double) n;
        sum = 0.0;
        for (i = rango+1; i <= n; i += numprocs) {
            x = h * ((double)i - 0.5);
            sum += 4.0 / (1.0 + x*x);
        }
        pi = h * sum;
        //el proceso 0 recupera el dato pi de todos los procesos
        MPI_FlattreeColectiva(&pi, &pitotal, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);  //se utiliza Reduce porque el Gather crearía un array, en vez de sumar los resultados
        
        if (rango ==0) 
            printf("pi is approximately %.16f, Error is %.16f\n", pitotal, fabs(pitotal - PI25DT));
        
    }
    MPI_Finalize();
}
//Probar el código poniendo intervalos de la integral grandes