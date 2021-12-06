// speaker.h

#ifndef SPEAKER_H_
#define SPEAKER_H_

#include <stdint.h>
#include <stdbool.h>

void initspeakerHw();
void timer2Isr();
void playAlert();

#endif /* SPEAKER_H_ */
