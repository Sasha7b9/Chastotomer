#include "defines.h"
#include "Settings.h"
#include "Display/Display.h"
#include "Hardware/FPGA.h"
#include "Hardware/MathFPGA.h"
#include "Hardware/HAL/HAL.h"
#include "Menu/Hint.h"
#include "Menu/Menu.h"
#include "Menu/Pages/PageIndication.h"
#include "Menu/Pages/PageStatistics.h"
#include "Menu/Pages/Modes/PageModesA.h"
#include "Menu/Pages/Modes/PageModesB.h"
#include "Menu/Pages/Modes/PageModesC.h"
#include "Menu/Pages/Modes/PageModesD.h"
#include "Menu/Pages/Settings/PagesSettings.h"
#include "Utils/StringUtils.h"
#include <cstring>
#include <cstdio>
#include <stm32f4xx_hal.h>


#define READ_PIN_B14(x)                                                                 \
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);                                \
    HAL_TIM::DelayUS(2);                                                                \
    x = (char)HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);                                     \
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);                              \
    HAL_TIM::DelayUS(2);

#define CYCLE_READ_PIN_B14(num, x)                                                      \
    for (int i = 0; i < num; i++)                                                       \
    {                                                                                   \
        READ_PIN_B14(x[i]);                                                             \
    }                                                                                   \
    Display::Refresh();

#define WRITE_COMMAND(x)                                                                \
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, (x == 0) ? GPIO_PIN_RESET : GPIO_PIN_SET);    \
    HAL_TIM::DelayUS(2);                                                                \
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);                                \
    HAL_TIM::DelayUS(2);                                                                \
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);

#define CYCLE_WRITE_COMMAND(num, x)                                                     \
    for (int i = 0; i < num; i++)                                                       \
    {                                                                                   \
        WRITE_COMMAND(x[i]);                                                            \
    }

char FPGA::dataTimer[27] = { 0 };
char FPGA::dataIdent[4] = { 0 };
char FPGA::dataCAL1[24] = { 0 };
char FPGA::dataCAL2[24] = { 0 };

static char encData[10];
static bool autoMode = false;
static char calibBin[10];
static int calibNumber = 0;
static int NAC = 0;


void FPGA::Init()
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    
    GPIO_InitTypeDef is =
    {
        GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_15,
        GPIO_MODE_OUTPUT_PP,
        GPIO_PULLUP
    };
    HAL_GPIO_Init(GPIOB, &is);

    is.Pin = GPIO_PIN_14;
    is.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOB, &is);

    is.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    HAL_GPIO_Init(GPIOC, &is);
}


void FPGA::Update()
{
    if(autoMode)
    {
        if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) != 0)
        {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

            CYCLE_READ_PIN_B14(3, dataIdent);

            CYCLE_READ_PIN_B14(10, MathFPGA::Auto::dataMin);

            CYCLE_READ_PIN_B14(10, MathFPGA::Auto::dataMid);

            CYCLE_READ_PIN_B14(10, MathFPGA::Auto::dataMax);

            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
            HAL_TIM::DelayUS(8);
        }
    }
    else
    {
        if (ModeMeasureDuration::Current().Is_Ndt_1ns() && PageModesA::InterpolateCheck())
        {
            if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) != 0)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

                CYCLE_READ_PIN_B14(3, dataIdent);

                CYCLE_READ_PIN_B14(24, dataTimer);

                CYCLE_READ_PIN_B14(24, dataCAL1);

                CYCLE_READ_PIN_B14(24, dataCAL2);

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
                HAL_TIM::DelayUS(8);
            }
        }
        else if((ModeMeasureDuration::Current().Is_Dcycle() || ModeMeasureDuration::Current().Is_Phase()) && PageModesA::DCycleCheck())
        {
            if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) != 0)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

                CYCLE_READ_PIN_B14(32, MathFPGA::Measure::dataPeriod);

                CYCLE_READ_PIN_B14(32, MathFPGA::Measure::dataDuration);

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
                HAL_TIM::DelayUS(8);
            }
        }
        else if (CURRENT_CHANNEL_IS_A && (PageModesA::modeMeasureFrequency.IsComparator() && PageModesA::typeMeasure.IsFrequency())) 
        {
            static char comparatorFx[32] = { 0 };
            static char comparatorTizm[16] = { 0 };
            static char comparatorNkal[16] = { 0 };

            if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) != 0)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

                CYCLE_READ_PIN_B14(3, dataIdent);

                CYCLE_READ_PIN_B14(32, comparatorFx); //-V525

                CYCLE_READ_PIN_B14(16, comparatorTizm);

                CYCLE_READ_PIN_B14(16, comparatorNkal);

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

                HAL_TIM::DelayUS(8);

                uint decFx = MathFPGA::BinToUint32(comparatorFx);

                int decNkal = MathFPGA::BinToUint16(comparatorNkal);

                int decTizm = MathFPGA::BinToUint16(comparatorTizm);

                if (decNkal != 0)
                {
                    if (comparatorTizm[0] == 1)
                    {
                        decTizm -= 65536;
                    }

                    ValuePICO dx(decTizm);
                    dx.Div((uint)decNkal);
                    dx.Div(2 * 5000000);

                    ValuePICO k(5000000);
                    k.Sub(ValuePICO((int)decFx));
                    k.Div(5000000);
                    k.Sub(dx);
                    k.Mul(1000000);

                    k.SetSign(1);

                    MathFPGA::Measure::valueComparator = k;

                    PageStatistics::AppendValue(k.ToDouble());
                }
            }
        }
        else
        {
            if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) != 0)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

                CYCLE_READ_PIN_B14(32, MathFPGA::Measure::dataFrequencyA);

                if(CurrentModeMeasureFrequency::Is_RatioAC_or_RatioBC() && PageModesA::RelationCheck())
                {
                    CYCLE_READ_PIN_B14(32, MathFPGA::Measure::dataFrequencyB);
                }

                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
                HAL_TIM::DelayUS(8);
            }
        }
    }
}


