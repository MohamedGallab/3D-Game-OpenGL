#include <iostream>
#include <glut.h>
#include <math.h>
using namespace std;

#define FPS 60
#define TO_RADIANS 3.14/180.0

//width and height of the window ( Aspect ratio 16:9 )
const int width = 16 * 80;
const int height = 9 * 80;

float pitch = 0.0, yaw = 180;
float camX = 0.0, camZ = 0.0, camY = 0.0;
float camSensitivity = 50;

bool isAirborne = false;
bool isFalling = false;
bool isFirstPerson = false;
bool isShooting = false;

double bullet_x = 0;
double bullet_z = 0;
double bullet_angle = 0;

void display();
void reshape(int w, int h);
void gameLoop(int);
void passiveMouseCB(int, int);
void camera();
void keyboard(unsigned char key, int x, int y);
void keyboard_up(unsigned char key, int x, int y);

struct Motion
{
	bool Forward, Backward, Left, Right;
};

Motion motion = { false,false,false,false };

// draw a z-axis, with cone at end
void axis(double length)
{
	glPushMatrix();

	glBegin(GL_LINES);
	glVertex3d(0, 0, 0);
	glVertex3d(0, 0, length); // along the z-axis
	glEnd();

	glTranslated(0, 0, length - 0.2);
	glutWireCone(0.04, 0.2, 12, 9);

	glPopMatrix();
}

void drawAxes(double length)
{
	glPushMatrix();
	glColor3d(0, 0, 1); // draw black lines
	axis(length);			// z-axis
	glColor3d(0, 1, 0); // draw black lines
	glRotated(90, 0, 1.0, 0);
	axis(length); // y-axis
	glRotated(-90.0, 1, 0, 0);
	glColor3d(1, 0, 0); // draw black lines
	axis(length); // x-axis
	glPopMatrix();
}

/* This function just draws the scene. I used Texture mapping to draw
   a chessboard like surface. If this is too complicated for you ,
   you can just use a simple quadrilateral */

void drawFloor()
{
	glPushMatrix();
	glColor3f(1, 1, 1);
	glEnable(GL_TEXTURE_2D);
	GLuint texture;
	glGenTextures(1, &texture);

	unsigned char texture_data[2][2][4] =
	{
		0,0,0,255,  255,255,255,255,
		255,255,255,255,    0,0,0,255
	};

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_NEAREST);

	glBegin(GL_QUADS);

	glTexCoord2f(0.0, 0.0);  glVertex3f(-50.0, -5.0, -50.0);
	glTexCoord2f(25.0, 0.0);  glVertex3f(50.0, -5.0, -50.0);
	glTexCoord2f(25.0, 25.0);  glVertex3f(50.0, -5.0, 50.0);
	glTexCoord2f(0.0, 25.0);  glVertex3f(-50.0, -5.0, 50.0);

	glEnd();

	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
}

void drawPlayer() {
	glPushMatrix();
	//glTranslatef(camX + 2, camY - 2, camZ);
	//glRotatef(yaw, 0.0, 1.0, 0.0);    //Along Y axis
	//glRotatef(180, 0.0, 1.0, 0.0);

	// head
	glColor3f(0.9f, 0.75f, 0.67f);
	glutSolidSphere(0.25, 25, 25);
	// left eye
	glPushMatrix();
	glTranslatef(-0.1, 0.1, 0.15);
	glColor3f(1.0f, 1.0f, 1.0f);
	glutSolidSphere(0.1, 25, 25);
	glPopMatrix();
	// right eye
	glPushMatrix();
	glTranslatef(0.1, 0.1, 0.15);
	glColor3f(1.0f, 1.0f, 1.0f);
	glutSolidSphere(0.1, 25, 25);
	glPopMatrix();
	// torso
	glPushMatrix();
	glScalef(1, 1, 0.5);
	glColor3f(0.8f, 0.2f, 0.2f);
	glTranslatef(0, -0.75, 0);
	glutSolidCube(1);
	// right sleeve
	glPushMatrix();
	glColor3f(0.8f, 0.2f, 0.2f);
	glTranslatef(-0.75, 0.25, 0);
	glScalef(-0.5, 0.5, 1);
	glutSolidCube(1);
	glPopMatrix();
	// right arm
	glPushMatrix();
	glColor3f(0.9f, 0.75f, 0.67f);
	glTranslatef(-0.75, -0.25, 0);
	glScalef(-0.5, 0.5, 1);
	glutSolidCube(1);
	glPopMatrix();
	// left sleeve
	glPushMatrix();
	glColor3f(0.8f, 0.2f, 0.2f);
	glTranslatef(0.75, 0.25, 0);
	glScalef(-0.5, 0.5, 1);
	glutSolidCube(1);
	glPopMatrix();
	// left arm
	glPushMatrix();
	glColor3f(0.9f, 0.75f, 0.67f);
	glTranslatef(0.75, -0.25, 0);
	glScalef(-0.5, 0.5, 1);
	glutSolidCube(1);
	glPopMatrix();
	// right short
	glPushMatrix();
	glColor3f(0.9f, 0.9f, 0.9f);
	glTranslatef(-0.25, -0.75, 0);
	glScalef(-0.5, 0.5, 1);
	glutSolidCube(1);
	glPopMatrix();
	// right leg
	glPushMatrix();
	glColor3f(0.9f, 0.75f, 0.67f);
	glTranslatef(-0.25, -1.25, 0);
	glScalef(-0.5, 0.5, 1);
	glutSolidCube(1);
	glPopMatrix();
	// left short
	glPushMatrix();
	glColor3f(0.9f, 0.9f, 0.9f);
	glTranslatef(0.25, -0.75, 0);
	glScalef(-0.5, 0.5, 1);
	glutSolidCube(1);
	glPopMatrix();
	// left leg
	glPushMatrix();
	glColor3f(0.9f, 0.75f, 0.67f);
	glTranslatef(0.25, -1.25, 0);
	glScalef(-0.5, 0.5, 1);
	glutSolidCube(1);
	glPopMatrix();
	glPopMatrix();

	glPopMatrix();
}

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90, 16.0 / 9.0, 1, 100);
	glMatrixMode(GL_MODELVIEW);

}



