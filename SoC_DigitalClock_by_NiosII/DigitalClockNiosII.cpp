#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "system.h"
#include "sys/alt_irq.h"
#include "altera_avalon_timer_regs.h"
#include "altera_avalon_pio_regs.h"
#include "altera_avalon_uart_regs.h"

int DAY = 30, MONTH = 4, YEAR = 1975;
int HOUR_CUR = 11, MINUTE_CUR = 29, SECOND_CUR = 30;
int HOUR_ALARM = 11, MINUTE_ALARM = 30, SECOND_ALARM = 0;
int YEAR_MIN = 1975;
int TIME_CHANGE = 0;
int TIME_WAIT = 30;
int daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const int TIME_ON_ALARM = 60;
const int hex[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};

int MODE = 0;
char COMMAND_UART[21];
int LENGTH_COMMAND_UART = 0;
int CHECK_LENGTH = 7;

int FUNCTION = 0;
int FLAG_UART = 0;
int FLAG_TIMER = 0;
int FLAG_DISPLAY = 0;
int FLAG_SET_ALARM = 0;
int FLAG_CONTROL_ALARM = 0;

int OLD_SW = 0, NEW_SW = 0;

enum MODE_CODE {
    EXIT = 0,
    SET_ALARM = 1,
    CHANGE_TIME = 2,
    CHANGE_DATE = 3
};

enum FUNCTION_KEY {
    ENTER_KEY1 = 1,
    MOVE_KEY2 = 2,
    ADJUST_KEY3 = 3
};

enum DISPLAY_CODE {
    DISPLAY_TIME = 0,
    DISPLAY_DATE = 1
};

enum LOCATION_CODE {
    LOC_SEC_YEAR      = 1,
    LOC_MIN_MONTH     = 2,
    LOC_HOUR_DAY      = 3
};

enum OPTION_CODE {
    TIME = 0,
    DATE = 1
};

enum STATUS_ALARM {
    ON = 1,
    OFF = 2
};

//Bat dau code cho LCD
void delay(int a)
{
    volatile int i = 0;
    while(i < a * 100)
    {
        i++;
    }
}

void lcd_command(char data)
{
    IOWR_ALTERA_AVALON_PIO_DATA(LCD_RS_BASE, 0x00);
    IOWR_ALTERA_AVALON_PIO_DATA(LCD_RW_BASE, 0x00);
    IOWR_ALTERA_AVALON_PIO_DATA(LCD_D_BASE, data & 0xFF);
    IOWR_ALTERA_AVALON_PIO_DATA(LCD_EN_BASE, 0x01);
    delay(10);
    IOWR_ALTERA_AVALON_PIO_DATA(LCD_EN_BASE, 0x00);
    delay(10);
}

void lcd_data(char data)
{
    IOWR_ALTERA_AVALON_PIO_DATA(LCD_RS_BASE, 0x01);
    IOWR_ALTERA_AVALON_PIO_DATA(LCD_RW_BASE, 0x00);
    IOWR_ALTERA_AVALON_PIO_DATA(LCD_D_BASE, data & 0xFF);
    IOWR_ALTERA_AVALON_PIO_DATA(LCD_EN_BASE, 0x01);
    delay(10);
    IOWR_ALTERA_AVALON_PIO_DATA(LCD_EN_BASE, 0x00);
    delay(10);
}

void lcd_init()
{
    lcd_command(0x38); delay(10);
    lcd_command(0x0C); delay(10);
    lcd_command(0x06); delay(10);
    lcd_command(0x01); delay(10);
    IOWR_ALTERA_AVALON_PIO_DATA(LCD_ON_BASE, 0x01);
    IOWR_ALTERA_AVALON_PIO_DATA(LCD_BLON_BASE, 0x01);
}

void lcd_string(const char *str)
{
    int i = 0;
    while(str[i] != 0)
    {
        lcd_data(str[i]);
        i++;
    }
}

void lcd_gotoxy(int col, int row)
{
    int pos = 0;
    if (row == 1) pos = 0x80 + col;
    else if (row == 2) pos = 0xC0 + col;
    lcd_command(pos);
}
//Ket thuc code cho LCD

