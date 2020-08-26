/* main.c --- 
 * 
 * Filename: main.c
 * Description: 
 * Author: 
 * Maintainer: 
 * Created: 
 * Last-Updated: 
 *           By: 
 *     Update #: 0
 * Keywords: 
 * Compatibility: 
 * 
 */

/* Commentary: 
 * 
 * 
 * 
 */

/* Change log:
 * 
 * 
 */
/* Code: */

#include <stm32f30x.h> // Pull in include files for F30x standard drivers 
#include <f3d_led.h> // Pull in include file for the local drivers
#include <f3d_uart.h> 
#include <f3d_i2c.h> 
#include <f3d_rtc.h> 
#include <f3d_systick.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>

#define TIMER 20000
#define AUDIOBUFSIZE 128

extern uint8_t Audiobuf[AUDIOBUFSIZE];
extern int audioplayerHalf;
extern int audioplayerWhole;

extern unsigned char __500hz_8_wav[];

struct ckhd {
  uint32_t ckID;
  uint32_t cksize;
};

struct fmtck {
  uint16_t wFormatTag;
  uint16_t nChannels;
  uint32_t nSamplesPerSec;
  uint32_t nAvgBytesPerSec;
  uint16_t nBlockAlign;
  uint16_t wBitsPerSample;
};

struct ckhd readckhd2(unsigned char * data, struct ckhd * hd, uint32_t ckID);

struct ckhd readckhd2(unsigned char * data, struct ckhd * hd, uint32_t ckID) {
  hd = (struct ckhd * ) data;
  printf("ckID: %x\n",ckID );
  printf("hd->ckID: %x\n", hd->ckID);
  printf("cksize: %i\n",hd->cksize);
  if (ckID && (ckID != hd->ckID))
    exit(-1);
  return *hd;
}

int main(void) {

  unsigned int retval;
  int bytesread;
  unsigned char * datapos = __500hz_8_wav;

  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  f3d_timer2_init();
  f3d_dac_init();
  f3d_delay_init();
  f3d_rtc_init();
  f3d_systick_init();
  f3d_uart_init();

  printf("Reset\n");

  struct ckhd hd;
  uint32_t waveid;
  struct fmtck fck;

  hd  = readckhd2(datapos, &hd, 'FFIR');
  datapos += sizeof(struct ckhd);

  waveid = * ((uint32_t * ) datapos);
  datapos += sizeof(waveid);
  if (waveid != 'EVAW')
    return -1;

  hd = readckhd2(datapos, &hd, ' tmf');
  datapos += sizeof(struct ckhd);

  fck = * ((struct fmtck * ) datapos);
  datapos += sizeof(fck);

  printf("audio format 0x%x\n", fck.wFormatTag);
  printf("channels %d\n", fck.nChannels);
  printf("sample rate %d\n", fck.nSamplesPerSec);
  printf("data rate %d\n", fck.nAvgBytesPerSec);
  printf("block alignment %d\n", fck.nBlockAlign);
  printf("bits per sample %d\n", fck.wBitsPerSample);

  // now skip all non-data chunks !

  while (1) {
    hd = readckhd2(datapos, &hd, 0);
    datapos += sizeof(struct ckhd);
    if (hd.ckID == 'atad')
      break;
    datapos += hd.cksize;
  }

  printf("Samples %d\n", hd.cksize);

  // Play it !

  memcpy(Audiobuf, datapos, AUDIOBUFSIZE);
  datapos += AUDIOBUFSIZE;
  hd.cksize -= AUDIOBUFSIZE;
  audioplayerStart();
  while (hd.cksize) {
    int next = hd.cksize > AUDIOBUFSIZE / 2 ? AUDIOBUFSIZE / 2 : hd.cksize;
    if (audioplayerHalf) {
      if (next < AUDIOBUFSIZE / 2)
        bzero(Audiobuf, AUDIOBUFSIZE / 2);
      memcpy(Audiobuf, datapos, next);
      datapos += next;
      hd.cksize -= next;
      audioplayerHalf = 0;
    }
    if (audioplayerWhole) {
      if (next < AUDIOBUFSIZE / 2)
        bzero(&Audiobuf[AUDIOBUFSIZE / 2], AUDIOBUFSIZE / 2);
      memcpy(&Audiobuf[AUDIOBUFSIZE / 2], datapos, next);
      datapos += next;
      hd.cksize -= next;
      audioplayerWhole = 0;
    }
  }
  audioplayerStop();

  printf("\nClose the file.\n");

  while (1);
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t * file, uint32_t line) {
  /* Infinite loop */
  /* Use GDB to find out why we're here */
  while (1);
}
#endif

/* main.c ends here */
