#include <stdio.h>
#include "TextureBuilder.h"
#include "Model_3DS.h"
#include "GLTexture.h"
#include <math.h>
#include <random>
#include <glut.h>
#include <iostream>
#include <tuple>

using namespace std;

const double FPS = 60;
const double TO_RADIANS = 3.14 / 180.0;

struct Motion {
	bool Forward, Backward, Left, Right;
};

struct Bullet {
	double x, y, z, angle;
};

struct Hitbox {
	double x, y, z, size;
};

//width and height of the window ( Aspect ratio 16:9 )
const int width = 16 * 80;
const int height = 9 * 80;

float pitch = 0.0, yaw = 180;
float camX = 0.0, camZ = 0.0, camY = 0.0;
float camSensitivity = 50;

int stage = 1;

bool isFalling = false;
bool isRising = false;
bool isFirstPerson = false;
bool isShooting = false;
bool gameover = false;

double field_width = 20;
double field_depth = 40;

double player_size = 1;
int bullet_damage = 1;
double bullet_size = 0.5;
int bullet_speed = 5;
int score = 0;
int bossFireRate = 1 * 1000;
double bossSpeed = 0.5;
double bossHp = 1000;
double bossMaxHp = 1000;

// 0 = no weapon
// 1 = ak
// 2 = wand
int weaponChoice = 0;

Motion motion = { false,false,false,false };
vector<Bullet> player_bullets;
vector<Bullet> boss_bullets;
vector<Hitbox> platforms;
vector<Hitbox> coins;
vector<Hitbox> powerUps;
Hitbox boss = { 0,2,field_depth,8 };

// Models
Model_3DS player_model;
Model_3DS weapon_model;
Model_3DS boss_model;
Model_3DS wand_model;

// Textures
GLTexture tex_ground;
GLuint cave_tex;
GLuint spaceTex;

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

void print(int x, int y, char* string)
{
	glRasterPos2f(x, y);
	int len, i;
	len = (int)strlen(string);
	for (i = 0; i < len; i++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, string[i]);
	}
}

void RenderGround()
{
	glDisable(GL_LIGHTING); // Disable lighting

	glColor3f(0.6, 0.6, 0.6); // Dim the ground texture a bit

	glEnable(GL_TEXTURE_2D); // Enable 2D texturing

	glBindTexture(GL_TEXTURE_2D, tex_ground.texture[0]); // Bind the ground texture

	glPushMatrix();
	glBegin(GL_QUADS);
	glNormal3f(0, 1, 0); // Set quad normal direction.
	glTexCoord2f(0, 0);	 // Set tex coordinates ( Using (0,0) -> (5,5) with texture wrapping set to GL_REPEAT to simulate the ground repeated grass texture).
	glVertex3f(-field_width, -4, -field_depth);
	glTexCoord2f(5, 0);
	glVertex3f(field_width, -4, -field_depth);
	glTexCoord2f(5, 5);
	glVertex3f(field_width, -4, field_depth);
	glTexCoord2f(0, 5);
	glVertex3f(-field_width, -4, field_depth);
	glEnd();
	glPopMatrix();

	glDisable(GL_TEXTURE_2D); // Enable 2D texturing
	//glEnable(GL_LIGHTING); // Enable lighting again for other entites coming throung the pipeline.

	glColor3f(1, 1, 1); // Set material back to white instead of grey used for the ground texture.
}

void renderCave() {
	// sky box
	GLUquadricObj* qobj;
	qobj = gluNewQuadric();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, cave_tex);
	gluQuadricTexture(qobj, true);
	gluQuadricNormals(qobj, GL_SMOOTH);

	glPushMatrix();
	glTranslated(0, 0, -field_depth);
	gluCylinder(qobj, field_width, field_width, field_depth * 2, 100, 100);
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, 0, field_depth);
	glScaled(1, 1, 0.01);
	gluSphere(qobj, field_width, 100, 100);
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, 0, -field_depth);
	glScaled(1, 1, 0.01);
	gluSphere(qobj, field_width, 100, 100);
	glPopMatrix();

	gluDeleteQuadric(qobj);
	glDisable(GL_TEXTURE_2D);
}

