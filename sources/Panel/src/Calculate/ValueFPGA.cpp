#include "defines.h"
#include "Log.h"
#include "Calculate/ValueFPGA.h"
#include "Calculate/ValueFPGA.h"
#include "Display/Display.h"
#include "Display/Objects.h"
#include "Hardware/FPGA.h"
#include "Menu/Pages/Channels/Channels.h"
#include "Menu/Pages/Modes/Modes.h"
#include "Utils/Buffer.h"
#include "Utils/StringUtils.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>


ValueFPGA *ValueFPGA::valueFPGA = nullptr;
String ValueFPGA::value;


void ValueFPGA::Create(uint value1, uint value2, uint value3, uint value4, uint value5)
{
    ProgressBarTimeMeasureZone::Reset();

    SynchroZone::Fire();

    TypeMeasure &type = Channel::Current()->mod.typeMeasure;

    if (valueFPGA != nullptr)
    {
        delete valueFPGA;
        valueFPGA = nullptr;
    }

    if (type.IsFrequency())
    {
        switch (Channel::Current()->mod.modeFrequency)
        {
        case ModeFrequency::Frequency:  valueFPGA = new ValueFrequency_Frequency(value1);           break;
        case ModeFrequency::T_1:        valueFPGA = new ValueFrequency_T_1(value1);                 break;
        case ModeFrequency::Tachometer: valueFPGA = new ValueFrequency_Tachometer(value1);          break;
        case ModeFrequency::Comparator:
            valueFPGA = new ValueFrequency_Comparator(value1, (int)value2, (int)value3, (int)value4, (int)value5);
            break;
        case ModeFrequency::RatioAB:
        case ModeFrequency::RatioAC:
        case ModeFrequency::RatioBA:
        case ModeFrequency::RatioBC:
        case ModeFrequency::RatioCA:
        case ModeFrequency::RatioCB:    valueFPGA = new ValueFrequency_Ratio(value1, value2);       break;
        }
    }
    else if (type.IsPeriod())
    {
        switch (Channel::Current()->mod.modePeriod)
        {
        case ModePeriod::Period:        valueFPGA = new ValuePeriod_Period(value1);     break;
        case ModePeriod::F_1:           valueFPGA = new ValuePeriod_F_1(value1);        break;
        }
    }
    else if (type.IsDuration())
    {
        switch (Channel::Current()->mod.modeDuration)
        {
        case ModeDuration::Ndt_1ns:     valueFPGA = new ValueDuration_Ndt_1ns(value1, value2, value3);
            break;;
        case ModeDuration::FillFactor:
        case ModeDuration::Phase:       valueFPGA = new ValueDuration_Phase_FillFactor(value1, value2);
            break;;
        case ModeDuration::Ndt:
        case ModeDuration::StartStop:           valueFPGA = new ValueDuration_Ndt_StartStop(value1);
            break;
        }
    }
    else if (type.IsCountPulse())
    {
        valueFPGA = new ValueCountPulse(value1);
    }
}


int ValueFPGA::NumDigitsInNumber(uint number) const
{
    if (number == 0)
    {
        return 1;
    }

    int result = 0;

    while (number > 0)
    {
        result++;
        number /= 10;
    }

    return result;
}


char *ValueFPGA::GetSuffixUnit(int order) const
{
    if (order >= 15)       { return "P"; }
    else if (order >= 12)  { return "T"; }
    else if (order >= 9)   { return "G"; }
    else if (order >= 6)   { return "M"; }
    else if (order >= 3)   { return "k"; }
    else if (order >= 0)   { return "";  }
    else if (order >= -3)  { return "m"; }
    else if (order >= -6)  { return "u"; }
    else if (order >= -9)  { return "n"; }
    else if (order >= -12) { return "p"; }
    
    return "ph";
}


