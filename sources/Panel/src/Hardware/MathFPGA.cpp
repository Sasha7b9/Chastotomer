#include "defines.h"
#include "Settings.h"
#include "Hardware/FPGA.h"
#include "Hardware/MathFPGA.h"
#include "Menu/Pages/PageStatistics.h"
#include "Menu/Pages/Modes/PagesModes.h"
#include "Utils/StringUtils.h"
#include "Utils/Value.h"
#include <cstdio>
#include <cstring>


int    MathFPGA::NA = 0; //-V707
int    MathFPGA::NB = 0; //-V707
       
uint   MathFPGA::Auto::fpgaMin = 0;
uint   MathFPGA::Auto::fpgaMid = 0;
uint   MathFPGA::Auto::fpgaMax = 0;
       
float  MathFPGA::DutyCycle::value = 0.0F;
int    MathFPGA::DutyCycle::zeroes = 0;
uint   MathFPGA::DutyCycle::fpgaDuration = 0;
uint   MathFPGA::DutyCycle::fpgaPeriod = 0;
       
int       MathFPGA::Measure::decDA = 1;
int       MathFPGA::Measure::emptyZeros = 0;
ValuePICO MathFPGA::Measure::valueComparator(0);
uint      MathFPGA::Measure::fpgaFrequencyA = 0;
uint      MathFPGA::Measure::fpgaFrequencyB = 0;
ValueNANO MathFPGA::Measure::decDataA(0);
ValueNANO MathFPGA::Measure::decDataB(0);
ValueNANO MathFPGA::Measure::decDataC(0);


void MathFPGA::Measure::Calculate()
{
    int x = 0;
    int manualZeros = 1;

    if (TypeMeasure::Current().IsFrequency())
    {
        x = CalculateFrequency(manualZeros);
    }
    else if (TypeMeasure::Current().IsDuration())
    {
        x = CalculateDuration();
    }
    else if (TypeMeasure::Current().IsPeriod())
    {
        x = CalculatePeriod();
    }

    if (CURRENT_CHANNEL_IS_D)
    {
        decDataA.FromDouble(decDataC.ToDouble() * 2.0);
    }

    decDataA.Div((uint)(2 * x));

    emptyZeros = x;

    if (manualZeros != 1) //-V1051
    {
        emptyZeros = manualZeros;
    }
}


int MathFPGA::Measure::CalculateFrequency(int &manualZeros)
{
    int result = 0;

    ModeMeasureFrequency &mode = ModeMeasureFrequency::Current();

    if (mode.IsT_1())
    {
        manualZeros = 10 * PageModesA::periodTimeLabels.ToZeros() / 1000 * PageModesA::numberPeriods.ToAbs();

        decDataA.Div((uint)PageModesA::periodTimeLabels.ToZeros());

        double test1 = decDataA.ToDouble();

        double test2 = test1 / PageModesA::numberPeriods.ToAbs();

        decDataA.FromDouble((float)(4 / test2));

        decDA = (int)(decDataA.ToDouble() / 2.0);

        if (decDA < 1000)           { }
        else if (decDA < 1000000)   { decDataA.Div(1000);    }
        else                        { decDataA.Div(1000000); }

        result = 1;
    }
    else if (mode.IsRatioAB() || mode.IsRatioBA())
    {
        result = PageModesA::numberPeriods.ToAbs();
    }
    else if (mode.IsRatioCA() || mode.IsRatioCB())
    {
        decDataA.Mul(100);
        result = PageModesC::numberPeriods.ToAbs();
    }
    else if (mode.IsRatioAC() || mode.IsRatioBC())
    {
        int sT = PageModesA::timeMeasure.ToMS();

        decDataA.FromDouble(decDataA.ToDouble() / decDataB.ToDouble() / 32);
        manualZeros = 1000000 * sT;
        result = 1;
    }
    else
    {
        int mhz = 1000 * PageModesA::timeMeasure.ToMS();
        int khz = PageModesA::timeMeasure.ToMS();

        if (((decDataA.ToDouble() / (float)khz) / 2.0F) < 1000.0F)
        {
            result = khz;
        }
        else
        {
            result = mhz;
        }
        decDA = (int)((decDataA.ToDouble() / (double)khz) / 2.0);

        if (CURRENT_CHANNEL_IS_C)
        {
            if (decDataA.ToUINT64() < 10000)
            {
                decDataC.FromDouble(decDataA.ToDouble());
                khz = khz * 10;
                result = khz;
            }
            else
            {
                decDataC.FromDouble(decDataA.ToDouble());
                mhz = mhz * 10;
                result = mhz;
            }
        }

        if (CURRENT_CHANNEL_IS_D)
        {
            if (decDataA.ToDouble() * 64.0F / (1000.0F * (float)khz) > 19000.0F)
            {
                decDataC.FromDouble(0.0);
                result = khz;
            }
            else
            {
                decDataC.FromDouble(decDataA.ToDouble() * 64 / 1000);
                result = mhz;
            }

            decDA = (int)decDataC.ToUINT64();
        }
    }

    return result;
}


