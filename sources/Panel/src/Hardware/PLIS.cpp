#include "defines.h"
#include "stm32f4xx_hal.h"
#include "Hardware/HAL.h"
#include "Utils/String.h"
#include "Menu/Menu.h"
#include "Settings.h"
#include <cstring>
#include <string.h>
#include <math.h>
#include "stdio.h"
#include "Menu/Pages/PageChannelA.h"
#include "Menu/Pages/PageChannelB.h"
#include "Menu/Pages/PageModes.h"
#include "Menu/Pages/PageModesB.h"
#include "Menu/Pages/PageModesC.h"
#include "Menu/Pages/PageModesD.h"
#include "Menu/Pages/PageIndication.h"
#include "Menu/Hint.h"

#define    DWT_CYCCNT    *(volatile unsigned long *)0xE0001004
#define    DWT_CONTROL   *(volatile unsigned long *)0xE0001000
#define    SCB_DEMCR     *(volatile unsigned long *)0xE000EDFC
    
static char dataA [32]; 
static char dataB [32];
static char procData[40] = { 0 };
static char autoData[20];
static char minAutoData[7];
static char maxAutoData[7];

static float decDataA;
static float decDataB;
static float decDataC;
static int decDA;
static char procDataInterpol[30] = { 0 };

static char procDataDcycle[30] = { 0 };
//static int DACA = 0;
//static int DACB = 0;

static int NA = 0;
static int NB = 0;

static char encData[10];
static char ident[3];
static char timer1[27];
static int decTimer1;
static int decCAL1;
static int decCAL2;

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
    if((CURRENT_CHANNEL_IS_A && PageModes::modeMeasureFrequency == ModeMeasureFrequency::AC) || 
       (CURRENT_CHANNEL_IS_C && (PageModesC::modeMeasureFrequencyC == ModeMeasureFrequencyC::CA || PageModesC::modeMeasureFrequencyC == ModeMeasureFrequencyC::CB)) || 
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
    if(CURRENT_CHANNEL_IS_C && (PageModesC::typeMeasureC != TypeMeasureC::CountPulse))
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
    decPeriod = decPeriod/2;
    for (int i = len - 1; i >= 0; i--) 
    { 
        if (duration[i] == 1) 
        {
            decDuration += base2;
        }            
        base2 = base2 * 2; 
    }   
    
    decDuration = decDuration/2;
    dutyCycle = (float)decDuration/decPeriod;
}

