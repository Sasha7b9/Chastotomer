#include "defines.h"
#include "Menu/Pages/Channels/Channels.h"


Channel::E Channel::current = Channel::_A;

Channel Channel::A(&pageChannelA);
Channel Channel::B(&pageChannelB);
Channel Channel::C(&pageChannelC);
Channel Channel::D(&pageChannelD);


PeriodTimeLabels Channel::timeLabels(PeriodTimeLabels::T_8);
NumberPeriods    Channel::numberPeriods(NumberPeriods::_1);
TimeMeasure      Channel::timeMeasure(TimeMeasure::_1ms);


Channel::Channel(Page *pSettings) : settings(pSettings), couple(InputCouple::AC), impedance(InputImpedance::_1MOmh)
{
}


Channel *Channel::Current()
{
    static Channel * const channels[Count] = { &A, &B, &C, &D };

    return channels[current];
}


Page *Channel::PageForChannel(Channel::E channel)
{
    Page *pages[Channel::Count] =
    {
        PageModesA::self,
        PageModesB::self,
        PageModesC::self,
        PageModesD::self
    };

    return pages[channel];
}


#ifdef WIN32
#pragma warning(push)
#pragma warning(disable: 4062)
#endif


bool Channel::IsActiveTimeLabels(TypeMeasure *type, int m)
{
    if (type->IsFrequency())
    {
        ModeMeasureFrequency::E mode = (ModeMeasureFrequency::E)m;

        switch (mode)
        {
        case ModeMeasureFrequency::T_1:
        case ModeMeasureFrequency::RatioCA:
        case ModeMeasureFrequency::RatioCB:     return true;

        case ModeMeasureFrequency::Frequency:   return CURRENT_CHANNEL_IS_A_OR_B ? FreqMeter::modeTest.IsEnabled() : false;

        case ModeMeasureFrequency::Tachometer:  return FreqMeter::modeTest.IsEnabled();
        }
    }
    else if (type->IsPeriod())
    {
        ModeMeasurePeriod::E mode = (ModeMeasurePeriod::E)m;

        switch (mode)
        {
        case ModeMeasurePeriod::Period:      return true;

        case ModeMeasurePeriod::F_1:         return FreqMeter::modeTest.IsEnabled();
        }
    }
    else if (type->IsDuration())
    {
        ModeMeasureDuration::E mode = (ModeMeasureDuration::E)m;

        switch (mode)
        {
        case ModeMeasureDuration::Ndt:
        case ModeMeasureDuration::StartStop:
        case ModeMeasureDuration::FillFactor:
        case ModeMeasureDuration::Phase:        return true;
        }
    }

    return false;
}


bool Channel::IsActiveTimeMeasure(TypeMeasure *type, int m)
{
    if (type->IsFrequency())
    {
        ModeMeasureFrequency::E mode = (ModeMeasureFrequency::E)m;

        switch (mode)
        {
        case ModeMeasureFrequency::Frequency:
        case ModeMeasureFrequency::RatioAC:
        case ModeMeasureFrequency::RatioBC:     return true;
        }
    }
    else if (type->IsPeriod())
    {
        ModeMeasurePeriod::E mode = (ModeMeasurePeriod::E)m;

        switch (mode)
        {
        case ModeMeasurePeriod::F_1:            return true;
        }
    }

    return false;
}


bool Channel::IsActiveNumberPeriods(TypeMeasure *type, int m)
{
    if (type->IsFrequency())
    {
        ModeMeasureFrequency::E mode = (ModeMeasureFrequency::E)m;

        switch (mode)
        {
        case ModeMeasureFrequency::T_1:
        case ModeMeasureFrequency::RatioAB:
        case ModeMeasureFrequency::RatioBA:
        case ModeMeasureFrequency::RatioCA:
        case ModeMeasureFrequency::RatioCB:     return true;
        }
    }
    else if (type->IsPeriod())
    {
        ModeMeasurePeriod::E mode = (ModeMeasurePeriod::E)m;

        switch (mode)
        {
        case ModeMeasurePeriod::Period:         return true;
        }
    }
    else if (type->IsCountPulse())
    {
        ModeMeasureCountPulse::E mode = (ModeMeasureCountPulse::E)m;

        switch (mode)
        {
        case ModeMeasureCountPulse::ATB:
        case ModeMeasureCountPulse::BTA:
        case ModeMeasureCountPulse::CTA:
        case ModeMeasureCountPulse::CTB:        return true;
        }
    }

    return false;
}


#ifdef WIN32
#pragma warning(pop)
#endif
