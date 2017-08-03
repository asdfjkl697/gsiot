// A sample to test record and playback.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "sndtools.h"

int main() {
	char *buf;
	int dwSize;
	if (!OpenSnd()) {
		printf("Open sound device error!\\n");
		return -1;
	}
//	SetFormat(FMT16BITS, FMT8K, WAVOUTDEV);
//	SetChannel(MONO, WAVOUTDEV);
	SetFormat(FMT16BITS, FMT8K);
	SetChannel(MONO);
	buf = (char *) malloc(320);
	if (buf == NULL)
		exit(-1);
	for (int i = 0; i < 1000; i++) {
		dwSize = Record(buf, 640);
		dwSize = Play(buf, dwSize);
	}
	return 1;
}
