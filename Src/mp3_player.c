/*
 * mp3_player.c
 *
 *  Created on: 9 February 2019
 *      Author: javi
 */

#include "stm32f411e_discovery_audio.h"
#include "stm32f4xx_hal.h"
#include "ff.h"
#include "spiritMP3Dec.h"

#define DEFAULT_VOLUME_LEVEL	65
#define BUFFER_LENGTH_BYTES		(576*16/*32768*/)

FIL currentSong;
uint16_t playbackBuffer[BUFFER_LENGTH_BYTES/2];

typedef enum {
	IDLE,
	TRANSFER_COMPLETE,
	HALF_TRANSFER,
}Mp3playbackState_t;

static volatile Mp3playbackState_t playbackState = IDLE;
TSpiritMP3Decoder decoder;

static int StartPlaybackFromFirstFrame() {
	int rc = 0;

	TSpiritMP3Info mp3Info;

	if (SpiritMP3Decode(&decoder, (short*)&playbackBuffer[0], BUFFER_LENGTH_BYTES/4, &mp3Info) != BUFFER_LENGTH_BYTES/4)
	{
		// Error, EOF
		printf("found end of file too soon... error.\r\n");
		return -1;
	}

	printf("Using sample rate %d\r\n", mp3Info.nSampleRateHz);
	rc = BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, DEFAULT_VOLUME_LEVEL, mp3Info.nSampleRateHz);
	if (AUDIO_OK != rc) {
		printf("Error initializing audio device\r\n");
		return -1;
	}

	if (AUDIO_OK != BSP_AUDIO_OUT_Play(playbackBuffer, BUFFER_LENGTH_BYTES)) {
		printf("error starting audio playback\r\n");
		rc = -1;
	}
	return rc;
}

static unsigned int getDataFromFile(void* mp3data, unsigned int size, void* token) {
	unsigned int read;

	if (f_read(&currentSong, mp3data, size, &read) != FR_OK)
	{
		printf("error reading file\r\n");
		return 0;
	}

	return read;
}

int playMP3Song(char *path) {
	int rc = 0;
	rc = f_open(&currentSong, path, FA_READ);

	  /* Enables and resets CRC-32 from STM32 HW */
	  __HAL_RCC_CRC_CLK_ENABLE();
	  CRC->CR = CRC_CR_RESET;

	if (rc == FR_OK) {
		/* Initialize MP3 decoder */
		SpiritMP3DecoderInit(&decoder, getDataFromFile, NULL, NULL);
		StartPlaybackFromFirstFrame();
	}

	return rc;
}

int stopMP3Song() {
	BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW);
	f_close(&currentSong);

	return 0;
}

void playMP3Task() {
	UINT bytesRead = 0;

	switch (playbackState) {
	case TRANSFER_COMPLETE:
		SpiritMP3Decode(&decoder, (short*)&playbackBuffer[BUFFER_LENGTH_BYTES/4], BUFFER_LENGTH_BYTES/8, NULL);
		playbackState = IDLE;
		break;
	case HALF_TRANSFER:
		SpiritMP3Decode(&decoder, (short*)&playbackBuffer[0], BUFFER_LENGTH_BYTES/8, NULL);
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
