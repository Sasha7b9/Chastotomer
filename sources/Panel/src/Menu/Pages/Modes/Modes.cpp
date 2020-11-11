#include "Settings.h"
#include "Menu/Pages/Modes/PagesModes.h"


ModeMeasureCountPulse &ModeMeasureCountPulse::Current()
{
    static const bool correct[1] = { true };
    static ModeMeasureCountPulse def(ModeMeasureCountPulse::Count, correct, 1);

    static ModeMeasureCountPulse *const modes[Channel::Count] =
    {
        &PageModesA::modeMeasureCountPulse,
        &PageModesB::modeMeasureCountPulse,
        &PageModesC::modeMeasureCountPulse,
        &def
    };

    return *modes[CURRENT_CHANNEL];
}


bool ModeMeasureCountPulse::IsFromPeriod() const
{
    return (value == ATB) || (value == BTA) || (value == CTA) || (value == CTB);
}


TypeMeasure &TypeMeasure::Current()
{
    static TypeMeasure *const types[TypeMeasure::Count] =
    {
        &PageModesA::typeMeasure,
        &PageModesB::typeMeasure,
        &PageModesC::typeMeasure,
        &PageModesD::typeMeasure
    };

    return *types[CURRENT_CHANNEL];
}
