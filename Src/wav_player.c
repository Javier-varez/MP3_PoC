/*
 * wav_player.c
 *
 *  Created on: 16 sept. 2018
 *      Author: javi
 */

#include "stm32f411e_discovery_audio.h"
#include "stm32f4xx_hal.h"
#include "ff.h"

#define DEFAULT_VOLUME_LEVEL	65
#define BUFFER_LENGTH_BYTES		32768

typedef struct __attribute__((packed)) {
	char chunkID[4];		// 0
	uint32_t chunkSize;		// 4
	char format[4];			// 8
	char subchunkID1[4];	// 12
	uint32_t subchunk1Size;	// 16
	uint16_t audioFormat;	// 20
	uint16_t numChannels;	// 22
	uint32_t sampleRate;	// 24
	uint32_t byteRate;		// 28
	uint16_t blockAlign;	// 32
	uint16_t bitsPerSample;	// 34
	char subchunkID2[4];	// 36
	uint32_t subchunk2Size;	// 40
	// uint8_t data[subchunk2Size]; // 44
} WAV_Header_t;

FIL currentSong;
WAV_Header_t songHeader;
uint16_t playbackBuffer[BUFFER_LENGTH_BYTES/2];

typedef enum {
	IDLE,
	TRANSFER_COMPLETE,
	HALF_TRANSFER,
}playbackState_t;

volatile playbackState_t playbackState = IDLE;

static int verifyHeader(WAV_Header_t *header) {
	int rc = 0;
	 if (header->audioFormat != 1) {
		printf("Audio format is not PCM\r\n");
		rc = -1;
	} else {
		printf("numChannels = %d\r\n", header->numChannels);
		printf("sampleRate = %u\r\n", (unsigned int) header->sampleRate);
		printf("bitsPerSample = %d\r\n", header->bitsPerSample);
		printf("byteRate = %u\r\n", (unsigned int) header->byteRate);
	}

	return rc;
}

static int playWAVSongInternal(FIL *songFile, WAV_Header_t *header) {
	int rc = 0;

	rc = BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, DEFAULT_VOLUME_LEVEL, header->sampleRate);
	if (AUDIO_OK != rc) {
		printf("Error initializing audio device\r\n");
		return -1;
	}

	UINT bytesRead = 0;
	rc = f_read(songFile, playbackBuffer, BUFFER_LENGTH_BYTES, &bytesRead);
	if (rc == FR_OK) {
		if (AUDIO_OK != BSP_AUDIO_OUT_Play(playbackBuffer, BUFFER_LENGTH_BYTES)) {
			printf("error starting audio playback\r\n");
			rc = -1;
		}
	} else {
		printf("error reading data\r\n");
		rc = -1;
	}

	return rc;
}

int playWAVSong(char *path) {
	int rc = 0;
	rc = f_open(&currentSong, path, FA_READ);

	if (rc == FR_OK) {
		UINT readLen = 0;
		rc = f_read(&currentSong, &songHeader, sizeof(songHeader), &readLen);

		if (rc == FR_OK) {
			rc = verifyHeader(&songHeader);
			if (rc == 0) {
				rc = playWAVSongInternal(&currentSong, &songHeader);
			}
		}
	}

	return rc;
}

int stopWAVSong() {
	BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);
	f_close(&currentSong);

	return 0;
}

void playWork() {
	UINT bytesRead = 0;

	switch (playbackState) {
	case TRANSFER_COMPLETE:
		f_read(&currentSong, &playbackBuffer[BUFFER_LENGTH_BYTES/4], BUFFER_LENGTH_BYTES/2, &bytesRead);
		playbackState = IDLE;
		break;
	case HALF_TRANSFER:
		f_read(&currentSong, &playbackBuffer[0], BUFFER_LENGTH_BYTES/2, &bytesRead);
		playbackState = IDLE;
		break;
	case IDLE:
		break;
	default:
		break;
	}
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void) {
	BSP_AUDIO_OUT_ChangeBuffer(playbackBuffer, BUFFER_LENGTH_BYTES/2);
	playbackState = TRANSFER_COMPLETE;
}

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void) {
	playbackState = HALF_TRANSFER;
}


void BSP_AUDIO_OUT_Error_CallBack(void) {
	printf("BSP_AUDIO_OUT_Error_CallBack\r\n");
}
