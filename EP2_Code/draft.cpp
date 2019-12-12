/*
------ HCMUTE - FACULTY OF ELECTRICAL AND ELECTRONIC ENGINEERING -------
@Date:          11/2019
@Project:       Electronics Scale 
@Author:        Nguyen Tan Hung
                Nguyen Minh Tuan
@Consultor:     Ths. Nguyen Dinh Phu
*/
#include "Arduino.h"
#include "HX711.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Keypad.h>
#include <string.h>

#define     ALERT               13
//DEFINE HX711 PIN
#define     HX711_DATA          A0
#define     HX711_SCK           A1
//DEFINE LCD PIN
#define     LCD_D4              9
#define     LCD_D5              8
#define     LCD_D6              7
#define     LCD_D7              6
#define     E                   10
#define     RW                  11
#define     RS                  12
//DEFINE MATRIX KEY PIN
#define     ROW0                5
#define     ROW1                4
#define     ROW2                3
#define     ROW3                2
#define     COL0                A2
#define     COL1                A3
#define     COL2                A4
#define     COL3                A5

//DEFINE ADDRESS ON EEPROM
#define     ADDR_PD_PRICE       0   // FROM 0 TO 29
#define     ADDR_PD_KIND        30  // FROM 30 TO 39
#define     ADDR_DF_PRICE       40
#define     ADDR_DF_KIND        43      
#define     ADDR_MODE           44    
#define     ADDR_DF             45
#define     ADDR_ID             46
#define     ADDR_ZERO           50
#define     ADDR_SGN_ZERO       53
#define     ADDR_SAMPLE         54

//DEFINE KEY'S FUNCTION
#define     CLEAR               12
#define     UNDO                13
#define     ENTER               11
#define     CANCEL              10
#define     KIND                15

#define     DEFAULT             15
#define     CORRECT             14
#define     EDIT                10

// CONFIGURE LCD
LiquidCrystal lcd(RS, RW, E, LCD_D4,LCD_D5, LCD_D6, LCD_D7);

// CONFIGURE AND DECLARATION GLOBAL VARIABLE FOR HX711
HX711 scale(HX711_DATA, HX711_SCK);         
float zero, weight, amount;
float sample;
bool sgn_zero;

// CONFIGURE AND DECLARATION GLOBEL VARIABLE FOR MATRIX KEY
unsigned char   key = 16, key_value ;
const byte              col_array[]={COL0,COL1,COL2,COL3};
const byte              row_array[]={ROW0,ROW1,ROW2,ROW3};
const unsigned char     key_array[]={16, 1, 2, 3, 10, 4, 5, 6, 11, 7, 8, 9, 12, 14, 0, 15, 13 };
char key_map[4][4] ={   {1, 2, 3, 4},
                        {5, 6, 7, 8},
                        {9, 10, 11, 12},
                        {13, 14, 15, 16}};
Keypad keypad =  Keypad(makeKeymap(key_map), row_array, col_array, 4, 4);

// PRODUCT'S DATA  AND DECLARATION GLOBAL VARIABLE 
const char*        product_array[10]={"Beef","Pork","Fish","Chicken","Banana","Tomato","Egg","Milk","Pen","Book"};
unsigned long int  price_array[10]={};
bool               kind_array[10]={};
int     id;
char*   product;
unsigned long int pd_price, price_tmp, total,df_price, pass;
bool df_kind, mode, pd_kind, default_, kind_tmp;