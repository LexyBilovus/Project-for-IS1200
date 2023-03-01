/* mipslabwork.c

   This file written 2015 by F Lundevall
   Updated 2017-04-21 by F Lundevall

   This file should be changed by YOU! So you must
   add comment(s) here with your name(s) and date(s):

   This file modified 2017-04-31 by Ture Teknolog
   This file modified 2018-02-xx by Oscar Eklund and Vilhelm Elofsson
   For copyright and licensing, see file COPYING */
#include <stdint.h>	 /* Declarations of uint_32 and the like */
#include <pic32mx.h> /* Declarations of system-specific addresses etc */
#include "mipslab.h" /* Declatations for these labs */

volatile int *porte = (volatile int *)0xbf886110;
int start = 1;
int col = 0;
int level = 0;
int page  = 2;

int gameovercheck = 0;
int record = 0;

/* Interrupt Service Routine */
void user_isr(void)
{
	if (IFS(0) & 0x100) 
	{
		IFS(0) &= 0xfeff;

		while (gameovercheck) 
		{
			display_string(0, "   GAME OVER");
			display_string(1, " ");
			display_string(2, " BTN2 - RESTART");
			display_update();

			level = 0;
			col = 0;
			page = 3;
			record = 0;
			*porte = 0x0;

			if (getbtns() == 0b001)	
			{
				print_empty_screen();
				start = 1;
				gameovercheck = 0;
			}
		}

		while (start) 
		{
			display_string(0, "  DOODLE JUMP");
			display_string(2, "  BTN2 - START");
			display_update();

			level = 0;
			col = 0;
			page = 3;
			record = 0;
			*porte = 0x0;


			if (getbtns() == 0b001) 
			{
				print_empty_screen();
				check_level();
				//display_image(0, Screen); //не біло
				start = 0;
			}
		}
	}
}

/* Lab-specific initialization goes here */
void labinit(void)
{
	volatile int *trise = (volatile int *)0xbf886100;
	*trise = *trise & 0xffffff00;
	*porte = 0x00;
	TRISDSET = 0xfe;

	T2CON = 0x70;			   // set tckps bits 1:256 (value 111) - the other bits set to zero (disable other functions - we want well-defined behaviour)
	PR2 = 80000000 / 256 / 10; // set in period register the value timer has to reach
	TMR2 = 0;
	T2CONSET = 0x8000; // Start the timer (activate the ON bit)

	/*THE THREE STEPS TO ENABLE INTERRUPT*/
	IECSET(0) = 0x900;	// set bit 8 to 1 (T2IE bit)
	IPC(2) = 0x3000004; // set non-zero priority (any value between 4 and 31 works)

	enable_interrupt(); // set to 1 the IE bit in CP0 Regiter via "ei" instruction

	return;
}

/* This function is called repetitively from the main program */
void labwork(void)
{
	page = 3;

	if (level == 3) 
	{
		gameovercheck = 1;
	}

	switch (getbtns()) 
	{
		case 0b100:
			if(level == 2)
				change_coins();
			page = 2;
			jump(col);

			break;

		case 0b010:
		
			if (col > 128) // take last column as well    col==128
			{
				level++;
				check_level();
				col = 0;
			}

			else 
			{
				moveright(col+2);//moveright(col)
				col+=3;//col++1; - made it faster
			}
			break;
		
	}

	check_coins();
}

void check_level()
{
	restartscreen();
	coin1.check = 0;
	coin2.check = 0;
	coin3.check = 0;
	switch (level)
	{
		case 0:
		{
			display_string(1, "    LEVEL 0");
			display_update();
			quicksleep(10000000);

			coin1.x = 31;
			coin1.page = 3;
			coin2.x = 61;
			coin2.page = 3;
			coin3.x = 91;
			coin3.page = 3;
			break;
		}
		case 1:
		{
			display_string(1, "    LEVEL 1");
			display_update();
			quicksleep(10000000);

			coin1.page = 2;
			coin3.page = 2;
			break;
		}
		case 2:
		{
			display_string(1, "    LEVEL 2");
			display_update();
			quicksleep(10000000);

			coin1.page = 3;
			coin2.page = 2;
			coin3.page = 3;
			break;
		}
	}	
	update_screen_level();
	display_image(0, Screen); 
}

void update_screen_level()
{
	int i, j, k;
    for (i = 1; i < 4; i++) 
	{ 
        for(j = 0; j < 128; j++)
		{
			if((i == coin1.page && j == coin1.x)  || (i == coin2.page && j == coin2.x) || (i == coin3.page && j == coin3.x))
			{
				for ( k = 0; k < 3; k++)
				{
					Screen[i*128 + j + k] = Start_screen[i*128 + j + k] | coin[k];
				}
			}
			if(j < 11)
				Screen[3*128 + j] = Start_screen[i*128 + j] | doodle[j];
		}
    }  
}

void check_coins()
{
	if(page == coin1.page && (coin1.x <= col+10) && coin1.check!=1)
	{
		coin1.check = 1;
		record++;
		*porte = *porte + 1;
	}
	if(page == coin2.page && (coin2.x <= col+10) && coin2.check!=1) 
	{
		coin2.check = 1;
		record++;
		*porte = *porte + 1;
	}
	if(page == coin3.page && (coin3.x <= col+10) && coin3.check!=1)
	{
		coin3.check = 1;
		record++;
		*porte = *porte + 1;
	}

}	

void change_coins()
{
	int j, k, i;
	coin1.page = coin2.page;
	coin2.page = coin3.page;
	coin3.page = coin1.page;

	for (i = 2; i < 4; i++) 
	{
        for(j = col; j < 128; j++)
		{
			if((i == coin1.page && j == coin1.x)  || (i == coin2.page && j == coin2.x) || (i == coin3.page && j == coin3.x))
			{
				for ( k = 0; k < 3; k++)
					Screen[i*128 + j + k] = Start_screen[i*128 + j + k] | coin[k];
				j+=k;
			}
			else
				Screen[i*128 + j ] = Start_screen[i*128 + j ];
		}
	}
	
}

	
