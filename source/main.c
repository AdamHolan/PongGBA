#include "Intellisense.h"

// Define some basic types
#include <stdint.h>
#include <stdbool.h>

// Jamie Stewart's channel on GBA Dev: https://www.youtube.com/channel/UCQvQbVuWMM7OYTVrmGQq0hA
#pragma region tutorialCode
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef int8_t		s8;
typedef int16_t		s16;
typedef int32_t		s32;

// Define volatile types
typedef volatile uint8_t	vu8;
typedef volatile uint16_t	vu16;
typedef volatile uint32_t	vu32;
typedef volatile int8_t		vs8;
typedef volatile int16_t	vs16;
typedef volatile int32_t	vs32;

// Set display 
#define REG_DISPCNT *((vu32*)(0x04000000))
#define VIDEOMODE_3 0x0003
#define BG_ENABLE2  0x0400

#define SCREENBUFFER ((vu16*)(0x06000000))
#define SCREEN_W 240
#define SCREEN_H 160

u16 setColour(u8 a_red, u8 a_green, u8 a_blue)
{
	return (a_red & 0x1F) | (a_green & 0x1F) << 5 | (a_blue & 0x1F) << 10;
}

void plotPixel(s32 a_x, s32 a_y, u16 a_colour) 
{
	SCREENBUFFER[a_x + a_y * SCREEN_W] = a_colour;
}

void drawRect(s32 a_left, s32 a_top, s32 a_width, s32 a_height, u16 a_colour)
{
	for(s32 y = 0; y < a_height; y++)
	{
		for (s32 x = 0; x < a_width; x++)
		{
			plotPixel(a_left + x, a_top + y, a_colour);
		}
	}
}

// Function to calculate absolute value
// No branching, only using bit shifting
// I have no idea how the fuck this works, but it does, very well
s32 abs(s32 a_val)
{
	s32 mask = a_val >> 31;
	return (a_val ^ mask) - mask;
}

// Bresenhams method (?) for drawing a line
void drawLine(s32 a_x, s32 a_y, s32 a_xEnd, s32 a_yEnd, u16 a_colour) 
{
	// get horisontal and vertical displacment of the line
	s32 w = a_xEnd - a_x;
	s32 h = a_yEnd - a_y;
	// Find the change in (delta) x and y
	s32 dx1 = 0, dx2 = 0, dy1 = 0, dy2 =0;

	if (w<0) dx1 = dx2 = -1; else if (w>0) dx1 = dx2 = 1;
	if (h<0) dy1 = -1; else if (h>0) dy1 = 1;
	// Figure out longest side
	s32 longest = abs(w);
	s32 shortest = abs(h);
	// Test our assumption
	if (shortest > longest)
	{
		// Use xor to swap value
		longest ^= shortest; shortest ^= longest; longest ^= shortest;
		if (h<0) dy2 = -1; else if (h>0) dy2 = 1;
		dx2 = 0;
	}
	// Get half of the longest displacement
	s32 numerator = longest >> 1;
	for (s32 i = 0; i <= longest; i++)
	{
		plotPixel(a_x, a_y, a_colour);
		// Increase the numerator by the shortest span
		numerator += shortest;
		if (numerator > longest)
		{
			numerator -= longest;
			a_x += dx1;
			a_y += dy1;
		}
		else 
		{
			a_x += dx2;
			a_y += dy2;
		}
	}
}
#pragma endregion

// GBA Random Generation: https://www.youtube.com/watch?v=RDehQfeNNNs
#pragma region randomGeneration
// Random number generation
s32 __gba_rand_seed = 42;

s32 seed_gba_rand(s32 a_seed)
{
	s32 oldSeed = __gba_rand_seed;
	__gba_rand_seed = a_seed;
	return oldSeed;
}

// Generate a random value using "LCG" values from a book called "Numerical Recipies"
s32 gba_rand()
{
	__gba_rand_seed = 1664525 * __gba_rand_seed + 1013904223;
	return (__gba_rand_seed >> 16) & 0x7FFF;
}

// Random vlue within a range
s32 gba_rand_range(s32 min, s32 max)
{
	return (gba_rand() * (max - min) >> 15) + min;
}
#pragma endregion 

// All code surrounding the pong ball
#pragma region pongBall

typedef struct Ball{
	s32 x, y, xChange, yChange, size;
	u16 colour;
// This v Apparantly lets you point to Ball without using struct
}Ball;

// Start the ball moving! if it isn't moving, move it! Randomly!
void startBall(Ball* ball)
{
	while(ball -> xChange == 0)
	{
		ball-> xChange = gba_rand_range(-1, 2);
	}
	ball->yChange = gba_rand_range(-1, 2);
}