void passiveMouseCB(int x, int y)
{
	/* two variables to store X and Y coordinates, as observed from the center
	  of the window
	*/
	int dev_x, dev_y;
	dev_x = (width / 2) - x;
	dev_y = (height / 2) - y;

	/* apply the changes to pitch and yaw*/
	yaw += (float)dev_x / camSensitivity;
	pitch += (float)dev_y / camSensitivity;
}

void camera()
{

	if (motion.Forward)
	{
		camX += cos((yaw + 90) * TO_RADIANS) / 2.0;
		camZ -= sin((yaw + 90) * TO_RADIANS) / 2.0;
	}
	if (motion.Backward)
	{
		camX += cos((yaw + 90 + 180) * TO_RADIANS) / 2.0;
		camZ -= sin((yaw + 90 + 180) * TO_RADIANS) / 2.0;
	}
	if (motion.Left)
	{
		camX += cos((yaw + 90 + 90) * TO_RADIANS) / 2.0;
		camZ -= sin((yaw + 90 + 90) * TO_RADIANS) / 2.0;
	}
	if (motion.Right)
	{
		camX += cos((yaw + 90 - 90) * TO_RADIANS) / 2.0;
		camZ -= sin((yaw + 90 - 90) * TO_RADIANS) / 2.0;
	}

	/*limit the values of pitch
	  between -60 and 70
	*/
	if (isFirstPerson) {
		if (pitch >= 70)
			pitch = 70;
		if (pitch <= -60)
			pitch = -60;
	}
	else
		pitch = -30;

	if (isFirstPerson) {
		glPushMatrix();
		glTranslated(0, -2, -2);
		glutSolidTeapot(1);
		glPopMatrix();
	}

	glRotatef(-pitch, 1.0, 0.0, 0.0);

	if (!isFirstPerson) {
		glTranslated(0, -1, -1); // zoom out
		drawPlayer();
	}

	glRotatef(-yaw, 0.0, 1.0, 0.0);

	glTranslatef(-camX, -camY, -camZ);
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'W':
	case 'w':
		motion.Forward = true;
		break;
	case 'A':
	case 'a':
		motion.Left = true;
		break;
	case 'S':
	case 's':
		motion.Backward = true;
		break;
	case 'D':
	case 'd':
		motion.Right = true;
		break;
	case 'C':
	case 'c':
		isFirstPerson = !isFirstPerson;
		break;
	case ' ':
		if (!isAirborne)
			isAirborne = true;
		break;
	}
}

void keyboard_up(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'W':
	case 'w':
		motion.Forward = false;
		break;
	case 'A':
	case 'a':
		motion.Left = false;
		break;
	case 'S':
	case 's':
		motion.Backward = false;
		break;
	case 'D':
	case 'd':
		motion.Right = false;
		break;
	}
}

void MouseCB(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		isShooting = true;
	}
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	camera();
	drawFloor();
	drawAxes(3);

	if (isShooting)
	{
		bullet_angle = yaw + 180;
		isShooting = false;
		bullet_x = camX;
		bullet_z = camZ;
	}

	if (bullet_angle > 0)
	{
		glPushMatrix();
		//glRotated(bullet_angle, 0, 1, 0);
		glTranslated(bullet_x, 0, bullet_z);
		glutSolidCube(1);
		glPopMatrix();
		bullet_x += sin(bullet_angle * TO_RADIANS) / 2.0;
		bullet_z += cos(bullet_angle * TO_RADIANS) / 2.0;
	}
	
	glutSwapBuffers();
}

/*this funtion is used to keep calling the display function periodically
  at a rate of FPS times in one second. The constant FPS is defined above and
  has the value of 60
*/
void gameLoop(int)
{
	if (isAirborne)
	{
		if (isFalling)
			if (camY < 0)
			{
				isFalling = false;
				isAirborne = false;
				camY = 0;
			}
			else
				camY -= 0.5;
		else
			if (camY > 15)
				isFalling = true;
			else
				camY += 0.5;
	}
	glutPostRedisplay();
	cout << "Cam X: ";
	cout << camX;
	cout << " Cam Y: ";
	cout << camY;
	cout << " Cam Z: ";
	cout << camZ;
	cout << " Yaw";
	cout << yaw;
	cout << '\n';
	cout << " bullet_angle";
	cout << bullet_angle;
	cout << " bullet_x";
	cout << sin(bullet_angle);
	cout << " bullet_z";
	cout << cos(bullet_angle);
	cout << '\n';
	glutWarpPointer(width / 2, height / 2);
	glutTimerFunc(1000 / FPS, gameLoop, 0);
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(width, height);
	glutCreateWindow("Projectile Motion - 3D Simulation");

	glutSetCursor(GLUT_CURSOR_NONE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glutWarpPointer(width / 2, height / 2);

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutPassiveMotionFunc(passiveMouseCB);
	glutTimerFunc(0, gameLoop, 0);    //more info about this is given below at definition of timer()
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboard_up);
	glutMouseFunc(MouseCB);

	glutMainLoop();
	return 0;
}