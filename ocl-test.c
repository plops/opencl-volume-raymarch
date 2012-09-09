// VolumetricMethodsInVisualEffects2010 p.59 raymarching

#include <CL/opencl.h>
#include <CL/cl_gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glfw.h>
#include <GL/glx.h>
#include <unistd.h>
#include <sys/time.h>

#define len(x) ((sizeof((x))/sizeof((x)[0])))

const char *source = 
  "\
const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE|			\
                          CLK_ADDRESS_CLAMP_TO_EDGE|			\
                          CLK_FILTER_LINEAR;				\
__kernel void red(int n, __write_only image2d_t rgba,                   \
                  __read_only image3d_t vol)				\
{									\
  int x=get_global_id(0),y=get_global_id(1);		\
  float val = 0.0;\
  int z=0;\
  for(z=0;z<128;z++){\
    float4 v=read_imagef(vol,sampler,(int4)(x,y,z,0));	\
    val += v.x;\
 }						\
write_imagef(rgba,(int2)(x,y),(float4)(val,val,val,1.0));					\
}";

/*
read_imageui(vol,sampler,coords);				\


float xx=x/1360.-.5, yy=y/1360.-.5,r=sqrt(xx*xx+yy*yy);		\
float xx2=xx-.05,yy2=yy+.1,r2=sqrt(xx2*xx2+yy2*yy2);			\
float v = sin(931*r*r-.035*n);					\
float w = sin(631*r2*r2-.035*n);					\
float z=w*v;								\
z = (z<0)?-1:1;							\
float4 col = (z<0)? (float4)(-z,.2*-z,0,1.0f): (float4)(0,.2*z,z,1.0f); \

*/

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

enum { W = 1360, H=730};

int main()
{

  if(!glfwInit())
    exit(EXIT_FAILURE);
  int width=W,height=H;
  
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
  glfwSwapInterval(0);
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
  
  enum {BLOCK_SIZE=W, BLOCKS=H, DIMS=BLOCK_SIZE*BLOCKS};



  cl_int err;

  // allocate 3D texture (contains volume data)
  
  enum{VW=128,VH=VW,VD=VW};

  unsigned int vtex;
  float *vtex_buf = malloc(VW*VH*VD*sizeof(float));
  { int i,j,k;
    for(k=0;k<VD;k++)
      for(j=0;j<VH;j++)
	for(i=0;i<VW;i++){
	  float 
	    x=(1.*i)/VW-.5,
	    y=(1.*j)/VH-.5,
	    z=(1.*k)/VD-.5,
	    r2=x*x+y*y+z*z;
	  vtex_buf[i+VW*(j+VH*k)]=(r2<.4*.4)?1.0:0.0;
	}
  }
  glGenTextures(1,&vtex);
  glBindTexture(GL_TEXTURE_3D,vtex);
  glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glEnable(GL_TEXTURE_3D);
  glTexImage3D(GL_TEXTURE_3D,0,GL_LUMINANCE,
	       VW,VH,VD,0,GL_LUMINANCE,GL_FLOAT,vtex_buf);

  cl_mem vimg=
    clCreateFromGLTexture3D(ctx      /* context */,
			    CL_MEM_READ_ONLY    /* flags */,
			    GL_TEXTURE_3D       /* target */,
			    0        /* miplevel */,
			    vtex       /* texture */,
			    &err        /* errcode_ret */);
  if(err!=CL_SUCCESS)
    printf("create-from-gl-tex %d\n",err);
  glDisable(GL_TEXTURE_3D);


  // allocate 2D texture (to display result)
  unsigned int tex;
  float *tex_buf = malloc(W*H*4*sizeof(float));
  { int i,j;
    for(i=0;i<W;i++)
      for(j=0;j<H;j++){
	tex_buf[(i+W*j)*4+0]=1.0;
	tex_buf[(i+W*j)*4+1]=0.0;
	tex_buf[(i+W*j)*4+2]=.5;
	tex_buf[(i+W*j)*4+0]=0.0;
      }
  }
  glGenTextures(1,&tex);
  glBindTexture(GL_TEXTURE_2D,tex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glEnable(GL_TEXTURE_2D);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,
	       W,H,0,GL_RGBA,GL_FLOAT,tex_buf);

  cl_mem img=
    clCreateFromGLTexture2D(ctx      /* context */,
			    CL_MEM_WRITE_ONLY    /* flags */,
			    GL_TEXTURE_2D       /* target */,
			    0        /* miplevel */,
			    tex       /* texture */,
			    &err        /* errcode_ret */);
  if(err!=CL_SUCCESS)
    printf("create-from-gl-tex %d\n",err);



  // prepare render commands for textured quad
  uint l=glGenLists(1);
  glNewList(l,GL_COMPILE);  
  glBegin(GL_TRIANGLE_FAN);
  { float a=-1.,b=1.;
    glTexCoord2f(0,0);    glVertex2f(a,a);
    glTexCoord2f(0,1);    glVertex2f(a,b);
    glTexCoord2f(1,1);    glVertex2f(b,b);
    glTexCoord2f(1,0);    glVertex2f(b,a);
  }
  glEnd();
  glEndList();


     

  int frame_count= 1;

  struct timeval tv;    
  suseconds_t old_usec=0;
  time_t old_sec=0;

  
  while(running){
    
    gettimeofday(&tv,0);
    int frames=100;
    if(0==(frame_count%frames)){
      float dt = (tv.tv_usec/1000.-old_usec/1000.)+(tv.tv_sec/1000.-old_sec/1000.);
      printf("frame-count=%d sec=%lu usec=%lu dt_ms=%3.3g, rate_hz=%8.9g\n",
	     frame_count,tv.tv_sec,tv.tv_usec,
	     dt,1000/dt);
    }
    { 
      //glFinish(); // ensure memory is up-to-date, glFlush might be faster
      glFlush();
      const cl_mem objs[]={img,vimg};
      err = clEnqueueAcquireGLObjects(q,len(objs),objs,0,0,0);
      if(err!=CL_SUCCESS)
	printf("acquire %d\n",err);

      
      
      { // call cl kernel here 
	
	frame_count++;
	if(CL_SUCCESS!=clSetKernelArg(k,0,sizeof(frame_count),&frame_count))
	  printf("error set kernel arg\n"); 
	if(CL_SUCCESS!=clSetKernelArg(k,1,sizeof(img),&img))
	  printf("error set kernel arg\n"); 
	if(CL_SUCCESS!=clSetKernelArg(k,2,sizeof(vimg),&vimg))
	  printf("error set kernel arg\n"); 
	const size_t d[]={VW,VH};
	clEnqueueNDRangeKernel(q,k,len(d),0,
			       d,0,0,0,0);
      }

      err = clEnqueueReleaseGLObjects(q,len(objs),objs,0,0,0);
      if(err!=CL_SUCCESS)
	printf("release %d\n",err);

      err = clFlush(q);
      
      
      if(err!=CL_SUCCESS)
	printf("flush %d\n",err);
      
    }
    glClear(GL_COLOR_BUFFER_BIT);
    glCallList(l);

    //usleep(1000000/60);

    glfwSwapBuffers();
    old_usec=tv.tv_usec;
    old_sec=tv.tv_sec;  

  }

  glDeleteTextures(1,&tex);
  free(tex_buf);

  glDeleteTextures(1,&vtex);
  free(vtex_buf);


  clReleaseKernel(k);
  clReleaseProgram(prog);
  clReleaseCommandQueue(q);
  clReleaseContext(ctx);

  glfwCloseWindow();
  glfwTerminate();
  
  return 0;
}
