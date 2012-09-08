#include <CL/opencl.h>
#include <stdio.h>
int
main()
{
  cl_uint n;
  clGetPlatformIDs(0,NULL,&n);
  
  {
    cl_platform_id p[n];
    clGetPlatformIDs(n,p,NULL);
    int i;
    for(i=0;i<n;i++){
      char buf[1024];
      clGetPlatformInfo(p[i],CL_PLATFORM_NAME,sizeof(buf),buf,NULL);
      printf("platform %d %s\n",i,buf);
    }
  }
  



  return 0;
}
