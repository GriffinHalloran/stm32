
#include <stm32f30x.h>  // Pull in include files for F30x standard drivers 
#include <f3d_led.h>     // Pull in include file for the local drivers
#include <f3d_uart.h>
#include <f3d_gyro.h>
#include <f3d_lcd_sd.h>
#include <f3d_i2c.h>
#include <f3d_accel.h>
#include <f3d_mag.h>
#include <f3d_nunchuk.h>
#include <f3d_rtc.h>
#include <f3d_systick.h>
#include <ff.h>
#include <diskio.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <f3d_user_btn.h> 
#include "bmp.h"

#define TIMER 20000
#define AUDIOBUFSIZE 128

extern uint8_t Audiobuf[AUDIOBUFSIZE];
extern int audioplayerHalf;
extern int audioplayerWhole;
struct bmppixel pixel;
struct bmpfile_magic magic;
BITMAPINFOHEADER info;
struct bmpfile_header header; 

FATFS Fatfs;		/* File system object */
FIL fid;		/* File object */
BYTE Buff[512];		/* File read buffer */
int ret;
nunchuk_t nun;

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

void readckhd(FIL *fid, struct ckhd *hd, uint32_t ckID) {
  f_read(fid, hd, sizeof(struct ckhd), &ret);
  if (ret != sizeof(struct ckhd))
    exit(-1);
  if (ckID && (ckID != hd->ckID))
    exit(-1);
}

void die (FRESULT rc) {
  //printf("Failed with rc=%u.\n", rc);
  while (1);
}



int audio(char p[]) {
  FRESULT rc;                   /* Result code */
  DIR dir;                      /* Directory object */
  FILINFO fno;                  /* File information object */
  UINT bw, br;
  unsigned int retval;
  int bytesread;


  //printf("Reset\n"); 
  f_mount(0, &Fatfs);/* Register volume work area */

  rc = f_open(&fid, p, FA_READ);
  if (rc) die(rc);  
  if (!rc) {
    struct ckhd hd;
    uint32_t  waveid;
    struct fmtck fck;
    
    readckhd(&fid, &hd, 'FFIR');
    
    f_read(&fid, &waveid, sizeof(waveid), &ret);
    if ((ret != sizeof(waveid)) || (waveid != 'EVAW'))
      return -1;
    
    readckhd(&fid, &hd, ' tmf');
    
    f_read(&fid, &fck, sizeof(fck), &ret);
    
    // skip over extra info
        
    if (hd.cksize != 16) {
        //printf("extra header info %d\n", hd.cksize - 16);
        f_lseek(&fid, hd.cksize - 16);
    }
    /*                            
    printf("audio format 0x%x\n", fck.wFormatTag);
    printf("channels %d\n", fck.nChannels);
    printf("sample rate %d\n", fck.nSamplesPerSec);
    printf("data rate %d\n", fck.nAvgBytesPerSec);
    printf("block alignment %d\n", fck.nBlockAlign);
    printf("bits per sample %d\n", fck.wBitsPerSample);
     */                                         
    // now skip all non-data chunks !
                                                       
    while(1){
        readckhd(&fid, &hd, 0);
        if (hd.ckID == 'atad')
            break;
        f_lseek(&fid, hd.cksize);
        }
                                                                                                         
    //printf("Samples %d\n", hd.cksize);

    // Play it !
    //      audioplayerInit(fck.nSamplesPerSec);

    f_read(&fid, Audiobuf, AUDIOBUFSIZE, &ret);
    hd.cksize -= ret;
    audioplayerStart();
    while (hd.cksize) {
      int next = hd.cksize > AUDIOBUFSIZE/2 ? AUDIOBUFSIZE/2 : hd.cksize;
      if (audioplayerHalf) {
        if (next < AUDIOBUFSIZE/2)
          bzero(Audiobuf, AUDIOBUFSIZE/2);
        f_read(&fid, Audiobuf, next, &ret);
        hd.cksize -= ret;
        audioplayerHalf = 0;
      }
      if (audioplayerWhole) {
        if (next < AUDIOBUFSIZE/2)
          bzero(&Audiobuf[AUDIOBUFSIZE/2], AUDIOBUFSIZE/2);
        f_read(&fid, &Audiobuf[AUDIOBUFSIZE/2], next, &ret);
        hd.cksize -= ret;
        audioplayerWhole = 0;
      }
    }
    audioplayerStop();
  }

  //printf("\nClose the file.\n"); 
  rc = f_close(&fid);

  if (rc) die(rc);
}