void Timer_Init(void)
{
    unsigned int period = 50000000 - 1;
    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_BASE, ALTERA_AVALON_TIMER_CONTROL_STOP_MSK);
    IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE, period);
    IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE, (period >> 16));
    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_BASE,
        ALTERA_AVALON_TIMER_CONTROL_CONT_MSK |
        ALTERA_AVALON_TIMER_CONTROL_ITO_MSK |
        ALTERA_AVALON_TIMER_CONTROL_START_MSK);
}

void Timer_IRQ_Handler(void* isr_context)
{
    if(MODE == 0) FLAG_TIMER = 1;
    else TIME_CHANGE++;
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE, ALTERA_AVALON_TIMER_STATUS_TO_MSK);
}

void SW_ISR(void *isr_context)
{
    delay(500);
    int SW_STATUS = IORD(SW_BASE, 0);
    FLAG_DISPLAY = SW_STATUS & 0x01; // SW0

    NEW_SW = SW_STATUS ^ OLD_SW;
    OLD_SW = SW_STATUS;

    // SW1 - SET ALARM
    if ((NEW_SW & 0x02) && (SW_STATUS & 0x02))
    {
        if (MODE == EXIT)
        {
            MODE = SET_ALARM;
            FLAG_SET_ALARM = ON;
        }
    }
    else if ((NEW_SW & 0x02) && !(SW_STATUS & 0x02))
    {
        if (MODE == SET_ALARM)
        {
            MODE = EXIT;
            FLAG_SET_ALARM = OFF;
        }
    }

    // SW2 - CHANGE TIME
    if ((NEW_SW & 0x04) && (SW_STATUS & 0x04))
    {
        if (MODE == EXIT)
            MODE = CHANGE_TIME;
    }
    else if ((NEW_SW & 0x04) && !(SW_STATUS & 0x04))
    {
        if (MODE == CHANGE_TIME)
            MODE = EXIT;
    }

    // SW3 - CHANGE DATE
    if ((NEW_SW & 0x08) && (SW_STATUS & 0x08))
    {
        if (MODE == EXIT)
            MODE = CHANGE_DATE;
    }
    else if ((NEW_SW & 0x08) && !(SW_STATUS & 0x08))
    {
        if (MODE == CHANGE_DATE)
            MODE = EXIT;
    }
    printf("MODE = %d\n", MODE);
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(SW_BASE, 0xFF);
}

void KEY_ISR(void *isr_context)
{
    delay(300);
    int status_key = IORD(KEY_BASE, 0);
    printf("KEY_STATUS: %d\n", status_key);
    if(!(status_key & 0x01) && (FLAG_SET_ALARM == ON || MODE)) // key 1: enter
        FUNCTION = ENTER_KEY1;
    if(!(status_key & 0x02) && MODE) // key 2: giam
        FUNCTION = MOVE_KEY2;
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEY_BASE, 0x03);
}

void UART_ISR(void* isr_context)
{
    COMMAND_UART[LENGTH_COMMAND_UART] = IORD_ALTERA_AVALON_UART_RXDATA(UART_BASE);
    LENGTH_COMMAND_UART++;
    if(LENGTH_COMMAND_UART == 5)
        FLAG_UART = 1;
    COMMAND_UART[LENGTH_COMMAND_UART] = '\0';
}

void display_to_hex(int option, int HEX45, int HEX23, int HEX01)
{
    if(option == 0)
    {
        IOWR(HEX_2_BASE, 0, hex[HEX01 % 10]);
        IOWR(HEX_3_BASE, 0, hex[HEX01 / 10]);

        IOWR(HEX_4_BASE, 0, hex[HEX23 % 10]);
        IOWR(HEX_5_BASE, 0, hex[HEX23 / 10]);

        IOWR(HEX_6_BASE, 0, hex[HEX45 % 10]);
        IOWR(HEX_7_BASE, 0, hex[HEX45 / 10]);

        IOWR(HEX_0_BASE, 0, 0xFF);
        IOWR(HEX_1_BASE, 0, 0xFF);
    }
    else
    {
        IOWR(HEX_0_BASE, 0, hex[HEX01 % 10]);
        IOWR(HEX_1_BASE, 0, hex[(HEX01 / 10) % 10]);
        IOWR(HEX_2_BASE, 0, hex[(HEX01 / 100) % 10]);
        IOWR(HEX_3_BASE, 0, hex[HEX01 / 1000]);

        IOWR(HEX_4_BASE, 0, hex[HEX23 % 10]);
        IOWR(HEX_5_BASE, 0, hex[HEX23 / 10]);

        IOWR(HEX_6_BASE, 0, hex[HEX45 % 10]);
        IOWR(HEX_7_BASE, 0, hex[HEX45 / 10]);
    }
}

