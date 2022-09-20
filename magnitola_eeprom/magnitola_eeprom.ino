#include <EEPROM.h> // подключаем библиотеку EEPROM

int track = 1;
int freque = 10740;
int volDF = 1;
int volFM = 15;
int mono = 1;

byte hi, low;

void setup() {
  hi = highByte(track); // старший байт
  low = lowByte(track); // младший байт
  EEPROM.write(0, hi);  // записываем в ячейку 0 старший байт
  EEPROM.write(1, low); // записываем в ячейку 1 младший байт
  hi = highByte(freque); // старший байт
  low = lowByte(freque); // младший байт
  EEPROM.write(2, hi);  // записываем в ячейку 2 старший байт
  EEPROM.write(3, low); // записываем в ячейку 3 младший байт
  EEPROM.write(4, volDF); // запись числа в ячейку 4
  EEPROM.write(5, volFM); // запись числа в ячейку 5
  EEPROM.write(6, mono); // запись числа в ячейку 6
}

void loop() {
  
}
