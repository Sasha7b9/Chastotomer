#include "defines.h"
#include "stm32f4xx_hal.h"
#include "Display.h"
#include "Hardware/HAL.h"
#include "Display/Primitives.h"
#include "Menu/Menu.h"
#include <cstring>


using namespace Display::Primitives;

/// ����� � �����������
static uint8 buffer[256 * 64 / 8];


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8* Display::GetBuff()
{
    return buffer;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Display::Init()
{
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Display::Update()
{
    BeginScene(Color::BLACK);

    static int x0 = 0;
    static int y0 = 0;

    x0++;
    y0++;

    if (x0 == 2)
    {
        x0 = 0;
        y0 = 0;
    }

    int x = x0 + (320 - 256) / 2;
    int y = y0 + (240 - 64) / 2;

    Rectangle(256, 64).Draw(x, y, Color::WHITE);

    Menu::Draw(x + 30, y + 30);

    EndScene();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Display::BeginScene(Color color)
{
    Rectangle(320, 240).Fill(0, 0, color);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Display::EndScene()
{

}
