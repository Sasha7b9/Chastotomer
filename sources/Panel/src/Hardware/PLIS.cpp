#include "defines.h"
#include "Settings.h"
#include "Hardware/HAL.h"
#include "Menu/Hint.h"
#include "Menu/Menu.h"
#include "Menu/Pages/PageChannelA.h"
#include "Menu/Pages/PageChannelB.h"
#include "Menu/Pages/PageModesA.h"
#include "Menu/Pages/PageModesB.h"
#include "Menu/Pages/PageModesC.h"
#include "Menu/Pages/PageModesD.h"
#include "Menu/Pages/PageIndication.h"
#include "Utils/StringUtils.h"
#include <cstring>
#include <cstdio>
#include <stm32f4xx_hal.h>


#define    DWT_CYCCNT    *(volatile unsigned long *)0xE0001004
#define    DWT_CONTROL   *(volatile unsigned long *)0xE0001000
#define    SCB_DEMCR     *(volatile unsigned long *)0xE000EDFC
    
static int dcycleZeros = 0;

static char dataA [32]; 
static char dataB [32];
static char procData[40] = { 0 };
static char identInfo[10] = { 0 };
static char autoData[20];
static char minAutoData[7];
static char maxAutoData[7];

static float decDataA;
static float decDataB;
static float decDataC;
static int decDA;
static char procDataInterpol[30] = { 0 };
static char spec[10] = { 0 };

static char procDataDcycle[30] = { 0 };
//static int DACA = 0;
//static int DACB = 0;

static int NA = 0;
static int NB = 0;

static char encData[10];
static char ident[4];
static char timer1[27];
static int decTimer1;
static int decCAL1;
static int decCAL2;

static char binFx[32];
static char binTizm[16];
static char binNkal[16];
static float decFx;
static float decTizm;
static float decNkal;

static char CAL1[24];
static char CAL2[24];

static char minAuto[10] = { 0 };
static char midAuto[10] = { 0 };
static char maxAuto[10] = { 0 };
static int decMinAuto;
static int decMidAuto;
static int decMaxAuto;

static int decPeriod;
static int decDuration;
static char period[32];
static char duration[32];

static int emptyZeros;
static int manualZeros = 1;

static float interpol;

static float dutyCycle;

static bool autoMode = false;

static char calibBin[10];
static int calibNumber = 0;
static int NAC = 0;

void DWT_Init(void)
{
        //��������� ������������ �������
        SCB_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
         //�������� �������� �������� ��������
	DWT_CYCCNT  = 0;
         //��������� �������
	DWT_CONTROL |= DWT_CTRL_CYCCNTENA_Msk; 
}

static __inline uint32_t delta(uint32_t t0, uint32_t t1)
{
    return (t1 - t0); 
}
void delay_us(uint32_t us)
{
      uint32_t t0 =  DWT->CYCCNT;
      uint32_t us_count_tic =  us * (SystemCoreClock/1000000);
      while (delta(t0, DWT->CYCCNT) < us_count_tic) ;
}

static void BinToDec() 
{ 
    int len = 32; 
    decDataA = 0; 
    int baseA = 1; 
    for (int i = len - 1; i >= 0; i--) 
    { 
        if (dataA[i] == 1) 
        {
            decDataA += baseA;
        }            
        baseA = baseA * 2; 
    }  
    if((CURRENT_CHANNEL_IS_A && PageModesA::modeMeasureFrequency == ModeMeasureFrequency::AC) || 
       (CURRENT_CHANNEL_IS_B && PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::BC))
    {
        decDataB = 0;
        int baseB = 1; 
        for (int i = len - 1; i >= 0; i--) 
        { 
            if (dataB[i] == 1) 
            {
                decDataB += baseB;
            }            
            baseB = baseB * 2; 
        }  
    }
    if(CURRENT_CHANNEL_IS_C)
    {
        decDataA = decDataA*64/100;
    }
}

static void CalculationDcycle() 
{     
    decPeriod = 0;
    decDuration = 0;
    int base1 = 1; 
    int base2 = 1; 
    int len = 32;  
    for (int i = len - 1; i >= 0; i--) 
    { 
        if (period[i] == 1) 
        {
            decPeriod += base1;
        }            
        base1 = base1 * 2; 
    }
    for (int i = len - 1; i >= 0; i--) 
    { 
        if (duration[i] == 1) 
        {
            decDuration += base2;
        }            
        base2 = base2 * 2; 
    }   
    dutyCycle = (float)decDuration/decPeriod;
    if((PageModesA::modeMeasureDuration == ModeMeasureDuration::Phase && CURRENT_CHANNEL_IS_A) || 
       (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Phase && CURRENT_CHANNEL_IS_B))
    {
        dutyCycle = dutyCycle*360;
    }
    else
    {
//        dutyCycle = dutyCycle*1000;
        if(dutyCycle < 0)
        {
            dutyCycle = dutyCycle*(-1);
        }
        if(dutyCycle != 0)
        {
            while(dutyCycle < 1)
            {
                dutyCycle = dutyCycle*10;
                dcycleZeros++;
            }
        }
    }
}