int draw_ball(int g, int i) {
	f3d_lcd_square(g, i, g+5, i + 5, WHITE);
}

void remove_ball(int g, int i){
	f3d_lcd_square(g, i, g + 5, i + 5, GREEN);	
}	


void level_1_bound() {
  f3d_lcd_square(0, 10, 150, 15, BLACK);
 

  f3d_lcd_square(30, 80, 40, 120, BLACK);

  f3d_lcd_square(80, 50, 150, 60, BLACK);

  f3d_lcd_square(40, 110, 80, 120, BLACK);


}

void draw_level_1(int p, int q) {
  f3d_lcd_square(0, 0, 128, 160, GREEN);
  f3d_lcd_drawString(42, 1, "LEVEL 1", BLACK, GREEN); 
  level_1_bound();	
  f3d_lcd_square(90, 27, 93, 45, YELLOW);
	

  f3d_lcd_square(87, 45, 97, 47, BLACK);

  f3d_lcd_square(93, 27, 100, 31, RED); 
  draw_ball(p, q);
}

int level_2_bound() {

  f3d_lcd_square(0, 10, 150, 15, BLACK);
  f3d_lcd_square(0, 80, 90, 90, BLACK);
  f3d_lcd_square(40, 120, 50, 160, BLACK);
  f3d_lcd_square(60, 15, 70, 35, BLACK);

}

int draw_level_2(int p, int q) {
  f3d_lcd_fillScreen(GREEN);
  f3d_lcd_drawString(42, 1, "LEVEL 2", BLACK, GREEN);

  level_2_bound();
 

  f3d_lcd_square(20, 38, 23, 55, YELLOW);
  f3d_lcd_square(17, 55, 27, 57, BLACK);
  f3d_lcd_square(23, 38, 30, 42, RED);
  draw_ball(p, q);
}

int level_3_bound(){
 f3d_lcd_square(0, 10, 150, 15, BLACK);
 f3d_lcd_square(30, 60, 40, 160, BLACK);
 f3d_lcd_square(80, 60, 90, 120, BLACK);
 f3d_lcd_square(40, 60, 90, 70, BLACK);
 }

