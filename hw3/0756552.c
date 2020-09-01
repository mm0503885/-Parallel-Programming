#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#ifndef W
#define W 20                                    // Width
#endif
int main(int argc, char **argv) {
  int L = atoi(argv[1]);                        // Length
  int iteration = atoi(argv[2]);                // Iteration
  srand(atoi(argv[3]));                         // Seed
  float d = (float) random() / RAND_MAX * 0.2;  // Diffusivity
  int *temp = malloc(L*W*sizeof(int));          // Current temperature
  int *next = malloc(L*W*sizeof(int));          // Next time step


  int my_rank; /* rank of process */
  int size;
  MPI_Status status;
  int top[W];
  int down[W];
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  
    for (int i = 0; i < L; i++) {
    for (int j = 0; j < W; j++) {
      temp[i*W+j] = random()>>3;
    }
  }
   int count = 0, balance = 0, all_balance = 0;  
  while (iteration--) {     // Compute with up, left, right, down points
    balance = 1;
    count++;
	
	if(my_rank==0)
	{
	  MPI_Send(temp+(L/size-1)*W, W, MPI_INT, 1, 111,MPI_COMM_WORLD);
	  MPI_Recv(down, W, MPI_INT, 1, 111,MPI_COMM_WORLD, &status);
	}
	else if(my_rank==1)
	{
	  MPI_Send(temp+(L/size*2-1)*W, W, MPI_INT, 2, 111,MPI_COMM_WORLD);
	  MPI_Send(temp+(L/size)*W, W, MPI_INT, 0, 111,MPI_COMM_WORLD);
	  MPI_Recv(down, W, MPI_INT, 2, 111,MPI_COMM_WORLD, &status);
	  MPI_Recv(top, W, MPI_INT, 0, 111,MPI_COMM_WORLD, &status);
	
    }
	else if(my_rank==2)
	{
	  MPI_Send(temp+(L/size*3-1)*W, W, MPI_INT, 3, 111,MPI_COMM_WORLD);
	  MPI_Send(temp+(L/size*2)*W, W, MPI_INT, 1, 111,MPI_COMM_WORLD);
	  MPI_Recv(down, W, MPI_INT, 3, 111,MPI_COMM_WORLD, &status);
	  MPI_Recv(top, W, MPI_INT, 1, 111,MPI_COMM_WORLD, &status);

    }
	else
	{
	  MPI_Send(temp+(L/size*3)*W, W, MPI_INT, 2, 111,MPI_COMM_WORLD);
      MPI_Recv(top, W, MPI_INT, 2, 111,MPI_COMM_WORLD, &status);
	  
	}
	
	for (int i = my_rank*L/4; i < (my_rank+1)*L/4; i++) {
      for (int j = 0; j < W; j++) {
        float t = temp[i*W+j] / d;
        t += temp[i*W+j] * -4;
		if(i==L/4*my_rank&&i!=0) t+= top[j];
		else if (i-1<0) t+=temp[j];
		else t+=temp[(i-1)*W+j];
		if((i+1)>=L/4*(my_rank+1)&&my_rank!=(size-1)) t+= down[j];
		else if (i==L-1) t+=temp[i*W+j];
		else t+=temp[(i+1)*W+j];
        
        t += temp[i*W+(j - 1 <  0 ? 0 : j - 1)];
        t += temp[i*W+(j + 1 >= W ? j : j + 1)];
        t *= d;
        next[i*W+j] = t ;
        if (next[i*W+j] != temp[i*W+j]) {
          balance = 0;
        }
      }
    }
	MPI_Allreduce(&balance, &all_balance, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
	if (all_balance) {
      break;
    }
	int *tmp = temp;
    temp = next;
    next = tmp;
	

  }
  int total_min=0;
  int min = temp[my_rank*L/4*W];
  for (int i = my_rank*L/4; i < (my_rank+1)*L/4; i++) {
    for (int j = 0; j < W; j++) {
      if (temp[i*W+j] < min) {
        min = temp[i*W+j];
      }
    }
  }
  MPI_Reduce(&min,&total_min,1,MPI_INT,MPI_MIN,0,MPI_COMM_WORLD);
  if(my_rank==0)
    printf("Size: %d*%d, Iteration: %d, Min Temp: %d\n", L, W, count, total_min);
  MPI_Finalize();
  return 0;
}