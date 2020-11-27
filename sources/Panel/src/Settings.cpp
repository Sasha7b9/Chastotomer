#include "defines.h"
#include "Settings.h"
#include "Hardware/FPGA.h"
#include "Menu/Pages/Channels/Channels.h"
#include "Utils/Math.h"


Settings set =
{
    {
        {0, 1}, {0, 1}                          // levelSynch
    },
    { 
        TypeSynch::Manual, TypeSynch::Manual    // typeSynch
    },
    false
};


void InputCouple::LoadToFPGA()
{
    if (CURRENT_CHANNEL_IS_A_OR_B)
    {
        Command command(Command::Couple);

        if (Channel::Current()->couple.IsDC())
        {
            command.SetBit(9);
        }

        FPGA::WriteCommand(command);
    }
}


void InputCouple::Set(InputCouple::E v)
{
    value = (uint8)v;

    LoadToFPGA();
}


void LevelSynch::Change(int delta)
{
    if (CURRENT_CHANNEL_IS_A_OR_B)
    {
        int prev = LEVEL_SYNCH(CURRENT_CHANNEL);

        LEVEL_SYNCH(CURRENT_CHANNEL) += delta;

        int MIN = -800;
        int MAX = 800;

        if (TypeSynch::Current().IsHoldoff())
        {
            MIN = 1;
        }

        LIMITATION(LEVEL_SYNCH(CURRENT_CHANNEL), MIN, MAX);

        if (prev != LEVEL_SYNCH(CURRENT_CHANNEL))
        {
            if (delta < 0) { FPGA::DecreaseN(); }
            else           { FPGA::IncreaseN(); }

            FPGA::WriteDataGovernor();
        }
    }
}


const ModeFront &ModeFront::Current()
{
    static const ModeFront null(Count);

    static const ModeFront *modes[Channel::Count] = { &PageSettingsA::modeFront, &PageSettingsB::modeFront, &null, &null };

    return *modes[CURRENT_CHANNEL];
}


void ModeFront::LoadToFPGA()
{
    if (CURRENT_CHANNEL_IS_A_OR_B)
    {
        Command command(Command::ModeFront);

        if (!Current().IsFront())
        {
            command.SetBit(9);
        }

        FPGA::WriteCommand(command);
    }
}


void TypeSynch::LoadToFPGA()
{
    if (CURRENT_CHANNEL_IS_A_OR_B)
    {
        Command command(Command::TypeSynch);

        if (TypeSynch::Current().IsHoldoff())
        {
            command.SetBit(9);
        }

        FPGA::WriteCommand(command);
    }
}


const TypeSynch &TypeSynch::Current()
{
    static const TypeSynch null(Count);

    static const TypeSynch *types[Channel::Count] = { &PageSettingsA::typeSynch, &PageSettingsB::typeSynch, &null, &null };

    return *types[CURRENT_CHANNEL];
}


void ModeFilter::Set(ModeFilter::E v)
{
    if (CURRENT_CHANNEL_IS_A_OR_B)
    {
        Channel::Current()->modeFilter.value = (uint8)v;

        LoadToFPGA();
    }
}


void ModeFilter::LoadToFPGA()
{
    if (CURRENT_CHANNEL_IS_A_OR_B)
    {
        Command command(Command::ModeFilter);

        if (Channel::Current()->modeFilter.IsOff())
        {
            command.SetBit(9);
        }

        FPGA::WriteCommand(command);
    }
}


const Divider &Divider::Current()
{
    static const Divider null(Count);

    static const Divider *dividers[Channel::Count] = { &PageSettingsA::divider, &PageSettingsB::divider, &null, &null };

    return *dividers[CURRENT_CHANNEL];
}


void Divider::LoadToFPGA()
{
    if (CURRENT_CHANNEL_IS_A_OR_B)
    {
        Command command(Command::Divider);

        if (!Divider::Current().Is1())
        {
            command.SetBit(9);
        }

        FPGA::WriteCommand(command);
    }
}
