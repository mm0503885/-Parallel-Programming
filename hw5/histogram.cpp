#include <fstream>
#include <iostream>
#include <string>
#include <ios>
#include <vector>
#include <stdio.h>
#include <CL/opencl.h>



typedef struct
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t align;
} RGB;

typedef struct
{
    bool type;
    uint32_t size;
    uint32_t height;
    uint32_t weight;
    RGB *data;
} Image;

Image *readbmp(const char *filename)
{
    std::ifstream bmp(filename, std::ios::binary);
    char header[54];
    bmp.read(header, 54);
    uint32_t size = *(int *)&header[2];
    uint32_t offset = *(int *)&header[10];
    uint32_t w = *(int *)&header[18];
    uint32_t h = *(int *)&header[22];
    uint16_t depth = *(uint16_t *)&header[28];
    if (depth != 24 && depth != 32)
    {
        printf("we don't suppot depth with %d\n", depth);
        exit(0);
    }
    bmp.seekg(offset, bmp.beg);

    Image *ret = new Image();
    ret->type = 1;
    ret->height = h;
    ret->weight = w;
    ret->size = w * h;
    ret->data = new RGB[w * h]{};
    for (int i = 0; i < ret->size; i++)
    {
        bmp.read((char *)&ret->data[i], depth / 8);
    }
    return ret;
}

int writebmp(const char *filename, Image *img)
{

    uint8_t header[54] = {
        0x42,        // identity : B
        0x4d,        // identity : M
        0, 0, 0, 0,  // file size
        0, 0,        // reserved1
        0, 0,        // reserved2
        54, 0, 0, 0, // RGB data offset
        40, 0, 0, 0, // struct BITMAPINFOHEADER size
        0, 0, 0, 0,  // bmp width
        0, 0, 0, 0,  // bmp height
        1, 0,        // planes
        32, 0,       // bit per pixel
        0, 0, 0, 0,  // compression
        0, 0, 0, 0,  // data size
        0, 0, 0, 0,  // h resolution
        0, 0, 0, 0,  // v resolution
        0, 0, 0, 0,  // used colors
        0, 0, 0, 0   // important colors
    };

    // file size
    uint32_t file_size = img->size * 4 + 54;
    header[2] = (unsigned char)(file_size & 0x000000ff);
    header[3] = (file_size >> 8) & 0x000000ff;
    header[4] = (file_size >> 16) & 0x000000ff;
    header[5] = (file_size >> 24) & 0x000000ff;

    // width
    uint32_t width = img->weight;
    header[18] = width & 0x000000ff;
    header[19] = (width >> 8) & 0x000000ff;
    header[20] = (width >> 16) & 0x000000ff;
    header[21] = (width >> 24) & 0x000000ff;

    // height
    uint32_t height = img->height;
    header[22] = height & 0x000000ff;
    header[23] = (height >> 8) & 0x000000ff;
    header[24] = (height >> 16) & 0x000000ff;
    header[25] = (height >> 24) & 0x000000ff;

    std::ofstream fout;
    fout.open(filename, std::ios::binary);
    fout.write((char *)header, 54);
    fout.write((char *)img->data, img->size * 4);
    fout.close();
}

cl_program load_program(cl_context context,cl_device_id device, const char* filename)
{
	std::ifstream in(filename, std::ios_base::binary);
	if(!in.good()) {
		return 0;
	}

	// get file length
	in.seekg(0, std::ios_base::end);
	size_t length = in.tellg();
	in.seekg(0, std::ios_base::beg);

	// read program source
	std::vector<char> data_(length + 1);
	in.read(&data_[0], length);
	data_[length] = 0;

	// create and build program 
	const char* source = &data_[0];
	cl_program program = clCreateProgramWithSource(context, 1, &source, 0, 0);
	if(program == 0) {
		return 0;
	}
	cl_int i = clBuildProgram(program, 0, 0, 0, 0, 0);
    if (i != CL_SUCCESS) {
	char *buff_erro;
	cl_int errcode;
	size_t build_log_len;
	errcode = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &build_log_len);
	if (errcode) {
            printf("clGetProgramBuildInfo failed at line %d\n", __LINE__);
            exit(-1);
        }

    buff_erro = new char(build_log_len);
    if (!buff_erro) {
        printf("malloc failed at line %d\n", __LINE__);
        exit(-2);
    }

    errcode = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, build_log_len, buff_erro, NULL);
    if (errcode) {
        printf("clGetProgramBuildInfo failed at line %d\n", __LINE__);
        exit(-3);
    }

    fprintf(stderr,"Build log: \n%s\n", buff_erro); //Be careful with  the fprint
    free(buff_erro);
    fprintf(stderr,"clBuildProgram failed\n");
    exit(EXIT_FAILURE);
	}
	return program;
}

