// v. aj.jed 4 sep 2021
#include <Wire.h>
#include <SH1106Wire.h>   // legacy: #include "SH1106.h"
#include <Keypad_I2C.h>
#include <rdm6300.h>
#include <WiFi.h>
#include <time.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "string.h"
#include "stdlib.h"
#include "dw_font.h"
#include <EEPROM.h>

#define PIN_COUNTER 5       // count machine
#define I2CADDR 0x20        //------   oled seting Address
#define RDM6300_RX_PIN 16
//---------- RX ESP32 pin 26 and Tx pin 17
#define LED 2               //build-in 2
#define time_interval 1000
#define connecttime 50000
#define connecttime1 500
#define EEPROM_SIZE 512

#define ERR_TOUCH_001     1
#define ERR_COUNT_002     2
#define ERR_BREAK_003     3
#define ERR_COUNT_007     7
#define ERR_TOUCH_008     8
#define ERR_BREAK_009     9
#define ERR_QUITE_010     10
#define ERR_QUITE_011     11
#define ERR_CONTINUE_012  12
#define ERR_BREAK_013     13
#define ERR_BREAK_014     14
#define ERR_DOWNTIME_015  15
#define ERR_TECH_016      16

#define STATUS_OK         0
#define ERR_GET_NULL     -2
#define ERR_JSON_STACK   -1

#define LINE1       8
#define LINE2       23
#define LINE3       38
#define LINE4       53
#define LINE5       68
/*
  #define LINE1       10
  #define LINE2       25
  #define LINE3       40
  #define LINE4       55
  #define LINE5       70
*/
#define TAB0        0
#define TAB5        5
#define TAB10       10
#define TAB15       15
#define TAB20       20
#define TAB40       40

unsigned int time_out = 0x05FF;
unsigned int time_out1 = 0x0FFF;
bool flag_count_down = 0;
short counter_ = 0;

SH1106Wire display(0x3c, 21, 22);                     // ADDRESS, SDA, SCL
extern dw_font_info_t font_th_sarabun_new_regular14;  // font TH
dw_font_t myfont;

unsigned long period_get = 30000;
unsigned long period_post = 3000;
unsigned long last_time_get = 0;
unsigned long last_time_post = 0;

// setting WiFi and Time server
const char* ssid;
const char* password;
String setssid;
char setpassword;
String readssid;
String readpassword;

String wifi1 = "";

// --------------------------- set keypad
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1', '4', '7', '*'},
  {'2', '5', '8', '0'},
  {'3', '6', '9', '#'},
  {'a', 'b', 'C', 'D'}
};
byte rowPins[ROWS] = {3, 2, 1, 0}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {7, 6, 5, 4}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad_I2C customKeypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2CADDR);
Rdm6300 rdm6300;
//-------------------------------------------------------
int a = 32, b = 0, staterf = 0, con = 0;
int set = 0, c = 0, key = 0, num1;
int RFstate = 0; int keystate = 0; int setsh ; int setb = 0;
int settingmachine = 0; int u = 10; int numrole;
String l; String pass;  String IDcard;  String IDcard1; String IDcard2;
String machine; String customKeyset; String readmachine;
int confirmRF = 0;  int confirmtime = 0; int settingmenu = 0;
unsigned int prev_time, prev_time1;
byte led_state = 0;
char customKey1; char customKey; char customKey2;

int dataqty , datacount, addeeqty = 2, addeecount = 7; // address EEPROM set qty = 2,
int addchackee = 1; int addmac = 14;                   // count = 7, chack = 1, machine = 14.
int readcount, readqty , chack;                         // chack = ตรวจสถานะไฟตก
int addssid = 100; int addpass = 30;
int n; int pauseset = 0; int f = 0;

String tem; String tem1; String code;
//-------------------------------------------------------
// set employ
typedef struct employ_touch {
  char* id_staff ;
  char* name_first ;
  char* name_last ;
  int role ;
  char * id_task ;
  char * id_job ;
  char * item_no ;
  char* operation ;
  char * op_name ;
  char * qty_order ;
  char * qty_comp ;
  char * qty_open ;
  uint8_t flag_err ;
} employ_touch_TYPE ;

typedef enum {
  BR_NOBREAK,
  BR_CUTOFF ,
  BR_TOILET ,
  BR_LUNCH
} break_type ;


break_type state_break ;

volatile bool interruptWork;
volatile bool interruptBreak;
int workCounter;
int breakCounter;

volatile bool isWork = 0;
volatile bool isBreak = 0;
const int interruptPin = 21;

volatile uint32_t count, qty;
volatile int8_t flag = 0;
char buff[300];
char buff1[300];
char buff2[300];
char buff3[300];
String msg, msg1;

static employ_touch_TYPE dst;

String str = "";

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimerWork() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptWork = 1;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void IRAM_ATTR onTimerBreak() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptBreak = 1;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void IRAM_ATTR countUp ( void )
{
  qty++;
  flag = 1;
}

String translate_hh_mm_cc( int sec )
{
  int h, m, s;
  String buff = "" ;
  h = (sec / 3600);
  m = (sec - (3600 * h)) / 60;
  s = (sec - (3600 * h) - (m * 60));
  buff += ( h > 9 ) ? "" : "0";
  buff += h ;
  buff += ": " ;
  buff += ( m > 9 ) ? "" : "0";
  buff += m ;
  buff += ": " ;
  buff += ( s > 9 ) ? "" : "0";
  buff += s ;
  return buff;
}

