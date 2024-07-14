/*
две кнопки задают состояние программы
кнопка на селекторе меняет состояние между ожиданием и вводом mode 0 -> 1
кнопка старт меняет состояние между ожиданием и отсчетом mode 1 -> 2
*/

// пины энкодера
#define S1 2
#define S2 3

// пины дисплея TM1637
#define CLK 5
#define DIO 6

// пины кнопок
#define set_btn_pin 4
#define start_btn_pin 7

// пин баззера
#define buzzer_pin 8

// пин лампочки
#define light_pin 9

// адрес памяти
#define addr_min 1

byte mode = 0; // флаг переключающий состояния первого уровня
byte second_mode = 0; // флаг переключающий состояния второго уровня
int min;
int sec = 0;
int old = 0; //  тут хранится старое состояние при настройке времени
long t_blink = 0; // counter for blinking
bool t_flag = false; // flag for blinking

#include <TM1637Display.h>
#include <Encoder.h>
#include <Button.h>
#include <EEPROM.h>
TM1637Display disp(CLK, DIO);
Encoder enc(S1, S2);
Button set_btn(set_btn_pin);
Button start_btn(start_btn_pin);

long old_position = enc.read(); //  инициализация стартового состояния энкодера

void setup() {
  //  очистка и установка яркости дисплея (1-7)
  disp.clear();
  disp.setBrightness(7);
  // читаем минуты сохраненные при прошлом запуске
  min = EEPROM.read(addr_min);
  // инициализируем кнопки
  set_btn.begin();
  start_btn.begin();

}

void loop() {
  
  switch (mode) { // главное меню - переключение между состояниями
    case 0: waiting(); break;
    case 1: setting(); break;
    case 2: counting(); break;
  }

}

void waiting() { // состоние в котором просто отображаются цифры перед нажатием старт
  disp.showNumberDecEx(min*100+sec, 0b01000000, true);

  if (set_btn.released()){ // переходим в режим настройки
    disp.clear(); // приятно глазу когда дисплей мгновенно откликается
    mode = 1;
  }
  if (start_btn.released()){ // переходим в режим отсчета
    mode = 2;
  }

}

void setting() { // состоние в котором настривается время

  old = min;
  min = min + enc_tick();
  if (old != min){ // чтобы не мигало когда крутишь энкодер
    t_blink = millis();
    t_flag = true;
  }

  if (min >= 100){
    min = 1;
  }
  else if (min < 1){
    min = 99;
  }

  if (millis() - t_blink > 800){
    t_blink = millis();
    t_flag = !t_flag;
  }

  if (t_flag == false){
    disp.clear();
  }
  else if(t_flag == true){
    disp.showNumberDec(min, true, 2, 0);
  }

  if (set_btn.released()){
    EEPROM.write(addr_min, min);
    mode = 0;
  }

}

void counting() { // режим отсчета времени

  if (second_mode == 0){ // состояние отсчета времени
    if (millis() - t_blink > 1000){
      t_blink = millis();
      t_flag = !t_flag;
      sec = sec - 1;
    }

    if (sec < 0){
      sec = 59;
      min --;
    }

    if (min < 0){
      second_mode = 1;
      t_blink = millis();
    }

    if (t_flag == true){
      disp.showNumberDecEx(min*100+sec, 0b01000000, true);
    }
    else if (t_flag == false){
      disp.showNumberDec(min*100+sec, true);
    }
  }
  else if (second_mode == 1){ // состояние сирены и мигания

    if (millis() - t_blink < 4000){
      pinMode(buzzer_pin, OUTPUT);
      tone(buzzer_pin, 1000);
    }
    else {
      noTone(buzzer_pin);
      pinMode(buzzer_pin, INPUT);
      min = EEPROM.read(addr_min);
      sec = 0;
      mode = 2;
      second_mode = 0;
    }
    
    disp.showNumberDec(0, true);
    if (start_btn.released()){
      min = EEPROM.read(addr_min);
      sec = 0;
      mode = 2;
      second_mode = 0;
    }
  }
}

int enc_tick(){ //  возвращает -1 при повороте налево и +1 направо

  long newPosition = enc.read();

  if (newPosition - old_position >= 4) {
    old_position = newPosition;
    return +1;
  }
  else if (newPosition - old_position <= -4) {
    old_position = newPosition;
    return -1;
  }
  return 0;
}
