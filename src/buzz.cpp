#include <tone32.h>
#include "periphery.h"

void buzzer()
{

    tone(BUZZER_PIN, NOTE_C5, 100, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
}

void buzzStartup()
{
    tone(BUZZER_PIN, NOTE_F5, 100, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_A5, 100, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_C6, 100, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_E6, 100, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_G6, 100, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_F6, 100, BUZZER_CHANNEL);
}

void buzzNote(int note)
{

    tone(BUZZER_PIN, note, 50, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
}

void buzzMultipleTimes(int x)
{

    for (int i = 0; i < x; i++)
    {
        tone(BUZZER_PIN, NOTE_C5, 100, BUZZER_CHANNEL);
        noTone(BUZZER_PIN, BUZZER_CHANNEL);
        tone(BUZZER_PIN, NOTE_C1, 100, BUZZER_CHANNEL);
        noTone(BUZZER_PIN, BUZZER_CHANNEL);
    }
}

void buzzAlarm()
{

    tone(BUZZER_PIN, NOTE_A5, 50, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_A5, 50, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
    tone(BUZZER_PIN, NOTE_A5, 50, BUZZER_CHANNEL);
    noTone(BUZZER_PIN, BUZZER_CHANNEL);
}