void setup() {
  Wire.begin();
  EEPROM.begin(EEPROM_SIZE);
  customKeypad.begin(); // keypad
  rdm6300.begin(RDM6300_RX_PIN);
  Serial.begin(115200);
  Serial.println("\n\n\n");
  Serial.println("========================================");
  Serial.println("Majorette (Thailand) Co.,LTD. ---> Start");
  pinMode(LED, OUTPUT);
  pinMode(PIN_COUNTER, INPUT );
  //attachInterrupt(digitalPinToInterrupt(PIN_COUNTER), countUp, CHANGE );
  //--- timer
  timer = timerBegin(0, 80, true);
  timerDetachInterrupt(timer);

  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  dw_font_init(&myfont,
               128,
               64,
               draw_pixel,
               clear_pixel);
  dw_font_setfont(&myfont, &font_th_sarabun_new_regular14);

  // Wifi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  EEPROM.get(addssid, readssid);
  EEPROM.get(addpass, readpassword);
  Serial.print("SSID: ");
  Serial.println(readssid);
  Serial.print("Password: ");
  Serial.println(readpassword);
  ssid = readssid.c_str();
  password = readpassword.c_str();
  WiFi.begin(ssid, password);

  if (WiFi.status() != WL_CONNECTED ) {
    display.clear();

    dw_font_goto(&myfont, TAB0, LINE1);
    dw_font_print(&myfont, "Majorette (Thailand) Co.,LTD");
    dw_font_goto(&myfont, TAB0, LINE2);
    dw_font_print(&myfont, "-----------------------------------------");
    dw_font_goto(&myfont, TAB0, LINE3);
    dw_font_print(&myfont, "กำลังเชื่อมต่อ WiFi");
    ssid = readssid.c_str();
    password = readpassword.c_str();
    dw_font_goto(&myfont, TAB40, LINE4);
    sprintf( buff , "SSID: %s" ,  ssid);
    dw_font_print(&myfont, buff);
    display.display();
    ssid = ""; password = "";
    con = 0;
  }
  Serial.println("v.1.4");
}
void loop() {
  // show Wifi options

  if (WiFi.status() != WL_CONNECTED) {
    customKey1 = customKeypad.getKey();
    while (customKey1 != NO_KEY && customKey1 == 'a' && WiFi.status() != WL_CONNECTED) {
      if (con == 0) {
        key = 0; set = 0; b = 0;
        ssid = "";  password = ""; pass = "";
        display.resetDisplay();
        display.clear();
        dw_font_goto(&myfont, 35, LINE1);
        dw_font_print(&myfont, "กำลังสแกน WiFi");
        display.display();
        con = 1;
      }
      if (con == 1) {
        if (set == 0) {
          int n = WiFi.scanNetworks();
          if (n == 0) {
            display.drawString(19, 0, "No networks found.");
          }
          display.clear();
          display.drawString(0, 0, "Select SSID:");
          display.display();
          for (int i = 0; i < 5; ++i) {
            String ion = WiFi.SSID(i);
            wifi1 = String(i + 1) + String(": ") + String(ion);
            Serial.println(wifi1);
            b = b + 10;
            display.drawString(0, b, wifi1);
            display.display();
            set = 1 ;
            customKey = NO_KEY;
          }
        }
        //get value keypad to select Wifi
        String readstr;
        char customKey = customKeypad.getKey();
        if (customKey != NO_KEY && key == 0) {
          readstr += customKey;
          num1 = readstr.toInt();
          l = WiFi.SSID(num1 - 1);
          setssid = l;
          ssid = l.c_str();
          display.clear();
          display.drawString(0, LINE1, "SSID:");
          display.drawString(30, LINE1, ssid);

          display.drawString(0, LINE2, "Pass: ");
          dw_font_goto(&myfont, 5, LINE4);
          dw_font_print(&myfont, "* ยืนยัน              ย้อนกลับ #");
          display.display();
          key = 1;
          customKey = NO_KEY;
        }
        // put password in SerialMonitor and connext
        if (customKey != NO_KEY && key == 1) {
          if (customKey != '*' && customKey != '#') {
            pass += customKey;
            String customKey1 = String(customKey);
            display.drawString(a, 10, customKey1);
            a = a + 7;
            if (a >= 120) {
              a = 32;
            }
            display.display();
            customKey = NO_KEY;
          }
          if (customKey == '*') {
            password = pass.c_str();
            keystate = 1; b = 0; a = 32;
            customKey = NO_KEY;
          }
          else if (customKey == '#') {
            set = 0; b = 0; key = 0; a = 32;
            ssid = "";  password = ""; pass = ""; readstr = "";
            customKey = NO_KEY;
            display.clear();
          }
          if (keystate == 1) {
            keystate = 0;
            display.drawString(24, LINE3, "Connecting WiFi.");
            dw_font_goto(&myfont, 40, LINE4);
            sprintf( buff , "SSID: %s" , ssid);
            dw_font_print(&myfont, buff);
            //display.drawString(24, 55, "Just the moment.");
            display.display();
            WiFi.begin(ssid, password);
            EEPROM.put(addssid, setssid);
            EEPROM.put(addpass, pass);
            EEPROM.commit();
            delay(2000);
            // connect wait completed
            while (WiFi.status() != WL_CONNECTED) {
              if (millis() - prev_time > connecttime) {
                key = 0; con = 1; set = 0; b = 0;
                prev_time = millis();
                ssid = "";  password = ""; pass = "";
                display.drawString(24, LINE3, "WiFi Not connected");
                display.display();
                delay(5000);
                display.clear();
                break;
              }
            }

            if (key == 1) {
              display.drawString(24, LINE3, "WiFi connected");
              display.display();
              key = 0, con = 0, set = 1;
              //ssid = "";  password = ""; pass = "";
              delay(3000);
              display.clear();
              display.resetDisplay();
            }
          }
        }
      }
    }

    if (customKey1 != NO_KEY && customKey1 == 'b' && WiFi.status() != WL_CONNECTED && con != 1) {
      if (customKey1 == 'b') {
        display.resetDisplay();
        settingmachine = 1;
        settingmenu = 0;
        customKey1 = NO_KEY;
      }
    }
    if (settingmachine == 1) {
      dw_font_goto(&myfont, 10, LINE1);
      dw_font_print(&myfont, "โปรดใส่เลขรหัสเครื่อง");
      display.display();
      if (u == 24) {
        display.drawString(24, LINE1, "-");
        machine += "-";
        u = 31;
      }
      else if (u == 45) {
        dw_font_goto(&myfont, 5, LINE4);
        dw_font_print(&myfont, "* ยืนยัน              ย้อนกลับ #");
        display.display();
      }
      else if (u >= 46) {
        dw_font_goto(&myfont, 20, LINE2);
        dw_font_print(&myfont, "เกิดข้อผิดพลาด");
        display.display();
        delay(3000);
        display.resetDisplay();
        machine = ""; customKeyset = "";
        u = 10;
      }
      // put numbet machine
      customKey = customKeypad.getKey();
      if (customKey != NO_KEY) {
        if (customKey != NO_KEY && customKey != '*' && customKey != '#') {
          machine += customKey;
          customKeyset = String(customKey);
          display.drawString(u, 10, customKeyset);
          u = u + 7;
          display.display();
          customKey = NO_KEY;
        }
        else if (customKey == '*') {
          customKey = NO_KEY;
          settingmachine = 0;
          Serial.println(machine);
          EEPROM.put(addmac, machine);
          EEPROM.commit();
          machine = ""; u = 10;
          display.resetDisplay();
          dw_font_goto(&myfont, 20, LINE2);
          dw_font_print(&myfont, "Please restart ");
          display.display();
        }
        else if (customKey == '#') {
          u = 10; customKey = NO_KEY;
          settingmachine = 1; machine = "";
          display.resetDisplay();
        }
      }
    }
  }

  //-------------------- upper scen WiFi and config ----------------\\

  // connect completed
  if (WiFi.status() == WL_CONNECTED )
  {

    //    int a = query_Touch_GetMethod( "1234", "5678" , &dst);
    //    Serial.print("Debug : ");
    //    Serial.println(a);

    if (RFstate == 0 && pauseset != 1 && confirmRF != 2) {
      EEPROM.get(addmac, readmachine);
      display.clear();
      //dw_font_goto(&myfont, 15, LINE2);
      //dw_font_print(&myfont, "mc/no."); // pop

      dw_font_goto(&myfont, 25, LINE2);
      sprintf( buff1 , "mc/no. %s", readmachine);
      dw_font_print(&myfont, buff1);

      dw_font_goto(&myfont, 15, LINE3);
      dw_font_print(&myfont, "โปรดทำการสแกนบัตร");
      display.display();
      //------------
      customKey1 = customKeypad.getKey();
      if (customKey1 != NO_KEY)
      {
        Serial.print("Count : ");
        Serial.println(counter_);
        if (customKey1 == 'a' && !flag_count_down)
        {

          flag_count_down = 1;
          counter_++;
        }
        if (customKey1 == 'C' && counter_ == 1)
        {
          Serial.println("C");
          counter_++;
        }
        if (customKey1 == 'b' && counter_ == 2)
        {
          //             counter_++;
          Serial.println("A + C + B !!");
          Serial.println("Success !");
          WiFi.disconnect();

          EEPROM.put(addssid, 0);
          EEPROM.put(addpass, 0);
          EEPROM.put(addeecount, 0);
          EEPROM.commit();
          
          con = 0;
          //time out
          time_out = 0x05FF;
          flag_count_down = 0;
          counter_ = 0;

          display.clear();
          dw_font_goto(&myfont, 15, 36);
          dw_font_print(&myfont, "โปรดเลือก A เพื่อเลือก WIFI");
          display.display();

         

        }
      }
      else
      {
        if ( flag_count_down )
        {
          if (time_out < 100)
          {
            Serial.println("Time Out !");
            time_out = 0x05FF;
            flag_count_down = 0;
            counter_ = 0;
          }
          time_out--;
        }


      }

      //-----------
      if (rdm6300.update() )
      {
        msg = String(rdm6300.get_tag_id());
        if (strlen ((const char *)msg.c_str()) == 8)
          msg = String("00") + msg;
        else if (strlen((const char *)msg.c_str()) == 7)
          msg = String("000") + msg;
        Serial.println(msg);

        if (msg != "")
        {
          switch (query_Touch_GetMethod( (const char *)readmachine.c_str(), (const char *)msg.c_str() , &dst)) // <-- Normal Case
            //          switch(query_Touch_GetMethod( (const char *)"1234", (const char *)"1234" , &dst)) // <-- Test Case Error!!
          {
            case 0 :    attachInterrupt(digitalPinToInterrupt(PIN_COUNTER), countUp, CHANGE );
              sprintf( buff , "ID : %s TIMESTAMP : %s VALUE : %s" , dst.id_staff , dst.name_first , dst.name_last );
              numrole = dst.role;

              IDcard = msg;
              display.resetDisplay();

              dw_font_goto(&myfont, 0, LINE1);
              sprintf( buff1 , "mc/no. %s", readmachine);
              dw_font_print(&myfont, buff1);
              display.display();

              dw_font_goto(&myfont, 0, LINE2);
              sprintf( buff , "ID : %s" , dst.id_staff);
              dw_font_print(&myfont, buff);
              //display.display();

              dw_font_goto(&myfont, 60, LINE2);
              //sprintf( buff , "Role : %d" , dst.role);  // --------------------???
              Serial.print("Role = ");
              if (dst.role == 1) {
                Serial.println("Operator");
                sprintf( buff , "%s" , "[Operator]");
              }
              else {
                Serial.println("Technician");
                sprintf( buff , "%s" , "[Technician]");
              }
              dw_font_print(&myfont, buff);

              dw_font_goto(&myfont, 0, LINE3);
              sprintf( buff , "ชื่อ : %s" , dst.name_first);
              dw_font_print(&myfont, buff);

              //Serial.println(dst.name_last);
              Serial.println(dst.name_last[0]);
              //sprintf( buff , " %c." ,  dst.name_last[0]);
              sprintf( buff , " %s" ,  dst.name_last);
              dw_font_print(&myfont, buff);
              display.display();
              //sprintf( buff , "นามสกุล : %s" ,  dst.name_last);

              dw_font_goto(&myfont, 5, LINE4);
              dw_font_print(&myfont, "* ยืนยัน                 ยกเลิก #");
              display.display();

              noInterrupts();
              msg = "";
              tem = "";
              confirmRF = 2;
              interrupts();

              rdm6300.update();
              while (!rdm6300.update()) {
                break;
              }
              break ;

            case ERR_TOUCH_001 :
              Serial.println("ERR_TOUCH_001");
              display.clear();
              dw_font_goto(&myfont, 15, LINE3);
              dw_font_print(&myfont, "ERR_TOUCH_001");
              display.display();
              delay(1500);
              rdm6300.update();
              while (!rdm6300.update()) {
                break;
              }
              break ;
          }
          noInterrupts();
          rdm6300.update();
          while (!rdm6300.update()) {
            break;
          } // switch
          interrupts();
        } // if

      }
    }

    if (confirmRF == 2) {
      // confrim Data RFID
      customKey1 = customKeypad.getKey();
      if (customKey1 != NO_KEY) {
        //confrim
        if (customKey1 == '*') {
          confirmRF = 1; RFstate = 1;
          timerAttachInterrupt(timer, &onTimerWork, true);
          timerAlarmWrite(timer, 1000000, true);
          timerAlarmEnable(timer);
          if (numrole == 2)
            settingmenu = 1;
          display.clear();
          display.resetDisplay();
          msg = ""; tem = "";

          if (chack == 0)
          {
            int v = 1;

            EEPROM.get(addeeqty, dataqty);

            EEPROM.put(addchackee, v);
            datacount = 1;
            //              dataqty = 0;
            EEPROM.put(addeecount , datacount);
            EEPROM.put(addeeqty, dataqty);
            EEPROM.commit();
            display.resetDisplay();
          }

          rdm6300.update();
          customKey1 = NO_KEY;
          while (!rdm6300.update()) {
            rdm6300.update();
            break;
          }
        }
        // unconfrimed
        else if (customKey1 == '#') {
          confirmRF = 0;  RFstate = 0;
          settingmenu = 0; tem = ""; msg = "";
          display.clear();
          display.resetDisplay();

          rdm6300.update();
          customKey1 = NO_KEY;
          while (!rdm6300.update()) {
            rdm6300.update();
            break;
          }
        }
      }
    }
    //--------- upper scen employ ---------\\

    // show qty and cont
    if (confirmRF == 1 && numrole == 1) {
      // up qty and cont to dashboard
      if (RFstate == 1) {
        if (interruptWork) {
          portENTER_CRITICAL_ISR(&timerMux);
          interruptWork = 0;
          portEXIT_CRITICAL_ISR(&timerMux);
          workCounter++;
          //Serial.print("Work : ");
          //Serial.println(translate_hh_mm_cc(workCounter));
        }

        dw_font_goto(&myfont, 0, LINE1);
        //EEPROM.get(addmac, readmachine);
        sprintf( buff1 , "mc/no. %s", readmachine);
        dw_font_print(&myfont, buff1);

        dw_font_goto(&myfont, 78 , LINE1);
        sprintf(buff3 , "%s", translate_hh_mm_cc(workCounter));
        dw_font_print(&myfont, buff3);

        dw_font_goto(&myfont, 0, LINE2);
        sprintf( buff1 , "ชื่อ: %s %s", dst.name_first , dst.name_last);
        dw_font_print(&myfont, buff1);

        dw_font_goto(&myfont, 0, LINE3);
        EEPROM.get(addeeqty, readqty);
        sprintf( buff1 , "Qty: %d / %s", readqty, dst.qty_order);
        dw_font_print(&myfont, buff1);

        dw_font_goto(&myfont, 100 , LINE3);
        EEPROM.get(addeecount, readcount);
        sprintf(buff2 , "%d", readcount);
        dw_font_print(&myfont, buff2);
        display.display();

        //------------------------------------------

        if ( millis() - last_time_get > period_get)
        {
          last_time_get = millis();
          // EEPROM
          if (datacount != count) {
            int chack = EEPROM.read(addchackee);
            Serial.println(chack);
            if (chack == 1) {
              EEPROM.get(addeeqty, readqty);
              EEPROM.get(addeecount, readcount);
              Serial.println(readcount);
              datacount = readcount + 1;
              dataqty = readqty;
              EEPROM.put(addeeqty, dataqty);
              EEPROM.put(addeecount , datacount);
              EEPROM.commit();
            }
          }
          count++;

          EEPROM.get(addeecount, readcount);
          EEPROM.get(addeeqty, readqty);
          //EEPROM.get(addmac, readmachine);
          sprintf( buff , "https://bunnam.com/projects/majorette_pp/update/count.php?id_task=%s&id_mc=%s&no_send=%d&no_pulse1=%d&no_pulse2=0&no_pulse3=0", dst.id_task, readmachine.c_str() , readcount, readqty);
          Serial.println( buff );
          msg = httpPOSTRequest(buff, "");
          Serial.println( msg );
          digitalWrite(LED, led_state);
          led_state = !led_state;
//          display.clear();
        }

        if ( flag )
        {
          int chack = EEPROM.read(addchackee);
          Serial.print("chack :");
          Serial.println(chack);

          if (chack == 1) {
            EEPROM.get(addeeqty, dataqty);
            dataqty++;
            EEPROM.put(addeeqty, dataqty);
            EEPROM.commit();
            Serial.println(dataqty);
          }
          flag = 0;
        }

        // stop and pause time
        if (rdm6300.update()) {
          tem1 = String(rdm6300.get_tag_id());
          if (strlen ((const char *)tem1.c_str()) == 8)
            IDcard1 = String("00") + tem1;
          else if (strlen((const char *)tem1.c_str()) == 7)
            IDcard1 = String("000") + tem1;

          if (IDcard1 == IDcard) {

            confirmtime = 1;
            IDcard1 = ""; tem1 = ""; 
            flag_count_down = 1;
            time_out = 0x05FF;
            dw_font_goto(&myfont, 0, LINE4);
            dw_font_print(&myfont, "* พักเบรก    หยุดการทำงาน #");
            display.display();
            
            rdm6300.update();
            while (!rdm6300.update()) {
              break;
            }
          }
          else {
            display.resetDisplay();
            dw_font_goto(&myfont, 15, LINE3);
            dw_font_print(&myfont, "ID card ไม่ตรง 1");
            IDcard1 = ""; tem1 = ""; f = 0;
            confirmtime = 0;
            display.display();
            delay(3000);
            display.resetDisplay();
          }
          rdm6300.update();
          while (!rdm6300.update()) {
            rdm6300.update();
            break;
          }
        }

        
        if( flag_count_down ) {
//          Serial.println(confirmtime);

            if(time_out < 100){
                flag_count_down = 0;
                confirmtime = 0;
                display.clear();
              }
            time_out--;
          }

        // ------------------------------------------------timeout
        /*if (time_out1 == 0x0FFF) {
          display.resetDisplay();
          f = 0; IDcard1 = ""; tem1 = ""; f = 0;
          confirmtime = 0;

          //flag_count_down = 0;
          //counter_ = 0;
        }
        if ( flag_count_down )
        {
          if (time_out1 < 100)
          {
            Serial.println("Time Out !");
            time_out1 = 0x0FFF;
            flag_count_down = 0;
            counter_ = 0;
          }
          time_out1--;
        }*/
        //-----------------------------------------------------
        // confirm time stop and pause
        if (confirmtime == 1) {
          customKey1 = customKeypad.getKey();
          if (customKey1 != NO_KEY) {
            // stop
            if (customKey1 == '#') {
              //EEPROM.get(addmac, readmachine);
              sprintf( buff , "https://bunnam.com/projects/majorette_pp/update/count.php?id_task=%s&id_mc=%s&no_send=%d&no_pulse1=%d&no_pulse2=0&no_pulse3=0", dst.id_task, readmachine.c_str() , readcount, qty);
              //Serial.println( buff );
              msg = httpPOSTRequest(buff, "");

              //- timer
              EEPROM.get(addeecount, readcount);
              String h = String(readcount);
              String strqty = String(qty);
              timerDetachInterrupt(timer);
              /* DB4 */
              //EEPROM.get(addeeqty, readqty);
              query_Quit_GetMethod((char*)IDcard.c_str() , dst.id_job, dst.operation, (char*)readmachine.c_str(), (char*)h.c_str(), (char*)strqty.c_str(), "0", "0");
              /* DB4 */

              display.clear();
              dw_font_goto(&myfont, 30, LINE2);
              dw_font_print(&myfont, "ออกการทำงาน");
              dw_font_goto(&myfont, 40 , LINE3);
              sprintf(buff2 , "%s", translate_hh_mm_cc(workCounter));
              dw_font_print(&myfont, buff2);
              display.display();

              delay(2000);

              workCounter = 0;
              qty = 0;
              confirmRF = 0 , RFstate = 0, count = 0; readcount = 0;
              confirmtime = 0; msg = ""; f = 0;
              EEPROM.put(addeecount, readcount);
              display.resetDisplay();
              display.clear();
              int v = 0;
              EEPROM.put(addchackee, v);
              EEPROM.commit();

              customKey1 = NO_KEY;

              flag_count_down = flag_count_down ? 0 : 1;
              //attachInterrupt(digitalPinToInterrupt(PIN_COUNTER), countUp, CHANGE );
              detachInterrupt(digitalPinToInterrupt(PIN_COUNTER));
            }

            //pause
            else if (customKey1 == '*') {
              customKey1 = NO_KEY;
              
              noInterrupts();
              confirmtime = 0;
              RFstate = 0;
              pauseset = 1;
              setsh = 1;
              interrupts(); 
              
              f = 0;

              flag_count_down = flag_count_down ? 0 : 1;
              
              display.clear();
              display.resetDisplay();
            }
          }
        }
      }
      //
      else if (pauseset == 1 && RFstate != 1) {
        if ( interruptBreak )
        {
          if (setb == 1) {
            dw_font_goto(&myfont, 0, LINE1);
            //EEPROM.get(addmac, readmachine);
            sprintf( buff1 , "mc/no. %s", readmachine);
            dw_font_print(&myfont, buff1);

            dw_font_goto(&myfont, 0, LINE2);
            sprintf( buff1 , "ชื่อ: %s %s", dst.name_first , dst.name_last);
            dw_font_print(&myfont, buff1);

            dw_font_goto(&myfont, 25, LINE3);
            dw_font_print(&myfont, "พักเบรกเข้าห้องน้ำ");
            display.display();
          }
          else if (setb == 2) {
            dw_font_goto(&myfont, 0, LINE1);
            //EEPROM.get(addmac, readmachine);
            sprintf( buff1 , "mc/no. %s", readmachine);
            dw_font_print(&myfont, buff1);

            dw_font_goto(&myfont, 0, LINE2);
            sprintf( buff1 , "ชื่อ: %s %s", dst.name_first , dst.name_last);
            dw_font_print(&myfont, buff1);

            dw_font_goto(&myfont, 25, LINE3);
            dw_font_print(&myfont, "พักเบรกทานอาหาร");
            display.display();
          }
          portENTER_CRITICAL_ISR(&timerMux);
          interruptBreak = 0;
          portEXIT_CRITICAL_ISR(&timerMux);
          breakCounter++;
          dw_font_goto(&myfont, 40 , LINE4);
          sprintf(buff2 , "%s", translate_hh_mm_cc(breakCounter));
          dw_font_print(&myfont, buff2);
          display.display();
        }

        if (setsh == 1) {
          dw_font_goto(&myfont, 0, LINE1);
          //EEPROM.get(addmac, readmachine);
          sprintf( buff1 , "mc/no. %s", readmachine);
          dw_font_print(&myfont, buff1);
          dw_font_goto(&myfont, 0, LINE2);
          dw_font_print(&myfont, "กด A พักเบรกเข้าห้องน้ำ");

          dw_font_goto(&myfont, 0 , LINE3);
          dw_font_print(&myfont, "กด B พักเบรกทานอาหาร");
          display.display();
        }

        char pauseKey = customKeypad.getKey();
        if (pauseKey != NO_KEY) {
          if (pauseKey == 'a') {
            display.clear();
            display.resetDisplay();
            timerAttachInterrupt(timer, &onTimerBreak, true);
            timerAlarmWrite(timer, 1000000, true);
            timerAlarmEnable(timer);
            setb = 1;
            /* DB3 */
            query_Break_GetMethod( (char*)IDcard.c_str() , dst.id_job, dst.operation, (char*)readmachine.c_str(), "1" );
            pauseKey = NO_KEY;
            setsh = 0;
          }
          else if (pauseKey == 'b') {
            display.clear();
            display.resetDisplay();
            timerAttachInterrupt(timer, &onTimerBreak, true);
            timerAlarmWrite(timer, 1000000, true);
            timerAlarmEnable(timer);
            setb = 2;
            /* DB3 */
            query_Break_GetMethod( (char*)IDcard.c_str() , dst.id_job, dst.operation, (char*)readmachine.c_str(), "2" );
            pauseKey = NO_KEY;
            setsh = 0;
          }
        }

        if (rdm6300.update()) {
          tem1 = String(rdm6300.get_tag_id());


          if (strlen ((const char *)tem1.c_str()) == 8)
            IDcard1 = String("00") + tem1;
          if (strlen((const char *)tem1.c_str()) == 7)
            IDcard1 = String("000") + tem1;

          if (IDcard1 == IDcard) {
            
            noInterrupts();
            RFstate = 1;
            setsh = 1;
            pauseset = 0;
            interrupts();
            
            IDcard1 = ""; 
            tem1 = "";
            
            timerAttachInterrupt(timer, &onTimerWork, true);
            timerAlarmWrite(timer, 1000000, true);
            timerAlarmEnable(timer);
            /* DB3 */
            query_Continue_GetMethod((char*)readmachine.c_str(), (char*)IDcard.c_str());
            breakCounter = 0;
            display.clear();
            display.resetDisplay();
            //------------------------------------------------------------------------- เพิ่ม
            dw_font_goto(&myfont, 15, LINE3);
            dw_font_print(&myfont, "หยุดพักเบรค");
            display.display();
            delay(2000);
            rdm6300.update();
            while (!rdm6300.update()) {
              rdm6300.update();
              break;
            }

          }
          else {
            display.resetDisplay();
            dw_font_goto(&myfont, 15, LINE3);
            dw_font_print(&myfont, "ID card ไม่ตรง 2");
            IDcard1 = "";   tem1 = ""; f = 0;
            display.display();
            noInterrupts();
            rdm6300.update();
            while (!rdm6300.update()) {
              break;
            }
            delay(3000);
            display.resetDisplay();

          }
          rdm6300.update();
          while (!rdm6300.update()) {
            break;
          }
        }
        interrupts();
      }
    }

    // ID_task == 2 set machine
    //----------------------------------------------------
    // setting menu
    else if (settingmenu == 1 && numrole == 2) {
      if (interruptWork) {
        portENTER_CRITICAL_ISR(&timerMux);
        interruptWork = 0;
        portEXIT_CRITICAL_ISR(&timerMux);
        workCounter++;
        Serial.print("Work : ");
        Serial.println(translate_hh_mm_cc(workCounter));
      }
      dw_font_goto(&myfont, 30, LINE1);
      dw_font_print(&myfont, "เลือกการตั้งค่า");
      dw_font_goto(&myfont, 5, LINE2);
      dw_font_print(&myfont, "กด A ตั้งค่า รหัสเครื่อง");
      dw_font_goto(&myfont, 5, LINE3);
      dw_font_print(&myfont, "กด B ใส่รหัสการทำงาน");
      dw_font_goto(&myfont, 5, LINE4);
      dw_font_print(&myfont, "               หยุดการทำงาน #");
      display.display();

      customKey2 = customKeypad.getKey();
      if (customKey2 != NO_KEY) {
        // setting machine
        if (customKey2 == 'a') {
          display.resetDisplay();
          settingmachine = 1;
          settingmenu = 0;
          customKey2 = NO_KEY;
        }
        else if (customKey2 == 'b') {
          display.resetDisplay();
          settingmachine = 2;
          settingmenu = 0;
          customKey2 = NO_KEY;
        }
        else if (customKey2 == '#') {
          display.resetDisplay();
          settingmachine = 3;
          settingmenu = 0;
          while (!rdm6300.update())
          {
            customKey2 = NO_KEY; break;
          }
        }
      }
    }

    // setting machine
    else if (settingmachine == 1) {
      if (interruptWork) {
        portENTER_CRITICAL_ISR(&timerMux);
        interruptWork = 0;
        portEXIT_CRITICAL_ISR(&timerMux);
        workCounter++;
        Serial.print("Work : ");
        Serial.println(translate_hh_mm_cc(workCounter));
      }

      dw_font_goto(&myfont, 5, LINE1);
      dw_font_print(&myfont, "โปรดใส่เลขรหัสเครื่อง");
      display.display();
      if (u == 24) {
        display.drawString(24, LINE1, "-");
        machine += "-";
        u = 31;
      }
      else if (u == 45) {
        dw_font_goto(&myfont, 5, LINE4);
        dw_font_print(&myfont, "* ยืนยัน              ย้อนกลับ #");
        display.display();
      }
      else if (u >= 46) {
        dw_font_goto(&myfont, 20, LINE2);
        dw_font_print(&myfont, "เกิดข้อผิดพลาด");
        display.display();
        delay(3000);
        display.resetDisplay();
        machine = ""; customKeyset = "";
        u = 10;
      }
      // put numbet machine
      customKey = customKeypad.getKey();
      if (customKey != NO_KEY) {
        if (customKey != NO_KEY && customKey != '*' && customKey != '#') {
          machine += customKey;
          customKeyset = String(customKey);
          display.drawString(u, 10, customKeyset);
          u = u + 7;
          display.display();
          customKey = NO_KEY;
        }
        else if (customKey == '*') {
          settingmenu = 1; customKey = NO_KEY;
          settingmachine = 0;
          Serial.println(machine);
          EEPROM.put(addmac, machine);
          EEPROM.commit();
          machine = ""; u = 10;
          display.resetDisplay();
        }
        else if (customKey == '#') {
          u = 10; settingmenu = 1; customKey = NO_KEY;
          settingmachine = 0; machine = "";
          display.resetDisplay();
        }
      }
    }
    //------------------------------- downtime code
    else if (settingmachine == 2) {
      if (interruptWork) {
        portENTER_CRITICAL_ISR(&timerMux);
        interruptWork = 0;
        portEXIT_CRITICAL_ISR(&timerMux);
        workCounter++;
        Serial.print("Work : ");
        Serial.println(translate_hh_mm_cc(workCounter));
      }

      dw_font_goto(&myfont, 5, LINE1);
      dw_font_print(&myfont, "โปรดใส่รหัสการทำงาน");
      display.display();

      if (u == 31) {
        dw_font_goto(&myfont, 5, LINE4);
        dw_font_print(&myfont, "* ยืนยัน              ย้อนกลับ #");
        display.display();
      }
      else if (u >= 32) {
        dw_font_goto(&myfont, 20, LINE2);
        dw_font_print(&myfont, "เกิดข้อผิดพลาด");
        display.display();
        delay(3000);
        display.resetDisplay();
        code = ""; customKeyset = "";
        u = 10;
      }

      customKey = customKeypad.getKey();
      if (customKey != NO_KEY) {
        if (customKey != NO_KEY && customKey != '*' && customKey != '#') {
          code += customKey;
          customKeyset = String(customKey);
          display.drawString(u, 10, customKeyset);
          u = u + 7;
          display.display();
          customKey = NO_KEY;
        }
        else if (customKey == '*') {
          settingmenu = 1; customKey = NO_KEY;
          settingmachine = 0;
          Serial.println(code);
          query_Downtime_GetMethod(dst.id_job , dst.operation , (char*)readmachine.c_str() , (char*)code.c_str());
          u = 10; code = "";
          display.resetDisplay();
        }
        else if (customKey == '#') {
          display.resetDisplay();
          u = 10; settingmenu = 1; customKey = NO_KEY;
          settingmachine = 0;  code = "";
        }
      }
    }
    //---------------------------------------------------- quit code
    else if (settingmachine == 3) {
      dw_font_goto(&myfont, 15, LINE3);
      dw_font_print(&myfont, "สแกนบัตรออกการทำงาน");
      display.display();

      if (rdm6300.update()) {
        tem1 = String(rdm6300.get_tag_id());
        if (strlen ((const char *)tem1.c_str()) == 8)
          IDcard2 = String("00") + tem1;
        else if (strlen((const char *)tem1.c_str()) == 7)
          IDcard2 = String("000") + tem1;

        if (IDcard2 == IDcard) {
          query_Quit_DT_GetMethod( (char*)IDcard2.c_str(), dst.id_job , dst.operation , (char*)readmachine.c_str() );

          display.clear();
          dw_font_goto(&myfont, 30, LINE3);
          dw_font_print(&myfont, "ออกการทำงาน");
          dw_font_goto(&myfont, 40 , LINE4);
          sprintf(buff2 , "%s", translate_hh_mm_cc(workCounter));
          dw_font_print(&myfont, buff2);
          display.display();

          delay(2000);

          RFstate = 0; settingmenu = 0; settingmachine = 0; confirmRF = 0;
          timerDetachInterrupt(timer);
          workCounter = 0;
          IDcard2 = ""; tem1 = "";
          rdm6300.update();
          while (!rdm6300.update()) {
            break;
          }

        }
        else {
          display.resetDisplay();
          dw_font_goto(&myfont, 15, LINE3);
          dw_font_print(&myfont, "ID card ไม่ตรง 3");
          IDcard2 = ""; tem1 = "";
          settingmachine = 3;
          display.display();
          delay(3000);
          display.resetDisplay();
          rdm6300.update();
          while (!rdm6300.update()) {
            break;
          }
        }
      }
    }
  }
}