int MathFPGA::Measure::CalculatePeriod()
{
    int result = 1;

    if (ModeMeasurePeriod::Current().IsF_1())
    {
        int sT = PageModesA::timeMeasure.ToMS();

        decDA = (int)(decDataA.ToDouble() / (2.0 * (double)sT));
        decDataA.FromDouble(4 / decDataA.ToDouble());

        decDataA.Mul((uint)sT);
        decDataA.Mul((uint)sT);

        if (decDA >= 1000)      { decDataA.Mul(10000000); }
        else if (decDA <= 1)    { decDataA.Mul(10);       }
        else                    { decDataA.Mul(10000);    }

        result = sT * 10;
    }
    else
    {
        if (PageModesA::periodTimeLabels.IsT_7() || PageModesA::periodTimeLabels.IsT_4())
        {
            result *= 10;
        }
        else if (PageModesA::periodTimeLabels.IsT_8() || PageModesA::periodTimeLabels.IsT_5())
        {
            result *= 100;
        }

        result *= PageModesA::numberPeriods.ToAbs();
    }

    return result;
}


int MathFPGA::Measure::CalculateDuration()
{
    if (PageModesA::periodTimeLabels.IsT_7())
    {
        return 10;
    }
    else if (PageModesA::periodTimeLabels.IsT_8() || PageModesA::periodTimeLabels.IsT_5())
    {
        return 100;
    }

    return 1;
}


void MathFPGA::Measure::BinToDec()
{
    decDataA.FromDouble((double)fpgaFrequencyA);

    if (ModeMeasureFrequency::Current().IsRatioAC() || ModeMeasureFrequency::Current().IsRatioBC())
    {
        decDataB.FromDouble(fpgaFrequencyB);
    }

    if (CURRENT_CHANNEL_IS_C)
    {
        decDataA.Mul(64);
        decDataA.Div(100);
    }
}


int MathFPGA::Auto::Mid()
{
    return (int)fpgaMid;
}


int MathFPGA::Auto::Min()
{
    return (int)fpgaMin;
}


int MathFPGA::Auto::Max()
{
    return (int)fpgaMax;
}


char *MathFPGA::Auto::Give()
{
    static char result[20] = { 0 };

    char minAutoData[7] = { 0 };
    char maxAutoData[7] = { 0 };

    SU::Int2String(((int)fpgaMin - 512) * 2, minAutoData);
    SU::Int2String(((int)fpgaMax - 512) * 2, maxAutoData);
    std::strcpy(result, "���� ");
    std::strcat(result, maxAutoData);
    std::strcat(result, " ��� ");
    std::strcat(result, minAutoData);

    if (CURRENT_CHANNEL_IS_A)
    {
        LEVEL_SYNCH_A = ((int)fpgaMid - 512) * 2;
        NA = (int)fpgaMid - 512;
    }

    if (CURRENT_CHANNEL_IS_B)
    {
        LEVEL_SYNCH_B = ((int)fpgaMid - 512) * 2;
        NB = (int)fpgaMid - 512;

    }

    return result;
}


void MathFPGA::DutyCycle::Calculate()
{
    value = (float)fpgaDuration / (float)fpgaPeriod;

    if (ModeMeasureDuration::Current().Is_Phase())
    {
        value *= 360;
    }
    else
    {
        if (value < 0)
        {
            value *= -1;
        }

        if (value != 0.0F) //-V2550 //-V550
        {
            while (value < 1)
            {
                value *=  10;
                zeroes++;
            }
        }
    }
}


void MathFPGA::DecToBin(int dec, char *bin)
{
    int x = dec;
    for (int i = 0; i < 10; i++)
    {
        if (x % 2 != 0) { bin[i] = 1; }
        else            { bin[i] = 0; }
        x = x / 2;
    }
}


float MathFPGA::Interpolation::Calculate()
{
    return (float)(100 * FPGA::fpgaTimer) / (float)(FPGA::fpgaCAL2 - FPGA::fpgaCAL1);
}


