
//
// GL.c
//
// <see corresponding .h file>
// 
//

#include "cel.h"
#include "runtime.h"
#include "svm32.h"
#include "array.h"


#include <GL/glut.h>

#ifdef WIN32
#else
#endif



//
//  GL - this is the wrapper for the OpenGL and Glut routines for Cel
// 

extern char *GLSL[];

void GLglutInitDisplay(void)
{
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); 
    VMReturn(Cpu, (unsigned int) nil, 3);
}


void GLglutSetWindowWidthHeight(void)
{
    int w,h;
    Proto tmp;

    tmp = (Proto) stackAt(Cpu, 4);
    w = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 5);
    h = objectIntegerValue(tmp);

    glutInitWindowSize(w, h);
    VMReturn(Cpu, (unsigned int) nil, 5);
}

void GLglutSetWindowXY(void)
{
    int x,y;
    Proto tmp;

    tmp = (Proto) stackAt(Cpu, 4);
    x = objectIntegerValue(tmp);

    tmp = (Proto) stackAt(Cpu, 5);
    y = objectIntegerValue(tmp);

    glutInitWindowPosition(x, y);
    VMReturn(Cpu, (unsigned int) nil, 5);
}


void GLglutCreateWindowNamed(void)
{
    Proto tmp;
    char * p;

    tmp = (Proto) stackAt(Cpu, 4);
    p = string2CString(tmp);

    glutCreateWindow(p);
    VMReturn(Cpu, (unsigned int) nil, 4);
}


static Proto GLdispObj;
static Proto GLdispMethod;
void GLdisplay(void)
{
   VMSetupCall(Cpu, (uint32) GLdispObj, (uint32) GLdispMethod);
   VMRun(Cpu, 1);
   // Remove the returned result
   pull(Cpu);
}


void GLglutDisplayDelegatewithMethod(void)
{
    GLdispObj    = (Proto) stackAt(Cpu, 4);
    GLdispMethod = (Proto) stackAt(Cpu, 5);

    glutDisplayFunc(GLdisplay);
    VMReturn(Cpu, (unsigned int) nil, 5);
}

static Proto GLreshapeObj;
static Proto GLreshapeMethod;
void GLreshape(int width, int height)
{
   Proto tmp;
   Proto pArray[2];

   tmp = objectNewInteger(width);
   pArray[0] = tmp;
   tmp = objectNewInteger(height);
   pArray[1] = tmp;

   VMSetupCallWithArgs(Cpu, (uint32) GLreshapeObj, (uint32) GLreshapeMethod, 2, (unsigned int *) pArray);
   VMRun(Cpu, 1);
   // Remove the returned result
   pull(Cpu);
}


void GLglutReshapeDelegatewithMethod(void)
{
    GLreshapeObj    = (Proto) stackAt(Cpu, 4);
    GLreshapeMethod = (Proto) stackAt(Cpu, 5);

    glutReshapeFunc(GLreshape);
    VMReturn(Cpu, (unsigned int) nil, 5);
}

static Proto GLkeybObj;
static Proto GLkeybMethod;
void GLkeyboard(unsigned char key, int x, int y)
{
   Proto tmp;
   Proto pArray[3];
   char  tmpStr[2];

   GCOFF();

   tmpStr[0] = key;
   tmpStr[1] = 0;

   tmp = (Proto) stringToAtom(tmpStr);
   pArray[0] = tmp;
   tmp = objectNewInteger(x);
   pArray[1] = tmp;
   tmp = objectNewInteger(y);
   pArray[2] = tmp;

   VMSetupCallWithArgs(Cpu, (uint32) GLkeybObj, (uint32) GLkeybMethod, 3, (unsigned int *) pArray);
   VMRun(Cpu, 1);
   // Remove the returned result
   pull(Cpu);

   GCON();
}


void GLglutKeyboardDelegatewithMethod(void)
{
    GLkeybObj    = (Proto) stackAt(Cpu, 4);
    GLkeybMethod = (Proto) stackAt(Cpu, 5);

    glutKeyboardFunc(GLkeyboard);
    VMReturn(Cpu, (unsigned int) nil, 5);
}

static Proto GLmouseObj;
static Proto GLmouseMethod;
void GLmouse(int button, int state, int x, int y)
{
   Proto tmp;
   Proto pArray[4];

   GCOFF();

   tmp = objectNewInteger(button);
   pArray[0] = tmp;
   tmp = objectNewInteger(state);
   pArray[1] = tmp;
   tmp = objectNewInteger(x);
   pArray[2] = tmp;
   tmp = objectNewInteger(y);
   pArray[3] = tmp;

   VMSetupCallWithArgs(Cpu, (uint32) GLmouseObj, (uint32) GLmouseMethod, 4, (unsigned int *) pArray);
   VMRun(Cpu, 1);
   // Remove the returned result
   pull(Cpu);

   GCON();
}


