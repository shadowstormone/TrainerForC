#pragma once
#include <Windows.h>

struct Star
{
	COLORREF color;
	int x, y;
	int speed;
};

class StarFieldEffect
{
	int _starCount = 0;
	int _starsPerPlane = 50; // Max = 437
	int _planesCount = 2;
	int _speed = 1;
	int _width = 0;
	int _height = 0;
	Star* _mStars = NULL;

	HDC workDC = NULL;
	HBITMAP workBitMap = NULL;
	HGDIOBJ oldBitMap = NULL;
	DWORD* _bits = NULL;
	
	//Direction - направление звезд (-1 в лево, 1 в право)
	void MoveStar(Star* star, int derection);
	void PlotPixel(COLORREF color, int x, int y);
	HBITMAP CreateImageForCanvas(HDC hdc);

	int random(int min, int max)
	{
		return min + (rand() % static_cast<int>(max - min + 1));
	}

public:
	StarFieldEffect(HDC sourceDC, int starsPerPlane, int planesCount, int speed, int width, int height);
	void UpdateStars();
	void DrowToDest(HDC destDC, int x, int y);

	~StarFieldEffect();
};