void FPGA::WriteCommand(const char command[4], const char argument[6])
{
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9) == 0)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET); //-V525
        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);

        CYCLE_WRITE_COMMAND(4, command);

        CYCLE_WRITE_COMMAND(6, argument);

        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET); //-V525
        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
    }
}

void FPGA::IncreaseN()
{
    if(PageIndication::calibration.Is(Calibration::Pressed))
    {
        NAC++;
    }
    else
    {
        if(CURRENT_CHANNEL_IS_A)
        {
            MathFPGA::NA++;
        }
        else if(CURRENT_CHANNEL_IS_B)
        {
            MathFPGA::NB++;
        }
    }
}

void FPGA::DecreaseN()
{       
    if(PageIndication::calibration.Is(Calibration::Pressed))
    {
        NAC--;
    }
    else
    {
        if(CURRENT_CHANNEL_IS_A)
        {
            MathFPGA::NA--;
        }
        else if(CURRENT_CHANNEL_IS_B)
        {
            MathFPGA::NB--;
        }
    }
}


void FPGA::SwitchAuto()
{
    if(!autoMode)
    {
        autoMode = true;
    }
    else
    {
        autoMode = false;
    }
}


bool FPGA::AutoMode()
{
    return autoMode;
}


void FPGA::ReadCalibNumber()
{
    while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) == 0)
    {
    }

    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) != 0)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

        CYCLE_READ_PIN_B14(3, dataIdent);

        CYCLE_READ_PIN_B14(10, calibBin);

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
        HAL_TIM::DelayUS(8);
    }

    int len = 10;
    calibNumber = 0;
    int base = 1;

    for (int i = len - 1; i >= 0; i--)
    {
        if (calibBin[i] == 1)
        {
            calibNumber += base;
        }
        base = base * 2;
    }
}


void FPGA::WriteData()
{
    int negative = 1024;

    if(PageIndication::calibration.Is(Calibration::Pressed))
    {
        if(calibNumber + NAC < 0)
        {
            calibNumber = 0;
            NAC = 0;
        }
        calibNumber = calibNumber + NAC;
        MathFPGA::DecToBin(calibNumber, encData);
        NAC = 0;
    }
    else
    {
        if(CURRENT_CHANNEL_IS_A)
        {
                if(MathFPGA::NA < 0)
                {
                    MathFPGA::DecToBin(negative + MathFPGA::NA, encData);
                }
                else
                {
                    MathFPGA::DecToBin(MathFPGA::NA, encData);
                }
        }
        else if(CURRENT_CHANNEL_IS_B)
        {
            if(MathFPGA::NB < 0)
            {
                MathFPGA::DecToBin(negative + MathFPGA::NB, encData);
            }
            else
            {
                MathFPGA::DecToBin(MathFPGA::NB, encData);
            }
        }
    }

    if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9) == 0)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET); //-V525
        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);

        for(int i = 9; i > -1; i--)
        {
            WRITE_COMMAND(encData[i]);
        }

        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET); //-V525
        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_TIM::DelayUS(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
    }
}


int FPGA::CalibNumber()
{
    return calibNumber;
}