char *ValueFPGA::GetSuffixUnitRelated(int order)
{
    if (order >= 15) { return "E+15"; }
    else if (order >= 12) { return "E+12"; }
    else if (order >= 9) { return "E+9"; }
    else if (order >= 6) { return "E+6"; }
    else if (order >= 3) { return "E+3"; }
    else if (order >= 0) { return "E+0"; }
    else if (order >= -3) { return "E-3"; }
    else if (order >= -6) { return "E-6"; }
    else if (order >= -9) { return "E-9"; }
    else if (order >= -12) { return "E-12"; }

    return "E-15";
}


String ValueFPGA::GiveDigits()
{
    const char *firstSpace = std::strchr(value.c_str(), ' ');

    if (firstSpace == nullptr)
    {
        return String(value.c_str());
    }

    int size = firstSpace - value.c_str() + 1;

    char *buffer = new char[(uint)size];

    std::memcpy(buffer, value.c_str(), size - 1U);
    buffer[size - 1] = '\0';

    String result(buffer);

    delete[]buffer;

    return result;
}


String ValueFPGA::GiveUnits()
{
    const char *lastSpace = std::strrchr(value.c_str(), ' ');

    if (lastSpace == nullptr)
    {
        return String("");
    }

    int size = &value.c_str()[std::strlen(value.c_str())] - lastSpace + 1;

    char *buffer = new char[(uint)size];

    std::memcpy(buffer, lastSpace + 1, size - 1U);
    buffer[size - 1] = '\0';

    String result(buffer);

    delete[]buffer;

    return result;
}


char *ValueFPGA::GiveStringValue()
{
    return value.c_str();
}


void ValueFPGA::SetValue(char *format, ...)
{
    if (FPGA::IsOverloaded())
    {
        value.Set(UGO::OVERLAPPED);
    }
    else
    {
        std::va_list args;
        va_start(args, format);

        int numSymbols = std::vsnprintf(nullptr, 0, format, args);

        Buffer buffer(numSymbols + 1);

        std::vsnprintf(buffer.DataChar(), (uint)numSymbols + 1U, format, args);

        value.Set(buffer.DataChar());

        va_end(args);
    }

    Display::zoneData->Refresh();
}


void ValueFPGA::SetValue(ValueSTRICT strict, uint counter, bool isOrdered)
{
    if (FPGA::IsOverloaded())
    {
        value.Set(UGO::OVERLAPPED);
        return;
    }

    int order = 0;
    int numDigitsInCounter = NumDigitsInNumber(counter);

    if(isOrdered)
    {
        if (numDigitsInCounter < 1)
        {
            numDigitsInCounter = 1;
        }

        if (strict.ToDouble() < 0.0)
        {
            while (strict.ToUnits(Order::Milli) < 1)
            {
                strict.MulUINT(1000);
                order -= 3;
            }
        }
        else
        {
            while (strict.ToDouble() >= 1000.0)
            {
                strict.DivUINT(1000);
                order += 3;
            }
        }

        if (!strict.IsZero())
        {
            while ((uint)strict.ToDouble() < 1)
            {
                strict.MulUINT(1000);
                order -= 3;
            }

            if (NumDigitsInNumber((uint)strict.ToDouble()) > numDigitsInCounter)
            {
                strict.DivUINT(1000);
                order += 3;
            }
        }
    }

    char buffer[30];
    std::sprintf(buffer, "%10.10f", strict.ToDouble());

    if (strict.IsZero())
    {
        SU::LeaveFewDigits(buffer, 29, numDigitsInCounter);
    }
    else
    {
        SU::LeaveFewSignedDigits(buffer, 29, numDigitsInCounter);
    }

    std::strcat(buffer, " ");

    if (isOrdered)
    {
        std::strcat(buffer, GetSuffixUnit(order));
    }

    std::strcat(buffer, mainUnits.c_str());

    SetValue(buffer);
}


void ValueFPGA::SetInfiniteValue()
{
    SetValue(UGO::DivNULL);
}


void ValueFPGA::SetIntegerValue(uint counter)
{
    SetValue("%d", counter);
}


void ValueFPGA::SetInvalidData()
{
    value.Set(UGO::EMPTY_VALUE);

    Display::zoneData->Refresh();
}
