// (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Settings.h"
#include "Calculate/ValueFPGA.h"
#include "Display/Primitives.h"
#include "Display/Text.h"
#include "Menu/Menu.h"
#include "Menu/MenuItemsDef.h"
#include "Menu/MenuItems.h"
#include "Utils/Math.h"
#include "Menu/Pages/Pages.h"


using namespace Primitives;


extern Item *items[7];
extern Switch sModeFrequency;
extern Switch sModePeriod;
extern Switch sModeDuration;
extern Switch sModeCountPulse;


static void OnChanged_TypeMeasure()
{
    Channel::Current()->OnChanged_TypeMeasure();
}


DEF_SWITCH_4(sTypeMeasure,
    "���������", "Measure",
    "����� ���������", "Measurement selection",
    "�������",   "������", "����.",    "��. ���.",
    "Frequency", "Period", "Duration", "Counting",
    Channel::A->mod.typeMeasure, OnChanged_TypeMeasure
);


static void OnPress_Statistics()
{
    Menu::SetOpenedPage(PageStatistics::self);
}

DEF_BUTTON(bStatistics,
    "����", "Stat",
    "��������� ���� ����������", "Opens statistics window",
    OnPress_Statistics);


static void OnChanged_ModeFrequency()
{
    items[1] = &sModeFrequency;

    Relation::Off();

    if (Channel::A->mod.modeFrequency.IsFrequency())
    {
        items[2] = ModesChannel::switchTimeMeasue;
        items[3] = FreqMeter::modeTest.IsEnabled() ? ModesChannel::switchTimeLabels : nullptr;
        items[4] = nullptr;
    }
    else if (Channel::A->mod.modeFrequency.IsRatioAB())
    {
        items[2] = ModesChannel::switchNumberPeriods;
        items[3] = nullptr;
        Relation::On();
    }
    else if (Channel::A->mod.modeFrequency.IsRatioAC())
    {
        items[2] = ModesChannel::switchTimeMeasue;
        items[3] = nullptr;
        Relation::On();
    }
    else if (Channel::A->mod.modeFrequency.IsT_1())
    {
        items[2] = ModesChannel::switchNumberPeriods;
        items[3] = ModesChannel::switchTimeLabels;
        items[4] = nullptr;
    }
    else if (Channel::A->mod.modeFrequency.IsTachometer())
    {
        items[2] = FreqMeter::modeTest.IsEnabled() ? ModesChannel::switchTimeLabels : nullptr;
        items[3] = nullptr;
    }
    else if (Channel::A->mod.modeFrequency.IsComparator())
    {
        items[2] = ModesChannel::switchTimeComparator;
        items[3] = &bStatistics;
        items[4] = nullptr;
    }

    ModeFrequency::LoadToFPGA();
}

// ����� ������ ��������� �������, ��������� ������, "���������"
DEF_SWITCH_6(sModeFrequency,
    "�����", "Mode",
    "��������� �������", "Frequency measurement",
    "�������",   "f=1/T", "f(A)/f(B)", "f(A)/f(C)", "��������",   "����������",
    "Frequency", "f=1/T", "f(A)/f(B)", "f(A)/f(C)", "Tachometer", "Comparator",
    Channel::A->mod.modeFrequency, OnChanged_ModeFrequency
);


Switch *switchModeFrequencyA = &sModeFrequency;


static void OnChanged_ModePeriod()
{
    items[1] = &sModePeriod;

    if (Channel::A->mod.modePeriod.IsPeriod())
    {
        items[2] = ModesChannel::switchNumberPeriods;
        items[3] = ModesChannel::switchTimeLabels;
        items[4] = nullptr;
    }
    else if (Channel::A->mod.modePeriod.IsF_1())
    {
        items[2] = ModesChannel::switchTimeMeasue;
        items[3] = FreqMeter::modeTest.IsEnabled() ? ModesChannel::switchTimeLabels : nullptr;
        items[4] = nullptr;
    }

    Relation::Off();
    
    ModePeriod::LoadToFPGA();
}

// ����� ������ ��������� �������
DEF_SWITCH_2(sModePeriod,
    "�����", "Mode",
    "��������� �������", "Period measurement",
    "������", "T=1/f",
    "Period", "T=1/f",
    Channel::A->mod.modePeriod, OnChanged_ModePeriod
);


Switch *switchModePeriodA = &sModePeriod;


static void OnChanged_ModeDuration()
{
    items[1] = &sModeDuration;

    switch (Channel::A->mod.modeDuration.value)
    {
    case ModeDuration::Ndt_1ns:
        items[2] = nullptr;
        break;

    case ModeDuration::Ndt:
    case ModeDuration::StartStop:
    case ModeDuration::FillFactor:
    case ModeDuration::Phase:
        items[2] = ModesChannel::switchTimeLabels; 
        items[3] = nullptr;
        break;
    }

    Relation::Off();

    ModeDuration::LoadToFPGA();
}

// ����� ������ ��������� ������������ ���������, ����������, ������������ ����������, �������� ���
DEF_SWITCH_5(sModeDuration,
    "�����", "Mode",
    "��������� ������������", "Duration measurement",
    "ndt", "ndt/1��", "��������", "�����. ���.", "����",
    "ndt", "ndt/1��", "Interval", "Fill factor", "Phase",
    Channel::A->mod.modeDuration, OnChanged_ModeDuration
);


Switch *switchModeDurationA = &sModeDuration;


static void OnChanged_ModeCountPulse()
{
    items[1] = &sModeCountPulse;
    items[2] = nullptr;

    switch (Channel::A->mod.modeCountPulse.value)
    {
    case ModeCountPulse::AtB:
        break;

    case ModeCountPulse::ATB:
        items[2] = ModesChannel::switchNumberPeriods;
        items[3] = nullptr;
        break;

    case ModeCountPulse::StartStop:
        break;
    }

    Relation::Off();

    ModeCountPulse::LoadToFPGA();
}

// ����� ������ ����� ���������
DEF_SWITCH_3(sModeCountPulse,
    "�����", "Mode",
    "���� ����� ���������", "Pulse counting",
    "�(tB)", "�(TB)", "�����/����",
    "�(tB)", "�(TB)", "Start/Stop",
    Channel::A->mod.modeCountPulse, OnChanged_ModeCountPulse
);


Switch *switchModeCountPulseA = &sModeCountPulse;


static Item *items[7] =
{
    &sTypeMeasure,
    &sModeFrequency,
    ModesChannel::switchTimeMeasue,
    nullptr
};


static void OnChanged_ModeTest()
{
    bool test = FreqMeter::modeTest.IsEnabled();

    switch(Channel::A->mod.typeMeasure.value)
    {
    case TypeMeasure::Frequency:
        if (Channel::A->mod.modeFrequency.IsFrequency())
        {
            items[3] = test ? ModesChannel::switchTimeLabels : nullptr;
        }
        else if (Channel::A->mod.modeFrequency.IsTachometer())
        {
            items[2] = test ? ModesChannel::switchTimeLabels : nullptr;
        }
        break;

    case TypeMeasure::Period:
        items[3] = test ? ModesChannel::switchTimeLabels : nullptr;
        break;
    }

    Channel::A->pageModes->VerifySelectedItem();
}

static void OnEvent(EventType::E event)
{
    if (event == EventType::ModeTestChanged)
    {
        OnChanged_ModeTest();
    }
}


PageModes pageModesA(items, OnEvent);
