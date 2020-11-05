#pragma once
#include "defines.h"


struct PSymbol
{
    uchar width;
    uchar bytes[8];
};

struct PTypeFont
{
    enum E
    {
        GOST16B,
        Count,
        None
    } value;
};

struct Font
{
    int _height;
    PSymbol symbols[256];

public:
    // ������������� ������� �����. ����� ������������� ����� ������������ �������� Pop()
    static void Set(const PTypeFont::E typeFont);
    static PTypeFont::E Current();
    // ��������������� �����, ������ ������� ����� ��������� ������� SetCurrent()
    static void Pop();
    static uint8 GetWidth(uint8 symbol);
    static uint8 GetWidth(char symbol);
    static uint8 GetHeight();
    static bool RowNotEmpty(uint8 symbol, int row);
    static bool BitIsExist(uint8 symbol, int row, int bit);
    // ������������� ���������� ������ �������� ����� ���������.
    static void SetSpacing(int spacing);
    static int GetSpacing();
};
