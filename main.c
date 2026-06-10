#include "black_box.h"
#include "clcd.h"
#include "adc.h"
#include "matrix_keypad.h"
#include "can.h"
#include "i2c.h"
#include "ds1307.h"
#include<xc.h>
#include "uart.h"
State_t state;


void init_config(void)
{
    ADCON1 = 0x0F;
     
    init_uart();
    init_clcd();
    init_adc();
    init_matrix_keypad();
    init_can();
    init_i2c();
    init_ds1307();

    state = e_dashboard;

    CLEAR_DISP_SCREEN;
    CURSOR_HOME;
}


void main(void)
{
    init_config();

    while (1)
    {
        switch (state)
        {
            case e_dashboard:
                view_dashboard();
                break;

            case e_main_menu:
                display_main_menu();
                break;

            case e_view_log:
                view_log();
                break;
                
            case e_download_log:
                download_log();
                break;
                
            case e_set_time:
            set_time();
            break;
                

            case e_clear_log:
                clear_log();
                break;

            default:
                state = e_dashboard;
                break;
        }
    }
}