void renderSpace() {
	// sky box
	GLUquadricObj* qobj;
	qobj = gluNewQuadric();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, spaceTex);
	gluQuadricTexture(qobj, true);
	gluQuadricNormals(qobj, GL_SMOOTH);

	glPushMatrix();
	glTranslated(0, 0, -field_depth);
	gluSphere(qobj, 1000, 100, 100);
	glPopMatrix();

	gluDeleteQuadric(qobj);
	glDisable(GL_TEXTURE_2D);
}

void generatePlatforms() {
	Hitbox platform = {
			0,
			-8,
			0,
			5
	};
	platforms.push_back(platform);

	// Providing a seed value
	srand((unsigned)time(NULL));

	for (int i = 30; i <= 300; i += 30)
	{
		int x = rand() % 40;



		if (i % 120 == 0) {
			Hitbox weapon1 = {
				-20,
				-5,
				i,
				1
			};
			powerUps.push_back(weapon1);

			Hitbox weapon2 = {
				20,
				-5,
				i,
				1
			};
			powerUps.push_back(weapon2);

			Hitbox platform1 = {
			-20,
			-8,
			i,
			5
			};
			platforms.push_back(platform1);

			Hitbox platform2 = {
			20,
			-8,
			i,
			5
			};
			platforms.push_back(platform2);
		}
		else {
			Hitbox platform = {
			x - 20,
			-8,
			i,
			5
			};

			platforms.push_back(platform);
			Hitbox pickable = {
				x - 20,
				-5,
				i,
				1
			};
			coins.push_back(pickable);
		}
	}

	platform = {
			0,
			-8,
			330,
			5
	};

	platforms.push_back(platform);
}

void drawPlatforms() {
	for (auto& platform : platforms)
	{
		glPushMatrix();
		glTranslated(platform.x, platform.y, platform.z);
		glColor3f(1, 1, 1);
		glScaled(1, 0.2, 1);
		glutSolidCube(platform.size * 2);
		glPopMatrix();
	}
}

void drawCoins() {
	for (auto& coin : coins)
	{
		glPushMatrix();
		glTranslated(coin.x, coin.y, coin.z);
		glColor3f(0, 1, 1);
		glutSolidCube(coin.size * 2);
		glPopMatrix();
	}
}

void drawPowerUps() {
	for (auto& powerUp : powerUps)
	{
		glPushMatrix();
		glTranslated(powerUp.x, powerUp.y, powerUp.z);
		if (powerUp.z == 120) {
			glColor3f(1, 1, 1);
			if (powerUp.x == 20)
			{
				glScaled(0.1, 0.1, 0.1);
				weapon_model.Draw();
			}
			else {
				glTranslated(0, 3, 0);
				glScaled(0.3, 0.3, 0.3);
				glRotated(90, 1, 0, 0);
				wand_model.Draw();
			}
			glDisable(GL_TEXTURE_2D);
		}
		else {
			glColor3f(1, 0, 1);
			glutSolidCube(powerUp.size * 2);
		}
		glPopMatrix();
	}
}

void drawBullets() {
	glColor3f(0.5, 1, 1);
	for (auto& bullet : player_bullets) // access by reference to avoid copying
	{
		glPushMatrix();
		//glRotated(bullet_angle, 0, 1, 0); // you can rotate then translate but makes collision detection harder
		glTranslated(bullet.x, bullet.y, bullet.z);
		glutSolidSphere(bullet_size, 20, 20);
		glPopMatrix();
	}
	glColor3f(1, 0, 0);
	for (auto& bullet : boss_bullets) // access by reference to avoid copying
	{
		glPushMatrix();
		//glRotated(bullet_angle, 0, 1, 0); // you can rotate then translate but makes collision detection harder
		glTranslated(bullet.x, bullet.y, bullet.z);
		glutSolidSphere(bullet_size, 20, 20);
		glPopMatrix();
	}
}

