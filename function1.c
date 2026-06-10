#include "black_box.h"
#include "clcd.h"
#include "adc.h"
#include "matrix_keypad.h"
#include "can.h"
#include "i2c.h"
#include "ds1307.h"
#include<xc.h>


/* EEPROM Config */
#define LOG_SIZE     10
#define MAX_LOGS     10
#define EEPROM_START 0x00


/* RTC Buffers */
static unsigned char time[9];
static unsigned char clock_reg[3];


/* Event Names */
static const unsigned char event_name[][3] =
{
    "GN","G1","G2","G3","G4","G5","GR"
};


/* Menu */
static const unsigned char *menu_list[] =
{
    "View Log",
    "Down Log",
    "Set Time",
    "Clear Log"
};

#define MENU_COUNT 4


/* Log Index */
static unsigned char log_index = 0;
static unsigned char log_count = 0;


/* ---------------- RTC ---------------- */

static void get_time(void)
{
    clock_reg[0] = read_ds1307(HOUR_ADDR);
    clock_reg[1] = read_ds1307(MIN_ADDR);
    clock_reg[2] = read_ds1307(SEC_ADDR);

    time[0] = '0' + ((clock_reg[0] >> 4) & 0x03);
    time[1] = '0' + (clock_reg[0] & 0x0F);

    time[2] = ':';

    time[3] = '0' + ((clock_reg[1] >> 4) & 0x0F);
    time[4] = '0' + (clock_reg[1] & 0x0F);

    time[5] = ':';

    time[6] = '0' + ((clock_reg[2] >> 4) & 0x0F);
    time[7] = '0' + (clock_reg[2] & 0x0F);

    time[8] = '\0';
}


/* ---------------- Store Log ---------------- */

void store_log(unsigned int speed, unsigned char gear)
{
    unsigned char addr;
    unsigned char count;
    unsigned char index;

    unsigned char hh, mm, ss;


    /* Read RTC */
    hh = ((clock_reg[0] >> 4) & 0x03) * 10 + (clock_reg[0] & 0x0F);
    mm = ((clock_reg[1] >> 4) & 0x0F) * 10 + (clock_reg[1] & 0x0F);
    ss = ((clock_reg[2] >> 4) & 0x0F) * 10 + (clock_reg[2] & 0x0F);


    /* Read Index & Count */
    index = read_internal_eeprom(LOG_INDEX_ADDR);
    count = read_internal_eeprom(LOG_COUNT_ADDR);


    /* Calculate Address */
    addr = LOG_START_ADDR + (index * RECORD_SIZE);


    /* Store Data */

    write_internal_eeprom(addr++, hh);
    write_internal_eeprom(addr++, mm);
    write_internal_eeprom(addr++, ss);

    write_internal_eeprom(addr++, speed);
    write_internal_eeprom(addr++, gear);

    write_internal_eeprom(addr++, 0x00);   // Reserved


    /* Update Index */
    index++;

    if (index >= LOG_SIZE)
        index = 0;


    /* Update Count */
    if (count < LOG_SIZE)
        count++;


    /* Save Back */

    write_internal_eeprom(LOG_INDEX_ADDR, index);
    write_internal_eeprom(LOG_COUNT_ADDR, count);
}


/* ---------------- Dashboard ---------------- */

void view_dashboard(void)
{
    static unsigned char gear = 0;
    static unsigned char prev_gear = 0;
    static unsigned char init = 0;

    unsigned char key;
    unsigned int speed;

    key = read_switches(STATE_CHANGE);

    /* Enter Menu */
    if (key == MK_SW8)
    {
        __delay_ms(200);
        init = 0;                
        state = e_main_menu;
        CLEAR_DISP_SCREEN;
        return;
    }

    /* Gear Increase */
    if (key == MK_SW1 && gear < 6)
    {
        gear++;
        __delay_ms(150);
    }

    /* Gear Decrease */
    else if (key == MK_SW2 && gear > 0)
    {
        gear--;
        __delay_ms(150);
    }

    speed = read_adc(CHANNEL4) / 10.23;

    get_time();

    /* Store Log If Gear Changed */
    if (gear != prev_gear)
    {
        store_log(speed, gear);
        prev_gear = gear;
    }

    if (init == 0)
    {
        CLEAR_DISP_SCREEN;
        CURSOR_HOME;

        clcd_print((unsigned char *)"TIME", LINE1(0));
        clcd_print((unsigned char *)"EV",   LINE1(9));
        clcd_print((unsigned char *)"SPD",  LINE1(13));

        init = 1;
    }

    /* Update dynamic values only */

    clcd_print(time, LINE2(0));
    clcd_print(event_name[gear], LINE2(9));

    clcd_putch((speed / 100) % 10 + '0', LINE2(13));
    clcd_putch((speed / 10)  % 10 + '0', LINE2(14));
    clcd_putch((speed % 10) + '0',       LINE2(15));

    __delay_ms(200);     // Smooth refresh rate
}
/* ---------------- Main Menu ---------------- */

