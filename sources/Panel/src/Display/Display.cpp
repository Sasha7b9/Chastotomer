#include "defines.h"
#include "stm32f4xx_hal.h"
#include "Display.h"
#include "Hardware/HAL.h"
#include <cstring>

#include "Display/Primitives.h"
#include "Menu/Menu.h"


using namespace Display::Primitives;

/// ���� ����� ������������ �� ������
static uint8 front[240][320];
/// � ���� ������ ������
static uint8 RGB565_240x320[240][320] = { 0x00000000 };

LTDC_HandleTypeDef hltdc;
static void MX_LTDC_Init(void);
static void MX_GPIO_Init(void);

static void BeginScene(Color color);
/// �������� ������ ����� �� �����
static void EndScene();


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8* Display::GetBuff()
{
    return &(RGB565_240x320[0][0]);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Display::Init()
{
    MX_GPIO_Init();
    MX_LTDC_Init();

    uint ColorTable[2] = { 0x00000000U, 0xFFFFFFFFU };

    HAL_LTDC_EnableCLUT(&hltdc, 0);

    HAL_LTDC_ConfigCLUT(&hltdc, ColorTable, 2, 0);

    HAL_LTDC_SetAddress(&hltdc, (uint)&front, 0);
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
static void BeginScene(Color color)
{
    Rectangle(320, 240).Fill(0, 0, color);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void EndScene()
{
    std::memcpy(front, RGB565_240x320, 240 * 320);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void MX_LTDC_Init(void)
{
    LTDC_LayerCfgTypeDef pLayerCfg = { 0 };

    hltdc.Instance = LTDC;
    hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AH;
    hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IIPC;
    hltdc.Init.HorizontalSync = 0;
    hltdc.Init.VerticalSync = 0;
    hltdc.Init.AccumulatedHBP = 70;
    hltdc.Init.AccumulatedVBP = 13;
    hltdc.Init.AccumulatedActiveW = 390;
    hltdc.Init.AccumulatedActiveH = 253;
    hltdc.Init.TotalWidth = 408;
    hltdc.Init.TotalHeigh = 263;
    hltdc.Init.Backcolor.Blue = 0;
    hltdc.Init.Backcolor.Green = 0;
    hltdc.Init.Backcolor.Red = 0;

    if (HAL_LTDC_Init(&hltdc) != HAL_OK)
    {
        ERROR_HANDLER();
    }

    pLayerCfg.WindowX0 = 0;
    pLayerCfg.WindowX1 = 320;
    pLayerCfg.WindowY0 = 0;
    pLayerCfg.WindowY1 = 240;
    pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_L8;
    pLayerCfg.Alpha = 0xFF;
    pLayerCfg.Alpha0 = 0xFF;
    pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
    pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
    pLayerCfg.FBStartAdress = (uint32_t)&(RGB565_240x320[0][0]);
    pLayerCfg.ImageWidth = 320;
    pLayerCfg.ImageHeight = 240;
    pLayerCfg.Backcolor.Blue = 0;
    pLayerCfg.Backcolor.Green = 0;
    pLayerCfg.Backcolor.Red = 0;

    if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
    {
        ERROR_HANDLER();
    }
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}
