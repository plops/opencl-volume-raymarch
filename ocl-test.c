#include <CL/opencl.h>
#include <stdio.h>

char *source = 
  "  __kernel void\
  vectorAdd(__global const float *a,\
	    __global const float *b,\
	    __global float *c)\
{\
  int i=get_global_id(0);\
  c[i]=a[i]+b[i];\
}";

int main()
{
  cl_uint n;
  clGetPlatformIDs(0,NULL,&n);
  
  cl_platform_id p[n];
  clGetPlatformIDs(n,p,NULL);
  int i;
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
  


  return 0;
}