void drawPlayer() {

}

void drawBoss() {
	glPushMatrix();
	glTranslated(boss.x, boss.y, boss.z);
	glScaled(0.1, 0.1, 0.1);
	boss_model.Draw();
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
}

void DrawRectangle(int x, int y, int w, int h)
{
	glBegin(GL_POLYGON);
	glVertex2f(x, y);
	glVertex2f(x + w, y);
	glVertex2f(x + w, y + h);
	glVertex2f(x, y + h);
	glEnd();
}

void DrawBossHealthBar()
{
	glColor3f(1, 1, 1);
	DrawRectangle(45, height - 30, width - 100, 25);
	glColor3f(1, 0.3, 0.3);
	DrawRectangle(50, height - 25, (width - 110) * (bossHp / bossMaxHp), 15);
}

void drawHUD() {
	glDisable(GL_TEXTURE_2D);
	DrawBossHealthBar();
	glColor3d(1, 1, 1);
	char result[100]; // enough to hold all numbers up to 64-bits
	print(0, 25, "Stats: ");
	sprintf(result, "  Score: %d", score);
	print(0, 50, result);
	sprintf(result, "  Bullet Damage: %d", bullet_damage);
	print(0, 75, result);
	sprintf(result, "  Bullet Speed: %d", bullet_speed);
	print(0, 100, result);
	sprintf(result, "  Bullet Size: %f", bullet_size);
	print(0, 125, result);

	glBegin(GL_LINES);
	if (isFirstPerson) {
		glVertex2i(width / 2, height / 2 - 20);
		glVertex2i(width / 2, height / 2 + 20);
		glVertex2i(width / 2 - 20, height / 2);
		glVertex2i(width / 2 + 20, height / 2);
	}
	else {
		glVertex2i(width / 2, height / 2 - 20 - 200);
		glVertex2i(width / 2, height / 2 + 20 - 200);
		glVertex2i(width / 2 - 20, height / 2 - 200);
		glVertex2i(width / 2 + 20, height / 2 - 200);
	}
	glEnd();
}

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90, 16.0 / 9.0, 1, 100);
	glMatrixMode(GL_MODELVIEW);
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
		pitch = 0;
		isFirstPerson = !isFirstPerson;
		break;
	case ' ':
		if (!isRising && !isFalling)
			isRising = true;
		break;
	case 27:
		exit(EXIT_SUCCESS);
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

void MouseMotionCB(int x, int y)
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

void MouseClickCB(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		isShooting = true;
	}

	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		isShooting = false;
	}
}

// detects collision using rectanges for hitboxes
// x1,y1,z1 = center of object 1
// size1 = size of object 1
// x2,y2,z2 = center of object 2
// size2 = size of object 2
bool IsCollision(double x1, double y1, double z1, double size1, double x2, double y2, double z2, double size2) {
	tuple<double, double, double> top_right1 = make_tuple(x1 + size1, y1 + size1, z1 + size1);
	tuple<double, double, double> bottom_left1 = make_tuple(x1 - size1, y1 - size1, z1 - size1);

	tuple<double, double, double> top_right2 = make_tuple(x2 + size2, y2 + size2, z2 + size2);
	tuple<double, double, double> bottom_left2 = make_tuple(x2 - size2, y2 - size2, z2 - size2);

	// If one rectangle is on left side of other
	if (get<0>(top_right1) < get<0>(bottom_left2) || get<0>(top_right2) < get<0>(bottom_left1))
		return false;

	// If one rectangle is above other
	if (get<1>(top_right1) < get<1>(bottom_left2) || get<1>(top_right2) < get<1>(bottom_left1))
		return false;

	// If one rectangle is behind other
	if (get<2>(top_right1) < get<2>(bottom_left2) || get<2>(top_right2) < get<2>(bottom_left1))
		return false;

	return true;
}