static void Calculation()
{
    int x = 0;
    if((CURRENT_CHANNEL_IS_A && (PageModesA::typeMeasure.IsFrequency())) ||
       (CURRENT_CHANNEL_IS_B && (PageModesB::typeMeasure == TypeMeasureB::Frequency)) ||
       (CURRENT_CHANNEL_IS_C && (PageModesC::typeMeasureC == TypeMeasureC::Frequency))
       || CURRENT_CHANNEL_IS_D)
    {
        if((CURRENT_CHANNEL_IS_A && (PageModesA::modeMeasureFrequency == ModeMeasureFrequency::T_1)) || 
           (CURRENT_CHANNEL_IS_B && (PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::T_1)))
        {
//            int msF = 1;
            int tmet = 1;
            int n = 1;
            manualZeros = 10;
            double test1;
            double test2;
            double test3;
//            decDataA = decDataA*10;
            if(PageModesA::periodTimeLabels == PeriodTimeLabels::T_3)
            {
                tmet = tmet*1000;
            }
            else if(PageModesA::periodTimeLabels == PeriodTimeLabels::T_4)
            {
                tmet = tmet*10000;
                manualZeros = manualZeros*10;
            }
            else if(PageModesA::periodTimeLabels == PeriodTimeLabels::T_5)
            {
                tmet = tmet*100000;
                manualZeros = manualZeros*100;
            }
            else if(PageModesA::periodTimeLabels == PeriodTimeLabels::T_6)
            {
                tmet = tmet*1000000;
                manualZeros = manualZeros*1000;
            }
            else if(PageModesA::periodTimeLabels == PeriodTimeLabels::T_7)
            {
                tmet = tmet*10000000;
                manualZeros = manualZeros*10000;
            }
            else if(PageModesA::periodTimeLabels == PeriodTimeLabels::T_8)
            {
                tmet = tmet*100000000;
                manualZeros = manualZeros*100000;
            }
            
            n *= PageModesA::numberPeriods.ToAbs();
            manualZeros *= PageModesA::numberPeriods.ToAbs();
           
            test1 = (double)decDataA/tmet;
            test2 = test1/n;
            test3 = 4/test2;
            //decDataA = (float)4/((decDataA/tmet)/n);
            decDataA = (float)test3;
            decDA = decDataA/2;
            if(decDA < 1000)
            {
            }
            else if(decDA >= 1000 && decDA < 1000000)
            {
                decDataA = decDataA/1000;
            }
            else if(decDA >= 1000000)
            {
                decDataA = decDataA/1000000;
            }
            x = 1;
            
        }
        else if((CURRENT_CHANNEL_IS_A && (PageModesA::modeMeasureFrequency == ModeMeasureFrequency::AB)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::BA)))
        {           
            x = PageModesA::numberPeriods.ToAbs();
        }
        else if(CURRENT_CHANNEL_IS_C && ((PageModesC::modeMeasureFrequencyC == ModeMeasureFrequencyC::CA) ||
               (PageModesC::modeMeasureFrequencyC == ModeMeasureFrequencyC::CB)))
        {
            decDataA = decDataA*100;
            int rel = 1;
            if(CURRENT_CHANNEL_IS_C && (PageModesC::numberPeriods == NumberPeriods::_1))
            {
                rel = rel*1;
            }
            else if(CURRENT_CHANNEL_IS_C && (PageModesC::numberPeriods == NumberPeriods::_10))
            {
                rel = rel*10;
            }
            else if(CURRENT_CHANNEL_IS_C && (PageModesC::numberPeriods == NumberPeriods::_100))
            {
                rel = rel*100;
            }
            else if(CURRENT_CHANNEL_IS_C && (PageModesC::numberPeriods == NumberPeriods::_1K))
            {
                rel = rel*1000;
            }
            else if(CURRENT_CHANNEL_IS_C && (PageModesC::numberPeriods == NumberPeriods::_10K))
            {
                rel = rel*10000;
            }
            else if(CURRENT_CHANNEL_IS_C && (PageModesC::numberPeriods == NumberPeriods::_100K))
            {
                rel = rel*100000;
            }
            else if(CURRENT_CHANNEL_IS_C && (PageModesC::numberPeriods == NumberPeriods::_1000K))
            {
                rel = rel*1000000;
            }
            x = rel;
        }
        else if((CURRENT_CHANNEL_IS_A && (PageModesA::modeMeasureFrequency == ModeMeasureFrequency::AC)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::BC)))
        {
            int sT = PageModesA::timeMeasure.ToMS();

            decDataA = decDataA/decDataB;
            decDataA = decDataA/32;
            manualZeros = 1000000;
            manualZeros = manualZeros*sT;
            x = 1;
//            decDA = decDataB;
//            int i = 1;
//            while(decDA > 0)
//            {
//                decDA = decDA/10;
//            }
        }
        else
        {
            int mhz = 1000 * PageModesA::timeMeasure.ToMS();
            int khz = PageModesA::timeMeasure.ToMS();
            
            if(((decDataA/khz)/2) < 1000)
            {
                x = khz; 
            }
            else
            {
                x = mhz;            
            }
            decDA = (decDataA/khz)/2;
            if(CURRENT_CHANNEL_IS_C)
            {
                if(decDataA < 10000)
                {
                    decDataC = decDataA;               
                    khz = khz*10;
                    x = khz;
                }
                else
                {
                    decDataC = decDataA;
                    mhz = mhz*10;
                    x = mhz;
                }
            }
            if(CURRENT_CHANNEL_IS_D)
            {
                if(decDataA*64/(1000*khz) > 19000)
                {
                    decDataC = 0;
                    x = khz;
                }
                else
                {
                    decDataC = (float)decDataA*64/1000;
                    x = mhz;
                }
                decDA = decDataC;
            }
        }
    }
    else if((CURRENT_CHANNEL_IS_A && (PageModesA::typeMeasure.IsDuration())) ||
       (CURRENT_CHANNEL_IS_B && (PageModesB::typeMeasure == TypeMeasureB::Duration)))
    {
        int us = 1;
        if(PageModesA::periodTimeLabels == PeriodTimeLabels::T_6)
        {
        } 
        else if(PageModesA::periodTimeLabels == PeriodTimeLabels::T_7)
        {
            us = us*10;
        } 
        else if((PageModesA::periodTimeLabels == PeriodTimeLabels::T_8) || (PageModesA::periodTimeLabels == PeriodTimeLabels::T_5))
        {
            us = us*100;
        } 
        x = us;
    }
    else if((CURRENT_CHANNEL_IS_A && (PageModesA::typeMeasure.IsPeriod())) ||
       (CURRENT_CHANNEL_IS_B && (PageModesB::typeMeasure == TypeMeasureB::Period)))
    {
        if((CURRENT_CHANNEL_IS_A && (PageModesA::modeMeasurePeriod == ModeMeasurePeriod::F_1)) || 
            (CURRENT_CHANNEL_IS_B && (PageModesB::modeMeasurePeriodB == ModeMeasurePeriodB::F_1)))
        {
            int sT = 1;
            
            if(PageModesA::timeMeasure == TimeMeasure::_1ms)
            {
                sT = sT*1;
            }
            else if(PageModesA::timeMeasure == TimeMeasure::_10ms)
            {
                sT = sT*10;
            }
            else if(PageModesA::timeMeasure == TimeMeasure::_100ms)
            {
                sT = sT*100;
            }
            else if(PageModesA::timeMeasure == TimeMeasure::_1s)
            {
                sT = sT*1000;
            }
            else if(PageModesA::timeMeasure == TimeMeasure::_10s)
            {
                sT = sT*10000;
            }
            else if(PageModesA::timeMeasure == TimeMeasure::_100s)
            {
                sT = sT*100000;
            }
            else if(PageModesA::timeMeasure == TimeMeasure::_1000s)
            {
                sT = sT*1000000;
            }
            decDA = decDataA/(2*sT);
            decDataA = 4/decDataA;
            if(decDA >= 1000)
            {
                decDataA = decDataA*10000000*sT*sT;
            }  
            else if(decDA <= 1)
            {
                decDataA = decDataA*10*sT*sT;
            }
            else
            {
                decDataA = decDataA*10000*sT*sT;
            }
            x = sT*10;
        }
        else
        {
            int usT = 1;
            if((PageModesA::periodTimeLabels == PeriodTimeLabels::T_6) ||
               (PageModesA::periodTimeLabels == PeriodTimeLabels::T_3))
            {
                usT = usT*1;
            }
            else if((PageModesA::periodTimeLabels == PeriodTimeLabels::T_7) ||
                    (PageModesA::periodTimeLabels == PeriodTimeLabels::T_4))
            {
                usT = usT*10;
            }
            else if((PageModesA::periodTimeLabels == PeriodTimeLabels::T_8) ||
                    (PageModesA::periodTimeLabels == PeriodTimeLabels::T_5))
            {
                usT = usT*100;
            }
            
            x = usT * PageModesA::numberPeriods.ToAbs();
        }
    }
    if(CURRENT_CHANNEL_IS_D)
    {
        decDataA = decDataC*2;
    }
    decDataA = (float)decDataA/(2*x);
    emptyZeros = x;
    if(manualZeros != 1)
    {
        emptyZeros = manualZeros;
        manualZeros = 1;
    }
}