String httpGETRequest(const char* serverName) {
  HTTPClient http;

  // Your IP address with path or Domain name with URL path
  http.begin(serverName);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "--";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
  return payload;
}

String httpPOSTRequest(const char* serverName , const char* msg) {
  HTTPClient http;

  // Your IP address with path or Domain name with URL path
  http.begin(serverName);
  //  http.addHeader("Content-Type", "application/json");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  // Send HTTP POST request
  int httpResponseCode = http.POST(msg);

  String payload = "--";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

int query_Touch_GetMethod( const char * id_mc , const char * id_rfid , employ_touch_TYPE * result)
{
  String msg = " ";
  char buff[300];

  sprintf( buff , "http://bunnam.com/projects/majorette_pp/update/touch_v3.php?id_mc=%s&id_rfid=%s", id_mc, id_rfid );
  Serial.println(id_mc);
  msg = httpGETRequest(buff);

  if ( msg != "null" )
  {
    Serial.println( msg );
    Serial.println( msg.length() );

    DynamicJsonDocument  doc( msg.length() + 256 ) ;
    DeserializationError error = deserializeJson(doc, msg);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      Serial.println(" : ID is not found !");
      result->flag_err = 0;
      return -1;
    }
    else if (doc["code"])
    {
      return ((doc["code"]).as<int>());
    }
    else
    {
      result->id_staff = strdup(doc["id_staff"]); // "000002"
      result->name_first = strdup(doc["name_first"]); // "Thanasin"
      result->name_last = strdup(doc["name_last"]); // "Bunnam"
      result->role = doc["role"];
      result->id_task = strdup(doc["id_task"]); // "8"
      result->id_job = strdup(doc["id_job"]);
      result->item_no = strdup(doc["item_no"]);
      result->operation = strdup(doc["operation"]);
      result->op_name = strdup(doc["op_name"]);
      result->qty_order = strdup(doc["qty_order"]);
      result->qty_comp = strdup(doc["qty_comp"]);
      result->qty_open = strdup(doc["qty_open"]);
      result->flag_err = 0 ;
      //sprintf( buff , "%s" , result->id_staff);
      //Serial.println( buff ) ;
      Serial.println( "--------------" ) ;
      return 0;
    }
  }
  else
  {
    result->flag_err = 1;
    Serial.println("Error!");
    return -2;
  }
}