int draw_level_3(int p, int q) {
  f3d_lcd_fillScreen(GREEN);
  f3d_lcd_drawString(42, 1, "LEVEL 3", BLACK, GREEN);

  level_3_bound();
  draw_ball(p, q);
	
 f3d_lcd_square(60, 75, 63, 93, YELLOW); 
 f3d_lcd_square(57, 93, 67, 95, BLACK);
 f3d_lcd_square(63, 75, 70, 79, RED);
} 
int draw_lines(int g, int i) {
	
    float mag[3] = {};
    float acc[3] = {};
    
    float heading1;  
    int yval = i;
    i = 0;
    int y;
    int q = 0;
    int p = 0; 
    while(1){
	delay(10);
	f3d_accel_read(acc);
        f3d_mag_read(mag);
      	for (i = 0; i < 3; i++){ 
		mag[i] = mag[i]* (180/M_PI);
	}
		
	 heading1 = atan2(mag[1],mag[0])*(180/M_PI);
	if(heading1 >= -46 && heading1 <= 4.99){ // controls ball heading so these statments need to be our predefined angles 
		//SOUTH
		q = yval + 5;
		p = q + 10;
		for(q;q < p; q++) {
			f3d_lcd_drawPixel(g + 2, q, RED);
		}        
        	if(user_btn_read()) {return 1;} 
        }else {
	
		q = yval + 5;
		p = q + 10;
		for(q;q < p; q++) {
			f3d_lcd_drawPixel(g + 2, q, GREEN);
		}        
 	
        }if (heading1 >= -90 && heading1 <= -45.99) {
		//SW
		q = g;
		p = q - 10;
		y = yval + 5;
		for(q; q > p; q--) {
			f3d_lcd_drawPixel(q, y, RED);
			y++;
		}
         if(user_btn_read()) {return 2;} 
		 
			
	} else {
		q = g;
		p = q - 10;
		y = yval+ 5;
		for(q; q > p; q--) {
			f3d_lcd_drawPixel(q, y, GREEN);
			y++;
		}
	}if (heading1 >= 5  && heading1 <= 69.99) {
		//SE
		q = g + 5;
		p = q + 10;
		y = yval + 5;
		for(q; q < p; q++) {
			f3d_lcd_drawPixel(q, y, RED);
			y++;
		}
         	if(user_btn_read()) {return 3;} 
		 
	} else {
		q = g + 5;
		p = q + 10;
		y = yval+ 5;
		for(q; q < p; q++) {
			f3d_lcd_drawPixel(q, y, GREEN);
			y++;
		}
	}if(heading1 >= 70 && heading1 <= 122.99){
		//EAST
		q = g+5;
		p = q + 10;
        	for(q; q < p; q++) {
			f3d_lcd_drawPixel(q, yval + 2, RED);
		} 
		 if(user_btn_read()) {return 4;} 
		 
        }else {
		q = g+5;
		p = q + 10;
        	for(q; q < p; q++) {
			f3d_lcd_drawPixel(q, yval + 2, GREEN);
		} 
        }if(heading1 >= 123 && heading1 <= 143.99) {
		//NE
		q = yval;
		p = q - 10;
		y = g + 5;
		for(q; q > p; q--) {
			f3d_lcd_drawPixel(y, q, RED);
			y++;
		}
		if(user_btn_read()) {return 5; }
			
	} else {
		q = yval;
		p = q - 10;
		y = g+ 5;
		for(q; q > p; q--) {
			f3d_lcd_drawPixel(y, q, GREEN);
			y++;
		}
	}if((heading1 <= 180.00 && heading1 >=144)){
		//NORTH
		q = yval;
		p = q - 10;
        	for(q; q > p; q--) {
			f3d_lcd_drawPixel(g + 2, q, RED);
		} 
		if(user_btn_read()) {return 6;} 
		 
        }else {
		q = yval;
		p = q - 10;
        	for(q; q > p; q--) {
			f3d_lcd_drawPixel(g+2, q, GREEN);
		} 
        }if(heading1 >= -140.99 && heading1 <= -89.99){
		//WEST
        	q = g;
		p = q - 10;
        	for(q; q > p; q--) {
			f3d_lcd_drawPixel(q, yval + 3, RED);
		}
		if(user_btn_read()) {return 7;} 
		 
        }else {
		q = g;
		p = q - 10;
        	for(q; q > p; q--) {
			f3d_lcd_drawPixel(q, yval + 3, GREEN);
		} 
	}if (heading1 <= -141 && heading1 >= -179.99) {
		//NW
		q = yval;
		p = q - 10;
		y = g;
		for(q; q > p; q--) {
			f3d_lcd_drawPixel(y, q, RED);
			y--;
		}
	
		 if(user_btn_read()) {return 8;} 

			
	} else {
		q = yval;
		p = q - 10;
		y = g;
		for(q; q > p; q--) {
			f3d_lcd_drawPixel(y, q, GREEN);
			y--;
		}
	}
   }
}