String MathFPGA::Measure::GiveData()
{
    if (FPGA::IsOverloaded())
    {
        return String("������������");
    }

    if (TypeMeasure::Current().IsCountPulse())
    {
        BinToDec();
        decDataA.Div(2);

        if (CURRENT_CHANNEL_IS_C)
        {
            decDataA.Mul(100);
        }

        if (ModeMeasureCountPulse::Current().IsFromPeriod())
        {
            decDataA.Div((uint)PageModesA::numberPeriods.ToAbs());
        }

        return String("%10.0f", decDataA.ToFloat());
    }
    else
    {
        if (ModeMeasureFrequency::Current().IsTachometer())
        {
            BinToDec();
            decDataA.Div(2);

            return String("%10.0f", decDataA.ToFloat());
        }
        else if (ModeMeasureFrequency::Current().IsComparator())
        {
            return valueComparator.ToString();
        }
        else if (ModeMeasureDuration::Current().Is_Ndt_1ns())
        {
            return String("%10.2f", MathFPGA::Interpolation::Calculate());
        }
        else if (TypeMeasure::Current().IsDuration() && (ModeMeasureDuration::Current().Is_DutyCycle() || ModeMeasureDuration::Current().Is_Phase()))
        {
            DutyCycle::Calculate();

            if (ModeMeasureDuration::Current().Is_Phase())
            {
                return String("%10.3f", MathFPGA::DutyCycle::value);
            }
            
            return String("%10.7f", MathFPGA::DutyCycle::value);
        }
        else
        {
            BinToDec();
            Calculate();

            int pow = 0;

            while (emptyZeros >= 10)
            {
                pow++;
                emptyZeros /= 10;
            }

            emptyZeros = 1;

            if (pow < 10)
            {
                char format[10];
                std::strcpy(format, "%10.0f");
                format[4] = (char)(pow | 0x30);

                return String(format, decDataA.ToFloat());
            }
            else
            {
                char format[10];
                std::strcpy(format, "%10.10f");
                format[5] = (char)((pow - 10) | 0x30);

                return String(format, decDataA.ToFloat());
            }
        }
    }
}


char *MathFPGA::Measure::GiveSpec() //-V2008
{
    static char result[10] = { 0 };

    if (ModeMeasureDuration::Current().Is_Ndt_1ns())
    {
        std::strcpy(result, " ns");
    }
    else if(TypeMeasure::Current().IsDuration() && (ModeMeasureDuration::Current().Is_DutyCycle() || ModeMeasureDuration::Current().Is_Phase()))
    {
        if (ModeMeasureDuration::Current().Is_Phase())
        {
            std::strcpy(result, " $");
        }
        else
        {
            std::strcpy(result, " E-0");
            result[3] = (char)(DutyCycle::zeroes | 0x30);

            DutyCycle::zeroes = 0;
        }
    }
    else
    {
        if (ModeMeasureFrequency::Current().IsRatio() ||
            ModeMeasureFrequency::Current().IsTachometer() ||
            TypeMeasure::Current().IsCountPulse())
        {
            std::strcpy(result, " ");
        }
        else
        {
            if (TypeMeasure::Current().IsFrequency())
            {
                if (ModeMeasureFrequency::Current().IsT_1())
                {
                    if (decDA < 1000)           { std::strcpy(result, " Hz");  }
                    else if (decDA < 1000000)   { std::strcpy(result, " kHz"); }
                    else                        { std::strcpy(result, " MHz"); }
                }
                else if (PageModesA::modeMeasureFrequency.IsComparator() && CURRENT_CHANNEL_IS_A)
                {
                    std::strcpy(result, " E-6");
                }
                else
                {
                    if (CURRENT_CHANNEL_IS_C)
                    {
                        if (decDataC.ToUINT64() / 2 < 10000)    { std::strcpy(result, " MHz"); }
                        else                                    { std::strcpy(result, " GHz"); }
                    }
                    else if (CURRENT_CHANNEL_IS_D)   
                    {
                        std::strcpy(result, " GHz");
                    }
                    else
                    {
                        if (decDA < 1000)           { std::strcpy(result, " kHz"); }
                        else                        { std::strcpy(result, " MHz"); }
                    }
                }
            }
            else
            {
                if ((CURRENT_CHANNEL_IS_A && PageModesA::typeMeasure.IsPeriod() && PageModesA::modeMeasurePeriod.IsF_1()) ||
                    (CURRENT_CHANNEL_IS_B && PageModesB::typeMeasure.IsPeriod() && PageModesB::modeMeasurePeriod.IsF_1()))
                {
                    if (decDA >= 1000)      { std::strcpy(result, " ns"); }
                    else if (decDA <= 1)    { std::strcpy(result, " ms"); }
                    else                    { std::strcpy(result, " us"); }
                }
                else
                {
                    PeriodTimeLabels &current = PeriodTimeLabels::Current();

                    if (current.IsT_3() || current.IsT_4() || current.IsT_5())  { std::strcpy(result, " ms"); }
                    else                                                        { std::strcpy(result, " us"); }
                }
            }
        }
    }
    return result;
}


char *MathFPGA::Measure::GiveIdent()
{
    static char result[10];

    std::sprintf(result, "%ud", FPGA::fpgaIdent);

    return result;
}


void MathFPGA::Auto::Refresh()
{
    for (int i = 0; i < 10; i++)
    {
        fpgaMin = 0;
        fpgaMid = 0;
        fpgaMax = 0;
    }
}