int query_Continue_GetMethod( const char * id_mc , const char * id_rfid  )
{
  String msg = " ";
  char buff[300];
  sprintf( buff , "http://bunnam.com/projects/majorette_pp/update/continue_v2.php?id_mc=%s&id_rfid=%s" , id_mc , id_rfid);
  Serial.println(buff);
  msg = httpGETRequest(buff);

  if ( msg != "null" )
  {
    Serial.println( msg );
    Serial.println( msg.length() );

    DynamicJsonDocument  doc( msg.length() + 256 ) ;
    DeserializationError error = deserializeJson(doc, msg);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      Serial.println("Error Continue!");
      return -1;
    }
    if (doc["code"])
    {
      return ((doc["code"]).as<int>());
    }
    if ( doc["total_break"] )
    {
      Serial.println((const char *)(doc["total_break"]));
      return 0;
    }
  }
  else
  {
    Serial.println("Error!");
    return -2;
  }
}

int query_Break_GetMethod( char * id_rfid, char * id_job , char * operation , char * id_mc , char * break_code )
{
  String msg = " ";
  char buff[300];
  sprintf( buff , "http://bunnam.com/projects/majorette_pp/update/break_v2.php?id_rfid=%s&id_job=%s&operation=%s&id_mc=%s&break_code=%s" , id_rfid, id_job, operation, id_mc, break_code );
  Serial.println(buff);
  msg = httpGETRequest(buff);

  if ( msg != "null" )
  {

    Serial.println( msg );
    Serial.println( msg.length() );

    if ( msg == "OK" )
    {
      return 0;
    }
    else
    {
      DynamicJsonDocument  doc( msg.length() + 256 ) ;
      DeserializationError error = deserializeJson(doc, msg);
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        Serial.println("Error Break!");
        return -1;
      }
      if (doc["code"])
      {
        return ((doc["code"]).as<int>());
      }
    }
  }
  else
  {
    Serial.println("Error!");
    return -2;
  }
}

