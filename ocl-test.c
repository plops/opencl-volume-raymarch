#include <CL/opencl.h>
#include <stdio.h>
int
main()
{
  cl_uint NumPlatforms;
  clGetPlatformIDs(0,NULL,&NumPlatforms);
  printf("%d\n",NumPlatforms);
  return 0;
}
