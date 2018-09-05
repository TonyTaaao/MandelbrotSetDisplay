// Calculate and display the Mandelbrot set
// ECE4893/8893 final project, Fall 2011

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <GL/glut.h>
#include <GL/glext.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "complex.h"

using namespace std;
//char fileName[] = "testIter.txt";

// Min and max complex plane values
Complex  minC(-2.0, -1.2);
Complex  maxC( 1.0, 1.8);
int      maxIt = 2000;     // Max iterations for the set computations
Complex arr[512][512];
int colorArr[512][512];

int winW = 512;
int winH = 512;

// Each thread needs to know how many threads there are.
// This is similar to MPI_Comm_size
int nThreads = 16;
int activeThreads = 0;

// The mutex and condition variables allow the main thread to
// know when all helper threads are completed.
pthread_mutex_t assignMutex;
pthread_mutex_t activeMutex;
pthread_mutex_t exitMutex;
pthread_cond_t  allDoneCondition;

//Initialize the array
void initArr() {
  arr[0][0] = minC;
  arr[511][511] = maxC;
  for (int i = 0; i < 512; i++) {
    for (int j = 0; j < 512; j++) {
      arr[i][j] = Complex(-2.0 + 3.0/511*i, -1.2 + 3.0/511*j);
    }
  }
}

//find number of iterations for c to reach magnitude > 2.0
int NumOfIter(Complex c) {
  int itNum = 0;
  Complex Z = c;
  while (itNum <= 2000 && Z.Mag().real <= 2.0) {
    Z = Z*Z + c;
    itNum++;
  }
  return itNum;
}

//paint that pixel
void draw() {
  GLfloat red, green, blue;
  int itNum;

  for (int i = 0; i < 512; i++) {
    for (int j = 0; j < 512; j++) {
      itNum = colorArr[i][j];
      if (itNum > 2000) {
        red = 0.0;
        green = 0.0;
        blue = 0.0;
      } else {
        red = itNum/3.14159 - (int)(itNum/3.14159);
        green = itNum/6.6666 - (int)(itNum/6.6666);
        blue = itNum/2.7183 - (int)(itNum/2.7183);
      }
      glBegin(GL_POINTS);
      glColor3f(red, green, blue);
      glVertex2i((GLint)i, (GLint)j);
      glEnd();      
    }
  }
}

void* Thread(void* v)
{
  unsigned long myId = (unsigned long)v;

  int tempColor[512][32];
  //divide columns
  for (int cols = myId*32; cols < (myId + 1)*32; cols++) {
    for (int rows = 0; rows < 512; rows++) {
      int itNum = NumOfIter(arr[rows][cols]);
      tempColor[rows][cols-myId*32] = itNum;
    }
  }
  pthread_mutex_lock(&assignMutex);
  for (int col = 0; col < 32; col++) {
    for (int row = 0; row < 512; row++) {
      colorArr[row][myId*32 + col] = tempColor[row][col];
    }
  }
  pthread_mutex_unlock(&assignMutex);
  pthread_mutex_lock(&activeMutex);
  activeThreads--;
  if (activeThreads == 0)
   {
     pthread_mutex_lock(&exitMutex);
     pthread_cond_signal(&allDoneCondition);
     pthread_mutex_unlock(&exitMutex);
   }
  pthread_mutex_unlock(&activeMutex);
}

void MultiThreads()
{
  pthread_mutex_init(&assignMutex, 0);
  pthread_mutex_init(&activeMutex, 0);
  pthread_mutex_init(&exitMutex, 0);
  pthread_cond_init(&allDoneCondition, 0);
  
  pthread_mutex_lock(&exitMutex);

  activeThreads = nThreads;
  for (int i = 0; i < nThreads; ++i)
    {
      pthread_t t;
      pthread_create(&t, 0, Thread,  (void*)i);
    }
  pthread_cond_wait(&allDoneCondition, &exitMutex);
  //pthread_mutex_unlock(&exitMutex);
}


void display(void)
{ // Your OpenGL display code here
  glEnable(GL_LINE_SMOOTH); //enable anti-aliasing
  //clear all
  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);
  // Enable depth buffer
  glEnable(GL_DEPTH_TEST);
  //Clear the matrix
  glLoadIdentity();
  //Set the viewing transformation
  //gluLookAt(0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
  gluLookAt(0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0); //this works ...
  glPushMatrix();
  glScalef(1.0, 1.0, 1.0); 
  draw();

  glPopMatrix();
  glutSwapBuffers(); //If double buffering
}

void init()
{ // Your OpenGL initialization code here
  //select clearing (background) color
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_FLAT);
}

void reshape(int w, int h)
{ // Your OpenGL window reshape code here
  winW = w;
  winH = h;
  glViewport(0, 0, (GLsizei)w, (GLsizei)h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, w, 0, h, -w, w);
  glMatrixMode(GL_MODELVIEW);
}

void mouse(int button, int state, int x, int y)
{ // Your mouse click processing here
  // state == 0 means pressed, state != 0 means released
  // Note that the x and y coordinates passed in are in
  // PIXELS, with y = 0 at the top.
}

void motion(int x, int y)
{ // Your mouse motion here, x and y coordinates are as above
}

void keyboard(unsigned char c, int x, int y)
{ // Your keyboard processing here
}

int main(int argc, char** argv)
{
  // Initialize OpenGL, but only on the "master" thread or process.
  // See the assignment writeup to determine which is "master" 
  // and which is slave.
  initArr();
  MultiThreads();

  // Initialize glut  and create your window here
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(winW, winH);
  glutInitWindowPosition(100, 100);
  glutCreateWindow("Mandelbrot Set");  
  init();

  draw();

  // Set your glut callbacks here
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  // Enter the glut main loop here
  glutMainLoop();
  return 0;
}