// Initialize balls values, kinda like a constructor
void initBall(Ball* ball, s32 ax, s32 ay, s32 asize, u16 acolour)
{
	// Initialise values
	// I use the "a[attribute]" denotation here cause i dont wanna get confused
	ball->x = ax;
	ball->y = ay;
	ball->size = asize;
	ball->colour = acolour;
	ball->xChange = ball->yChange = 0;
	startBall(ball);
}

void moveBall(Ball* ball)
{
	// Apply movement
	ball->y += ball->yChange;
	
	// Check if ball went above screen and if it did, reverse direction
	if(ball->y < 0)
	{
		ball->y = 0;
		ball->yChange *= -1;
	}

	// Same but bottom of screen
	else if(ball ->y > SCREEN_H)
	{
		ball->y = SCREEN_H;
		ball->yChange *= -1;
	}

	// Now in the x direction
	ball->x += ball->xChange;

	// If ball goes out of bounds (x) reset it's position
	if (ball->x <0 || ball->x > SCREEN_W - ball->size)
	{
		// Bit-shifting by one is the same as dividing by two
		ball->x = (SCREEN_W >> 1) - (ball->size >> 1);
		ball->y = (SCREEN_H >> 1) - (ball->size >> 1);
		ball->xChange = ball->yChange = 0;
		startBall(ball);
	}
}

// Very simple ball drawy thing
void drawBall(Ball* ball)
{
	drawRect(ball->x, ball->y, ball->size, ball->size, ball->colour);
}

// Just clear the space the ball occupies rather than the whole screen cause the GBA sucks
void clearBall(Ball* ball)
{
	drawRect(ball->x, ball->y, ball->size, ball->size, setColour(0, 0, 0));
}


#pragma endregion

// Paddles!
#pragma region paddles

// Create Paddle
typedef struct Paddle{
	s32 x, y, width, height;
	u16 colour;
}Paddle;

// Initialize paddle
void initPaddle(Paddle* paddle, s32 ax, u16 acolour)
{
	// Initialise values
	// I use the "a[attribute]" denotation here cause i dont wanna get confused
	paddle->x = ax;
	paddle->y = 0;
	paddle->width = 2;
	paddle->height = 10;
	paddle->colour = acolour;
}

// For now, this is just a demo
void movePaddle(Paddle* paddle, Ball* ball)
{
	paddle->y = ball->y;
}

void drawPaddles(Paddle* paddle1, Paddle* paddle2)
{
	drawRect(paddle1->x, paddle1->y, paddle1->width, paddle1->height, paddle1->colour);
	drawRect(paddle2->x, paddle2->y, paddle2->width, paddle2->height, paddle2->colour);
}

void clearPaddles(Paddle* paddle1, Paddle* paddle2)
{
	drawRect(paddle1->x, paddle1->y, paddle1->width, paddle1->height, setColour(0, 0, 0));
	drawRect(paddle2->x, paddle2->y, paddle2->width, paddle2->height, setColour(0, 0, 0));
}

#pragma endregion

// GBA Vsync code, otherwise the ball blits around too fast it looks bad
#pragma region vsync

// Vsyncing... what the fuck does any of this mean
#define REG_VCOUNT (*(vu16*)(0x04000006))
void vsync()
{
	while(REG_VCOUNT >= SCREEN_H);
	while (REG_VCOUNT < SCREEN_H);
}

#pragma endregion

int main()
{
	// Colours :)
 	u16 white = setColour(31, 31, 31);

	//set GBA rendering context to MODE 3 Bitmap Rendering
	REG_DISPCNT = VIDEOMODE_3 | BG_ENABLE2;

	// Test random value
	seed_gba_rand(23343);

	Ball pongBall;
	// bitshift instead of div/2 for extra unreadability
	initBall(&pongBall, SCREEN_W >> 1, SCREEN_H >> 1, 10, white);

	Paddle paddleLeft;
	Paddle paddleRight;
	initPaddle(&paddleLeft, 0, white);
	initPaddle(&paddleRight, SCREEN_W - 5, white);


	while(1){
		vsync();
		clearBall(&pongBall);
		clearPaddles(&paddleLeft, &paddleRight);
		moveBall(&pongBall);
		movePaddle(&paddleLeft, &pongBall);
		movePaddle(&paddleRight, &pongBall);
		drawBall(&pongBall);
		drawPaddles(&paddleLeft, &paddleRight);
	}
	return 0;
}