void histogram(Image *img,uint32_t R[256],uint32_t G[256],uint32_t B[256],cl_context context,cl_command_queue queue,cl_device_id device,
				cl_mem cl_data,cl_mem cl_R,cl_mem cl_G,cl_mem cl_B){

	cl_int err;

    cl_program program = load_program(context,device, "histogram.cl");
	
	cl_kernel cl_histogram = clCreateKernel(program, "cl_histogram", &err);
	
	
	clSetKernelArg(cl_histogram, 0, sizeof(cl_mem), &cl_data);
	clSetKernelArg(cl_histogram, 1, sizeof(cl_mem), &cl_R);
	clSetKernelArg(cl_histogram, 2, sizeof(cl_mem), &cl_G);
	clSetKernelArg(cl_histogram, 3, sizeof(cl_mem), &cl_B);
	
	size_t work_size = img->size;
	err = clEnqueueNDRangeKernel(queue, cl_histogram, 1, 0, &work_size, 0, 0, 0, 0);
	
	clFinish(queue);

	if(err == CL_SUCCESS) {
		err = clEnqueueReadBuffer(queue, cl_R, CL_TRUE, 0, sizeof(uint32_t) * 256, R, 0, 0, 0);
		err = clEnqueueReadBuffer(queue, cl_G, CL_TRUE, 0, sizeof(uint32_t) * 256, G, 0, 0, 0);
		err = clEnqueueReadBuffer(queue, cl_B, CL_TRUE, 0, sizeof(uint32_t) * 256, B, 0, 0, 0);
		
	}
	
	clReleaseKernel(cl_histogram);
	clReleaseProgram(program);

}

int main(int argc, char *argv[])
{

    char *filename;
    if (argc >= 2)
    {
		cl_int err;
		cl_uint num_device, num;
		cl_device_id device;
		cl_platform_id platform_id;
		err = clGetPlatformIDs(1, &platform_id, &num);
		err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device,  &num_device);
		cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
		cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, 0, &err);
        int many_img = argc - 1;
        for (int i = 0; i < many_img; i++)
        {
            filename = argv[i + 1];
            Image *img = readbmp(filename);
			
			 std::cout << img->weight << ":" << img->height << "\n";


            uint32_t R[256];
            uint32_t G[256];
            uint32_t B[256];
			
			std::fill(R, R+256, 0);
			std::fill(G, G+256, 0);
			std::fill(B, B+256, 0);
		
			cl_mem cl_data = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(RGB) * (img->size), img->data, NULL);
			cl_mem cl_R = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(uint32_t) * 256, R, NULL);
			cl_mem cl_G = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(uint32_t) * 256, G, NULL);
			cl_mem cl_B = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(uint32_t) * 256, B, NULL);
		

            histogram(img,R,G,B,context,queue,device,cl_data,cl_R,cl_G,cl_B);
			
			clReleaseMemObject(cl_R);
			clReleaseMemObject(cl_G);
			clReleaseMemObject(cl_B);
			clReleaseMemObject(cl_data);
			
            int max = 0;
            for(int i=0;i<256;i++){
                max = R[i] > max ? R[i] : max;
                max = G[i] > max ? G[i] : max;
                max = B[i] > max ? B[i] : max;
            }
            Image *ret = new Image();
            ret->type = 1;
            ret->height = 256;
            ret->weight = 256;
            ret->size = 256 * 256;
            ret->data = new RGB[256 * 256];

            for(int i=0;i<ret->height;i++){
                for(int j=0;j<256;j++){
                    if(R[j]*256/max > i)
                        ret->data[256*i+j].R = 255;
					else
						ret->data[256*i+j].R = 0;
                    if(G[j]*256/max > i)
                        ret->data[256*i+j].G = 255;
					else
						ret->data[256*i+j].G = 0;
                    if(B[j]*256/max > i)
                        ret->data[256*i+j].B = 255;
					else
						ret->data[256*i+j].B = 0;
                }
            }
			

            std::string newfile = "hist_" + std::string(filename); 
            writebmp(newfile.c_str(), ret);
        }
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
    }else{
        printf("Usage: ./hist <img.bmp> [img2.bmp ...]\n");
    }
    return 0;
}