void GLglutMouseDelegatewithMethod(void)
{
    GLmouseObj    = (Proto) stackAt(Cpu, 4);
    GLmouseMethod = (Proto) stackAt(Cpu, 5);

    glutMouseFunc(GLmouse);
    VMReturn(Cpu, (unsigned int) nil, 5);
}


static Proto GLmotionObj;
static Proto GLmotionMethod;
void GLmotion(int x, int y)
{
   Proto tmp;
   Proto pArray[2];

   GCOFF();

   tmp = objectNewInteger(x);
   pArray[0] = tmp;
   tmp = objectNewInteger(y);
   pArray[1] = tmp;

   VMSetupCallWithArgs(Cpu, (uint32) GLmotionObj, (uint32) GLmotionMethod, 2, (unsigned int *) pArray);
   VMRun(Cpu, 1);
   // Remove the returned result
   pull(Cpu);

   GCON();
}


void GLglutMotionDelegatewithMethod(void)
{
    GLmotionObj    = (Proto) stackAt(Cpu, 4);
    GLmotionMethod = (Proto) stackAt(Cpu, 5);

    glutMotionFunc(GLmotion);
    VMReturn(Cpu, (unsigned int) nil, 5);
}



static Proto GLidleObj;
static Proto GLidleMethod;
void GLidle(void)
{
   VMSetupCall(Cpu, (uint32) GLidleObj, (uint32) GLidleMethod);
   VMRun(Cpu, 1);
   // Remove the returned result
   pull(Cpu);
}


void GLglutIdleDelegatewithMethod(void)
{
    GLidleObj    = (Proto) stackAt(Cpu, 4);
    GLidleMethod = (Proto) stackAt(Cpu, 5);

    if (GLidleObj == FalseObj) {
       glutIdleFunc(NULL);
    } else {
       // We do this to resolve the 'self', otherwise we get
       // the block context
       GLidleObj = locateObjectWithSlotVia(GLidleMethod, GLidleObj);
       glutIdleFunc(GLidle);
    }
    VMReturn(Cpu, (unsigned int) nil, 5);
}


void GLglutMainLoop(void)
{
    glutMainLoop();
    VMReturn(Cpu, (unsigned int) nil, 3);
}


void GLglutPostRedisplay(void)
{
    glutPostRedisplay();
    VMReturn(Cpu, (unsigned int) nil, 3);
}


void GLglutSwapBuffers(void)
{
    glutSwapBuffers();
    VMReturn(Cpu, (unsigned int) nil, 3);
}


void GLglutWireCube(void)
{
    Proto tmp;
    double f;

    tmp    = (Proto) stackAt(Cpu,4);

    f = objectFloatValue(tmp);

    glutWireCube(f);
    VMReturn(Cpu, (unsigned int) nil, 4);
}


void GLgluPerspectiveAngleAspectzNearzFar(void)
{
    Proto tmp;
    double yangle, aspect, znear, zfar;

    tmp    = (Proto) stackAt(Cpu,4);
    yangle = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,5);
    aspect = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,6);
    znear  = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,7);
    zfar   = objectFloatValue(tmp);


    gluPerspective(yangle, aspect, znear, zfar);

    VMReturn(Cpu, (unsigned int) nil, 7);
}

void GLgluOrtho2DLeftRightBottomTop(void)
{
    Proto tmp;
    double left, right, bottom, top;

    tmp    = (Proto) stackAt(Cpu,4);
    left   = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,5);
    right  = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,6);
    bottom = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,7);
    top    = objectFloatValue(tmp);


    gluOrtho2D(left, right, bottom, top);

    VMReturn(Cpu, (unsigned int) nil, 7);
}



void GLglProjectionMatrix(void)
{
    glMatrixMode(GL_PROJECTION);
    VMReturn(Cpu, (unsigned int) nil, 3);
}

void GLglModelViewMatrix(void)
{
    glMatrixMode(GL_MODELVIEW);
    VMReturn(Cpu, (unsigned int) nil, 3);
}

void GLglLoadIdentity(void)
{
    glLoadIdentity();
    VMReturn(Cpu, (unsigned int) nil, 3);
}

void GLglBegin(void)
{
    Proto tmp;
    uint32 i;

    tmp = (Proto) stackAt(Cpu, 4);
    i = objectIntegerValue(tmp);

    glBegin(i);
    VMReturn(Cpu, (unsigned int) nil, 4);
}

void GLglEnd(void)
{
    glEnd();
    VMReturn(Cpu, (unsigned int) nil, 3);
}


void GLglClear(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    VMReturn(Cpu, (unsigned int) nil, 3);
}

void GLglFlush(void)
{
    glFlush();
    VMReturn(Cpu, (unsigned int) nil, 3);
}

void GLglViewportXYWidthHeight(void)
{
    Proto tmp;
    int x, y, w, h; 

    tmp    = (Proto) stackAt(Cpu,4);
    x = objectIntegerValue(tmp);

    tmp    = (Proto) stackAt(Cpu,5);
    y = objectIntegerValue(tmp);

    tmp    = (Proto) stackAt(Cpu,6);
    w  = objectIntegerValue(tmp);

    tmp    = (Proto) stackAt(Cpu,7);
    h  = objectIntegerValue(tmp);

    glViewport(x,y,w,h);

    VMReturn(Cpu, (unsigned int) nil, 7);
}

