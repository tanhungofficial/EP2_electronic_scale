/*
--------- HCMUTE - FACULTY OF ELECTRICAL AND ELECTRONIC ENGINEERING -----------
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

//DEFINE KEY VALUE
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
const char*                 product_array[10]={"Beef","Pork","Fish","Chicken","Banana","Tomato","Egg","Milk","Pen","Book"};
unsigned long int           price_array[10]={};
bool                        kind_array[10]={};
int     id;
char*   product;
unsigned long int pd_price, price_tmp, total,df_price, pass;
bool df_kind, mode, pd_kind, default_, kind_tmp;

// ######################## SUB-FUNCTION ######################################
void alert(int time)
{
    digitalWrite(ALERT, HIGH);
    delay(time);
    digitalWrite(ALERT, LOW);
}

float transfer_weight(int n)
{
    float weight;
    weight = scale.get_units(n) - zero;
    if( weight < 0) weight =0;
    return weight;
}
void write_eeprom_3byte(int min_addr, unsigned long int value)
{
    EEPROM.write(min_addr, value/65536);
    delay(5);
    EEPROM.write(min_addr + 1, (value%65536)/256);
    delay(5);
    EEPROM.write(min_addr + 2, value%256);
    delay(5);
}

unsigned long int read_eeprom_3byte(int min_addr)
{
    unsigned long int value = 0;
    value += static_cast<unsigned  int> (EEPROM.read(min_addr))*65536;
    delay(5);
    value += static_cast<unsigned  int> (EEPROM.read(min_addr + 1))*256;
    delay(5);
    value += static_cast<unsigned  int> (EEPROM.read(min_addr + 2));
    delay(5);
    return static_cast<unsigned long int> (value);
}

void lcd_money_right_alignment(int lcd_col, int lcd_row, long int  number)
{  
    if(number < 1000000000)
    { int size = 1;
        long int diff = 1, number_copy = number;
        for(number_copy; number_copy > 9; number_copy /= 10) 
        {
            size++;
            diff *= 10;
        }
        if(size < 4) size = size; 
        else if(size < 7) size++;
        else size += 2;
        lcd.setCursor(lcd_col, lcd_row);
        for(int i = 0; i < 19 - lcd_col - size; i++) lcd.print(" ");
        lcd.setCursor(19, lcd_row);
        lcd.print('d');
        lcd.setCursor(19 - size, lcd_row);
        number_copy = number;
        while(size > 0)
        {
            if(size%4 == 0)   lcd.print(',');
            else
            {
                lcd.print(number_copy/diff);
                number_copy %= diff;
                diff /= 10;
            }
            size--;  
        } 
    }  
}

void lcd_text_right_alignment(int lcd_col, int lcd_row, char* string)
{
    int size;
    size= strlen(string);
    lcd.setCursor(lcd_col,lcd_row);
    for(int i=0;i<20-size-lcd_col;i++) lcd.print(" ");
    lcd.print(string);
}

void lcd_counter(void)
{
    lcd.setCursor(3, 0);   lcd.print("Counter:");
    lcd.setCursor(0, 1);   lcd.print("Price:");
    lcd.setCursor(0, 2);   lcd.print("Amount:");
    lcd.setCursor(0, 3);   lcd.print("Total:");
    if(default_ == 0)   
    {
        lcd.setCursor(12, 0);    lcd.print(product);
        lcd_money_right_alignment(7, 1, pd_price);
    }else                
    {   
        lcd.setCursor(12, 0);    lcd.print("Default");
        lcd_money_right_alignment(7, 1, df_price);
    }   
}

void lcd_weight(void)
{
    lcd.setCursor(4, 0);   lcd.print("Weigh:");
    lcd.setCursor(0, 1);   lcd.print("Price:");
    lcd.setCursor(0, 2);   lcd.print("Weight:");
    lcd.setCursor(0, 3);   lcd.print("Total:");
    if(default_ == 0)
    {
        lcd.setCursor(11, 0);    lcd.print(product);
        lcd_money_right_alignment(7, 1, pd_price);
    }else                
    {   
        lcd.setCursor(11, 0);    lcd.print("Default");
        lcd_money_right_alignment(7, 1, df_price);
    }
}

void lcd_calculation( float weight)
{
    if (default_ == 1)
    {
        if(df_kind == 0)
        {
            total = round(weight*df_price/1000);
            lcd_money_right_alignment(7, 3, total);
            if(weight < 1000)
            {
                lcd_text_right_alignment(10, 2, "Gam");
                if(weight >= 100)       lcd.setCursor(10, 2);
                else if (weight >= 10)  lcd.setCursor(11, 2);
                else                    lcd.setCursor(12, 2);
                lcd.print(weight);
            }else
            {
                weight = weight/1000;
                lcd_text_right_alignment(10, 2, "Kg");
                lcd.setCursor(13, 2);        lcd.print(weight);
            }    
        }else
        {
            amount = round(weight/(sample + 0.0001));
            if( amount < 1000)
            {
                total = df_price*amount;
                lcd.setCursor(15, 2);   lcd.print("     ");
                if(amount >= 100)       lcd.setCursor(17, 2);
                else if (amount > 9)    lcd.setCursor(18, 2);
                else                    lcd.setCursor(19, 2);
                lcd.print(static_cast<int>(amount));
                lcd_money_right_alignment(7, 3, total);
            }else
            {
                lcd.setCursor(17, 2);        lcd.print("---");
                lcd.setCursor(7, 3);         lcd.print("          ---");
            }
            
        }    
    }else
    {
        if(pd_kind == 0)
        {
            total = round(weight*pd_price/1000);
            lcd_money_right_alignment(7, 3, total);
            if(weight<1000)
            {
                lcd_text_right_alignment(10, 2, "Gam");
                if(weight >= 100)       lcd.setCursor(10, 2);
                else if (weight >= 10)  lcd.setCursor(11, 2);
                else                    lcd.setCursor(12, 2);
                lcd.print(weight);
            }else
            {
                weight = weight/1000;
                lcd_text_right_alignment(10, 2, "Kg");
                lcd.setCursor(13, 2);        lcd.print(weight);
            }
        }else
        {
            amount = round(weight/(sample + 0.0001));
            if (amount < 1000)
            {
                total = pd_price*amount;
                lcd.setCursor(15, 2);   lcd.print("     ");
                if(amount >= 100)       lcd.setCursor(17, 2);
                else if (amount >= 10)  lcd.setCursor(18, 2);
                else                    lcd.setCursor(19, 2);
                lcd.print(static_cast<long int>(amount));
                lcd_money_right_alignment(7, 3, total);
            }else
            {
                lcd.setCursor(17, 2);        lcd.print("---");
                lcd.setCursor(7, 3);         lcd.print("          ---");       
            }
        }     
    }    
}

void lcd_setting(void)
{
    lcd.setCursor(5, 0);   lcd.print("Setting");
    lcd.setCursor(0, 1);   lcd.print("Product:");
    lcd.setCursor(0, 2);   lcd.print("Mode:");
    lcd.setCursor(0, 3);   lcd.print("Price:");
    if(default_ == 0) lcd_text_right_alignment(10, 1, product);
    else lcd_text_right_alignment(10, 1, "default");

    if(kind_tmp == 0)   lcd_text_right_alignment(10, 2, "weigh");
    else                lcd_text_right_alignment(10, 2, "counter");
    lcd_money_right_alignment(8, 3, price_tmp);
}

void lcd_welcome(void)
{
    char  project[] = {'D','O',' ','A','N',' ','2',':',' ','C','A','N',' ','D','I','E','N',' ','T','U'};
    lcd.setCursor(4, 1); lcd.print("HCMUTE-FEEE");
    lcd.setCursor(0, 2);
    for(int i = 0; i < sizeof(project); i++) 
    {
      lcd.print(project[i]);  delay(90);
    }
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 0);    lcd.print("Author:");
    for(int i = 3; i > 0; i--)
    {
        lcd.setCursor(1, i);     lcd.print("1. NGUYEN TAN HUNG");
        delay(500);
        if(i > 1) 
        {
            lcd.setCursor(1, i);     lcd.print("                   ");
        }
    }
    for(int i = 3; i > 1; i--)
    {
        lcd.setCursor(1, i);     lcd.print("2. NGUYEN MINH TUAN");
        delay(500);
        if(i > 2) 
        {
            lcd.setCursor(1, i);     lcd.print("                   ");
        }
    }
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 1);             lcd.print("Consultor:");
    for(int i = 0; i < 5; i++)
    {
        if(i%2 == 0)  
        {
            lcd.setCursor(0, 2);     lcd.print("Ths. NGUYEN DINH PHU");
        }else
        {
            lcd.setCursor(0, 2);     lcd.print("                    ");
        }
        delay(500);   
    }
    delay(1000);
}

void lcd_refresh()
{
    lcd.clear();
    if (mode == 0)
    {
        if (default_ == 1)
        {
            if (df_kind == 0) lcd_weight();
            else              lcd_counter();
        }else
        {
            if (pd_kind == 0) lcd_weight();
            else              lcd_counter();
        }       
    }else lcd_setting();  
}

void lcd_correct(bool type)
{
    if (type == 0)
    {
        lcd.clear();
        char* wait[3] = {".  ",".. ","..."};
        lcd.setCursor(3, 1);
        lcd.print("Correcting");
        for(int i = 0; i < 3; i++)
        {
            lcd.setCursor(13, 1);
            lcd.print(wait[i]);
            delay(500);
        }
        zero = scale.get_units(5);
        if(zero < 0 )  sgn_zero = 1;
        else            sgn_zero = 0;
        write_eeprom_3byte(ADDR_ZERO, abs(zero));
        EEPROM.write(ADDR_SGN_ZERO, sgn_zero);
        lcd.setCursor(5, 2);
        lcd.print("Completed!");
        delay(400);
    }
    else
    {
        lcd.clear();
        char* wait[3] = {".  ",".. ","..."};
        lcd.setCursor(2,1);
        lcd.print("Taking sample");
        for(int i = 0; i < 3; i++)
        {
            lcd.setCursor(15, 1);
            lcd.print(wait[i]);
            delay(500);
        }
        sample = transfer_weight(5);
        write_eeprom_3byte(ADDR_SAMPLE, sample);
        lcd.setCursor(5, 2);
        lcd.print("Completed!");
        delay(400);
    } 
}

unsigned char get_key(int hold_delay)
{
    int state = 0;
    unsigned char key = 0;
    unsigned char temp = keypad.getKey();
    if((int)keypad.getState() ==  PRESSED) 
    {
        if(temp != 0) key = temp;
    }
    if((int)keypad.getState() ==  HOLD) 
    {
        state++;
        state = constrain(state, 1, 2);
        delay(hold_delay);
    }
    if((int)keypad.getState() ==  RELEASED) 
    {
        key += state;
    }
    return key;
}

void exe_key_when_scale(unsigned char key_value)
{
    if (key_value == DEFAULT )
    {
        alert(20);
        if(default_ == 0) 
        {
            default_ = 1;
            EEPROM.write(ADDR_DF, default_);
        }else
        {
            df_kind = !df_kind;
            EEPROM.write(ADDR_DF_KIND, df_kind);
        } 
        lcd_refresh();
    }
    if (key_value == CORRECT)
    {
        alert(20);
        if (default_ == 1)
        {
            lcd_correct(df_kind);
        }else
        {
            lcd_correct(pd_kind);
        }    
        lcd_refresh();
    }
    if(key_value==EDIT)
    { 
        alert(20);
        mode = 1;
        EEPROM.write(ADDR_MODE, mode);
        if(default_ == 1)   
        {
            price_tmp = df_price;
            kind_tmp = df_kind;
        }
        else  
        {
            price_tmp = pd_price;
            kind_tmp = pd_kind;
        }              
        lcd_refresh();
    }
    if(key_value<10)
    {
        alert(20);
        id=key_value;
        EEPROM.write(ADDR_ID, id);
        pd_price = price_array[id];
        pd_kind= kind_array[id];
        product = product_array[id];
        default_ = 0;
        EEPROM.write(ADDR_DF, default_);
        lcd_refresh();
    } 
}

void exe_key_when_setting(unsigned char key_value)
{
    char* wait[3]={".  ",".. ","..."};
    for(int i = 0; i < 3; i++)
    {
        lcd.setCursor(12, 0);
        lcd.print(wait[i]);
        delay(20);
    }
    if(key_value == CLEAR) 
    {
        price_tmp = 0;
        if(default_ == 1)   kind_tmp = df_kind;
        else                kind_tmp = pd_kind;        
        lcd_money_right_alignment(8,3,price_tmp); 
        if( kind_tmp == 0)  lcd_text_right_alignment(10, 2, "weigh");
        else                lcd_text_right_alignment(10, 2, "counter");
    }
    if(key_value == ENTER)
    {
        alert(20);
        if(default_ == 1)
        {
            df_price = price_tmp;
            write_eeprom_3byte(ADDR_DF_PRICE, df_price);
            df_kind = kind_tmp;
            EEPROM.write(ADDR_DF_KIND, df_kind);
            
        }else
        {
            
            write_eeprom_3byte(id*3, price_tmp);
            EEPROM.write(ADDR_PD_KIND + id, kind_tmp);
            price_array[id] = price_tmp;
            kind_array[id] = kind_tmp;
            pd_kind = kind_tmp;
            pd_price = price_tmp;
        }
        mode =0;
        EEPROM.write(ADDR_MODE, mode);
        lcd_refresh();
    }
    if(key_value<10) 
    {
        if(price_tmp<=100000)  price_tmp = price_tmp*10 + key_value;  
        lcd_money_right_alignment(8, 3, price_tmp);  
    } 
    if(key_value == CANCEL)
    {
        alert(20);
        mode = 0;
        EEPROM.write(ADDR_MODE, mode);
        lcd_refresh();
    }
    if(key_value == UNDO)
    {
        price_tmp /= 10;
        lcd_money_right_alignment(8, 3, price_tmp);  
    }
    if(key_value == KIND)
    {
        kind_tmp = !kind_tmp;
        if( kind_tmp == 0)  lcd_text_right_alignment(10, 2, "weigh");
        else                lcd_text_right_alignment(10, 2, "counter");
    }
}

// ########################### MAIN FUNCTION #######################################
void setup()
{
    pinMode(ALERT,  OUTPUT);
    pinMode(RS,     OUTPUT);
    pinMode(RW,     OUTPUT);
    pinMode(LCD_D4, OUTPUT);
    pinMode(LCD_D5, OUTPUT);
    pinMode(LCD_D6, OUTPUT);
    pinMode(LCD_D7, OUTPUT);
    pinMode(ROW0,   OUTPUT);
    pinMode(ROW1,   OUTPUT);
    pinMode(ROW2,   OUTPUT);
    pinMode(ROW3,   OUTPUT);
    pinMode(COL0,   INPUT);
    pinMode(COL1,   INPUT);
    pinMode(COL2,   INPUT);
    pinMode(COL3,   INPUT);
    digitalWrite(ALERT, LOW);

    //SETUP SCALE
    scale.set_scale(383.f);
    scale.tare();
    sample = read_eeprom_3byte(ADDR_SAMPLE);  
    zero = read_eeprom_3byte(ADDR_ZERO);
    sgn_zero = EEPROM.read(ADDR_SGN_ZERO);
    if (sgn_zero == 1) zero = -zero;

    //SETUP DATABASE OF PRODUCT
    default_ = EEPROM.read(ADDR_DF);
    mode = EEPROM.read(ADDR_MODE);
    df_kind = EEPROM.read(ADDR_DF_KIND);
    df_price = read_eeprom_3byte(ADDR_DF_PRICE);
    for(int i = 0; i < 10; i++) 
    {
        price_array[i] = read_eeprom_3byte(i*3);
        kind_array[i] = EEPROM.read(ADDR_PD_KIND + i);
        delay(5);
    }
    id =  EEPROM.read(ADDR_ID);
    pd_kind = kind_array[id];
    pd_price = price_array[id];
    product = product_array[id];
    pass = 0;
    total = 0;

    //SETUP LCD
    lcd.begin(20, 4);
    lcd_welcome();
    lcd_refresh();    
}
void loop()
{
    key = get_key(20);
    key_value = key_array[key];
    if(mode == 0) 
    {
        if(pass < 100000) pass++;
        else
        {
            weight = transfer_weight(5);
            if(weight > 5000) alert(200);
            lcd_calculation(weight);
            pass = 0;
        }
        exe_key_when_scale(key_value);
    }
    else 
    {
        exe_key_when_setting(key_value);
    } 
   
}
