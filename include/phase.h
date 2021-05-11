#ifndef YARC_PHASE_H
#define YARC_PHASE_H

enum phase
{
    IDLE = 0,
    PREHEAT = 1,
    SOAK = 2,
    REFLOW = 3,
    COOLDOWN = 4

};
extern phase currentPhase;

#endif