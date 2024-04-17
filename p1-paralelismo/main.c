#include <stdio.h>
#include <math.h>
#include <mpi.h>

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
            //hay q enviar el dato al resto de procesos
            for(j=0; j<numprocs; j++){
                if(j != rango)
                    MPI_Send(&n, 1, MPI_INT, j, 0, MPI_COMM_WORLD);
            }
            pitotal=0;
        } else {
            //el resto de procesos deben recibir el dato
            MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        if (n == 0) break; //n = nº intervalos

        h   = 1.0 / (double) n;
        sum = 0.0;
        for (i = rango+1; i <= n; i += numprocs) {
            x = h * ((double)i - 0.5);
            sum += 4.0 / (1.0 + x*x);
        }
        pi = h * sum;

        //los procesos deben enviar el dato(pi)
        if(rango == 0) {
            pitotal = pi;
            //el proceso 0 debe recibir el dato(pi) para poder imprimirlo
            for(j=1; j<numprocs; j++) {
                MPI_Recv(&pi, 1, MPI_DOUBLE, j, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                pitotal += pi;
                
            }
            printf("pi is approximately %.16f, Error is %.16f\n", pitotal, fabs(pitotal - PI25DT));
        } else {
            MPI_Send(&pi, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
    }
    MPI_Finalize();
}
//Probar el código poniendo intervalos de la integral grandes