                                                                                                                                                                                                                                                                                                                                    /*
 * speaker.h
 *
 *  Created on: Dec 1, 2021
 *      Author: Arcan
 */

#ifndef SPEAKER_H_
#define SPEAKER_H_

#include <stdint.h>
#include <stdbool.h>

void initspeakerHw();
void timer2Isr();
void playAlert();




#endif /* SPEAKER_H_ */
