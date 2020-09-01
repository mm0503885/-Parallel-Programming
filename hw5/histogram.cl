typedef struct
{
    uchar R;
    uchar G;
    uchar B;
    uchar align;
} RGB;



__kernel void cl_histogram(__global const RGB* cl_data, __global uint* cl_R, __global uint* cl_G, __global uint* cl_B)
{
	int idx = get_global_id(0);
	atomic_add(cl_R+cl_data[idx].R,1);
	atomic_add(cl_G+cl_data[idx].G,1);
	atomic_add(cl_B+cl_data[idx].B,1);
	barrier(CLK_GLOBAL_MEM_FENCE);
}