#include <CL/opencl.h>
#include <CL/cl_gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glfw.h>
#include <GL/glx.h>

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


int running = GL_TRUE;

void GLFWCALL
keyhandler(int key,int action)
{
  if(action!=GLFW_PRESS)
    return;
  if(key==GLFW_KEY_ESC)
    running=GL_FALSE;
  return;
}

int main()
{

  if(!glfwInit())
    exit(EXIT_FAILURE);
  int width=512,height=512;
  
  if(!glfwOpenWindow(width,height,8,8,8,
		     0,0,0,
		     GLFW_WINDOW
		     )){
    glfwTerminate();
    return -1;
  }
  
  glfwSetWindowTitle("bla");
  //glfwSetWindowPos(-8,-31);
  glfwSwapInterval(1);
  glfwSetKeyCallback(keyhandler);

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
 
  { // print extension string
    char buf[1024];
    size_t n=sizeof(buf);
    clGetDeviceInfo(d,CL_DEVICE_EXTENSIONS,n,buf,&n);
    printf("%s\n",buf);
  }
 
  cl_context ctx; //=clCreateContext(0,1,&d,NULL,NULL,NULL);

  { // create shared GLX CL context
    cl_context_properties properties[]={
      CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
      CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
      CL_CONTEXT_PLATFORM, (cl_context_properties)p[0],0
    };
    cl_device_id devices[32];
    size_t size;
    clGetGLContextInfoKHR_fn clGetGLContextInfoKHR=
      clGetExtensionFunctionAddress("clGetGLContextInfoKHR");
    clGetGLContextInfoKHR(properties,CL_DEVICES_FOR_GL_CONTEXT_KHR,
			  sizeof(devices),devices,&size);
    int count = size / sizeof(cl_device_id);
    ctx = clCreateContext(properties,count,devices,NULL,NULL,NULL);
  }
  
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

  while(running){
    glClear(GL_COLOR_BUFFER_BIT);

    unsigned int tex;
    unsigned char tex_buf[512*512];
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glEnable(GL_TEXTURE_2D);


    glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,
		 512,512,0,
		 GL_LUMINANCE,
		 GL_UNSIGNED_BYTE,
		 tex_buf);
    glBindTexture(GL_TEXTURE_2D,tex);
    { cl_int err;
      cl_mem img=
	clCreateFromGLTexture2D(ctx      /* context */,
				CL_MEM_READ_WRITE    /* flags */,
				GL_TEXTURE_2D       /* target */,
				0        /* miplevel */,
				tex       /* texture */,
				&err        /* errcode_ret */);
      printf("create-from-gl-tex %d\n",err);
      // -30 = invalid value
      // -34 invalid context
    }

    
    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0,0);    glVertex2f(-1,-1);
    glTexCoord2f(0,1);    glVertex2f(-1,1);
    glTexCoord2f(1,1);    glVertex2f(1,1);
    glTexCoord2f(1,0);    glVertex2f(1,-1);
    glEnd();

    glDeleteTextures(1,&tex);

    glfwSwapBuffers();
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

  glfwCloseWindow();
  glfwTerminate();
  
  return 0;
}
