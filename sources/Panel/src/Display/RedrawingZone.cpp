// (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Settings.h"
#include "Calculate/ValueFPGA.h"
#include "Display/Display.h"
#include "Display/Indicator.h"
#include "Display/RedrawingZone.h"
#include "Display/Primitives.h"
#include "Display/Text.h"
#include "Display/Font/Font.h"
#include "Menu/Menu.h"
#include "Menu/Pages/PageIndication.h"
#include "Menu/Pages/Channels/Channels.h"
#include "Utils/Math.h"
#include "Utils/String.h"
#include "Utils/StringUtils.h"
#include "Menu/Pages/Pages.h"
#include <cctype>
#include <cstdlib>
#include <cstring>


using namespace Primitives;


uint ProgressBarTimeMeasureZone::timeStart = 0;
uint SynchroZone::timeStart = 0;


void RedrawingZone::Update(RedrawingZone::ModeDraw::E mode)
{
    modeDraw = mode;

    x0 = (mode == ModeDraw::ToHardware) ? 0 : left;
    y0 = (mode == ModeDraw::ToHardware) ? 0 : top;

    if (mode == RedrawingZone::ModeDraw::ToBuffer)
    {
        if (Display::InDrawingPart(top, height))
        {
            Draw();
        }
    }
    else
    {
        if (needUpdate)
        {
            BeforeDraw();

            Display::Sender::Prepare(width, height);

            FillBackground();

            if (Draw())
            {
                needUpdate = false;
            }

            Display::Sender::SendToFSMC(left, top);

            Display::Sender::Restore();

            AfterDraw();
        }
    }
}


void RedrawingZone::Refresh()
{
    if (PageIndication::calibrationMode.IsEnabled())
    {
        return;
    }

    if (Menu::OpenedPage() == PageStatistics::self)
    {
        if (this == Display::zoneProgressBarTimeMeasure)
        {
            return;
        }
    }
    else if (Menu::OpenedPage()->IsAddition())
    {
        return;
    }

    needUpdate = true;
}


void RedrawingZone::FillBackground()
{
    Display::BeginScene(left, top);
}


static bool ConditionSplit(char *text)
{
    if (ValueFPGA::IsEmpty())
    {
        return false;
    }

    return !(SU::ExistSymbol(text, '.') || SU::ExistSymbol(text, ','));
}


#define RIGHT_DIGITS 360
#define LEFT_UNTIS   380


bool DataZone::Draw()
{
    String data = ValueFPGA::GiveDigits();

    Color::FILL.SetAsCurrent();

    if (data[0] != 0)
    {
        if (ValueFPGA::IsData())
        {
            if (glob_set.styleGUI.IsModern())
            {
                FontBig::WriteAboutRight(data.c_str(), x0 + RIGHT_DIGITS, y0, ConditionSplit(data.c_str()));
            }
            else
            {
                Indicator::DrawDataAboutRight(data.c_str(), x0 + RIGHT_DIGITS, y0 + 1, Color::FILL, Color::EMPTY);
            }
        }
        else
        {
            int x = x0;

            if (ValueFPGA::IsOverlapped())   { x += 40; }   // ������������
            else if (ValueFPGA::IsDivNULL()) { x += 150; }  // ������� �� ����

            if (glob_set.styleGUI.IsModern())
            {
                Font::Set(TypeFont::GOSTB28B);
                Text(data.c_str()).Write(x, y0 + 15);
                Font::Set(TypeFont::GOSTAU16BOLD);
            }
            else
            {
                Indicator::DrawDataAboutRight(data.c_str(), RIGHT_DIGITS, y0 + 1, Color::FILL, Color::EMPTY);
            }
        }
    }

    FontMid::Write(ValueFPGA::GiveUnits().c_str(), x0 + LEFT_UNTIS, y0 + 20);

    return true;
}


bool ProgressBarTimeMeasureZone::Draw()
{
    if (Channel::Current()->mod.ConsistTimeMeasure() && ModesChannel::timeMeasure.value > TimeMeasure::_10ms)
    {
        int timeCycle = ModesChannel::timeMeasure.ToMS();

        float part = ((float)(TIME_MS - timeStart) / (float)timeCycle);

        int w = (int)(46.0F * part) + 1;

        LIMITATION(w, 0, Width() - 1);

        Primitives::Rectangle(w, 5).Fill(x0, y0, Color::FILL);
    }

    return true;
}


void ProgressBarTimeMeasureZone::Reset()
{
    timeStart = TIME_MS;
}

void SynchroZone::BeforeDraw()
{
    oldColor = Color::FILL.Value();
}


void SynchroZone::AfterDraw()
{
    Color::FILL.SetValue(oldColor);
}


bool SynchroZone::Draw()
{
    int size = CalculateSize();

    if (size > 0)
    {
        int d = (MAX_SIZE - size) / 2;

        Primitives::Rectangle(size, size).Fill(x0 + d, y0 + d, Color::FILL);
    }

    return true;
}


uint SynchroZone::CalculateColor()
{
    uint8 color = (uint8)((1.0F - CalculateRelativeTime() * CalculateRelativeTime()) * 0xFF);

    return Color::Make(color, color, color);
}


int SynchroZone::CalculateSize()
{
//    if (CalculateRelativeTime() < 1.0F)
//    {
//        return MAX_SIZE;
//    }
//
//    return 0;

    return (int)(MAX_SIZE * (1.0F - CalculateRelativeTime()));
}


float SynchroZone::CalculateRelativeTime()
{
    const float timeLife = 2000.0F;

    float time = (float)(TIME_MS - timeStart);

    LIMITATION_ABOVE(time, timeLife);

    return (time / timeLife);
}


void SynchroZone::Fire()
{
    timeStart = TIME_MS;
}
