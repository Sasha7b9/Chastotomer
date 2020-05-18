#include "defines.h"
#include "PageModesC.h"
#include "PageModes.h"
#include "PageModesB.h"
#include "PageModesD.h"
#include "Menu/MenuItems.h"
#include "Display/Primitives.h"
#include "Display/Text.h"
#include "Utils/Math.h"
#include "Menu/MenuItemsDef.h"
#include "FreqMeter/FreqMeter.h"
#include "Menu/Pages/PageChannelA.h"
#include "Menu/Pages/PageChannelB.h"
#include "Menu/Pages/PageChannelC.h"
#include "Menu/Pages/PageChannelD.h"

using namespace Display::Primitives;
using namespace Display;

extern Item *items[7];
extern Switch sModeFrequencyC;
extern Switch sModeCountPulseC;
extern Switch sPeriodTimeLabelsC;
extern Switch sTimeMeasureC;
extern Switch sNumberPeriodsC;


TypeMeasureC             PageModesC::typeMeasureC(TypeMeasureC::Frequency);
ModeMeasureFrequencyC    PageModesC::modeMeasureFrequencyC(ModeMeasureFrequencyC::Freq);
ModeMeasureCountPulseC   PageModesC::modeMeasureCountPulseC(ModeMeasureCountPulseC::CTA);
PeriodTimeLabelsC        PageModesC::periodTimeLabelsC(PeriodTimeLabelsC::T_8);
NumberPeriodsC           PageModesC::numberPeriodsC(NumberPeriodsC::_1);
TimeMeasureC             PageModesC::timeMeasureC(TimeMeasureC::_1ms);

/// �������� ������ ���������� �� �����, ������� � i-�� �����
static void ClearItems(int i);

static void OnPress_ModeFrequencyC();