int move_ball_1(int angle, int ball[]){
	int p = 0;
	int n = 1;
	int r = 1;
	if (angle == 6) {
	//NORTH
		while(p < 30) {
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] <= 15) || (ball[1] <= 120 && ball[1] >= 115 && ball[0] <= 80 && ball[0] > 40) || (ball[1] <=60 && ball[1] >= 50 && ball[0] >=80 && ball[0] <= 150)) {
				n = -1;
			}
			ball[1] = ball[1] - n;
			draw_ball(ball[0], ball[1]);
			p++;
		}
	}
	if (angle == 4) {
	//EAST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if((ball[0] >= 128) || (ball[1] >= 50 && ball[1] <= 60 && ball[0] >= 80) || (ball[1] <= 120 && ball[1] >= 80 && ball[0] <= 40 && ball[0] >= 25)) {
				n = -1;
			}
			ball[0] = ball[0] + n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 1) {
	//SOUTH
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] >= 160) || (ball[1] <= 120 && ball[1] >= 80 && ball[0] >= 40 && ball[0] <= 50) || (ball[1] <=120 && ball[1] >=110 && ball[0] <= 80 && ball[0] >= 40) || (ball[0] >= 80 && ball[0] <= 150 && ball[1] >= 50 && ball[1] <= 60)) {
				n = -1;
			}
			ball[1] = ball[1] + n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 7) {
	//WEST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if((ball[0] <= 0) || (ball[1] <= 120 && ball[1] >= 80 && ball[0] <= 50 && ball[0] >= 40) || (ball[1] <= 120 && ball[1] >= 110 && ball[0] <= 80 && ball[0] >= 40)) {
				n = -1;
			}
			ball[0] = ball[0] - n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 3) {
	//SOUTH EAST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] >= 160) || (ball[1] <= 120 && ball[1] >= 80 && ball[0] >= 40 && ball[0] <= 50) || (ball[1] <=120 && ball[1] >=110 && ball[0] <= 80 && ball[0] >= 40) || (ball[0] >= 80 && ball[0] <= 150 && ball[1] >= 50 && ball[1] <= 60)) {
				n = -1;
			}
			if((ball[0] >= 128) || (ball[1] >= 50 && ball[1] <= 60 && ball[0] <= 80) || (ball[1] <= 120 && ball[1] >= 80 && ball[0] <= 40 && ball[0] >= 25)) {
				r = -1;
			}


			ball[0] = ball[0] + r;
			ball[1] = ball[1] + n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 2) {
		//SOUTH WEST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] >= 160) || (ball[1] <= 120 && ball[1] >= 80 && ball[0] >= 40 && ball[0] <= 50) || (ball[1] <=120 && ball[1] >=110 && ball[0] <= 80 && ball[0] >= 40) || (ball[0] >= 80 && ball[0] <= 150 && ball[1] >= 50 && ball[1] <= 60)) {
				r = -1;
			}		
			ball[0] = ball[0] - r;
			ball[1] = ball[1] +  n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 5) {
		//NORTH EAST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] <= 15) || (ball[1] <= 120 && ball[1] >= 115 && ball[0] <= 80 && ball[0] > 40) || (ball[1] <=60 && ball[1] >= 50 && ball[0] >=80 && ball[0] <= 150)) {
				n = -1;
			}
			if((ball[0] >= 128) || (ball[1] >= 50 && ball[1] <= 60 && ball[0] >= 80 && ball[0] <= 90) || (ball[1] <= 120 && ball[1] >= 80 && ball[0] <= 35 && ball[0] >= 25)) {
				r = -1;
			}

			ball[0] = ball[0] + r;
			ball[1] = ball[1] - n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 8) {
		//NORTH WEST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] <= 15) || (ball[1] <= 120 && ball[1] >= 110 && ball[0] <= 80 && ball[0] > 40) || (ball[1] <=60 && ball[1] >= 50 && ball[0] >=80 && ball[0] <= 150)) {
				n = -1;
			}
			if((ball[0] <= 0) || (ball[1] <= 120 && ball[1] >= 80 && ball[0] <= 50 && ball[0] >= 40) || (ball[1] <= 120 && ball[1] >= 110 && ball[0] <= 80 && ball[0] >= 40)) {
				r = -1;
			}	
			ball[0] = ball[0] - r;
			ball[1] = ball[1] - n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}


	return;
	
}