void display_to_lcd(int clear, int col, int row, char *data)
{
    if(clear) lcd_command(0x01);
    lcd_gotoxy(col, row);
    lcd_string(data);
}

void calculate_datetime()
{
    if (TIME_CHANGE)
    {
        int total_seconds = HOUR_CUR * 3600 + MINUTE_CUR * 60 + SECOND_CUR;
        total_seconds += TIME_CHANGE;

        int extra_days = total_seconds / 86400;
        total_seconds %= 86400;

        HOUR_CUR = total_seconds / 3600;
        MINUTE_CUR = (total_seconds % 3600) / 60;
        SECOND_CUR = total_seconds % 60;

        while (extra_days--) {
            DAY++;
            if (DAY > daysInMonth[MONTH - 1]) {
                DAY = 1;
                MONTH++;
                if (MONTH > 12) {
                    MONTH = 1;
                    YEAR++;
                    daysInMonth[1] = ((YEAR % 4 == 0 && YEAR % 100 != 0) || (YEAR % 400 == 0)) ? 29 : 28;
                }
            }
        }
        TIME_CHANGE = 0;
    }
    SECOND_CUR++;
    if(SECOND_CUR == 60)
    {
        SECOND_CUR = 0;
        MINUTE_CUR++;
        if(MINUTE_CUR == 60)
        {
            MINUTE_CUR = 0;
            HOUR_CUR++;
            if(HOUR_CUR == 24)
            {
                HOUR_CUR = 0;
                DAY++;
                if(daysInMonth[MONTH - 1] < DAY)
                {
                    DAY = 1;
                    MONTH++;
                    if(MONTH == 13)
                    {
                        MONTH = 1;
                        YEAR++;
                        daysInMonth[1] = ((YEAR % 4 == 0 && YEAR % 100 != 0) || (YEAR % 400 == 0)) ? 29 : 28;
                    }
                }
            }
        }
    }
}

void send_messenger(const char *str) {
    while (*str) {
        while (!(IORD_ALTERA_AVALON_UART_STATUS(UART_BASE) & ALTERA_AVALON_UART_STATUS_TRDY_MSK));
        IOWR_ALTERA_AVALON_UART_TXDATA(UART_BASE, *str);
        str++;
    }
}

void control_alarm(int status)
{
    switch (status)
    {
    case ON:
        IOWR(LEDR_BASE, 0, ~IORD(LEDR_BASE, 0));
        break;
    case OFF:
        IOWR(LEDR_BASE, 0, 0x00000);
        break;
    }
}

