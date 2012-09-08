#include <CL/opencl.h>
#include <stdio.h>
#include <stdlib.h>

const char *source = 
  "  __kernel void\
  vectorAdd(__global const float *a,\
	    __global const float *b,\
	    __global float *c)\
{\
  int i=get_global_id(0);\
  c[i]=a[i]+b[i];\
}";

void randomInit(float*a,int n)
{
  int i;
  for(i=0;i<n;i++)
    a[i]=drand48();
}

int main()
{
  cl_uint n;
  clGetPlatformIDs(0,NULL,&n);
  
  cl_platform_id p[n];
  clGetPlatformIDs(n,p,NULL);
  cl_uint i;
  for(i=0;i<n;i++){
    char buf[1024];
    clGetPlatformInfo(p[i],CL_PLATFORM_NAME,sizeof(buf),buf,NULL);
    printf("platform %d %s\n",i,buf);
  }
  
  cl_device_id d;
  clGetDeviceIDs(p[0],CL_DEVICE_TYPE_GPU,1,&d,NULL);
  
  cl_context ctx=clCreateContext(0,1,&d,NULL,NULL,NULL);
  cl_command_queue q=clCreateCommandQueue(ctx,d,0,NULL);
  cl_program prog=clCreateProgramWithSource(ctx,1,&source,0,0);
  clBuildProgram(prog,0,0,0,0,0);
  
  cl_kernel k=clCreateKernel(prog,"vectorAdd",0);
  
  enum {BLOCK_SIZE=98, BLOCKS=3, DIMS=BLOCK_SIZE*BLOCKS};
  
  float pa[DIMS], pb[DIMS], pc[DIMS];
  
  randomInit(pa,DIMS);
  randomInit(pb,DIMS);

  cl_mem 
    da=clCreateBuffer(ctx,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
		      DIMS*sizeof(cl_float),pa,0),
    db=clCreateBuffer(ctx,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
		      DIMS*sizeof(cl_float),pb,0),
    dc=clCreateBuffer(ctx,CL_MEM_READ_ONLY,
		      DIMS*sizeof(cl_float),0,0);

  clSetKernelArg(k,0,sizeof(cl_mem),&da);
  clSetKernelArg(k,1,sizeof(cl_mem),&db);
  clSetKernelArg(k,2,sizeof(cl_mem),&dc);

  {
    size_t d=DIMS;
    clEnqueueNDRangeKernel(q,k,1,0,&d,0,0,0,0);
  }
  clEnqueueReadBuffer(q,dc,CL_TRUE,0,
		      DIMS*sizeof(cl_float),pc,0,0,0);

  clReleaseKernel(k);
  clReleaseProgram(prog);
  clReleaseMemObject(da);
  clReleaseMemObject(db);
  clReleaseMemObject(dc);
  clReleaseCommandQueue(q);
  clReleaseContext(ctx);
  return 0;
}