int move_ball_2(int angle, int ball[]){
	int p = 0;
	int n = 1;
	int r = 1;
	if (angle == 6) {
	//NORTH
		while(p < 30) {
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] <= 15) || (ball[1] <= 90 && ball[1] >= 80 && ball[1] < 90 && ball[0] <= 90) || (ball[1] <=35 && ball[0] >=60 && ball[0] <= 70)) {
				n = -1;
			}
			ball[1] = ball[1] - n;
			draw_ball(ball[0], ball[1]);
			p++;
		}
	}
	if (angle == 4) {
	//EAST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if((ball[0] >= 120) || (ball[1] >= 120 && ball[1] <= 160 && ball[0] <= 50 && ball[0] >= 35) || (ball[1] <= 35 && ball[1] >= 15 && ball[0] <= 70 && ball[0] >= 60) || (ball[1] <= 85 && ball[1] >= 75 && ball[0] <= 90)) {
				n = -1;
			}
			ball[0] = ball[0] + n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 1) {
	//SOUTH
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] >= 160) || (ball[1] <= 90 && ball[1] >= 80 && ball[1] < 90 && ball[0] <= 90) || (ball[1] <=35 && ball[0] >=60 && ball[0] <= 70)) {
				n = -1;
			}
			ball[1] = ball[1] + n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 7) {
	//WEST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if((ball[0] <= 0) || (ball[1] >= 120 && ball[1] <= 160 && ball[0] <= 50 && ball[0] >= 40) || (ball[1] <= 35 && ball[1] >= 15 && ball[0] <= 70 && ball[0] >= 60) || (ball[1] <= 90 && ball[1] >= 75 && ball[0] <= 90)) {
				n = -1;
			}
			ball[0] = ball[0] - n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 3) {
	//SOUTH EAST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] >= 160) || (ball[1] <= 90 && ball[1] >= 80 && ball[1] < 90 && ball[0] <= 90) || (ball[1] <=35 && ball[0] >=60 && ball[0] <= 70)) {
				n = -1;
			}
			if((ball[0] >= 120) || (ball[1] >= 120 && ball[1] <= 160 && ball[0] <= 50 && ball[0] >= 35) || (ball[1] <= 35 && ball[1] >= 15 && ball[0] <= 70 && ball[0] >= 60) || (ball[1] <= 90 && ball[0] >= 85 && ball[1] >= 75 && ball[0] <= 90)) {
				r = -1;
			}

			ball[0] = ball[0] + r;
			ball[1] = ball[1] + n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 2) {
		//SOUTH WEST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] >= 160) || (ball[1] <= 90 && ball[1] >= 80 && ball[1] < 90 && ball[0] <= 90) || (ball[1] <=35 && ball[0] >=60 && ball[0] <= 70)) {
				n = -1;
			}
			if((ball[0] <= 0) || (ball[1] >= 120 && ball[1] <= 160 && ball[0] <= 50 && ball[0] >= 40) || (ball[1] <= 35 && ball[1] >= 15 && ball[0] <= 70 && ball[0] >= 60) || (ball[1] <= 90 && ball[1] >= 75 && ball[0] <= 90 && ball[0] >= 85)) {
				r = -1;
			}	
			ball[0] = ball[0] - r;
			ball[1] = ball[1] +  n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 5) {
		//NORTH EAST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] <= 15) || (ball[1] <= 90 && ball[1] >= 80 && ball[1] < 90 && ball[0] <= 90) || (ball[1] <=35 && ball[0] >=60 && ball[0] <= 70)) {
				n = -1;
			}
			if((ball[0] >= 120) || (ball[1] >= 120 && ball[1] <= 160 && ball[0] <= 50 && ball[0] >= 35) || (ball[1] <= 35 && ball[1] >= 15 && ball[0] <= 70 && ball[0] >= 60) || (ball[1] <= 90 && ball[0] >= 85 && ball[1] >= 75 && ball[0] <= 90)) {
				r = -1;
			}


			ball[0] = ball[0] + r;
			ball[1] = ball[1] - n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 8) {
		//NORTH WEST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] <= 15) || (ball[1] <= 90 && ball[1] >= 80 && ball[1] < 90 && ball[0] <= 90) || (ball[1] <=35 && ball[0] >=60 && ball[0] <= 70)) {
				n = -1;
			}
			if((ball[0] <= 0) || (ball[1] >= 120 && ball[1] <= 160 && ball[0] <= 50 && ball[0] >= 40) || (ball[1] <= 35 && ball[1] >= 15 && ball[0] <= 70 && ball[0] >= 60) || (ball[1] <= 90 && ball[1] >= 75 && ball[0] <= 90 && ball[0] >= 85)) {
				r = -1;
			}	
			ball[0] = ball[0] - r;
			ball[1] = ball[1] - n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}


	return;
	
}

