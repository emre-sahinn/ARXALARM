/*
   Author: Emre Şahin
   Project: Thief Alarm System
   Date: 15.12.2018
   Version: v1.0
*/
#include <IRremote.h>
const int RECV_PIN = 4;
IRrecv irrecv(RECV_PIN);
decode_results results;
int lcd_start;
bool is_first_start = true;
////////////////////////////////////////
#include <LiquidCrystal_I2C.h>
int lcd_pin = 2;
LiquidCrystal_I2C lcd(0x27, 16, lcd_pin);
////////////////////////////////////////
#include "DHT.h"
#define DHTPIN 3
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
////////////////////////////////////////
#include <Wire.h>
#include "ds3231.h"
#define BUFF_MAX 128
uint8_t time[8];
char recv[BUFF_MAX];
unsigned int recv_size = 0;
unsigned long prev, interval = 0;
void parse_cmd(char *cmd, int cmdsize);
struct ts t;
////////////////////////////////////////
const int pir = 2;
int pir_state = 0;
const int buzzer = A0;
int alarm_hour = 12;
int alarm_minute = 0;
bool alarm_menu;
int alarm2 = -1;
int alarm_hour2 = 12;
int alarm_minute2 = 0;
bool info_menu;
bool menu = true;
////////////////////////////////////////

void setup() {
  Wire.begin();
  DS3231_init(DS3231_CONTROL_INTCN);
  memset(recv, 0, BUFF_MAX);
  //Serial.println("GET time");
  //parse_cmd("T002523312122018", 16);//TSSmmHHwDDmmYYYY for setting clock

  irrecv.enableIRIn();
  dht.begin();
  pinMode(pir, INPUT);
  pinMode(buzzer, OUTPUT);
  lcd.begin();
  lcd.clear();
  //lcd.backlight();
  lcd.noBacklight();
  Serial.begin(9600);
}

void loop() {
  loop_alarm();
  ir_remote_reciever();
  clock_module();
}

void melody(String melody_type) {
  if (melody_type == "alarm") {
    for (int i = 0; i < 80; i++) {
      Serial.println(i);
      digitalWrite(buzzer, i);
      delay(i + 10);
      digitalWrite(buzzer, LOW);
      delay(i + 10);
    }
  } else if (melody_type == "opening") {
    digitalWrite(buzzer, 150);
    delay(250);
    digitalWrite(buzzer, LOW);
    delay(250);
    digitalWrite(buzzer, 250);
    delay(250);
    digitalWrite(buzzer, LOW);
  } else if (melody_type == "click") {
    digitalWrite(buzzer, 150);
    delay(250);
    digitalWrite(buzzer, LOW);
  }
}

void pir_sensor() {
  pir_state = digitalRead(pir);
  if (pir_state == HIGH) {
    lcd.setCursor(0, 1);
    lcd.print("                 *");
    lcd.setCursor(0, 0);
    lcd.print("HAREKET VAR !");
    melody("alarm");
  } else {
    digitalWrite(buzzer, LOW);
  }
}

void dht11_sensor() {
  lcd.setCursor(0, 0);
  lcd.print("Oda ");
  lcd.print(int(dht.readTemperature()));
  lcd.print(" C          ");
  lcd.setCursor(0, 1);
  lcd.print("Nemlilik %");
  lcd.print(int(dht.readHumidity()));
  lcd.print("         ");
}

void ir_remote_reciever() {
  if (alarm_menu == true) {
    set_alarm();
  }

  if (irrecv.decode(&results)) {
    if (results.value == 2672 && alarm_menu == true) {
      alarm2 = alarm2 * -1;
      melody("click");
    }

    if (results.value == 2704) {
      ir_escape();
    } else if (results.value == 2704) {//exit button
      melody("click");
    } else if (results.value == 16 && menu == true) {//button1
      menu = false;
      alarm_menu = true;
      info_menu = false;
      melody("click");
      set_alarm();
    } else if (results.value == 2064 && menu == true) {//button2
      menu = false;
      alarm_menu = false;
      info_menu = true;
      melody("click");
      dht11_sensor();
    } else if (results.value == 112) {//menu
      menu = true;
      alarm_menu = false;
      info_menu = false;
      melody("click");
      lcd_menu();
    } else if (alarm_menu == true) {
      if (alarm2 == 1) {
        if (results.value == 752) {//upbutton
          melody("click");
          if (alarm_hour >= 23) {
            alarm_hour = 23;
          } else {
            alarm_hour += 1;
          }
          set_alarm();
        } else if (results.value == 2800) {//downbutton
          melody("click");
          if (alarm_hour < 1) {
            alarm_hour = 0;
          } else {
            alarm_hour -= 1;
          }
          set_alarm();
        } else if (results.value == 3280) {//rightbutton
          melody("click");
          if (alarm_minute >= 50) {
            alarm_minute = 50;
          } else {
            alarm_minute += 10;
          }
          set_alarm();
        } else if (results.value == 720) {//leftbutton
          melody("click");
          if (alarm_minute < 10) {
            alarm_minute = 0;
          } else {
            alarm_minute -= 10;
          }
          set_alarm();
        }
      } else {
        if (results.value == 752) {//upbutton
          melody("click");
          if (alarm_hour2 >= 23) {
            alarm_hour2 = 23;
          } else {
            alarm_hour2 += 1;
          }
          set_alarm();
        } else if (results.value == 2800) {//downbutton
          melody("click");
          if (alarm_hour2 < 1) {
            alarm_hour2 = 0;
          } else {
            alarm_hour2 -= 1;
          }
          set_alarm();
        } else if (results.value == 3280) {//rightbutton
          melody("click");
          if (alarm_minute2 >= 50) {
            alarm_minute2 = 50;
          } else {
            alarm_minute2 += 10;
          }
          set_alarm();
        } else if (results.value == 720) {//leftbutton
          melody("click");
          if (alarm_minute2 < 10) {
            alarm_minute2 = 0;
          } else {
            alarm_minute2 -= 10;
          }
          set_alarm();
        }
      }
    }
    Serial.println(results.value);
    irrecv.resume();
  }
}