static void Calculation()
{
    int x;
    if((CURRENT_CHANNEL_IS_A && (PageModes::typeMeasure == TypeMeasure::Frequency)) ||
       (CURRENT_CHANNEL_IS_B && (PageModesB::typeMeasureB == TypeMeasureB::Frequency)) ||
       (CURRENT_CHANNEL_IS_C && (PageModesC::typeMeasureC == TypeMeasureC::Frequency))
       || CURRENT_CHANNEL_IS_D)
    {
        if((CURRENT_CHANNEL_IS_A && (PageModes::modeMeasureFrequency == ModeMeasureFrequency::T_1)) || 
           (CURRENT_CHANNEL_IS_B && (PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::T_1)))
        {
            int msF = 1;
            int tmet = 1;
            int n = 1;
            manualZeros = 10;
//            decDataA = decDataA*10;
            if((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_3)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_3)))
            {
                tmet = tmet*1000;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_4)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_4)))
            {
                tmet = tmet*10000;
                manualZeros = manualZeros*10;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_5)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_5)))
            {
                tmet = tmet*100000;
                manualZeros = manualZeros*100;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_6)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_6)))
            {
                tmet = tmet*1000000;
                manualZeros = manualZeros*1000;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_7)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_7)))
            {
                tmet = tmet*10000000;
                manualZeros = manualZeros*10000;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_8)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_8)))
            {
                tmet = tmet*100000000;
                manualZeros = manualZeros*100000;
            }
            if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_1)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_1)))
            {
                n = n*1;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_10)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_10)))
            {
                n = n*10;
                manualZeros = manualZeros*10;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_100)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_100)))
            {
                n = n*100;
                manualZeros = manualZeros*100;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_1K)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_1K)))
            {
                n = n*1000;
                manualZeros = manualZeros*1000;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_10K)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_10K)))
            {
                n = n*10000;
                manualZeros = manualZeros*10000;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_100K)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_100K)))
            {
                n = n*100000;
                manualZeros = manualZeros*100000;
            }
            decDataA = (float)4/((decDataA/tmet)/n);
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
            x = msF;
            
        }
        else if((CURRENT_CHANNEL_IS_A && (PageModes::modeMeasureFrequency == ModeMeasureFrequency::AB)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::BA)))
        {
            int rel = 1;
            if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_1)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_1)))
            {
                rel = rel*1;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_10)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_10)))
            {
                rel = rel*10;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_100)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_100)))
            {
                rel = rel*100;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_1K)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_1K)))
            {
                rel = rel*1000;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_10K)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_10K)))
            {
                rel = rel*10000;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_100K)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_100K)))
            {
                rel = rel*100000;
            }
            x = rel;
        }
        else
        {
        int mhz = 1000;
        int khz = 1;
        if((CURRENT_CHANNEL_IS_A && (PageModes::timeMeasure == TimeMeasure::_1ms)) ||
           (CURRENT_CHANNEL_IS_B && (PageModesB::timeMeasureB == TimeMeasureB::_1ms)) ||
           (CURRENT_CHANNEL_IS_C && (PageModesC::timeMeasureC == TimeMeasureC::_1ms)) ||
           (CURRENT_CHANNEL_IS_D && (PageModesD::timeMeasureD == TimeMeasureD::_1ms)))
        {
            mhz = mhz*1;
            khz = khz*1;
        }
        else if((CURRENT_CHANNEL_IS_A && (PageModes::timeMeasure == TimeMeasure::_10ms)) ||
           (CURRENT_CHANNEL_IS_B && (PageModesB::timeMeasureB == TimeMeasureB::_10ms)) ||
           (CURRENT_CHANNEL_IS_C && (PageModesC::timeMeasureC == TimeMeasureC::_10ms)) ||
           (CURRENT_CHANNEL_IS_D && (PageModesD::timeMeasureD == TimeMeasureD::_10ms)))
        {
            mhz = mhz*10;
            khz = khz*10;
        }
        else if((CURRENT_CHANNEL_IS_A && (PageModes::timeMeasure == TimeMeasure::_100ms)) ||
           (CURRENT_CHANNEL_IS_B && (PageModesB::timeMeasureB == TimeMeasureB::_100ms)) ||
           (CURRENT_CHANNEL_IS_C && (PageModesC::timeMeasureC == TimeMeasureC::_100ms)) ||
           (CURRENT_CHANNEL_IS_D && (PageModesD::timeMeasureD == TimeMeasureD::_100ms)))
        {
            mhz = mhz*100;
            khz = khz*100;
        }
        else if((CURRENT_CHANNEL_IS_A && (PageModes::timeMeasure == TimeMeasure::_1s)) ||
           (CURRENT_CHANNEL_IS_B && (PageModesB::timeMeasureB == TimeMeasureB::_1s)) ||
           (CURRENT_CHANNEL_IS_C && (PageModesC::timeMeasureC == TimeMeasureC::_1s)) ||
           (CURRENT_CHANNEL_IS_D && (PageModesD::timeMeasureD == TimeMeasureD::_1s)))
        {
            mhz = mhz*1000;
            khz = khz*1000;
        }
        else if((CURRENT_CHANNEL_IS_A && (PageModes::timeMeasure == TimeMeasure::_10s)) ||
           (CURRENT_CHANNEL_IS_B && (PageModesB::timeMeasureB == TimeMeasureB::_10s)) ||
           (CURRENT_CHANNEL_IS_C && (PageModesC::timeMeasureC == TimeMeasureC::_10s)) ||
           (CURRENT_CHANNEL_IS_D && (PageModesD::timeMeasureD == TimeMeasureD::_10s)))
        {
            mhz = mhz*10000;
            khz = khz*10000;
        }
        else if((CURRENT_CHANNEL_IS_A && (PageModes::timeMeasure == TimeMeasure::_100s)) ||
           (CURRENT_CHANNEL_IS_B && (PageModesB::timeMeasureB == TimeMeasureB::_100s)) ||
           (CURRENT_CHANNEL_IS_C && (PageModesC::timeMeasureC == TimeMeasureC::_100s)) ||
           (CURRENT_CHANNEL_IS_D && (PageModesD::timeMeasureD == TimeMeasureD::_100s)))
        {
            mhz = mhz*100000;
            khz = khz*100000;
        }
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
            if(decDataA*64/1000 < 1000)
            {
                decDataC = (float)decDataA*128/1000;
                decDA = decDataC;
                x = 10;
//                manualZeros = 10;
            }
            else
            {
                decDataC = (float)decDataA*128/1000;
                decDA = decDataC;
                decDataC = decDataC/1000;
                x = 1000;
            }
        }
        }
    }
    else if((CURRENT_CHANNEL_IS_A && (PageModes::typeMeasure == TypeMeasure::Duration)) ||
       (CURRENT_CHANNEL_IS_B && (PageModesB::typeMeasureB == TypeMeasureB::Duration)))
    {
        int us = 1;
        if((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_6)) ||
           (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_6)))
        {
        } 
        else if((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_7)) ||
           (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_7)))
        {
            us = us*10;
        } 
        else if(((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_8)) ||
                (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_8))) || 
                ((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_5)) ||
                (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_5))))
        {
            us = us*100;
        } 
        x = us;
    }
    else if((CURRENT_CHANNEL_IS_A && (PageModes::typeMeasure == TypeMeasure::Period)) ||
       (CURRENT_CHANNEL_IS_B && (PageModesB::typeMeasureB == TypeMeasureB::Period)))
    {
        if((CURRENT_CHANNEL_IS_A && (PageModes::modeMeasurePeriod == ModeMeasurePeriod::F_1)) || 
            (CURRENT_CHANNEL_IS_B && (PageModesB::modeMeasurePeriodB == ModeMeasurePeriodB::F_1)))
        {
            int sT = 1;
            
            if((CURRENT_CHANNEL_IS_A && (PageModes::timeMeasure == TimeMeasure::_1ms)) ||
               (CURRENT_CHANNEL_IS_B && (PageModesB::timeMeasureB == TimeMeasureB::_1ms)))
            {
                sT = sT*1;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::timeMeasure == TimeMeasure::_10ms)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::timeMeasureB == TimeMeasureB::_10ms)))
            {
                sT = sT*10;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::timeMeasure == TimeMeasure::_100ms)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::timeMeasureB == TimeMeasureB::_100ms)))
            {
                sT = sT*100;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::timeMeasure == TimeMeasure::_1s)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::timeMeasureB == TimeMeasureB::_1s)))
            {
                sT = sT*1000;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::timeMeasure == TimeMeasure::_10s)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::timeMeasureB == TimeMeasureB::_10s)))
            {
                sT = sT*10000;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::timeMeasure == TimeMeasure::_100s)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::timeMeasureB == TimeMeasureB::_100s)))
            {
                sT = sT*100000;
            }
            decDA = decDataA/(2*sT);
            decDataA = 4/decDataA;
            x = sT*10;
        }
        else
        {
            int usT = 1;
            if(((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_6)) ||
                    (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_6))) || 
                    ((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_3)) ||
                    (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_3))))
            {
                usT = usT*1;
            }
            else if(((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_7)) ||
                    (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_7))) || 
                    ((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_4)) ||
                    (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_4))))
            {
                usT = usT*10;
            }
            else if(((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_8)) ||
                    (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_8))) || 
                    ((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_5)) ||
                    (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_5))))
            {
                usT = usT*100;
            }
            if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_1)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_1)))
            {
                usT = usT*1;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_10)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_10)))
            {
                usT = usT*10;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_100)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_100)))
            {
                usT = usT*100;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_1K)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_1K)))
            {
                usT = usT*1000;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_10K)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_10K)))
            {
                usT = usT*10000;
            }
            else if((CURRENT_CHANNEL_IS_A && (PageModes::numberPeriods == NumberPeriods::_100K)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::numberPeriodsB == NumberPeriodsB::_100K)))
            {
                usT = usT*100000;
            }
            x = usT;
        }
    }
    
    decDataA = (float)decDataA/(2*x);
    if(CURRENT_CHANNEL_IS_D)
    {
        decDataA = decDataC;
    }
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
        if(((PageModes::modeMeasureDuration == ModeMeasureDuration::Ndt_1ns && CURRENT_CHANNEL_IS_A) || 
                (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Ndt_1ns && CURRENT_CHANNEL_IS_B))
                && PageModes::InterpoleCheck())
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
        else if(((PageModes::modeMeasureDuration == ModeMeasureDuration::Dcycle && CURRENT_CHANNEL_IS_A) || 
            (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Dcycle && CURRENT_CHANNEL_IS_B))
            && PageModes::DCycleCheck())
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
                if((CURRENT_CHANNEL_IS_A && PageModes::modeMeasureFrequency == ModeMeasureFrequency::AC) || 
                (CURRENT_CHANNEL_IS_C && (PageModesC::modeMeasureFrequencyC == ModeMeasureFrequencyC::CA || PageModesC::modeMeasureFrequencyC == ModeMeasureFrequencyC::CB)) || 
                (CURRENT_CHANNEL_IS_B && PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::BC))
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
    if((CURRENT_CHANNEL_IS_A && (PageModes::typeMeasure == TypeMeasure::CountPulse)) ||
       (CURRENT_CHANNEL_IS_B && (PageModesB::typeMeasureB == TypeMeasureB::CountPulse)) ||
       (CURRENT_CHANNEL_IS_C && (PageModesC::typeMeasureC == TypeMeasureC::CountPulse)))
    {
        BinToDec();
        sprintf(procData,"%10.0f",decDataA);
        return procData;
    }
    else
    {
        if(((PageModes::modeMeasureDuration == ModeMeasureDuration::Ndt_1ns && CURRENT_CHANNEL_IS_A) || 
            (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Ndt_1ns && CURRENT_CHANNEL_IS_B))
            && PageModes::InterpoleCheck())
        {
            CalculationInterpole();
            sprintf(procDataInterpol,"%10.2f",interpol);
            std::strcat(procDataInterpol, " ns");
            return procDataInterpol;
        }
        else if(((PageModes::modeMeasureDuration == ModeMeasureDuration::Dcycle && CURRENT_CHANNEL_IS_A) || 
            (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Dcycle && CURRENT_CHANNEL_IS_B))
            && PageModes::DCycleCheck())
        {
            if(((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_8)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_8))) || 
            ((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_7)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_7))) || 
            ((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_6)) ||
            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_6))))
            {
                CalculationDcycle();
                sprintf(procDataDcycle,"%10.2f",dutyCycle);
                Int2String(dutyCycle, procDataDcycle);
//                std::strcat(procDataDcycle, " E-3");
            }
            return procDataDcycle;
        }
        else
        {
            BinToDec();
            Calculation();
            
            if(emptyZeros == 1)
            {
                sprintf(procData,"%10.0f",decDataA);
            }
            else if(emptyZeros == 10)
            {
                sprintf(procData,"%10.1f",decDataA);
            }
            else if(emptyZeros == 100)
            {
                sprintf(procData,"%10.2f",decDataA);
            }
            else if(emptyZeros == 1000)
            {
                sprintf(procData,"%10.3f",decDataA);
            }
            else if(emptyZeros == 10000)
            {
                sprintf(procData,"%10.4f",decDataA);
            }
            else if(emptyZeros == 100000)
            {
                sprintf(procData,"%10.5f",decDataA);
            }
            else if(emptyZeros == 1000000)
            {
                sprintf(procData,"%10.6f",decDataA);
            }
            else if(emptyZeros == 10000000)
            {
                sprintf(procData,"%10.7f",decDataA);
            }
            else if(emptyZeros == 100000000)
            {
                sprintf(procData,"%10.8f",decDataA);
            }
            else if(emptyZeros == 1000000000)
            {
                sprintf(procData,"%10.9f",decDataA);
            }
            else
            {
                sprintf(procData,"%10.10f",decDataA);
            }
            emptyZeros = 1;
            if(((PageModes::modeMeasureFrequency == ModeMeasureFrequency::AB ||
                PageModes::modeMeasureFrequency == ModeMeasureFrequency::AC) && CURRENT_CHANNEL_IS_A) ||
                ((PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::BA || 
                PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::BC) && CURRENT_CHANNEL_IS_B) ||
                ((PageModesC::modeMeasureFrequencyC == ModeMeasureFrequencyC::CA || 
                PageModesC::modeMeasureFrequencyC == ModeMeasureFrequencyC::CB) && CURRENT_CHANNEL_IS_C))
            {
            }
            else
            {
                if((CURRENT_CHANNEL_IS_A && (PageModes::typeMeasure == TypeMeasure::Frequency)) ||
                (CURRENT_CHANNEL_IS_B && (PageModesB::typeMeasureB == TypeMeasureB::Frequency)) ||
                (CURRENT_CHANNEL_IS_C && (PageModesC::typeMeasureC == TypeMeasureC::Frequency))
                || CURRENT_CHANNEL_IS_D)
                {
                    if((CURRENT_CHANNEL_IS_A && (PageModes::modeMeasureFrequency == ModeMeasureFrequency::T_1)) || 
                    (CURRENT_CHANNEL_IS_B && (PageModesB::modeMeasureFrequencyB == ModeMeasureFrequencyB::T_1)))
                    {
                        if(decDA < 1000)
                        {
                            std::strcat(procData, " Hz");
                        }
                        else if(decDA >= 1000 && decDA < 1000000)
                        {
                            std::strcat(procData, " kHz");
                        }
                        else if(decDA >= 1000000)
                        {
                            std::strcat(procData, " MHz");
                        }
                    }
                    else
                    {
                        if(CURRENT_CHANNEL_IS_C)
                        {
                            if(decDataC < 10000)
                            {
                                std::strcat(procData, " MHz");
                            }
                            else
                            {
                                std::strcat(procData, " GHz");
                            }
                        }
                        else if(CURRENT_CHANNEL_IS_D)
                        {
                            if(decDA > 1000)
                            {
                                std::strcat(procData, " GHz");
                            }
                            else
                            {
                                std::strcat(procData, " MHz");
                            }
                        }
                        else
                        {
                            if(decDA < 1000)
                            {
                                std::strcat(procData, " kHz");
                                
                            }
                            else 
                            {
                                std::strcat(procData, " MHz");
                            }
                        }
                    }
                }
                else
                {
                    if((CURRENT_CHANNEL_IS_A && ((PageModes::typeMeasure == TypeMeasure::Period)) && (PageModes::modeMeasurePeriod == ModeMeasurePeriod::F_1)) ||
                    (CURRENT_CHANNEL_IS_B && ((PageModesB::typeMeasureB == TypeMeasureB::Period)) && (PageModesB::modeMeasurePeriodB == ModeMeasurePeriodB::F_1)))
                    {
                        if(decDA >= 1000)
                        {
                            std::strcat(procData, " ns");
                        }
                        else if(decDA < 1000 && decDA >= 1)
                        {
                            std::strcat(procData, " us");
                        }
                        else if(decDA < 1)
                        {
                            std::strcat(procData, " ms");
                        }
                    }
                    else
                    {
                    if((PageModes::modeMeasureDuration == ModeMeasureDuration::Dcycle && CURRENT_CHANNEL_IS_A) || 
                    (PageModesB::modeMeasureDurationB == ModeMeasureDurationB::Dcycle && CURRENT_CHANNEL_IS_B))
                    {
                        std::strcat(procData, " E-3");
                    }
                    else
                    {
                        if(((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_5)) ||
                            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_5))) || 
                            ((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_4)) ||
                            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_4))) ||
                            ((CURRENT_CHANNEL_IS_A && (PageModes::periodTimeLabels == PeriodTimeLabels::T_3)) ||
                            (CURRENT_CHANNEL_IS_B && (PageModesB::periodTimeLabelsB == PeriodTimeLabelsB::T_3))))
                        {
                            std::strcat(procData, " ms");
                        }
                        else
                        {
                            std::strcat(procData, " us");
                        }
                    }
                }
            }
        }
            return procData;
        }   
    }    
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
    Int2String((decMinAuto - 512)*2, minAutoData);
    Int2String((decMaxAuto - 512)*2, maxAutoData);
    std::strcpy(autoData, "���� ");
    std::strcat(autoData, maxAutoData);
    std::strcat(autoData, " ��� ");
    std::strcat(autoData, minAutoData);
    if (CURRENT_CHANNEL_IS_A)
    {
        LEVEL_SYNCH_A = (decMidAuto - 512)*2;
    }

    if (CURRENT_CHANNEL_IS_B) 
    {
        LEVEL_SYNCH_B = (decMidAuto - 512)*2;
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