int move_ball_3(int angle, int ball[]){
	int p = 0;
	int n = 1;
	int r = 1;
	if (angle == 6) {
	//NORTH
		while(p < 30) {
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] <= 15) || (ball[1] <= 120 && ball[1] >= 100 && ball[0] >= 80 && ball[0] <= 90) || (ball[1] <=70 && ball[1] >= 60 && ball[0] >=40 && ball[0] <= 80)) {
				n = -1;
			}
			ball[1] = ball[1] - n;
			draw_ball(ball[0], ball[1]);
			p++;
		}
	}
	if (angle == 4) {
	//EAST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if((ball[0] >= 128) || (ball[1] >= 60 && ball[1] <= 160 && ball[0] <= 35 && ball[0] >= 27) || (ball[1] <= 120 && ball[1] >= 60 && ball[0] <= 90 && ball[0] >= 80)) {
				n = -1;
			}
			ball[0] = ball[0] + n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 1) {
	//SOUTH
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] >= 160) || (ball[1] <= 70 && ball[1] >= 60 && ball[0] <= 90 && ball[0] >= 40)) {
				n = -1;
			}
			ball[1] = ball[1] + n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 7) {
	//WEST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if((ball[0] <= 0) || (ball[1] <= 120 && ball[1] >= 60 && ball[0] <= 90 && ball[0] >= 80) || (ball[1] <= 160 && ball[1] >= 60 && ball[0] <= 40 && ball[0] >= 30)) {
				n = -1;
			}
			ball[0] = ball[0] - n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 3) {
	//SOUTH EAST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] >= 160) || (ball[1] <= 70 && ball[1] >= 60 && ball[0] <= 90 && ball[0] >= 40)) {
				n = -1;
			}

			if((ball[0] >= 128) || (ball[1] >= 60 && ball[1] <= 160 && ball[0] <= 35 && ball[0] >= 27) || (ball[1] <= 120 && ball[1] >= 60 && ball[0] <= 90 && ball[0] >= 80)) {
				r = -1;
			}


			ball[0] = ball[0] + r;
			ball[1] = ball[1] + n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 2) {
		//SOUTH WEST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] >= 160) || (ball[1] <= 70 && ball[1] >= 60 && ball[0] <= 90 && ball[0] >= 40)) {
				n = -1;
			}

			if((ball[0] <= 0) || (ball[1] <= 120 && ball[1] >= 60 && ball[0] <= 90 && ball[0] >= 80) || (ball[1] <= 160 && ball[1] >= 60 && ball[0] <= 40 && ball[0] >= 30)) {
				r = -1;
			}	
			ball[0] = ball[0] - r;
			ball[1] = ball[1] +  n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 5) {
		//NORTH EAST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] <= 15) || (ball[1] <= 120 && ball[1] >= 100 && ball[0] >= 80 && ball[0] <= 90) || (ball[1] <=70 && ball[1] >= 60 && ball[0] >=40 && ball[0] <= 80)) {
				n = -1;
			}
			if((ball[0] >= 128) || (ball[1] >= 60 && ball[1] <= 160 && ball[0] <= 35 && ball[0] >= 27) || (ball[1] <= 120 && ball[1] >= 60 && ball[0] <= 90 && ball[0] >= 80)) {
				r = -1;
			}



			ball[0] = ball[0] + r;
			ball[1] = ball[1] - n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}
	if (angle == 8) {
		//NORTH WEST
		while(p < 30){
			delay(5);
			remove_ball(ball[0], ball[1]);
			if ((ball[1] <= 15) || (ball[1] <= 120 && ball[1] >= 100 && ball[0] >= 80 && ball[0] <= 90) || (ball[1] <=70 && ball[1] >= 60 && ball[0] >=40 && ball[0] <= 80)) {
				n = -1;
			}
			if((ball[0] <= 0) || (ball[1] <= 120 && ball[1] >= 60 && ball[0] <= 90 && ball[0] >= 80) || (ball[1] <= 160 && ball[1] >= 60 && ball[0] <= 40 && ball[0] >= 30)) {
				r = -1;
			}	
			ball[0] = ball[0] - r;
			ball[1] = ball[1] - n;
			draw_ball(ball[0], ball[1]);
			p++;
			}
	}


	return;
	
}

