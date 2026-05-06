
#ifndef __LC3_PLAY_H
#define __LC3_PLAY_H

#include <stdio.h>
#include <stdint.h>
#include <lc3.h>


int lc3_decoder_init(void *fp);

int l3c_decode_play(void *fp);

void lc3_decode_stop(void);

#endif /* __LC3_PLAY_H */