// prepare for 3D rendering
void ready3D() {
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90, 16.0 / 9.0, 0.1, 1200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
}

// prepare for 2D rendering
void ready2D() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluOrtho2D(0.0f, width, height, 0.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
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

	// limit pitch of the view
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
		if(weaponChoice == 1)
		{
		glTranslated(1, -1, -1);
			glRotated(190, 0, 1, 0);
			glScaled(0.05, 0.05, 0.05);
			glColor3f(1, 1, 1);
			weapon_model.Draw();
		}
		if (weaponChoice == 2)
		{
			glTranslated(1, -1, -1.5);
			glScaled(0.1, 0.1, 0.1);
			glColor3f(1, 1, 1);
			wand_model.Draw();
		}
		glPopMatrix();
	}

	glRotatef(-pitch, 1.0, 0.0, 0.0);

	if (!isFirstPerson) {
		glTranslated(0, -1, -1); // zoom out
		glColor3f(1, 1, 1);
		glPushMatrix();
		glScaled(2, 2, 2);
		glTranslated(0, -1, 0);
		glRotated(180, 0, 1, 0);
		player_model.Draw();
		glPopMatrix();
	}

	glDisable(GL_TEXTURE_2D); // drawing models ruins colors without this line

	glRotatef(-yaw, 0.0, 1.0, 0.0);

	glTranslatef(-camX, -camY, -camZ);
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	ready3D();

	camera();

	if (stage == 1) {
		renderSpace();
		drawPlatforms();
		drawCoins();
		drawPowerUps();
	}
	else {
		//drawFloor();
		RenderGround();
		drawBoss();
		renderCave();
	}

	drawBullets();

	drawAxes(3);
	// 1 height cubes for reference
	/*glPushMatrix();
	glTranslated(-1, 0, 0);
	glutSolidCube(1);
	glTranslated(3, 0, 0);
	glScaled(1, 4, 1);
	glutSolidCube(1);
	glPopMatrix();
	player_model.Draw();
	wand_model.Draw();*/

	ready2D();

	drawHUD();

	glutSwapBuffers();
}

void bossShootTimer(int x) {

	Bullet bullet = {
		boss.x ,
		-1,
		boss.z,
		180
	};
	boss_bullets.push_back(bullet);
	glutTimerFunc(bossFireRate, bossShootTimer, 0);
}

void LoadAssets()
{
	// Loading Model files
	player_model.Load("Models/player/lumberJack.3DS");
	weapon_model.Load("Models/weapon/Gun Jackhammer MK3A1 N290415.3ds");
	boss_model.Load("Models/dragon.3ds");
	wand_model.Load("Models/newtwand.3ds");

	// Loading texture files
	tex_ground.Load("Textures/ground.bmp");
	loadBMP(&cave_tex, "Textures/cave.bmp", true);
	loadBMP(&spaceTex, "Textures/space.bmp", true);
}

// calculate player collision with platforms
void platformsCollision() {
	// player jumping platfroms logic
	for (auto platform = platforms.begin(); platform < platforms.end(); platform++)
	{
		// collision with platfroms
		if (IsCollision(platform->x, platform->y, platform->z, platform->size, camX, camY - 2, camZ, player_size))
		{
			if (isFalling && camY >= 0)
				isFalling = false;
			if (platform->z == 330)
			{
				glutTimerFunc(bossFireRate, bossShootTimer, 0);
				stage = 2;
				camX = 0;
				camZ = 0;
			}
			break;
		}
		if (!isRising)
			isFalling = true;
	}
}

// calculate player collision with platforms
void coinsCollision() {
	// player jumping platfroms logic
	for (auto coin = coins.begin(); coin < coins.end();)
	{
		// collision with platfroms
		if (IsCollision(coin->x, coin->y, coin->z, coin->size, camX, camY - 2, camZ, 2))
		{
			coin = coins.erase(coin);
			score++;
			continue;
		}
		coin++;
	}
}