static void CalculationInterpole() 
{     
    decTimer1 = 0;
    decCAL1 = 0;
    decCAL2 = 0; 
    int base1 = 1; 
    int base2 = 1; 
    int base3 = 1; 
    int len = 24;   
    for (int i = len - 1; i >= 0; i--) 
    { 
        if (timer1[i] == 1) 
        {
            decTimer1 += base1;
        }            
        base1 = base1 * 2; 
    }
    for (int i = len - 1; i >= 0; i--) 
    { 
        if (CAL1[i] == 1) 
        {
            decCAL1 += base2;
        }            
        base2 = base2 * 2; 
    } 
    for (int i = len - 1; i >= 0; i--) 
    { 
        if (CAL2[i] == 1) 
        {
            decCAL2 += base3;
        }            
        base3 = base3 * 2; 
    }     
    interpol = (float)(100*decTimer1)/(decCAL2 - decCAL1);
}

static void CalculationComparator() 
{     
    decFx = 0;
    decTizm = 0;
    decNkal = 0; 
    int base1 = 1; 
    int base2 = 1; 
    int base3 = 1; 
    int len1 = 32;
    int len2 = 16; 
    for (int i = len1 - 1; i >= 0; i--) 
    { 
        if (binFx[i] == 1) 
        {
            decFx += base1;
        }            
        base1 = base1 * 2; 
    }
    for (int i = len2 - 1; i >= 0; i--) 
    { 
        if (binTizm[i] == 1) 
        {
            decTizm += base2;
        }            
        base2 = base2 * 2; 
    } 
    if (binTizm[0] == 1) 
    {
        decTizm = decTizm - 65536;
    }
    for (int i = len2 - 1; i >= 0; i--) 
    { 
        if (binNkal[i] == 1) 
        {
            decNkal += base3;
        }            
        base3 = base3 * 2; 
    }   
//    decFx = decFx/2;
//    decTizm = decTizm/2;
//    decNkal = decNkal/2;
}

