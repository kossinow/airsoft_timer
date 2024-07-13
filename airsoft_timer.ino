/*
две кнопки задают состояние программы
кнопка на селекторе меняет состояние между ожиданием и вводом mode 0 -> 1
кнопка старт меняет состояние между ожиданием и отсчетом mode 1 -> 2

*/

// пины энкодера
#define S1 2
#define S2 3

//  пины дисплея TM1637
#define CLK 5
#define DIO 6

//  пины кнопок
#define set_btn_pin 4
#define start_btn_pin 7

// пин баззера
#define buzzer_pin 8

// адреса памяти
#define addr_min 1
#define addr_sec 2

byte mode = 0; // флаг переключающий состояния первого уровня
byte second_mode = 0; // флаг переключающий состояния второго уровня (настройка минут, отсчет и звук сирены)
int min = 2;
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

const uint8_t SEG_DONE[] = {
	SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
	SEG_C | SEG_E | SEG_G,                           // n
	SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
	};

long oldPosition = enc.read(); //  инициализация стартового состояния энкодера

void setup() {
  disp.clear();
  disp.setBrightness(7); //  установка яркости дисплея (1-7)

  //pinMode(buzzer_pin, OUTPUT);

  min = EEPROM.read(addr_min);
  sec = EEPROM.read(addr_sec);

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

  if (set_btn.released()){ // переходим в режим настройки при нажатии кнопки эенкодера
    disp.clear();
    mode = 1; // TODO для удобства отладки потом поставить 1
  }
  if (start_btn.released()){ // переходим в режим отсчета
    //disp.clear();
    mode = 2;
  }

}


void setting() { // состоние в котором настривается время
  if (second_mode == 0){ // состояние настройки секунд
    old = sec;
    sec = sec + enc_tick();
    if (old != sec){ // чтобы не мигало когда крутишь энкодер
      t_blink = millis();
      t_flag = true;
    }

    if (sec >= 60){
      sec = 0;
    }
    if (sec < 0){
      sec = 59;
    }

    if (millis() - t_blink > 800){
      t_blink = millis();
      t_flag = !t_flag;
    }
    if (t_flag == false){
      disp.clear();
    }
    else if(t_flag == true){
      disp.showNumberDec(sec, true, 2, 2);
    }

    if (set_btn.released()){
      EEPROM.write(addr_sec, sec);
      second_mode = 1;
    }
  }

  else if(second_mode == 1){ // состояние настройки минут
    old = min;
    min = min + enc_tick();
    if (old != min){ // чтобы не мигало когда крутишь энкодер
      t_blink = millis();
      t_flag = true;
    }

    if (min >= 100){
      min = 0;
    }
    if (min < 0){
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
      second_mode = 0;
      mode = 0;
    }
  }



}

void counting() { // состоние отсчета времени

  if (second_mode == 0){ //  состояние отсчета времени
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

    if (start_btn.released()){
      min = EEPROM.read(addr_min);
      sec = EEPROM.read(addr_sec);
      mode = 2;
      second_mode = 0;
    }
    if (set_btn.released()){
      min = EEPROM.read(addr_min);
      sec = EEPROM.read(addr_sec);
      mode = 1;
      second_mode = 0;
    }
  }
  else if (second_mode == 1){ // таймер кончился, звенит звонок

    if (millis() - t_blink < 5000){
      pinMode(buzzer_pin, OUTPUT);
      tone(buzzer_pin, 1000);
    }
    else {
      noTone(buzzer_pin);
      pinMode(buzzer_pin, INPUT);
    }
    
    disp.setSegments(SEG_DONE);
    if (start_btn.released()){
      min = EEPROM.read(addr_min);
      sec = EEPROM.read(addr_sec);
      mode = 2;
      second_mode = 0;
    }
    if (set_btn.released()){
      min = EEPROM.read(addr_min);
      sec = EEPROM.read(addr_sec);
      mode = 1;
      second_mode = 0;
    }
  }
}

int enc_tick(){ //  возвращает -1 при повороте налево и +1 направо

  long newPosition = enc.read();

  if (newPosition - oldPosition >= 4) {
    oldPosition = newPosition;
    return +1;
  }
  else if (newPosition - oldPosition <= -4) {
    oldPosition = newPosition;
    return -1;
  }
  return 0;
}