// calculate player collision with platforms
void powerUpsCollision() {
	// player jumping platfroms logic
	for (auto powerUp = powerUps.begin(); powerUp < powerUps.end();)
	{
		if ((powerUp->x == -20 || powerUp->x == 20) && weaponChoice > 0 && powerUp->z <200)
		{
			powerUp = powerUps.erase(powerUp);
			continue;
		}
		// collision with platfroms
		if (IsCollision(powerUp->x, powerUp->y, powerUp->z, powerUp->size, camX, camY - 2, camZ, 2))
		{
			// damage boost
			if ((int)(powerUp->z) % 360 == 0) {
				bullet_damage++;
			}
			// speed boost
			else if ((int)(powerUp->z) % 240 == 0) {
				bullet_speed = 10;
			}
			// bullet size
			else {
				if (powerUp->x == -20 )
					weaponChoice = 2;
				else if (powerUp->x == 20)
					weaponChoice = 1;
			}
			powerUp = powerUps.erase(powerUp);
			continue;
		}
		powerUp++;
	}
}


/*this funtion is used to keep calling the display function periodically
  at a rate of FPS times in one second. The constant FPS is defined above and
  has the value of 60
*/
void gameLoop(int)
{
	// check if player is mid jump
	if (isRising) {
		if (camY > 15) {
			isRising = false;
			isFalling = true;
		}
		else
			camY += 0.5;
	}

	if (isFalling) {
		camY -= 0.5;
	}

	// collision logic for first stage
	if (stage == 1) {

		platformsCollision();
		coinsCollision();
		powerUpsCollision();
	}
	// collision logic for second stage
	else {
		if (camY <= 0)
			isFalling = false;
		if (boss.x >= field_width || boss.x <= -field_width)
			bossSpeed = -bossSpeed;
		boss.x += bossSpeed;
	}


	// create bullets if shooting;
	if (isShooting) {
		Bullet bullet = {
			camX ,
			camY - 1,
			camZ,
			yaw + 180 - 360
		};
		player_bullets.push_back(bullet);
	}

	// update player bullets' locations & detect hitting boss
	for (auto bullet = player_bullets.begin(); bullet < player_bullets.end(); )
	{
		// collision with boss
		if (IsCollision(bullet->x, bullet->y, bullet->z, bullet_size, boss.x, boss.y, boss.z, boss.size))
		{
			bullet = player_bullets.erase(bullet);
			bossHp--;
			continue;
		}

		// update location
		bullet->x += sin(bullet->angle * TO_RADIANS) * bullet_speed / 4.0;
		bullet->z += cos(bullet->angle * TO_RADIANS) * bullet_speed / 4.0;
		bullet++;
	}

	// update player bullets' locations & detect hitting boss
	for (auto bullet = boss_bullets.begin(); bullet < boss_bullets.end(); )
	{
		// collision with boss
		if (IsCollision(bullet->x, bullet->y, bullet->z, bullet_size, camX, camY, camZ, player_size))
		{
			bullet = boss_bullets.erase(bullet);
			bossHp--;
			continue;
		}

		// update location
		bullet->x += sin(bullet->angle * TO_RADIANS) * bullet_speed / 4.0;
		bullet->z += cos(bullet->angle * TO_RADIANS) * bullet_speed / 4.0;
		bullet++;
	}

	glutPostRedisplay();
	glutWarpPointer(width / 2, height / 2);
	glutTimerFunc(1000 / FPS, gameLoop, 0);
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(width, height);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow("Projectile Motion - 3D Simulation");

	glutSetCursor(GLUT_CURSOR_NONE);

	glutWarpPointer(width / 2, height / 2);

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutPassiveMotionFunc(MouseMotionCB);
	glutMotionFunc(MouseMotionCB);
	glutMouseFunc(MouseClickCB);
	glutTimerFunc(0, gameLoop, 0);    //more info about this is given below at definition of timer()
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboard_up);

	// randomly generate platforms once in per game
	generatePlatforms();
	LoadAssets();

	glutMainLoop();
	return 0;
}