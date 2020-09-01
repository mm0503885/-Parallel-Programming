/**********************************************************************
 * DESCRIPTION:
 *   Serial Concurrent Wave Equation - C Version
 *   This program implements the concurrent wave equation
 *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <cuda.h>

#define MAXPOINTS 1000000
#define MAXSTEPS 1000000
#define MINPOINTS 20
#define PI 3.14159265

void check_param(void);
void init_line(void);
void printfinal (void);


float *g_value, *gold_value, *g_new_value;
int size = MAXPOINTS+2;
int nsteps,                 	/* number of time steps */
    tpoints, 	     		/* total points along string */
    rcode;                  	/* generic return code */
float  values[MAXPOINTS+2], 	/* values at time t */
       oldval[MAXPOINTS+2], 	/* values at time (t-dt) */
       newval[MAXPOINTS+2]; 	/* values at time (t+dt) */
	   
	   
	   
__global__ void gpu_init_old_value(float *a, float *b, float *c, int i)
{
        int j=blockIdx.x*blockDim.x+threadIdx.x, m=gridDim.x*blockDim.x;
        for(int k=j; k<i; k+=m)
		{
		  a[k] = b[k];
        }
        __syncthreads();
}
__global__ void gpu_update_point(float *a, float *b, float *c, int p, int nsteps)
{
                int i=blockIdx.x*blockDim.x+threadIdx.x;
                float aval = a[i], bval = b[i];
                float cval;
                if (i < p) 
				{
                  for (int j = 0;j<nsteps;j++)
				  {
                    if ((i== 0) || (i  == p - 1))
                        cval = 0.0;
                    else
                        cval = (2.0 * bval) - aval + (0.09 * (-2.0)*bval);
                        aval = bval;
                        bval = cval;
                        __syncthreads();
                  }
                }
                b[i] = bval;
}

/**********************************************************************
 *	Checks input values from parameters
 *********************************************************************/
void check_param(void)
{
   char tchar[20];

   /* check number of points, number of iterations */
   while ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS)) {
      printf("Enter number of points along vibrating string [%d-%d]: "
           ,MINPOINTS, MAXPOINTS);
      scanf("%s", tchar);
      tpoints = atoi(tchar);
      if ((tpoints < MINPOINTS) || (tpoints > MAXPOINTS))
         printf("Invalid. Please enter value between %d and %d\n", 
                 MINPOINTS, MAXPOINTS);
   }
   while ((nsteps < 1) || (nsteps > MAXSTEPS)) {
      printf("Enter number of time steps [1-%d]: ", MAXSTEPS);
      scanf("%s", tchar);
      nsteps = atoi(tchar);
      if ((nsteps < 1) || (nsteps > MAXSTEPS))
         printf("Invalid. Please enter value between 1 and %d\n", MAXSTEPS);
   }

   printf("Using points = %d, steps = %d\n", tpoints, nsteps);

}

/**********************************************************************
 *     Initialize points on line
 *********************************************************************/
void init_line(void)
{
   int j;
   float x, fac, k, tmp;

   /* Calculate initial values based on sine curve */
   fac = 2.0 * PI;
   k = 0.0; 
   tmp = tpoints - 1;
   for (j = 1; j <= tpoints; j++) {
      x = k/tmp;
      values[j] = sin (fac * x);
      k = k + 1.0;
   } 

   cudaMemcpy(g_value, values, size, cudaMemcpyHostToDevice);
   cudaMemcpy(gold_value, oldval, size, cudaMemcpyHostToDevice);
   cudaMemcpy(g_new_value, newval, size, cudaMemcpyHostToDevice);
   gpu_init_old_value<<<30,512>>>(gold_value, g_value, g_new_value, tpoints);
   printf("Updating all points for all time steps...\n");
   gpu_update_point<<<(tpoints/512 + 1),512>>>(gold_value, g_value, g_new_value, tpoints, nsteps);
   cudaMemcpy(values, g_value, size, cudaMemcpyDeviceToHost);
}

/**********************************************************************
 *      Calculate new values using wave equation
 *********************************************************************/
void do_math(int i)
{
   float dtime, c, dx, tau, sqtau;

   dtime = 0.3;
   c = 1.0;
   dx = 1.0;
   tau = (c * dtime / dx);
   sqtau = tau * tau;
   newval[i] = (2.0 * values[i]) - oldval[i] + (sqtau *  (-2.0)*values[i]);
}


/**********************************************************************
 *     Print final results
 *********************************************************************/
void printfinal()
{
   int i;

   for (i = 1; i <= tpoints; i++) {
      printf("%6.4f ", values[i]);
      if (i%10 == 0)
         printf("\n");
   }
}

/**********************************************************************
 *	Main program
 *********************************************************************/
int main(int argc, char *argv[])
{
	sscanf(argv[1],"%d",&tpoints);
	sscanf(argv[2],"%d",&nsteps);
	check_param();
	cudaMalloc((void**)&g_value, size);
    cudaMalloc((void**)&gold_value, size);
    cudaMalloc((void**)&g_new_value, size);
	printf("Initializing points on the line...\n");
	init_line();
	printf("Printing final results...\n");
	printfinal();
	printf("\nDone.\n\n");
	
	return 0;
}