static void CalculationAuto() 
{     
    decMinAuto = 0;
    decMidAuto = 0;
    decMaxAuto = 0; 
    int base1 = 1; 
    int base2 = 1; 
    int base3 = 1; 
    int len = 10;   
    for (int i = len - 1; i >= 0; i--) 
    { 
        if (minAuto[i] == 1) 
        {
            decMinAuto += base1;
        }            
        base1 = base1 * 2; 
    }
    for (int i = len - 1; i >= 0; i--) 
    { 
        if (midAuto[i] == 1) 
        {
            decMidAuto += base2;
        }            
        base2 = base2 * 2; 
    } 
    for (int i = len - 1; i >= 0; i--) 
    { 
        if (maxAuto[i] == 1) 
        {
            decMaxAuto += base3;
        }            
        base3 = base3 * 2; 
    }     
}

void DecToBin(int dec, char* bin) 
{ 
    int x = dec;
    for(int i = 0; i < 10; i++)
    {
        if(x%2 != 0)
        {
            bin[i] = 1;
        }
        else
        {
            bin[i] = 0;
        }
        x = x/2;
    }   
}


void PLIS::Init()
{
    DWT_Init();
    
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

void PLIS::Update()
{
    if(autoMode == true)
    {
        if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) != 0)
            {            
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
                for(int i = 0; i < 3; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    ident[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                for(int i = 0; i < 10; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    minAuto[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                for(int i = 0; i < 10; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    midAuto[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                for(int i = 0; i < 10; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    maxAuto[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
                delay_us(8);
        }
    }
    else
    {
        if(((PageModesA::modeMeasureDuration == ModeMeasureDuration::Ndt_1ns && CURRENT_CHANNEL_IS_A) || 
                (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Ndt_1ns && CURRENT_CHANNEL_IS_B))
                && PageModesA::InterpoleCheck())
        {
            if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) != 0)
            {            
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
                for(int i = 0; i < 3; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    ident[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                for(int i = 0; i < 24; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    timer1[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                for(int i = 0; i < 24; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    CAL1[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                for(int i = 0; i < 24; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    CAL2[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
                delay_us(8);
                
            }  
        }
        else if(((PageModesA::modeMeasureDuration == ModeMeasureDuration::Dcycle && CURRENT_CHANNEL_IS_A) || 
                (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Dcycle && CURRENT_CHANNEL_IS_B) || 
                (PageModesA::modeMeasureDuration == ModeMeasureDuration::Phase && CURRENT_CHANNEL_IS_A) || 
                (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Phase && CURRENT_CHANNEL_IS_B))
                && PageModesA::DCycleCheck())
        {
            if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) != 0)
            {            
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
                for(int i = 0; i < 32; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    period[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                for(int i = 0; i < 32; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    duration[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
                delay_us(8);
            }
        }
        else if (CURRENT_CHANNEL_IS_A && ((PageModesA::modeMeasureFrequency == ModeMeasureFrequency::Comparator) && PageModesA::typeMeasure.IsFrequency())) 
        {
            if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) != 0)
            {            
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
                for(int i = 0; i < 3; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    ident[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                for(int i = 0; i < 32; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    binFx[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                for(int i = 0; i < 16; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    binTizm[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                for(int i = 0; i < 16; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    binNkal[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
                delay_us(8);
            }
        }
        else
        {
            if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) != 0)
            {            
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
                for(int i = 0; i < 32; i++)
                {
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                    delay_us(2);
                    dataA[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                    delay_us(2);
                }
                if(((CURRENT_CHANNEL_IS_A && PageModesA::modeMeasureFrequency == ModeMeasureFrequency::AC) || 
                (CURRENT_CHANNEL_IS_B && PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::BC))
                && PageModesA::RelationCheck())
                {
                    for(int i = 0; i < 32; i++)
                    {
                        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                        delay_us(2);
                        dataB[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
                        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
                        delay_us(2);
                    }
                }
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
                delay_us(8);
            }     
        }
    }
}


char* PLIS::GiveData()
{
    if((CURRENT_CHANNEL_IS_A && PageModesA::typeMeasure.IsCountPulse()) ||
       (CURRENT_CHANNEL_IS_B && (PageModesB::typeMeasure == TypeMeasureB::CountPulse)) ||
       (CURRENT_CHANNEL_IS_C && (PageModesC::typeMeasureC == TypeMeasureC::CountPulse)))
    {
        BinToDec();
        decDataA = decDataA/2;
        if(CURRENT_CHANNEL_IS_C)
        {
            decDataA = decDataA*100;
        }
        if((CURRENT_CHANNEL_IS_A && (PageModesA::modeMeasureCountPulse == ModeMeasureCountPulse::ATB)) 
          || (CURRENT_CHANNEL_IS_B && (PageModesB::modeMeasureCountPulseB == ModeMeasureCountPulseB::BTA_1))
          || (CURRENT_CHANNEL_IS_C && (PageModesC::modeMeasureCountPulseC == ModeMeasureCountPulseC::CTB_1))
          || (CURRENT_CHANNEL_IS_C && (PageModesC::modeMeasureCountPulseC == ModeMeasureCountPulseC::CTA_1)))
        {
            decDataA /= PageModesA::numberPeriods.ToAbs();
        }
        std::sprintf(procData,"%10.0f",decDataA);
        return procData;
    }
    else
    {
        if(((PageModesA::modeMeasureFrequency == ModeMeasureFrequency::Tachometer && CURRENT_CHANNEL_IS_A) || 
            (PageModesB::modeMeasureFrequencyB == ModeMeasureFrequency::Tachometer && CURRENT_CHANNEL_IS_B)))
        {
            BinToDec();
            decDataA = decDataA/2;
            std::sprintf(procData,"%10.0f",decDataA);
            return procData;
        }
        else if (CURRENT_CHANNEL_IS_A && ((PageModesA::modeMeasureFrequency == ModeMeasureFrequency::Comparator) && PageModesA::typeMeasure.IsFrequency())) 
        {
            CalculationComparator();
            int top = 200;
            int n = 5000000;
            float dx = ((decTizm*100)/decNkal);
//            decFx = decFx - 1;
            float k = (n - decFx)/n;
            decDataA = k - (dx/top)/n;
            decDataA = decDataA*1000000;
//            decDataA = (float)((decFx*top)/2)/(n*top + ((decTizm*100)/decNkal));
            std::sprintf(procData,"%10.3f",decDataA);                
            return procData;
        }
        else if(((PageModesA::modeMeasureDuration == ModeMeasureDuration::Ndt_1ns && CURRENT_CHANNEL_IS_A) || 
            (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Ndt_1ns && CURRENT_CHANNEL_IS_B))
            && PageModesA::InterpoleCheck())
        {
            CalculationInterpole();
            std::sprintf(procDataInterpol,"%10.2f",interpol);
            return procDataInterpol;
        }
        else if(((PageModesA::modeMeasureDuration == ModeMeasureDuration::Dcycle && CURRENT_CHANNEL_IS_A) || 
            (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Dcycle && CURRENT_CHANNEL_IS_B) || 
            (PageModesA::modeMeasureDuration == ModeMeasureDuration::Phase && CURRENT_CHANNEL_IS_A) || 
            (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Phase && CURRENT_CHANNEL_IS_B))
            && PageModesA::DCycleCheck())
        {
//            if(((CURRENT_CHANNEL_IS_A && (PageModesA::periodTimeLabels == PeriodTimeLabels::T_8)) ||
//            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_8))) || 
//            ((CURRENT_CHANNEL_IS_A && (PageModesA::periodTimeLabels == PeriodTimeLabels::T_7)) ||
//            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_7))) || 
//            ((CURRENT_CHANNEL_IS_A && (PageModesA::periodTimeLabels == PeriodTimeLabels::T_6)) ||
//            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_6))))
//            {
                CalculationDcycle();
//                Int2String(dutyCycle, procDataDcycle);
                if((PageModesA::modeMeasureDuration == ModeMeasureDuration::Phase && CURRENT_CHANNEL_IS_A) || 
                   (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Phase && CURRENT_CHANNEL_IS_B))
                {
                    std::sprintf(procDataDcycle,"%10.3f",dutyCycle);
                }
                else
                {   
                    //if(PageModesA::periodTimeLabels == PeriodTimeLabels::T_8)
                    //{
                    //    sprintf(procDataDcycle,"%10.2f",dutyCycle);
                    //}
                    //else if(PageModesA::periodTimeLabels == PeriodTimeLabels::T_7)
                    //{
                    //    sprintf(procDataDcycle,"%10.1f",dutyCycle);
                    //}
                    //else if(PageModesA::periodTimeLabels == PeriodTimeLabels::T_6)
                    //{
                    //    sprintf(procDataDcycle,"%10.0f",dutyCycle);
                    //}
                    //else 
                    //{
                    std::sprintf(procDataDcycle,"%10.7f",dutyCycle);
                    //}
                }
//            }
                return procDataDcycle;
        }
        else
        {
            BinToDec();
            Calculation();
            
            if(emptyZeros == 1)
            {
                std::sprintf(procData,"%10.0f",decDataA);
            }
            else if(emptyZeros == 10)
            {
                std::sprintf(procData,"%10.1f",decDataA);
            }
            else if(emptyZeros == 100)
            {
                std::sprintf(procData,"%10.2f",decDataA);
            }
            else if(emptyZeros == 1000)
            {
                std::sprintf(procData,"%10.3f",decDataA);
            }
            else if(emptyZeros == 10000)
            {
                std::sprintf(procData,"%10.4f",decDataA);
            }
            else if(emptyZeros == 100000)
            {
                std::sprintf(procData,"%10.5f",decDataA);
            }
            else if(emptyZeros == 1000000)
            {
                std::sprintf(procData,"%10.6f",decDataA);
            }
            else if(emptyZeros == 10000000)
            {
                std::sprintf(procData,"%10.7f",decDataA);
            }
            else if(emptyZeros == 100000000)
            {
                std::sprintf(procData,"%10.8f",decDataA);
            }
            else if(emptyZeros == 1000000000)
            {
                std::sprintf(procData,"%10.9f",decDataA);
            }
            else
            {
                std::sprintf(procData,"%10.10f",decDataA);
            }
            emptyZeros = 1;
            }
            return procData;
        }       
}

char* PLIS::GiveSpec()
{
        if(((PageModesA::modeMeasureDuration == ModeMeasureDuration::Ndt_1ns && CURRENT_CHANNEL_IS_A) || 
            (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Ndt_1ns && CURRENT_CHANNEL_IS_B))
            && PageModesA::InterpoleCheck())
        {
            std::strcpy(spec, " ns");
        }
        else if((PageModesA::typeMeasure.IsDuration() && (PageModesA::modeMeasureDuration == ModeMeasureDuration::Dcycle) && CURRENT_CHANNEL_IS_A) || 
            ((PageModesB::typeMeasure == TypeMeasureB::Duration) && (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Dcycle) && CURRENT_CHANNEL_IS_B) || 
            (PageModesA::typeMeasure.IsDuration() && (PageModesA::modeMeasureDuration == ModeMeasureDuration::Phase) && CURRENT_CHANNEL_IS_A) || 
            ((PageModesB::typeMeasure == TypeMeasureA::Duration) && (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Phase) && CURRENT_CHANNEL_IS_B))
        {
                if((PageModesA::modeMeasureDuration == ModeMeasureDuration::Phase && CURRENT_CHANNEL_IS_A) || 
                   (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Phase && CURRENT_CHANNEL_IS_B))
                {
                    std::strcpy(spec, " $");
//                    std::strcpy(spec, " grad.");
                }
                else
                {   
                    if(dcycleZeros == 0)
                    {
                        std::strcpy(spec, " E-0");
                    }
                    else if(dcycleZeros == 1)
                    {
                        std::strcpy(spec, " E-1");
                    }
                    else if(dcycleZeros == 2)
                    {
                        std::strcpy(spec, " E-2");
                    }
                    else if(dcycleZeros == 3)
                    {
                        std::strcpy(spec, " E-3");
                    }
                    else if(dcycleZeros == 4)
                    {
                        std::strcpy(spec, " E-4");
                    }
                    else if(dcycleZeros == 5)
                    {
                        std::strcpy(spec, " E-5");
                    }
                    else if(dcycleZeros == 6)
                    {
                        std::strcpy(spec, " E-6");
                    }
                    else if(dcycleZeros == 7)
                    {
                        std::strcpy(spec, " E-7");
                    }
                    else if(dcycleZeros == 8)
                    {
                        std::strcpy(spec, " E-8");
                    }
                    else
                    {
                        std::strcpy(spec, " E-9");
                    }
                    dcycleZeros = 0;
                }
        }
        else
        {
            if(((PageModesA::modeMeasureFrequency == ModeMeasureFrequency::AB ||
                PageModesA::modeMeasureFrequency == ModeMeasureFrequency::AC) && CURRENT_CHANNEL_IS_A) ||
                ((PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::BA || 
                PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::BC) && CURRENT_CHANNEL_IS_B) ||
                ((PageModesC::modeMeasureFrequencyC == ModeMeasureFrequencyC::CA || 
                PageModesC::modeMeasureFrequencyC == ModeMeasureFrequencyC::CB) && CURRENT_CHANNEL_IS_C)
                || ((PageModesA::modeMeasureFrequency == ModeMeasureFrequency::Tachometer && CURRENT_CHANNEL_IS_A) || 
                (PageModesB::modeMeasureFrequencyB == ModeMeasureFrequency::Tachometer && CURRENT_CHANNEL_IS_B))
                || (PageModesA::typeMeasure.IsCountPulse() || PageModesB::typeMeasure == TypeMeasureB::CountPulse || PageModesC::typeMeasureC == TypeMeasureC::CountPulse))
            {
                std::strcpy(spec, " ");
            }
            else
            {
                if((CURRENT_CHANNEL_IS_A && PageModesA::typeMeasure.IsFrequency()) ||
                (CURRENT_CHANNEL_IS_B && (PageModesB::typeMeasure == TypeMeasureB::Frequency)) ||
                (CURRENT_CHANNEL_IS_C && (PageModesC::typeMeasureC == TypeMeasureC::Frequency))
                || CURRENT_CHANNEL_IS_D)
                {
                    if((CURRENT_CHANNEL_IS_A && (PageModesA::modeMeasureFrequency == ModeMeasureFrequency::T_1)) || 
                    (CURRENT_CHANNEL_IS_B && (PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::T_1)))
                    {
                        if(decDA < 1000)
                        {
                            std::strcpy(spec, " Hz");
                        }
                        else if(decDA >= 1000 && decDA < 1000000)
                        {
                            std::strcpy(spec, " kHz");
                        }
                        else if(decDA >= 1000000)
                        {
                            std::strcpy(spec, " MHz");
                        }
                    }
                    else if((PageModesA::modeMeasureFrequency == ModeMeasureFrequency::Comparator) && CURRENT_CHANNEL_IS_A) 
                    {
                        std::strcpy(spec, " E-6");
                    }
                    else
                    {
                        if(CURRENT_CHANNEL_IS_C)
                        {
                            if(decDataC/2 < 10000)
                            {
                                std::strcpy(spec, " MHz");
                            }
                            else
                            {
                                std::strcpy(spec, " GHz");
                            }
                        }
                        else if(CURRENT_CHANNEL_IS_D)
                        {
                            {
                                std::strcpy(spec, " GHz");
                            }
                        }
                        else
                        {
                            if(decDA < 1000)
                            {
                                std::strcpy(spec, " kHz");
                                
                            }
                            else 
                            {
                                std::strcpy(spec, " MHz");
                            }
                        }
                    }
                }
                else
                {
                    if((CURRENT_CHANNEL_IS_A && PageModesA::typeMeasure.IsPeriod() && (PageModesA::modeMeasurePeriod == ModeMeasurePeriod::F_1)) ||
                    (CURRENT_CHANNEL_IS_B && ((PageModesB::typeMeasure == TypeMeasureB::Period)) && (PageModesB::modeMeasurePeriodB == ModeMeasurePeriodB::F_1)))
                    {
                        if(decDA >= 1000)
                        {
                            std::strcpy(spec, " ns");
                        }
                        else if(decDA <= 1)
                        {
                            std::strcpy(spec, " ms");
                        }
                        else
                        {
                            std::strcpy(spec, " us");
                        }
                    }
                    else
                    {
                        if(((CURRENT_CHANNEL_IS_A && (PageModesA::periodTimeLabels == PeriodTimeLabels::T_5)) ||
                            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_5))) || 
                            ((CURRENT_CHANNEL_IS_A && (PageModesA::periodTimeLabels == PeriodTimeLabels::T_4)) ||
                            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_4))) ||
                            ((CURRENT_CHANNEL_IS_A && (PageModesA::periodTimeLabels == PeriodTimeLabels::T_3)) ||
                            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_3))))
                        {
                            std::strcpy(spec, " ms");
                        }
                        else
                        {
                            std::strcpy(spec, " us");
                        }
                    } } } }   
            return spec;
}

char* PLIS::GiveIdent()
{
    if(ident[0] == 0)
    {
        std::strcpy(identInfo, "0");
    }
    else
    {
        std::strcpy(identInfo, "1");
    }
    for(int i = 1; i < 4; i++)
    {
        if(ident[i] == 0)
        {
            std::strcat(identInfo, "0");
        }
        else
        {
            std::strcat(identInfo, "1");
        }
    }
    return identInfo;
}


void PLIS::WriteCommand(char* command, char* argument)
{
    if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9) == 0)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
//        delay_us(2);
        for(int i = 0; i < 4; i++)
        {
            if (command[i] == 1)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
                delay_us(2);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                delay_us(2);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
            }
            else
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
                delay_us(2);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                delay_us(2);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
            }
        }
        for(int i = 0; i < 6; i++)
        {
            if (argument[i] == 1)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
                delay_us(2);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                delay_us(2);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
            }
            else
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
                delay_us(2);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                delay_us(2);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
            }
        }
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
    }
}

void PLIS::IncreaseN()
{
    if(PageIndication::calibration.Is(Calibration::Pressed))
    {
        NAC++;
    }
    else
    {
        if(CURRENT_CHANNEL_IS_A)
        {
            NA++;
        }
        else if(CURRENT_CHANNEL_IS_B)
        {
            NB++;
        }
    }
}

void PLIS::DecreaseN()
{       
    if(PageIndication::calibration.Is(Calibration::Pressed))
    {
        NAC--;
    }
    else
    {
        if(CURRENT_CHANNEL_IS_A)
        {
            NA--;
        }
        else if(CURRENT_CHANNEL_IS_B)
        {
            NB--;
        }
    }
}

void PLIS::SwitchAuto()
{
    if(autoMode == false)
    {
        autoMode = true;
    }
    else
    {
        autoMode = false;
    }
}

bool PLIS::AutoMode()
{
    return autoMode;
}

int PLIS::MidAuto()
{
    CalculationAuto();
    return decMidAuto;
}

int PLIS::MinAuto()
{
    CalculationAuto();
    return decMinAuto;
}

int PLIS::MaxAuto()
{
    CalculationAuto();
    return decMaxAuto;
}

void PLIS::RefreshAuto()
{
    for(int i = 0; i < 10; i++)
    {
        minAuto[i] = 0;
        midAuto[i] = 0;
        maxAuto[i] = 0;
    }
}

char* PLIS::GiveAuto()
{
    CalculationAuto();
    SU::Int2String((decMinAuto - 512)*2, minAutoData);
    SU::Int2String((decMaxAuto - 512)*2, maxAutoData);
    std::strcpy(autoData, "���� ");
    std::strcat(autoData, maxAutoData);
    std::strcat(autoData, " ��� ");
    std::strcat(autoData, minAutoData);
    if (CURRENT_CHANNEL_IS_A)
    {
        LEVEL_SYNCH_A = (decMidAuto - 512)*2;
        NA = decMidAuto - 512;
    }

    if (CURRENT_CHANNEL_IS_B) 
    {
        LEVEL_SYNCH_B = (decMidAuto - 512)*2;
        NB = decMidAuto - 512;
        
    }
    return autoData;
}

void PLIS::ReadCalibNumber()
{
    while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) == 0)
    {
    }
    if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) != 0)
    {            
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
        for(int i = 0; i < 3; i++)
        {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
            delay_us(2);
            ident[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
            delay_us(2);
        }
        for(int i = 0; i < 10; i++)
        {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
            delay_us(2);
            calibBin[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
            delay_us(2);
        }
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
        delay_us(8);
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


void PLIS::WriteData()
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
        DecToBin(calibNumber, encData);
        NAC = 0;
    }
    else
    {
        if(CURRENT_CHANNEL_IS_A)
        {
                if(NA < 0)
                {
                    DecToBin(negative + NA, encData);
                }
                else
                {
                    DecToBin(NA, encData);
                }
        }
        else if(CURRENT_CHANNEL_IS_B)
        {
            if(NB < 0)
            {
                DecToBin(negative + NB, encData);
            }
            else
            {
                DecToBin(NB, encData);
            }
        }
    }
    if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9) == 0)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
//        delay_us(2);
        for(int i = 9; i > -1; i--)
        {
            if (encData[i] == 1)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
                delay_us(2);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                delay_us(2);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
            }
            else
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
                delay_us(2);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
                delay_us(2);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
            }
        }
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
        delay_us(2);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
    }
}

int PLIS::CalibNumber()
{
    return calibNumber;
}