void display_main_menu(void)
{
    static unsigned char index = 0;
    unsigned char key;

    key = read_switches(STATE_CHANGE);
    
    /* UP */
    if (key == MK_SW1 && index > 0)
    {
        index--;
        __delay_ms(200);  
    }

    /* DOWN */
    else if (key == MK_SW2 && index < MENU_COUNT - 1)
    {
        index++;
        __delay_ms(200);  
    }

    /* ENTER */
    else if (key == MK_SW8)
    {
        if (index == 0) 
        state = e_view_log;
        else if (index == 1) 
        state = e_download_log;
        else if (index == 2) 
        state = e_set_time;
        else if (index == 3) 
        state = e_clear_log;

        CLEAR_DISP_SCREEN;
        CURSOR_HOME;

        __delay_ms(200);   // Debounce
        return;
    }

    CLEAR_DISP_SCREEN;
    CURSOR_HOME;

    clcd_putch('>', LINE1(0));
    clcd_print(menu_list[index], LINE1(2));

    if (index < MENU_COUNT - 1)
        clcd_print(menu_list[index + 1], LINE2(2));
}

/* ---------------- View Log ---------------- */

void view_log(void)
{
    unsigned char count;
    static unsigned char index = 0;

    unsigned char key;
    unsigned char addr;

    unsigned char hh, mm, ss;
    unsigned char spd, ev;


    /* Read total log count */
    count = read_internal_eeprom(LOG_COUNT_ADDR);


    /* If No Data */
    if (count == 0)
    {
        CLEAR_DISP_SCREEN;
        CURSOR_HOME;

        clcd_print((unsigned char *)"      NO", LINE1(0));
        clcd_print((unsigned char *)" DATA FOUND", LINE2(0));

        __delay_ms(1500);

        state = e_main_menu;
        return;
    }


    /* Read Key */
    key = read_switches(STATE_CHANGE);


    /* Exit View Log */
    if (key == MK_SW12)
    {
        CLEAR_DISP_SCREEN;
        CURSOR_HOME;

        state = e_dashboard;
        index = 0;
        return;
    }


    /* Scroll Down */
    if (key == MK_SW2)
    {
        if (index < count - 1)
            index++;

        __delay_ms(200);   // Debounce
    }

    /* Scroll Up */
    else if (key == MK_SW1)
    {
        if (index > 0)
            index--;

        __delay_ms(200);   // Debounce
    }

    /* Calculate EEPROM Address */
    addr = LOG_START_ADDR + (index * RECORD_SIZE);

    /* Read One Record */
    hh  = read_internal_eeprom(addr++);
    mm  = read_internal_eeprom(addr++);
    ss  = read_internal_eeprom(addr++);
    spd = read_internal_eeprom(addr++);
    ev  = read_internal_eeprom(addr++);

    /* Clear Display */
    CLEAR_DISP_SCREEN;
    CURSOR_HOME;

    clcd_print((unsigned char *)"L TIME     EV S", LINE1(0));

    /* Log Number */
    clcd_putch((index) + '0', LINE2(0));

    clcd_putch(' ', LINE2(1));


    /* HH */
    clcd_putch((hh / 10) + '0', LINE2(2));
    clcd_putch((hh % 10) + '0', LINE2(3));
    clcd_putch(':', LINE2(4));

    /* MM */
    clcd_putch((mm / 10) + '0', LINE2(5));
    clcd_putch((mm % 10) + '0', LINE2(6));
    clcd_putch(':', LINE2(7));

    /* SS */
    clcd_putch((ss / 10) + '0', LINE2(8));
    clcd_putch((ss % 10) + '0', LINE2(9));

    /* Spaces */
    clcd_putch(' ', LINE2(10));

    /* Event */
    clcd_putch('G', LINE2(11));
    clcd_putch(ev + '0', LINE2(12));

    /* Space */
    clcd_putch(' ', LINE2(13));

    /* Speed (2 Digits) */
    clcd_putch((spd / 10) + '0', LINE2(14));
    clcd_putch((spd % 10) + '0', LINE2(15));
}