int query_Quit_GetMethod( char* id_rfid, char * id_job , char * operation , char * id_machine , char  * no_send , char * no_pulse1 , char * no_pulse2 , char * no_pulse3 )
{
  String msg = " ";
  char buff[300];
  sprintf( buff , "http://bunnam.com/projects/majorette_pp/update/quit_v2.php?id_rfid=%s&id_job=%s&operation=%s&id_mc=%s&no_send=%s&no_pulse1=%s&no_pulse2=%s&no_pulse3=%s" , id_rfid, id_job, operation, id_machine, no_send, no_pulse1, no_pulse2, no_pulse3 );
  Serial.println(buff);
  msg = httpGETRequest(buff);

  if ( msg != "null" )
  {
    Serial.println( msg );
    Serial.println( msg.length() );
    DynamicJsonDocument  doc( msg.length() + 256 ) ;
    DeserializationError error = deserializeJson(doc, msg);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      Serial.println("Error Quit!");
      return -1;
    }
    if (doc["code"])
    {
      return ((doc["code"]).as<int>());
    }
    if ( doc["time_work"] )
    {
      Serial.println((const char *)(doc["time_work"]));
      return 0;
    }
  }
  else
  {
    Serial.println("Error!");
    return -2;
  }
}

int query_Downtime_GetMethod(  char * id_job , char * operation , char * id_machine , char * code_downtime )
{
  String msg = " ";
  char buff[300];
  sprintf( buff , "http://bunnam.com/projects/majorette_pp/update/downtime.php?id_job=%s&operation=%s&id_mc=%s&code_downtime=%s" , id_job, operation, id_machine, code_downtime );
  Serial.println(buff);
  msg = httpGETRequest(buff);

  if ( msg != "null" )
  {
    Serial.println( msg );
    Serial.println( msg.length() );

    if ( msg == "OK" )
    {
      return 0;
    }
    else
    {
      DynamicJsonDocument  doc( msg.length() + 256 ) ;
      DeserializationError error = deserializeJson(doc, msg);
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        Serial.println("Error Break!");
        return -1;
      }
      if (doc["code"])
      {
        return ((doc["code"]).as<int>());
      }
    }
  }
  else
  {
    Serial.println("Error!");
    return -2;
  }
}

int query_Quit_DT_GetMethod( char * id_rfid, char * id_job , char * operation , char * id_mc )
{
  String msg = " ";
  char buff[300];
  sprintf( buff , "http://bunnam.com/projects/majorette_pp/update/quit_dt.php?id_rfid=%s&id_job=%s&operation=%s&id_mc=%s" , id_rfid, id_job, operation, id_mc );
  Serial.println(buff);
  msg = httpGETRequest(buff);

  if ( msg != "null" )
  {
    Serial.println( msg );
    Serial.println( msg.length() );
    DynamicJsonDocument  doc( msg.length() + 256 ) ;
    DeserializationError error = deserializeJson(doc, msg);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      Serial.println("Error Quit!");
      return -1;
    }
    if (doc["code"])
    {
      return ((doc["code"]).as<int>());
    }
    if ( doc["time_work"] )
    {
      Serial.println((const char *)(doc["time_work"]));
      return 0;
    }
  }
  else
  {
    Serial.println("Error!");
    return -2;
  }
}

void draw_pixel(int16_t x, int16_t y)
{
  display.setColor(WHITE);
  display.setPixel(x, y);
}

void clear_pixel(int16_t x, int16_t y)
{
  display.setColor(BLACK);
  display.setPixel(x, y);
}
