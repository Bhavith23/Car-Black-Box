#include "black_box.h"
#include "clcd.h"
#include "adc.h"
#include "matrix_keypad.h"
#include "can.h"
#include "i2c.h"
#include "ds1307.h"
#include "uart.h"
#include<xc.h>

/* EEPROM Config */
#define LOG_SIZE     10
#define MAX_LOGS     10
#define EEPROM_START 0x00


void set_time(void)
{
    unsigned char hh = 0;
    unsigned char mm = 0;
    unsigned char ss = 0;

    unsigned char field = 0;   // 0=HH, 1=MM, 2=SS
    unsigned char key;


    while (1)
    {
        key = read_switches(STATE_CHANGE);

        //EXIT WITHOUT SAVE 
        if (key == MK_SW12)
        {
            CLEAR_DISP_SCREEN;
            CURSOR_HOME;

            state = e_main_menu;
            return;
        }

        //INCREMENT
        if (key == MK_SW1)
        {
            if (field == 0)          // Hour
            {
                hh++;
                if (hh > 23)
                    hh = 0;
            }
            else if (field == 1)     // Minute
            {
                mm++;
                if (mm > 59)
                    mm = 0;
            }
            else if (field == 2)     // Second
            {
                ss++;
                if (ss > 59)
                    ss = 0;
            }

            __delay_ms(200);   // Debounce
        }

        //MOVE RIGHT 
        else if (key == MK_SW2)
        {
            field++;

            if (field > 2)
                field = 0;

            __delay_ms(200);
        }

        //SAVE TIME
        else if (key == MK_SW8)
        {
            /* Write to RTC (BCD Format) */

            write_ds1307(HOUR_ADDR,
                        ((hh / 10) << 4) | (hh % 10));

            write_ds1307(MIN_ADDR,
                        ((mm / 10) << 4) | (mm % 10));

            write_ds1307(SEC_ADDR,
                        ((ss / 10) << 4) | (ss % 10));


            CLEAR_DISP_SCREEN;
            CURSOR_HOME;

            state = e_dashboard;
            return;
        }

        CLEAR_DISP_SCREEN;
        CURSOR_HOME;

        /* Time Display */
        clcd_putch((hh / 10) + '0', LINE1(5));
        clcd_putch((hh % 10) + '0', LINE1(6));
        clcd_putch(':',            LINE1(7));

        clcd_putch((mm / 10) + '0', LINE1(8));
        clcd_putch((mm % 10) + '0', LINE1(9));
        clcd_putch(':',            LINE1(10));

        clcd_putch((ss / 10) + '0', LINE1(11));
        clcd_putch((ss % 10) + '0', LINE1(12));

        /* Arrow Position */

        if (field == 0)         // Hour
            clcd_putch('^', LINE2(5));

        else if (field == 1)    // Minute
            clcd_putch('^', LINE2(8));

        else if (field == 2)    // Second
            clcd_putch('^', LINE2(11));
    }
}

void download_log(void)
{
    unsigned char count;
    unsigned char i;
    unsigned char addr;

    unsigned char hh, mm, ss;
    unsigned char spd, ev;

    char buf[20];

    /* Read number of logs */
    count = read_internal_eeprom(LOG_COUNT_ADDR);

    CLEAR_DISP_SCREEN;
    CURSOR_HOME;

    clcd_print((unsigned char *)"Downloading", LINE1(0));
    clcd_print((unsigned char *)"Logs...", LINE2(0));

    /* UART Header */

    puts("\r\n--------------------------------\r\n");
    puts("Sl  Time     EV   Speed\r\n");
    puts("--------------------------------\r\n");


    addr = LOG_START_ADDR;

    /* Read Each Record */
    for (i = 0; i < count; i++)
    {
        /* Read One Log */

        hh  = read_internal_eeprom(addr++);
        mm  = read_internal_eeprom(addr++);
        ss  = read_internal_eeprom(addr++);
        spd = read_internal_eeprom(addr++);
        ev  = read_internal_eeprom(addr++);
        addr++;     /* Reserved */

        /* Print Serial Number */
        putch((i + 1) / 10 + '0');
        putch((i + 1) % 10 + '0');
        putch(' ');
        putch(' ');


        /* Print Time */
        putch((hh / 10) + '0');
        putch((hh % 10) + '0');
        putch(':');

        putch((mm / 10) + '0');
        putch((mm % 10) + '0');
        putch(':');

        putch((ss / 10) + '0');
        putch((ss % 10) + '0');
        putch(' ');
        putch(' ');

        /* Print Event */
        putch('G');
        putch(ev + '0');
        putch(' ');
        putch(' ');

        /* Print Speed */
        putch((spd / 10) + '0');
        putch((spd % 10) + '0');


        puts("\r\n");
    }


    puts("--------------------------------\r\n");
    puts("Download Complete\r\n");


    __delay_ms(1500);


    /* Return to Menu */

    CLEAR_DISP_SCREEN;
    CURSOR_HOME;

    state = e_dashboard;
}

void clear_log(void)
{
    unsigned char i;

    /* Clear all stored records */
    for (i = 0; i < (LOG_SIZE * RECORD_SIZE); i++)
    {
        write_internal_eeprom(LOG_START_ADDR + i, 0xFF);
    }

    /* Reset log count */
    write_internal_eeprom(LOG_COUNT_ADDR, 0);

    /* Reset circular index */
    write_internal_eeprom(LOG_INDEX_ADDR, 0);


    /* Display message */
    CLEAR_DISP_SCREEN;
    CURSOR_HOME;

    clcd_print((unsigned char *)"   Cleared", LINE1(0));
    clcd_print((unsigned char *)" Successfully", LINE2(0));

    __delay_ms(2000);

    /* Return to main menu */
    CLEAR_DISP_SCREEN;
    CURSOR_HOME;

    state = e_main_menu;
}