void first_lcd_text(String text, String text2, String text3) {
  lcd.setCursor(0, 0);
  lcd.print(text);
  delay(1500);
  lcd.clear();
  lcd.print(text2);
  delay(2000);
  lcd.clear();
  lcd.print(text3);
  delay(1500);
  lcd_menu();
}

void lcd_menu() {
  lcd.clear();
  lcd.print("1:Alarm Kur");
  lcd.setCursor(0, 1);
  lcd.print("2:Hava Durumu");
}


void set_alarm() {
  lcd.print("               ");
  lcd.setCursor(0, 0);
  //first alarm
  if (alarm_hour < 10) {
    lcd.print("0");
    lcd.print(alarm_hour);
  } else {
    lcd.print(alarm_hour);
  }
  lcd.print(".");
  if (alarm_minute < 10) {
    lcd.print("0");
    lcd.print(alarm_minute);
  } else {
    lcd.print(alarm_minute);
  }
  lcd.print("  ||  ");
  //second alarm
  if (alarm_hour2 < 10) {
    lcd.print("0");
    lcd.print(alarm_hour2);
  } else {
    lcd.print(alarm_hour2);
  }
  lcd.print(".");
  if (alarm_minute2 < 10) {
    lcd.print("0");
    lcd.print(alarm_minute2);
  } else {
    lcd.print(alarm_minute2);
  }

  lcd.setCursor(0, 1);
  lcd.print("Saat: ");
  show_time();
  delay(150);
}

void loop_alarm() {
  delay(25);
  /*for debugging purpose
    Serial.println("******************************");
    Serial.println(t.hour + t.min / 100);
    Serial.println(alarm_hour + alarm_minute / 100);
    Serial.println(alarm_hour2 + alarm_minute2 / 100);*/
  if (alarm_hour + alarm_minute / 100 < alarm_hour2 + alarm_minute2 / 100) {
    if (t.hour + t.min / 100 >= alarm_hour + alarm_minute / 100 && t.hour + t.min / 100 <= alarm_hour2 + alarm_minute2 / 100) {
      pir_sensor();
      /*for debugging purpose
        Serial.println("******************************");
        Serial.println("Working, Straight alarm set");
        Serial.println(t.hour + t.min / 100);
        Serial.println(alarm_hour + alarm_minute / 100);
        Serial.println("******************************");
      */
    }
  } else if (alarm_hour2 + alarm_minute2 / 100 < alarm_hour + alarm_minute / 100) {
    if (t.hour >= alarm_hour + alarm_minute / 100 || t.hour <= alarm_hour2 + alarm_minute2 / 100) {
      pir_sensor();
      /*for debugging purpose
        Serial.println("******************************");
        Serial.println("Working, Reverse alarm set");
        Serial.println(t.hour + t.min / 100);
        Serial.println(alarm_hour + alarm_minute / 100);
        Serial.println("******************************");
      */
    }
  }

}

void show_time() {
  lcd.print(t.hour);
  lcd.print(".");
  if (t.min < 10)
  {
    lcd.print("0");
  }
  lcd.print(t.min);
  lcd.print(".");
  if (t.sec < 10)
  {
    lcd.print("0");
  }
  lcd.print(t.sec);
}

void ir_escape() {
  delay(125);
  if (lcd_start == 0) {
    lcd.backlight();
    lcd_start = 1;
    if (is_first_start) {
      melody("opening");
      is_first_start = false;
      first_lcd_text("ARX COMPANY 2019", "Sistem Kuruluyor", "Hos Geldiniz");
    }
  } else {
    lcd.noBacklight();
    lcd_start = 0;
  }
}

void ir_setting_one() {
  delay(250);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Emre Sahin");
  lcd.setCursor(0, 1);
  lcd.print("Arda Pecel");
}


void ir_setting_two() {
  delay(250);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Emre Sahin");
  lcd.setCursor(0, 1);
  lcd.print("Arda Pecel");
}