void GLglTranslateXYZ(void)
{
    Proto tmp;
    double x, y, z;

    tmp    = (Proto) stackAt(Cpu,4);
    x = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,5);
    y = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,6);
    z  = objectFloatValue(tmp);


    glTranslatef(x,y,z);

    VMReturn(Cpu, (unsigned int) nil, 6);
}

void GLglRotateAngleXYZ(void)
{
    Proto tmp;
    double angle, x, y, z;

    tmp    = (Proto) stackAt(Cpu,4);
    angle = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,5);
    x = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,6);
    y = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,7);
    z  = objectFloatValue(tmp);


    glRotatef(angle,x,y,z);

    VMReturn(Cpu, (unsigned int) nil, 7);
}

void GLglVertexXYZ(void)
{
    Proto tmp;
    double x, y, z;

    tmp    = (Proto) stackAt(Cpu,4);
    x = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,5);
    y = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,6);
    z  = objectFloatValue(tmp);


    glVertex3f(x,y,z);

    VMReturn(Cpu, (unsigned int) nil, 6);
}

void GLglColorRGB(void)
{
    Proto tmp;
    double r, g, b;

    tmp    = (Proto) stackAt(Cpu,4);
    r      = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,5);
    g      = objectFloatValue(tmp);

    tmp    = (Proto) stackAt(Cpu,6);
    b      = objectFloatValue(tmp);


    glColor3f(r,g,b);

    VMReturn(Cpu, (unsigned int) nil, 6);
}



char * GLSL[] = {
	"glutInitDisplay", "GLglutInitDisplay", (char *) GLglutInitDisplay,
	"glutSetWindowWidth:Height:", "GLglutSetWindowWidthHeight", (char *) GLglutSetWindowWidthHeight,
	"glutSetWindowX:Y:", "GLglutSetWindowXY", (char *) GLglutSetWindowXY,
	"glutCreateWindowNamed:", "GLglutCreateWindowNamed", (char *) GLglutCreateWindowNamed,
	"glutDisplayDelegate:withMethod:", "GLglutDisplayDelegatewithMethod", (char *) GLglutDisplayDelegatewithMethod,
	"glutReshapeDelegate:withMethod:", "GLglutReshapeDelegatewithMethod", (char *) GLglutReshapeDelegatewithMethod,
	"glutKeyboardDelegate:withMethod:", "GLglutKeyboardDelegatewithMethod", (char *) GLglutKeyboardDelegatewithMethod,
	"glutMouseDelegate:withMethod:", "GLglutMouseDelegatewithMethod", (char *) GLglutMouseDelegatewithMethod,
	"glutMotionDelegate:withMethod:", "GLglutMotionDelegatewithMethod", (char *) GLglutMotionDelegatewithMethod,
	"glutIdleDelegate:withMethod:", "GLglutIdleDelegatewithMethod", (char *) GLglutIdleDelegatewithMethod,
	"glutMainLoop", "GLglutMainLoop", (char *) GLglutMainLoop,
	"glutPostRedisplay", "GLglutPostRedisplay", (char *) GLglutPostRedisplay,
	"glutSwapBuffers", "GLglutSwapBuffers", (char *) GLglutSwapBuffers,
	"glutWireCube:", "GLglutWireCube", (char *) GLglutWireCube,
//
	"gluPerspectiveAngle:Aspect:zNear:zFar:", "GLgluPerspectiveAngleAspectzNearzFar", (char *) GLgluPerspectiveAngleAspectzNearzFar,
	"gluOrtho2DLeft:Right:Bottom:Top:", "GLgluOrtho2DLeftRightBottomTop", (char *) GLgluOrtho2DLeftRightBottomTop,
//
	"glProjectionMatrix", "GLglProjectionMatrix", (char *) GLglProjectionMatrix,
	"glModelViewMatrix", "GLglModelViewMatrix", (char *) GLglModelViewMatrix,
	"glLoadIdentity", "GLglLoadIdentity", (char *) GLglLoadIdentity,
	"glBegin:", "GLglBegin", (char *) GLglBegin,
	"glEnd", "GLglEnd", (char *) GLglEnd,
	"glClear", "GLglClear", (char *) GLglClear,
	"glFlush", "GLglFlush", (char *) GLglFlush,
	"glViewportX:Y:Width:Height:", "GLglViewportXYWidthHeight", (char *) GLglViewportXYWidthHeight,
	"glTranslateX:Y:Z:", "GLglTranslateXYZ", (char *) GLglTranslateXYZ,
	"glRotateAngle:X:Y:Z:", "GLglRotateAngleXYZ", (char *) GLglRotateAngleXYZ,
	"glVertexX:Y:Z:", "GLglVertexXYZ", (char *) GLglVertexXYZ,
	"glColorR:G:B:", "GLglColorRGB", (char *) GLglColorRGB,
	""
};

