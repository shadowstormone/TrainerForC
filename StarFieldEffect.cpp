#include "StarFieldEffect.h"

void StarFieldEffect::MoveStar(Star* star, int derection)
{
	star->x = star->x + star->speed * derection;

	if (star->x <= -1)
	{
		star->x = _width - 1;
		star->y = random(0, _height - 1);
	}

	if (star->x >= _width)
	{
		star->x = 0;
		star->y = random(0, _height - 1);
	}
}

void StarFieldEffect::PlotPixel(COLORREF color, int x, int y)
{
	int offset = y * _width + x;
	*(_bits + offset) = color;
}

HBITMAP StarFieldEffect::CreateImageForCanvas(HDC hdc)
{
	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof(BITMAPINFO));

	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = _width;
	bi.bmiHeader.biHeight = -_height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;

	return CreateDIBSection(hdc, &bi,DIB_RGB_COLORS, reinterpret_cast<LPVOID*>(&_bits), NULL, 0);
}

StarFieldEffect::StarFieldEffect(HDC sourceDC, int starsPerPlane, int planesCount, int speed, int width, int height) :
	_starsPerPlane(starsPerPlane), _planesCount(planesCount), _speed(speed), _width(width), _height(height)
{
	_starCount = starsPerPlane * planesCount;
	workDC = CreateCompatibleDC(sourceDC);
	workBitMap = CreateImageForCanvas(workDC);
	oldBitMap = SelectObject(workDC, workBitMap);

	_mStars = new Star[_starCount];
	for (int i = 0; i < _starCount; i++)
	{
		_mStars[i].x = random(0, _width);
		_mStars[i].y = random(0, _height);
		_mStars[i].speed = random(speed, speed + planesCount);
		_mStars[i].color = RGB(255, 255, 255);
	}
}

void StarFieldEffect::UpdateStars()
{
	memset(_bits, 0, _width * _height * 4);
	for (int i = 0; i < _starCount; i++)
	{
		Star* star = &_mStars[i];
		MoveStar(star, -1);
		PlotPixel(star->color, star->x, star->y);
		PlotPixel(star->color, star->x + 1, star->y);
		PlotPixel(star->color, star->x + 2, star->y);
	}
}

void StarFieldEffect::DrowToDest(HDC destDC, int x, int y)
{
	BitBlt(destDC, x, y, _width, _height, workDC, 0, 0, SRCCOPY);
}

StarFieldEffect::~StarFieldEffect()
{
	delete[] _mStars;
}

//#include "StarFieldEffect.h"
//
//// Метод для перемещения "звезды" (или текста) в определенном направлении
//void StarFieldEffect::MoveStar(Star* star, int direction)
//{
//    star->x = star->x + star->speed * direction;
//
//    if (star->x <= -1)
//    {
//        star->x = _width - 1;
//        star->y = random(0, _height - 1);
//    }
//
//    if (star->x >= _width)
//    {
//        star->x = 0;
//        star->y = random(0, _height - 1);
//    }
//}
//
//// Метод для создания изображения для холста
//HBITMAP StarFieldEffect::CreateImageForCanvas(HDC hdc)
//{
//    BITMAPINFO bi;
//    ZeroMemory(&bi, sizeof(BITMAPINFO));
//
//    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//    bi.bmiHeader.biWidth = _width;
//    bi.bmiHeader.biHeight = -_height;
//    bi.bmiHeader.biPlanes = 1;
//    bi.bmiHeader.biBitCount = 32;
//
//    return CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, reinterpret_cast<LPVOID*>(&_bits), NULL, 0);
//}
//
//// Конструктор класса StarFieldEffect
//StarFieldEffect::StarFieldEffect(HDC sourceDC, int starsPerPlane, int planesCount, int speed, int width, int height) :
//    _starsPerPlane(starsPerPlane), _planesCount(planesCount), _speed(speed), _width(width), _height(height)
//{
//    _starCount = starsPerPlane * planesCount;
//    workDC = CreateCompatibleDC(sourceDC);
//    workBitMap = CreateImageForCanvas(workDC);
//    oldBitMap = SelectObject(workDC, workBitMap);
//
//    _mStars = new Star[_starCount];
//    for (int i = 0; i < _starCount; i++)
//    {
//        _mStars[i].x = random(0, _width);
//        _mStars[i].y = random(0, _height);
//        _mStars[i].speed = random(speed, speed + planesCount);
//        _mStars[i].color = RGB(255, 255, 255);
//    }
//}
//
//// Метод для обновления и отрисовки текста на холсте
//void StarFieldEffect::UpdateStars()
//{
//    // Очистка экрана
//    memset(_bits, 0, _width * _height * 4);
//
//    // Устанавливаем параметры текста
//    SetBkMode(workDC, TRANSPARENT);
//    SetTextColor(workDC, RGB(255, 255, 255)); // Белый цвет текста
//
//    // Перебираем все "звезды" и рисуем текст вместо них
//    for (int i = 0; i < _starCount; i++)
//    {
//        Star* star = &_mStars[i];
//        MoveStar(star, -1); // Перемещаем текст (направление -1)
//
//        // Рисуем текст "ShadowStormOne" на позиции "звезды"
//        TextOut(workDC, star->x, star->y, L"ShadowStormOne", wcslen(L"ShadowStormOne"));
//    }
//}
//
//// Метод для копирования холста на конечный DC
//void StarFieldEffect::DrowToDest(HDC destDC, int x, int y)
//{
//    BitBlt(destDC, x, y, _width, _height, workDC, 0, 0, SRCCOPY);
//}
//
//// Деструктор для очистки ресурсов
//StarFieldEffect::~StarFieldEffect()
//{
//    delete[] _mStars;
//    SelectObject(workDC, oldBitMap);
//    DeleteObject(workBitMap);
//    DeleteDC(workDC);
//}