static void OnPress_ModeCountPulseC();


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void PageModesC::Init()
{
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void ClearItems(int num)
{
    for (int i = num; i < 7; i++)
    {
        items[i] = nullptr;
    }
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void OnPress_TypeMeasureC()
{
        switch (PageModesC::typeMeasureC.value)
        {
        case TypeMeasureC::Frequency:
            OnPress_ModeFrequencyC();
            break;
        case TypeMeasureC::CountPulse:
            OnPress_ModeCountPulseC();
            break;
        }        
}

DEF_SWITCH_2(sTypeMeasureC,
    "��� ���.", "����� ���������",
    "�������", "��. ���.",
    PageModesC::typeMeasureC, OnPress_TypeMeasureC
)

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void OnPress_ModeFrequencyC()
{
    ClearItems(2);
    items[2] = &sPeriodTimeLabelsC;   
    items[1] = &sModeFrequencyC;
    
    if (PageModesC::modeMeasureFrequencyC == ModeMeasureFrequencyC::CB)
    {
        items[3] = &sNumberPeriodsC;
    }
    else
    {
        items[3] = &sTimeMeasureC;
    }      
    FreqMeter::LoadModeMeasureFrequency();
}

/// ����� ������ ��������� �������, ��������� ������, "���������"
DEF_SWITCH_3(sModeFrequencyC,
    "����� ���.", "��������� �������",
    "�������", "f(C )/f(A)", "f(C )/f(B)",
    PageModesC::modeMeasureFrequencyC, OnPress_ModeFrequencyC);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void OnPress_ModeCountPulseC()
{
    ClearItems(2);

    items[1] = &sModeCountPulseC;

    if (PageModesC::modeMeasureCountPulseC == ModeMeasureCountPulseC::CTA_1)
    {
        items[2] = &sNumberPeriodsC;
    }

    FreqMeter::LoadModeMeasureCountPulse();
}

/// ����� ������ ����� ���������
DEF_SWITCH_4(sModeCountPulseC,
    "����� ���.", "���� ����� ���������",
    "C(tA)", "C(tB)", "C(TA)", "C(TB)",
    PageModesC::modeMeasureCountPulseC, OnPress_ModeCountPulseC
);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void OnPress_TimeLabelsC()
{
    switch (PageModesC::periodTimeLabelsC.value)
    {
        case PeriodTimeLabelsC::T_3:
            PageModesB::periodTimeLabelsB.value = PeriodTimeLabelsB::T_3;
            PageModes::periodTimeLabels.value = PeriodTimeLabels::T_3;
            break;
        case PeriodTimeLabelsC::T_4:
            PageModesB::periodTimeLabelsB.value = PeriodTimeLabelsB::T_4;
            PageModes::periodTimeLabels.value = PeriodTimeLabels::T_4;
            break;
        case PeriodTimeLabelsC::T_5:
            PageModesB::periodTimeLabelsB.value = PeriodTimeLabelsB::T_5;
            PageModes::periodTimeLabels.value = PeriodTimeLabels::T_5;
            break;
        case PeriodTimeLabelsC::T_6:
            PageModesB::periodTimeLabelsB.value = PeriodTimeLabelsB::T_6;
            PageModes::periodTimeLabels.value = PeriodTimeLabels::T_6;
            break;
        case PeriodTimeLabelsC::T_7:
            PageModesB::periodTimeLabelsB.value = PeriodTimeLabelsB::T_7;
            PageModes::periodTimeLabels.value = PeriodTimeLabels::T_7;
            break;
        case PeriodTimeLabelsC::T_8:
            PageModesB::periodTimeLabelsB.value = PeriodTimeLabelsB::T_8;
            PageModes::periodTimeLabels.value = PeriodTimeLabels::T_8;
            break;
    }
    FreqMeter::LoadPeriodTimeLabels();
}

/// ����� ������� ����� �������
DEF_SWITCH_6(sPeriodTimeLabelsC,
    "�����", "������������ ��������� �����",
    "10-3", "10-4", "10-5", "10-6", "10-7", "10-8",
    PageModesC::periodTimeLabelsC,  OnPress_TimeLabelsC
);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void OnPress_TimeMeasureC()
{
    switch (PageModesC::timeMeasureC.value)
    {
        case TimeMeasureC::_1ms:
            PageModesB::timeMeasureB.value = TimeMeasureB::_1ms;
            PageModes::timeMeasure.value = TimeMeasure::_1ms;
            PageModesD::timeMeasureD.value = TimeMeasureD::_1ms;
            break;
        case TimeMeasureC::_10ms:
            PageModesB::timeMeasureB.value = TimeMeasureB::_10ms;
            PageModes::timeMeasure.value = TimeMeasure::_10ms;
            PageModesD::timeMeasureD.value = TimeMeasureD::_10ms;
            break;
        case TimeMeasureC::_100ms:
            PageModesB::timeMeasureB.value = TimeMeasureB::_100ms;
            PageModes::timeMeasure.value = TimeMeasure::_100ms;
            PageModesD::timeMeasureD.value = TimeMeasureD::_100ms;
            break;
        case TimeMeasureC::_1s:
            PageModesB::timeMeasureB.value = TimeMeasureB::_1s;
            PageModes::timeMeasure.value = TimeMeasure::_1s;
            PageModesD::timeMeasureD.value = TimeMeasureD::_1s;
            break;
        case TimeMeasureC::_10s:
            PageModesB::timeMeasureB.value = TimeMeasureB::_10s;
            PageModes::timeMeasure.value = TimeMeasure::_10s;
            PageModesD::timeMeasureD.value = TimeMeasureD::_10s;
            break;
        case TimeMeasureC::_100s:
            PageModesB::timeMeasureB.value = TimeMeasureB::_100s;
            PageModes::timeMeasure.value = TimeMeasure::_100s;
            PageModesD::timeMeasureD.value = TimeMeasureD::_100s;
            break;
    }
    FreqMeter::LoadTimeMeasure();
}

/// ����� ������� ���������
DEF_SWITCH_6(sTimeMeasureC,
    "�����", "����� �����",
    "1ms", "10ms", "100ms", "1s", "10s", "100s",
    PageModesC::timeMeasureC, OnPress_TimeMeasureC
);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void OnPress_NumberPeriodsC()
{
    switch (PageModesC::numberPeriodsC.value)
    {
        case NumberPeriodsC::_1:
            PageModesB::numberPeriodsB.value = NumberPeriodsB::_1;
            PageModes::numberPeriods.value = NumberPeriods::_1;
            break;
        case NumberPeriodsC::_10:
            PageModesB::numberPeriodsB.value = NumberPeriodsB::_10;
            PageModes::numberPeriods.value = NumberPeriods::_10;
            break;
        case NumberPeriodsC::_100:
            PageModesB::numberPeriodsB.value = NumberPeriodsB::_100;
            PageModes::numberPeriods.value = NumberPeriods::_100;
            break;
        case NumberPeriodsC::_1K:
            PageModesB::numberPeriodsB.value = NumberPeriodsB::_1K;
            PageModes::numberPeriods.value = NumberPeriods::_1K;
            break;
        case NumberPeriodsC::_10K:
            PageModesB::numberPeriodsB.value = NumberPeriodsB::_10K;
            PageModes::numberPeriods.value = NumberPeriods::_10K;
            break;
        case NumberPeriodsC::_100K:
            PageModesB::numberPeriodsB.value = NumberPeriodsB::_100K;
            PageModes::numberPeriods.value = NumberPeriods::_100K;
            break;
    }
    FreqMeter::LoadNumerPeriodsMeasure();
}

/// ����� ����� ����������� �������� �������� �������
DEF_SWITCH_6(sNumberPeriodsC,
    "N", "����� �������� ���������",
    "1", "10", "100", "1K", "10K", "100K",
    PageModesC::numberPeriodsC,
    OnPress_NumberPeriodsC
);


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static Item *items[7] =
{
    &sTypeMeasureC,
    &sModeFrequencyC,
    &sPeriodTimeLabelsC,
    &sTimeMeasureC,
    nullptr,
    nullptr,
    nullptr
};

static Page pageModesC(items);

Page *PageModesC::self = &pageModesC;