int main_screen(void) {
  f3d_lcd_fillScreen(WHITE);
 
  f3d_lcd_drawString(42, 10, "MINI GOLF", BLACK, WHITE);
  int level = 1;
  while(1) {
	delay(20);
        f3d_nunchuk_read(&nun);
    if (nun.jy == 255){
	level--;
	if (level == 0) level = 3;
	}
    else if (nun.jy == 0) {
	level++;
	if(level == 4) level = 1;
	}
    if(level == 2){ 
		f3d_lcd_drawString(40, 110, "LEVEL 2", YELLOW, WHITE);
		if(nun.c == 1 || nun.z == 1){
			f3d_lcd_fillScreen(WHITE);
			play_level_2();
		}
	} else {
                  f3d_lcd_drawString(40,110, "LEVEL 2", BLACK, WHITE);
	} 
    if(level == 1){ 
        f3d_lcd_drawString(40, 100, "LEVEL 1", YELLOW, WHITE);
		if(nun.c == 1 || nun.z == 1){
			f3d_lcd_fillScreen(WHITE);
			play_level_1();
			
		}
	} else {
          f3d_lcd_drawString(40, 100, "LEVEL 1", BLACK, WHITE);
	} 
	  if(level == 3){ 
           	f3d_lcd_drawString(40, 120, "LEVEL 3", YELLOW, WHITE);struct bmppixel pixel;
		if(nun.c == 1 || nun.z == 1){
			f3d_lcd_fillScreen(WHITE);
			play_level_3(10, 150);
		}
	} else {
 		  f3d_lcd_drawString(40, 120, "LEVEL 3", BLACK, WHITE);
	} 

  }


}  


int victory_screen2(){
  audio("s.wav");

  uint16_t i;
  uint16_t j;

  FRESULT rc;			/* Result code */
  DIR dir;			/* Directory object */
  FIL Fil;		/* File object */
  FILINFO fno;			/* File information object */
  UINT bw, br;
  unsigned int retval;
  f_mount(0, &Fatfs);
		uint16_t r,g,b;
        uint16_t color;
     rc = f_open(&Fil, "tr.bmp", FA_READ);
     if (rc) die(rc);
	 for (i = 0; i < 160; i++){
	 for (j = 0; j < 128; j++){
	 	rc = f_read(&Fil, &pixel, sizeof(pixel), &br);					
		r = pixel.r >> 3;
		g = (pixel.g >> 2) << 5;
		b = (pixel.b >> 3) << 11;
		color = b | g | r;	
		f3d_lcd_drawPixel(j,i,color);	
		}
	 }
	  f3d_lcd_fillScreen(WHITE);
	  while(user_btn_read() != 1){
		  f3d_lcd_drawString(20, 10, "CONGATULATIONS!", BLUE, WHITE);
		  f3d_lcd_drawString(20, 90, "Press User Button", BLUE, WHITE);
		  f3d_lcd_drawString(20, 100, "To Play More", BLUE, WHITE);
         if(user_btn_read() ==1){
			 main_screen(); 
		 }
	  }     
	 

}


 

