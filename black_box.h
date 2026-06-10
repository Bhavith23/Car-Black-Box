#ifndef BLACK_BOX_H
#define BLACK_BOX_H

#include <xc.h>

#define LOG_START_ADDR   0x00
#define LOG_SIZE         10
#define RECORD_SIZE      6

#define LOG_COUNT_ADDR   0xF0
#define LOG_INDEX_ADDR   0xF1


/* Application States */
typedef enum
{
    e_dashboard,
    e_main_menu,
    e_view_log,
    e_set_time,
    e_download_log,
    e_clear_log
} State_t;


/* Global State Variable */
extern State_t state;


/* Initialization */
void init_config(void);


/* Dashboard */
void view_dashboard(void);


/* Menu */
void display_main_menu(void);


/* Menu Functions */
void view_log(void);
void set_time(void);
void download_log(void);
void clear_log(void);
void store_log(unsigned int speed, unsigned char gear);



#endif