int change_datetime_sw(int option, int *HOUR_DAY, int *MINUTE_MONTH, int *SECOND_YEAR)
{
    int hour_day = *HOUR_DAY, minute_month = *MINUTE_MONTH, second_year = *SECOND_YEAR;
    int location = LOC_SEC_YEAR;
    int sum_change = -1;
    int result = 0;
    int time_wait = 0;
    char messenger_to_lcd[100];
    switch (MODE)
    {
    case SET_ALARM:
        strcpy(messenger_to_lcd, " SET ALARM");
        break;
    case CHANGE_TIME:
        strcpy(messenger_to_lcd, "CHANGE TIME");
        break;
    case CHANGE_DATE:
        strcpy(messenger_to_lcd, "CHANGE DATE");
        break;
    }
    display_to_lcd(1, 3, 1, messenger_to_lcd);
    display_to_hex(option, hour_day, minute_month, second_year);
    if(option == 0) sprintf(messenger_to_lcd, "%02d:%02d:%02d", hour_day, minute_month, second_year);
    else sprintf(messenger_to_lcd, "%02d-%02d-%04d", hour_day, minute_month, second_year);
    display_to_lcd(0, (option == 0) ? 4 : 3, 2, messenger_to_lcd);
    while(MODE != EXIT)
    {
        if(FUNCTION == ENTER_KEY1)
            break;
        if(FUNCTION == MOVE_KEY2)
        {
            FUNCTION = 0;
            location++;
            if(location == 4)
                location = LOC_SEC_YEAR;
        }
        switch (location)
        {
        case LOC_SEC_YEAR:
            IOWR(LEDG_BASE, 0, 0x03);
            while(TIME_CHANGE - time_wait <= TIME_WAIT && MODE != EXIT && FUNCTION != MOVE_KEY2 && FUNCTION != ENTER_KEY1)
            {
                while(!(IORD(KEY_BASE, 0) & 0x04))
                {
                    time_wait = TIME_CHANGE;
                    second_year++;
                    if(option == TIME && second_year == 60)
                        second_year = 0;
                    else if(option == DATE && second_year < YEAR_MIN)
                        second_year = YEAR_MIN;
                    if(sum_change != hour_day + minute_month + second_year)
                    {
                        sum_change = hour_day + minute_month + second_year;
                        if(option == 0) sprintf(messenger_to_lcd, "%02d:%02d:%02d", hour_day, minute_month, second_year);
                        else sprintf(messenger_to_lcd, "%02d-%02d-%04d", hour_day, minute_month, second_year);
                        display_to_lcd(0, (option == 0) ? 4 : 3, 2, messenger_to_lcd);
                        display_to_hex(option, hour_day, minute_month, second_year);
                    }
                    int i;
                    for(i=0;i<2000;i++)
                    {
                        if(FUNCTION == MOVE_KEY2 || FUNCTION == ENTER_KEY1)
                            break;
                        else delay(1);
                    }
                }
            }
            break;
        case LOC_MIN_MONTH:
            IOWR(LEDG_BASE, 0, 0x0C);
            while(TIME_CHANGE - time_wait <= TIME_WAIT && MODE != EXIT && FUNCTION != MOVE_KEY2 && FUNCTION != ENTER_KEY1)
            {
                while(!(IORD(KEY_BASE, 0) & 0x04))
                {
                    time_wait = TIME_CHANGE;
                    minute_month++;
                    if(option == TIME && minute_month == 60)
                        minute_month = 0;
                    else if(option == DATE && minute_month == 13)
                        minute_month = 1;
                    if(sum_change != hour_day + minute_month + second_year)
                    {
                        sum_change = hour_day + minute_month + second_year;
                        if(option == 0) sprintf(messenger_to_lcd, "%02d:%02d:%02d", hour_day, minute_month, second_year);
                        else sprintf(messenger_to_lcd, "%02d-%02d-%04d", hour_day, minute_month, second_year);
                        display_to_lcd(0, (option == 0) ? 4 : 3, 2, messenger_to_lcd);
                        display_to_hex(option, hour_day, minute_month, second_year);
                    }
                    int i;
                    for(i=0;i<2000;i++)
                    {
                        if(FUNCTION == MOVE_KEY2 || FUNCTION == ENTER_KEY1)
                            break;
                        else delay(1);
                    }
                }
            }
            break;
        case LOC_HOUR_DAY:
            IOWR(LEDG_BASE, 0, 0x30);
            while(TIME_CHANGE - time_wait <= TIME_WAIT  && MODE != EXIT && FUNCTION != MOVE_KEY2 && FUNCTION != ENTER_KEY1)
            {
                while(!(IORD(KEY_BASE, 0) & 0x04))
                {
                    time_wait = TIME_CHANGE;
                    hour_day++;
                    if(option == TIME && hour_day == 24)
                        hour_day = 0;
                    else if(option == DATE && hour_day > daysInMonth[minute_month - 1])
                        hour_day = 1;
                    if(sum_change != hour_day + minute_month + second_year)
                    {
                        sum_change = hour_day + minute_month + second_year;
                        if(option == 0) sprintf(messenger_to_lcd, "%02d:%02d:%02d", hour_day, minute_month, second_year);
                        else sprintf(messenger_to_lcd, "%02d-%02d:-%04d", hour_day, minute_month, second_year);
                        display_to_lcd(0, (option == 0) ? 4 : 3, 2, messenger_to_lcd);
                        display_to_hex(option, hour_day, minute_month, second_year);
                    }
                    int i;
                    for(i=0;i<2000;i++)
                    {
                        if(FUNCTION == MOVE_KEY2 || FUNCTION == ENTER_KEY1)
                            break;
                        else delay(1);
                    }
                }
            }
            break;
        }
    }
    if(FUNCTION == ENTER_KEY1)
    {
        FUNCTION = 0;
        result = 1;
        *HOUR_DAY = hour_day;
        *MINUTE_MONTH = minute_month;
        *SECOND_YEAR = second_year;
    }
    MODE = 0;
    IOWR(LEDG_BASE, 0, 0x000);
    return result;
}

