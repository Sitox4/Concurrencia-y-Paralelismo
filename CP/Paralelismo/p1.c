#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

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

int main(int argc, char *argv[]){

  if(argc != 3){
    printf("Numero incorrecto de parametros\nLa sintaxis debe ser: program n L\n  program es el nombre del ejecutable\n  n es el tamaño de la cadena a generar\n  L es la letra de la que se quiere contar apariciones (A, C, G o T)\n");
    exit(1); 
  }
  int numprocs, rank; 
  int i, n, count=0, total=0;
  char *cadena;
  char L;

  //init
  MPI_Status status;
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD , &rank);

  if(rank==0){
  	n = atoi(argv[1]);
	  L = *argv[2]; //letra
    //Distribucion n y L con Send/Recv
    for(i=1; i<numprocs; i++){
	  MPI_Send(&n,1,MPI_INT,i,0, MPI_COMM_WORLD);
	  MPI_Send(&L,1,MPI_CHAR,i,0, MPI_COMM_WORLD);
  	}
	}else{
		MPI_Recv(&n,1,MPI_INT,0,0,MPI_COMM_WORLD,&status);
		MPI_Recv(&L,1,MPI_CHAR,0,0,MPI_COMM_WORLD,&status);
	}

  cadena = (char *) malloc(n*sizeof(char));
  inicializaCadena(cadena, n);
  
  //Reparto de carga
  for(i=rank; i<n; i+=numprocs){
    if(cadena[i] == L){
      count++;
    }
  }

  if(rank!=0){
    MPI_Send(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }
  else{
    for(i = 1; i < numprocs; i++){
        MPI_Recv(&total, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        count+=total;
     }
  }
  
  if(rank==0){
    printf("El numero de apariciones de la letra %c es %d\n", L, count);
  }
  free(cadena);
  
  MPI_Finalize();
  exit(0);
}