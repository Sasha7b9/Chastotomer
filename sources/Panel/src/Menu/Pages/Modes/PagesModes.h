#pragma once
#include "Settings.h"
#include "Menu/MenuItems.h"
#include "Menu/Pages/Modes/Modes.h"


struct CurrentPageModes
{
    // ���������� true, ���� ������� �������� ������� �������� ����� ���������
    static bool ConsistTimeMeasure();
};


// ����� ������� ����� ��� ���� ������� ������� �������
class PageModes
{
public:

    static void OnChanged_TypeMeasure();

    static void DrawParameters(int x, int y);

    static MenuPage *Current();

    static MenuPage *PageForChannel(Channel::E);

    static TimeMeasure       timeMeasure;    // ����� �����
    static NumberPeriods     numberPeriods;  // ����� �������� ���������
    static PeriodTimeLabels  timeLabels;     // ������ ����� �������

    static Switch *switchTimeMeasue;
    static Switch *switchNumberPeriods;
    static Switch *switchTimeLabels;

private:
    static bool IsActiveTimeLabels(TypeMeasure *type, int mode);
    static bool IsActiveTimeMeasure(TypeMeasure *type, int mode);
    static bool IsActiveNumberPeriods(TypeMeasure *type, int mode);
};


class PageModesA
{
public:

    static MenuPage *self;
    
    static void RelationOn();
    static void RelationOff();
    static bool RelationCheck();

    static void PressSetup();
    static bool StartStop();
    static void ToggleStartStop();

    // ���������� ��� ���������� ���� ���������
    static void OnChanged_TypeMeasure();

    // ���������� ��� ��������� ������� ���������
    static void OnChanged_ModeFrequency();
    static void OnChanged_ModePeriod();
    static void OnChanged_ModeDuration();
    static void OnChanged_ModeCountPulse();

    static TypeMeasure           typeMeasure;                // ��� ���������
    static ModeMeasureFrequency  modeMeasureFrequency;       // ����� ��������� �������
    static ModeMeasurePeriod     modeMeasurePeriod;          // ����� ��������� �������
    static ModeMeasureDuration   modeMeasureDuration;        // ����� ��������� ������������
    static ModeMeasureCountPulse modeMeasureCountPulse;      // ����� ����� ���������
};


struct PageModesB
{
    static MenuPage *self;

    static void PressSetup();

    // ���������� ��� ���������� ���� ���������
    static void OnChanged_TypeMeasure();

    // ���������� ��� ��������� ������� ���������
    static void OnChanged_ModeFrequency();
    static void OnChanged_ModePeriod();
    static void OnChanged_ModeDuration();
    static void OnChanged_ModeCountPulse();

    static TypeMeasure           typeMeasure;           // ��� ���������
    static ModeMeasureFrequency  modeMeasureFrequency;  // ����� ��������� �������
    static ModeMeasurePeriod     modeMeasurePeriod;     // ����� ��������� �������
    static ModeMeasureDuration   modeMeasureDuration;   // ����� ��������� ������������
    static ModeMeasureCountPulse modeMeasureCountPulse; // ����� ����� ���������
};


struct PageModesC
{
    static MenuPage *self;

    // ���������� ��� ���������� ���� ���������
    static void OnChanged_TypeMeasure();

    // ���������� ��� ��������� ������� ���������
    static void OnChanged_ModeFrequency();
    static void OnChanged_ModeCountPulse();

    static TypeMeasure           typeMeasure;           // ��� ���������
    static ModeMeasureFrequency  modeMeasureFrequency;  // ����� ��������� �������
    static ModeMeasureCountPulse modeMeasureCountPulse; // ����� ����� ���������
};



class PageModesD
{
public:
    static MenuPage *self;

    static TypeMeasure          typeMeasure;
    static ModeMeasureFrequency modeMeasureFrequency;
};