int check_data_uart(int option, char data[], int *HOUR_DAY, int *MINUTE_MONTH, int *SECOND_YEAR)
{
    // hh:mm:ss dd-MM-yyyy
    int count_char = 0;
    char tok[2];
    tok[0] = (option == 0) ? ':' : '-';
    tok[1] = '\0';
    int len = strlen(data);
    int i;
    for(i=0;i<len;i++)
    {
        if(data[i] == tok[0])
        {
            count_char++;
            if(count_char > 2)
                return 0;
        }
        else if(data[i] < '0' || data[i] > '9')
            return 0;
    }
    count_char = LOC_HOUR_DAY;
    int numb;
    int hour_day = *HOUR_DAY, minute_month = *MINUTE_MONTH, second_year = *SECOND_YEAR;
    char data_cp[sizeof(data)], *token;
    strcpy(data_cp, data);
    token = strtok(data_cp, tok);
    while(token)
    {
        numb = atoi(token);
        switch (count_char)
        {
        case LOC_HOUR_DAY:
            hour_day = numb;
            break;
        case LOC_MIN_MONTH:
            minute_month = numb;
            break;
        case LOC_SEC_YEAR:
            second_year = numb;
            break;
        }
        token = strtok(NULL, tok);
        count_char--;
    }
    if(option == TIME)
    {
        if((hour_day >= 0 && hour_day <= 23) && (minute_month >= 0 && minute_month <= 59) && (second_year >= 0 && second_year <= 59))
        {
            *HOUR_DAY = hour_day;
            *MINUTE_MONTH = minute_month;
            *SECOND_YEAR = second_year;
            return 1;
        }
    }else if(option == DATE)
    {
        if((minute_month >= 1 && minute_month <= 12) && (hour_day >= 1 && hour_day <= daysInMonth[minute_month - 1]) && (second_year >= YEAR_MIN))
        {
            *HOUR_DAY = hour_day;
            *MINUTE_MONTH = minute_month;
            *SECOND_YEAR = second_year;
            return 1;
        }
    }
    return 0;
}

int change_datetime_uart(int option, int *HOUR_DAY, int *MINUTE_MONTH, int *SECOND_YEAR)
{
    int hour_day = *HOUR_DAY, minute_month = *MINUTE_MONTH, second_year = *SECOND_YEAR;
    int result = 0;
    int time_wait = 0;
    int send_count = 3;
    char messenger_to_lcd[100];
    switch (MODE)
    {
    case SET_ALARM:
        strcpy(messenger_to_lcd, " SET ALARM");
        break;
    case CHANGE_TIME:
        strcpy(messenger_to_lcd, "CHANGE TIME");
        break;
    case CHANGE_DATE:
        strcpy(messenger_to_lcd, "CHANGE DATE");
        break;
    }
    display_to_lcd(1, 3, 1, messenger_to_lcd);
    send_messenger(messenger_to_lcd);
    COMMAND_UART[0] = '\0';
    LENGTH_COMMAND_UART = 0;
    CHECK_LENGTH = (option == 0) ? 8 : 10;
    while(send_count)
    {
        if(option == 0)
        {
            send_messenger("\nGui thoi gian (hh:mm:ss)\n");
            sprintf(messenger_to_lcd, "[hh:mm:ss]");
        }
        else
        {
            send_messenger("\nGui thoi gian (dd-MM-yyyy)\n");
            sprintf(messenger_to_lcd, "[dd-MM-yyyy]");
        }
        display_to_lcd(0, 3, 2, messenger_to_lcd);
        while(TIME_CHANGE - time_wait <= TIME_WAIT * 2)
        {
            delay(100);
            if(LENGTH_COMMAND_UART == CHECK_LENGTH)
                break;
        }
        if(TIME_CHANGE - time_wait >= TIME_WAIT * 2 || strcmp(COMMAND_UART, "EXNOW") == 0)
            break;
        if(check_data_uart(option, COMMAND_UART, &hour_day, &minute_month, &second_year))
        {
            *HOUR_DAY = hour_day;
            *MINUTE_MONTH = minute_month;
            *SECOND_YEAR = second_year;
            result = 1;
            MODE = 0;
            COMMAND_UART[0] = '\0';
            LENGTH_COMMAND_UART = 0;
            break;
        }
        else
        {
            send_count--;
            time_wait = TIME_CHANGE;
            printf("Khong ton tai thoi gian: '%s'.\nNhap lai....", COMMAND_UART);
            send_messenger("\nDu lieu khong hop le! Nhap lai....");
            COMMAND_UART[0] = '\0';
            LENGTH_COMMAND_UART = 0;
        }
    }
    CHECK_LENGTH = 7;
    return result;
}