void clock_module()
{
  char in;
  char buff[BUFF_MAX];
  unsigned long now = millis();
  //struct ts t;

  // show time once in a while
  if ((now - prev > interval) && (Serial.available() <= 0)) {
    DS3231_get(&t);

    // there is a compile time option in the library to include unixtime support
#ifdef CONFIG_UNIXTIME
#ifdef __AVR__
    snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d %ld", t.year,
#error AVR
#else
    snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d %d", t.year,
#endif
             t.mon, t.mday, t.hour, t.min, t.sec, t.unixtime);
#else
    snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d", t.year,
             t.mon, t.mday, t.hour, t.min, t.sec);
#endif

    Serial.println(buff);
    prev = now;
  }

  if (Serial.available() > 0) {
    in = Serial.read();

    if ((in == 10 || in == 13) && (recv_size > 0)) {
      parse_cmd(recv, recv_size);
      recv_size = 0;
      recv[0] = 0;
    } else if (in < 48 || in > 122) {
      ;       // ignore ~[0-9A-Za-z]
    } else if (recv_size > BUFF_MAX - 2) {   // drop lines that are too long
      // drop
      recv_size = 0;
      recv[0] = 0;
    } else if (recv_size < BUFF_MAX - 2) {
      recv[recv_size] = in;
      recv[recv_size + 1] = 0;
      recv_size += 1;
    }

  }
}

void parse_cmd(char *cmd, int cmdsize)
{
  uint8_t i;
  uint8_t reg_val;
  char buff[BUFF_MAX];
  struct ts t;

  //snprintf(buff, BUFF_MAX, "cmd was '%s' %d\n", cmd, cmdsize);
  //Serial.print(buff);

  // TssmmhhWDDMMYYYY aka set time
  if (cmd[0] == 84 && cmdsize == 16) {
    //T355720619112011
    t.sec = inp2toi(cmd, 1);
    t.min = inp2toi(cmd, 3);
    t.hour = inp2toi(cmd, 5);
    t.wday = cmd[7] - 48;
    t.mday = inp2toi(cmd, 8);
    t.mon = inp2toi(cmd, 10);
    t.year = inp2toi(cmd, 12) * 100 + inp2toi(cmd, 14);
    DS3231_set(t);
    Serial.println("OK");
  } else if (cmd[0] == 49 && cmdsize == 1) {  // "1" get alarm 1
    DS3231_get_a1(&buff[0], 59);
    Serial.println(buff);
  } else if (cmd[0] == 50 && cmdsize == 1) {  // "2" get alarm 1
    DS3231_get_a2(&buff[0], 59);
    Serial.println(buff);
  } else if (cmd[0] == 51 && cmdsize == 1) {  // "3" get aging register
    Serial.print("aging reg is ");
    Serial.println(DS3231_get_aging(), DEC);
  } else if (cmd[0] == 65 && cmdsize == 9) {  // "A" set alarm 1
    DS3231_set_creg(DS3231_CONTROL_INTCN | DS3231_CONTROL_A1IE);
    //ASSMMHHDD
    for (i = 0; i < 4; i++) {
      time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // ss, mm, hh, dd
    }
    uint8_t flags[5] = { 0, 0, 0, 0, 0 };
    DS3231_set_a1(time[0], time[1], time[2], time[3], flags);
    DS3231_get_a1(&buff[0], 59);
    Serial.println(buff);
  } else if (cmd[0] == 66 && cmdsize == 7) {  // "B" Set Alarm 2
    DS3231_set_creg(DS3231_CONTROL_INTCN | DS3231_CONTROL_A2IE);
    //BMMHHDD
    for (i = 0; i < 4; i++) {
      time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // mm, hh, dd
    }
    uint8_t flags[5] = { 0, 0, 0, 0 };
    DS3231_set_a2(time[0], time[1], time[2], flags);
    DS3231_get_a2(&buff[0], 59);
    Serial.println(buff);
  } else if (cmd[0] == 67 && cmdsize == 1) {  // "C" - get temperature register
    Serial.print("temperature reg is ");
    Serial.println(DS3231_get_treg(), DEC);
  } else if (cmd[0] == 68 && cmdsize == 1) {  // "D" - reset status register alarm flags
    reg_val = DS3231_get_sreg();
    reg_val &= B11111100;
    DS3231_set_sreg(reg_val);
  } else if (cmd[0] == 70 && cmdsize == 1) {  // "F" - custom fct
    reg_val = DS3231_get_addr(0x5);
    Serial.print("orig ");
    Serial.print(reg_val, DEC);
    Serial.print("month is ");
    Serial.println(bcdtodec(reg_val & 0x1F), DEC);
  } else if (cmd[0] == 71 && cmdsize == 1) {  // "G" - set aging status register
    DS3231_set_aging(0);
  } else if (cmd[0] == 83 && cmdsize == 1) {  // "S" - get status register
    Serial.print("status reg is ");
    Serial.println(DS3231_get_sreg(), DEC);
  } else {
    Serial.print("unknown command prefix ");
    Serial.println(cmd[0]);
    Serial.println(cmd[0], DEC);
  }
}