int victory_screen(){
  audio("s.wav");

  uint16_t i;
  uint16_t j;

  FRESULT rc;			/* Result code */
  DIR dir;			/* Directory object */
  FIL Fil;		/* File object */
  FILINFO fno;			/* File information object */
  UINT bw, br;
  unsigned int retval;
  f_mount(0, &Fatfs);
		uint16_t r,g,b;
        uint16_t color;
     rc = f_open(&Fil, "tr.bmp", FA_READ);
     if (rc) die(rc);
	 for (i = 0; i < 160; i++){
	 for (j = 0; j < 128; j++){
	 	rc = f_read(&Fil, &pixel, sizeof(pixel), &br);					
		r = pixel.r >> 3;
		g = (pixel.g >> 2) << 5;
		b = (pixel.b >> 3) << 11;
		color = b | g | r;	
		f3d_lcd_drawPixel(j,i,color);	
		}
	 }
	  f3d_lcd_fillScreen(WHITE);
	  while(user_btn_read() != 1){
		  f3d_lcd_drawString(20, 10, "CONGATULATIONS!", BLUE, WHITE);
		  f3d_lcd_drawString(20, 90, "Press User Button", BLUE, WHITE);
		  f3d_lcd_drawString(20, 100, "To Play More", BLUE, WHITE);
         if(user_btn_read() ==1){
			 main_screen(); 
		 }
	  }     
	 

}

 int play_level_2() {
    int endx = 12;
    int endx2 = 33;
    int endy = 30;
    int endy2 = 61;
    int angle = 0;
    int ball[2] = {10, 150};
    draw_level_2(ball[0], ball[1]);
    while(!(ball[0] > endx && ball[0] < endx2 && ball[1] > endy && ball[1] < endy2)) {
	delay(20);
    	angle = draw_lines(ball[0], ball[1]);
	move_ball_2(angle, ball);
	level_2_bound();
	}
	victory_screen();
}


int play_level_1() {
    int endx = 85;
    int endx2 = 110;
    int endy = 29;
    int endy2 = 50;
    int angle = 0;
    int ball[2] = {10, 150};
    draw_level_1(ball[0], ball[1]);
    while(!(ball[0] > endx && ball[0] < endx2 && ball[1] > endy && ball[1] < endy2)) {
	delay(20); 
    	angle = draw_lines(ball[0], ball[1]);
	move_ball_1(angle, ball);
	level_1_bound();
	}
	victory_screen2();
}

 

 int play_level_3() {
    int endx = 45;
    int endx2 = 80;
    int endy = 80;
    int endy2 = 110;
    int angle = 0;
    int ball[2] = {10, 150};
    draw_level_3(ball[0], ball[1]);
    while(!(ball[0] > endx && ball[0] < endx2 && ball[1] > endy && ball[1] < endy2)) {
	delay(20);
	angle = draw_lines(ball[0], ball[1]);
	move_ball_3(angle, ball);
	level_3_bound();
	}
	victory_screen();
}



		
int main(void) {
  clock_t t;
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  f3d_uart_init();
  f3d_timer2_init();
  f3d_lcd_init();
  f3d_dac_init();
  f3d_i2c1_init();
  f3d_accel_init();
  f3d_delay_init();
  f3d_rtc_init();
  f3d_systick_init();
  f3d_user_btn_init();
  f3d_gyro_init();
  f3d_mag_init(); 
  f3d_nunchuk_init();

  int i = 0;
  int ball_g = 10;
  int ball_i = 150;
  

  main_screen();
  draw_lines(ball_g, ball_i);

}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line) {
    /* Infinite loop */
    /* Use GDB to find out why we're here */
  while (1);
}
#endif