void control_main()
{
    int time_on_alarm = 0;
    int result = 0;
    int sum_change = -1;
    char messenger_to_lcd[100];
    FLAG_SET_ALARM = ((IORD(SW_BASE, 0) & 0x02) != 0);
    OLD_SW = IORD(SW_BASE, 0);
    while(1)
    {
        if(FLAG_TIMER == 1)
        {
            printf("MODE = %d\n", MODE);
            FLAG_TIMER = 0;
            calculate_datetime();
            if(FLAG_DISPLAY == 0)
            {
                printf("Time: %02d:%02d:%02d\n", HOUR_CUR, MINUTE_CUR, SECOND_CUR);
                display_to_hex(0, HOUR_CUR, MINUTE_CUR, SECOND_CUR);
            }
            else
            {
                printf("Date: %02d-%02d-%04d\n", DAY, MONTH, YEAR);
                display_to_hex(1, DAY, MONTH, YEAR);
            }
            sprintf(messenger_to_lcd, "%02d:%02d:%02d", HOUR_CUR, MINUTE_CUR, SECOND_CUR);
            display_to_lcd(0, 4, 2, messenger_to_lcd);
            if(sum_change != DAY + MONTH + YEAR)
            {
                sum_change = DAY + MONTH + YEAR;
                sprintf(messenger_to_lcd, "%02d-%02d-%04d", DAY, MONTH, YEAR);
                display_to_lcd(0, 3, 1, messenger_to_lcd);
            }
            if(FLAG_SET_ALARM == ON && HOUR_CUR == HOUR_ALARM && MINUTE_CUR == MINUTE_ALARM && SECOND_CUR == SECOND_ALARM)
                time_on_alarm = TIME_ON_ALARM;
            if(FLAG_SET_ALARM == ON && time_on_alarm && FUNCTION != ENTER_KEY1)
            {
                control_alarm(ON);
                time_on_alarm--;
            }
        }
        if(FLAG_SET_ALARM == OFF || time_on_alarm == 0 || FUNCTION == ENTER_KEY1)
        {
            control_alarm(OFF);
            FUNCTION = 0;
            time_on_alarm = 0;
        }
        if(LENGTH_COMMAND_UART == CHECK_LENGTH)
        {
            printf("COMMAND UART: %s\n", COMMAND_UART);
            if(strcmp(COMMAND_UART, "ONALARM") == 0)
                MODE = SET_ALARM;
            else if(strcmp(COMMAND_UART, "CHGTIME") == 0)
                MODE = CHANGE_TIME;
            else if(strcmp(COMMAND_UART, "CHGDATE") == 0)
                MODE = CHANGE_DATE;
            else if(strcmp(COMMAND_UART, "EXITNOW") == 0)
                MODE = EXIT;
            else if(strcmp(COMMAND_UART, "OFALARM") == 0)
            {
                control_alarm(OFF);
                time_on_alarm = 0;
                COMMAND_UART[0] = '\0';
                LENGTH_COMMAND_UART = 0;
                send_messenger("\nDa tat bao thuc.\n");
            }
            else
            {
                COMMAND_UART[0] = '\0';
                LENGTH_COMMAND_UART = 0;
            }
        }
        switch (MODE)
        {
        case SET_ALARM:
            if(FLAG_UART == 1)
            {
                FLAG_UART = 0;
                COMMAND_UART[0] = '\0';
                LENGTH_COMMAND_UART = 0;
                FLAG_SET_ALARM = ON;
                printf("Cai bao thuc bang UART\n");
                result = change_datetime_uart(0, &HOUR_ALARM, &MINUTE_ALARM, &SECOND_ALARM);
            }
            else result = change_datetime_sw(0, &HOUR_ALARM, &MINUTE_ALARM, &SECOND_ALARM);
            if(result)
            {
                printf("Cai bao thuc thanh cong.\n");
                send_messenger("\nCai bao thuc thanh cong.\n");
                display_to_lcd(1, 5, 1, "SUCCESS");
                delay(10000);
                lcd_command(0x01);
            }
            else
            {
                printf("Cai bao thuc khong thanh cong.\n");
                send_messenger("\nCai bao thuc khong thanh cong.\n");
                display_to_lcd(1, 5, 1, "FAILURE");
                delay(10000);
                lcd_command(0x01);
            }
            sprintf(messenger_to_lcd, "%02d-%02d-%04d", DAY, MONTH, YEAR);
            display_to_lcd(1, 3, 1, messenger_to_lcd);
            break;
        case CHANGE_TIME:
            printf("Thay doi TIME.\n");
            if(FLAG_UART)
            {
                FLAG_UART = 0;
                COMMAND_UART[0] = '\0';
                LENGTH_COMMAND_UART = 0;
                result = change_datetime_uart(0, &HOUR_CUR, &MINUTE_CUR, &SECOND_CUR);
            }
            else result = change_datetime_sw(0, &HOUR_CUR, &MINUTE_CUR, &SECOND_CUR);
            if(result)
            {
                TIME_CHANGE = 0;
                printf("Thay doi TIME thanh cong.\n");
                send_messenger("\nThay doi TIME thanh cong.\n");
                display_to_lcd(1, 5, 1, "SUCCESS");
                delay(10000);
                lcd_command(0x01);
            }
            else
            {
                printf("Thay doi TIME khong thanh cong.\n");
                send_messenger("\nThay doi TIME khong thanh cong.\n");
                display_to_lcd(1, 5, 1, "FAILURE");
                delay(10000);
                lcd_command(0x01);
            }
            sprintf(messenger_to_lcd, "%02d-%02d-%04d", DAY, MONTH, YEAR);
            display_to_lcd(1, 3, 1, messenger_to_lcd);
            break;
        case CHANGE_DATE:
            printf("Thay doi DATE.\n");
            if(FLAG_UART)
            {
                FLAG_UART = 0;
                COMMAND_UART[0] = '\0';
                LENGTH_COMMAND_UART = 0;
                result = change_datetime_uart(1, &DAY, &MONTH, &YEAR);
            }
            else result = change_datetime_sw(1, &DAY, &MONTH, &YEAR);
            if(result)
            {
                printf("Thay doi DATE thanh cong.\n");
                send_messenger("\nThay doi DATE thanh cong.\n");
                display_to_lcd(1, 5, 1, "SUCCESS");
                delay(10000);
                lcd_command(0x01);
            }
            else
            {
                printf("Thay doi DATE khong thanh cong.\n");
                send_messenger("\nThay doi DATE khong thanh cong.\n");
                display_to_lcd(1, 5, 1, "FAILURE");
                delay(10000);
                lcd_command(0x01);
            }
            sprintf(messenger_to_lcd, "%02d-%02d-%04d", DAY, MONTH, YEAR);
            display_to_lcd(1, 3, 1, messenger_to_lcd);
            break;
        }
    }
}

int main()
{
    lcd_init();

    //Dang ky ngat cho TIMER
    Timer_Init();
    alt_ic_isr_register(0, TIMER_0_IRQ, Timer_IRQ_Handler, (void*)0, (void*)0);

    //Dang ky ngat cho SW
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(SW_BASE, 0xFF);
    alt_ic_isr_register(SW_IRQ_INTERRUPT_CONTROLLER_ID, SW_IRQ, SW_ISR, (void*)0, (void*)0);

    //Dang ky ngat cho KEY
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(KEY_BASE, 0x03);
    alt_ic_isr_register(KEY_IRQ_INTERRUPT_CONTROLLER_ID, KEY_IRQ, KEY_ISR, (void*)0, (void*)0);

    //Dang ky ngat cho UART
    IOWR_ALTERA_AVALON_UART_CONTROL(UART_BASE, ALTERA_AVALON_UART_CONTROL_RRDY_MSK);
    alt_ic_isr_register(UART_IRQ_INTERRUPT_CONTROLLER_ID, UART_IRQ, UART_ISR, (void*)0, (void*)0);

    control_main();
    return 0;
}
