#include <CL/opencl.h>
#include <CL/cl_gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glfw.h>
#include <GL/glx.h>
#include <unistd.h>

const char *source = 
  "  __kernel void \ 
  red(int n, __write_only image2d_t rgba)				\
{									\
  int x=get_global_id(0),y=get_global_id(1);				\
  int2 coords = (int2)(x,y);						\
  float xx=x/1366.-.5, yy=y/1366.-.5,r=sqrt(xx*xx+yy*yy);		\
  float xx2=xx-.05,yy2=yy,r2=sqrt(xx2*xx2+yy2*yy2);			\
  float v = sin(1031*r-.35*n);						\
  float w = sin(1031*r2-.35*n);				\
  float z=v*w;						\
  float4 col = (z<0)? (float4)(-z,0,0,1.0f): (float4)(0,0,z,1.0f);	\
  write_imagef(rgba,coords,col);	\
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
  int width=1366,height=768;
  
  if(!glfwOpenWindow(width,height,8,8,8,
		     0,0,0,
		     //GLFW_FULLSCREEN
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
  {
    int err=clBuildProgram(prog,0,0,0,0,0);
    if(err!=CL_SUCCESS)
      printf("can't build kernel: %d\n",err);
  }
  cl_kernel k=clCreateKernel(prog,"red",0);
  
  enum {BLOCK_SIZE=1366, BLOCKS=768, DIMS=BLOCK_SIZE*BLOCKS};
  
  /* float pa[DIMS], pb[DIMS], pc[DIMS]; */
  
  /* randomInit(pa,DIMS); */
  /* randomInit(pb,DIMS); */

  /* cl_mem  */
  /*   da=clCreateBuffer(ctx,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, */
  /* 		      DIMS*sizeof(cl_float),pa,0), */
  /*   db=clCreateBuffer(ctx,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, */
  /* 		      DIMS*sizeof(cl_float),pb,0), */
  /*   dc=clCreateBuffer(ctx,CL_MEM_READ_ONLY, */
  /* 		      DIMS*sizeof(cl_float),0,0); */

  /* clSetKernelArg(k,0,sizeof(cl_mem),&da); */
  /* clSetKernelArg(k,1,sizeof(cl_mem),&db); */
  /* clSetKernelArg(k,2,sizeof(cl_mem),&dc); */

  unsigned int tex;
  float *tex_buf = malloc(1366*768*4*sizeof(float));
  { int i,j;
    for(i=0;i<1366;i++)
      for(j=0;j<768;j++){
	tex_buf[(i+1366*j)*4+0]=1.0;
	tex_buf[(i+1366*j)*4+1]=0.0;
	tex_buf[(i+1366*j)*4+2]=.5;
	tex_buf[(i+1366*j)*4+0]=0.0;
      }
  }
  glGenTextures(1,&tex);
  glBindTexture(GL_TEXTURE_2D,tex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glEnable(GL_TEXTURE_2D);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,
	       1366,768,0,GL_RGBA,GL_FLOAT,tex_buf);

  cl_int err;
  cl_mem img=
    clCreateFromGLTexture2D(ctx      /* context */,
			    CL_MEM_WRITE_ONLY    /* flags */,
			    GL_TEXTURE_2D       /* target */,
			    0        /* miplevel */,
			    tex       /* texture */,
			    &err        /* errcode_ret */);
  if(err!=CL_SUCCESS)
    printf("create-from-gl-tex %d\n",err);
  // -30 = invalid value
  // -34 invalid context
     

  int rot=1;
  while(running){
    


    { 
      glFinish(); // ensure memory is up-to-date, glFlush might be faster
      
      err = clEnqueueAcquireGLObjects(q,1,&img,0,0,0);
      if(err!=CL_SUCCESS)
	printf("acquire %d\n",err);

      
      
      { // call cl kernel here 
	
	rot++;
	if(CL_SUCCESS!=clSetKernelArg(k,0,sizeof(rot),&rot))
	  printf("error set kernel arg\n"); 
	if(CL_SUCCESS!=clSetKernelArg(k,1,sizeof(cl_mem),&img))
	  printf("error set kernel arg\n"); 
	const size_t d[]={1366,768};
	clEnqueueNDRangeKernel(q,k,sizeof(d)/sizeof(*d),0,
			       d,0,0,0,0);
      }

      err = clEnqueueReleaseGLObjects(q,1,&img,0,0,0);
      if(err!=CL_SUCCESS)
	printf("release %d\n",err);

      err = clFlush(q);
      
      
      if(err!=CL_SUCCESS)
	printf("flush %d\n",err);
      
    }
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBegin(GL_TRIANGLE_FAN);
    { float a=-1.,b=1.;
      glTexCoord2f(0,0);    glVertex2f(a,a);
      glTexCoord2f(0,1);    glVertex2f(a,b);
      glTexCoord2f(1,1);    glVertex2f(b,b);
      glTexCoord2f(1,0);    glVertex2f(b,a);
    }
    glEnd();

    //usleep(1000000/60);

    glfwSwapBuffers();
  }

  glDeleteTextures(1,&tex);


  /* clEnqueueReadBuffer(q,dc,CL_TRUE,0, */
  /* 		      DIMS*sizeof(cl_float),pc,0,0,0); */

  clReleaseKernel(k);
  clReleaseProgram(prog);
  /* clReleaseMemObject(da); */
  /* clReleaseMemObject(db); */
  /* clReleaseMemObject(dc); */
  clReleaseCommandQueue(q);
  clReleaseContext(ctx);

  glfwCloseWindow();
  glfwTerminate();
  
  return 0;
}
