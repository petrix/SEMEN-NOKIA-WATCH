#include <Wire.h>
#include "TimeLib.h"
#include "NOKIA5110_TEXT.h"
#include <iarduino_RTC.h>
// EncButton2<EB_ENCBTN> enc(INPUT_PULLUP, 2, 3, 4);  // энкодер с кнопкой
#define RST 8 // Reset pin
#define CE 7 // Chip enable
#define DC 6 // data or command
#define DIN 5 // Serial Data input
#define CLK 4 // Serial clock

iarduino_RTC rtc(RTC_DS1302, 9, 10, 11);
// Create an LCD object
NOKIA5110_TEXT lcd(RST, CE, DC, DIN, CLK);

#define inverse  false
#define contrast 0xBF // default is 0xBF set in LCDinit, Try 0xB1 - 0xBF if your display is too dark
#define bias 0x12 // LCD bias mode 1:48: Try 0x12, 0x13 or 0x14


#define TIME_HEADER 'T' // Header tag for serial time sync message

#define HOUR 'H'
#define MIN 'I'
#define SEC 'S'
#define DAY 'D'
#define MONTH 'M'
#define YEAR 'Y'
unsigned long currentTime;
unsigned long syncTime;
unsigned long backlightTimeout;
unsigned long unixTime;

//  Определяем системное время:                           // Время загрузки скетча.
const char *strM = "JanFebMarAprMayJunJulAugSepOctNovDec"; // Определяем массив всех вариантов текстового представления текущего месяца.

const char *sysT = __TIME__; // Получаем время компиляции скетча в формате "SS:MM:HH".
const char *sysD = __DATE__; // Получаем дату  компиляции скетча в формате "MMM:DD:YYYY", где МММ - текстовое представление текущего месяца, например: Jul.
//  Парсим полученные значения sysT и sysD в массив i:    // Определяем массив «i» из 6 элементов типа int, содержащий следующие значения: секунды, минуты, часы, день, месяц и год компиляции скетча.
const int iTime[6] {
  (sysT[6] - 48) * 10 + (sysT[7] - 48),
  (sysT[3] - 48) * 10 + (sysT[4] - 48),
  (sysT[0] - 48) * 10 + (sysT[1] - 48),
  (sysD[4] - 48) * 10 + (sysD[5] - 48),
  // (sysD[5] - 48),
  ((int)memmem(strM, 36, sysD, 3) + 3 - (int)&strM[0]) / 3,
  (sysD[9] - 48) * 10 + (sysD[10] - 48)
};

// const byte minOffset = 60;
// const unsigned int hourOffset = (unsigned int) 60 * 60;
// const unsigned long dayOffset = (unsigned long) 60 * 60 * 24;
// const unsigned long monthOffset = (unsigned long) 60 * 60 * 24 * 30;
// const unsigned long yearOffset = (unsigned long) 60 * 60 * 24 * 30 * 12;

// uint8_t selector[8] = {0x10, 0x09, 0x05, 0x03, 0x0F, 0x00, 0x00, 0x00};
uint8_t arrow[8] = {0x04, 0x02, 0x09, 0x02, 0x04, 0x00, 0x00, 0x00};
byte dd[] = {0x0E, 0x0A, 0x0E, 0x00, 0x0E, 0x0A, 0x0E, 0x00};
uint8_t s3[8] = {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00};
uint8_t s2[8] = {0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00};
uint8_t s1[8] = {0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00};
uint8_t s0[8] = {0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00};
void setup()
{
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  digitalWrite(2, LOW);
  digitalWrite(3, HIGH);
  //  delay(300);

  //  lcd.cursor_on();
  // lcd.blink_on();
  rtc.begin();
  //  rtc.settime(i[0], i[1], i[2], i[3], i[4], i[5]);
  setTime(rtc.Unix);
  //  Serial.begin(115200);
  Serial.begin(9600);
  while (!Serial)
    ; // Wait for Arduino Serial Monitor to open
  //  delay(100);
  if (timeStatus() != timeSet)
  {
    Serial.println("Unable to sync with the RTC");
  }
  else
  {
    Serial.println("RTC has set the system time");
    Serial.println(rtc.Unix);
    Serial.println(rtc.gettime("d-M-Y, H:i:s"));
    Serial.println(iTime[0]);
    Serial.println(iTime[1]);
    Serial.println(iTime[2]);
    Serial.println(iTime[3]);
    Serial.println(iTime[4]);
    Serial.println(iTime[5]);
    setTime(iTime[2], iTime[1], iTime[0], iTime[3], iTime[4], iTime[5]);
  }

  ///////////////////////////////////////////////////////////
  Serial.println(sysT);
  Serial.println(sysD);

  pinMode(13, OUTPUT);
  setSyncProvider(requestSync); // set function to call when sync required
  setSyncInterval(100);

  backlightTimeout = millis();

  //  lcd.init();
  //  lcd.LCDInit();
  delay(50);
  lcd.LCDInit(inverse, contrast, bias); // init the lCD

  lcd.LCDClear();
  lcd.LCDFont(LCDFont_Huge);
  lcd.LCDgotoXY(0, 0);
  lcd.LCDString(sysT);
  lcd.LCDgotoXY(0, 2);
  lcd.print(sysD);
  delay(1000);

}
boolean backLT_state = 1;
boolean editMode = 0;
boolean editH = 0;
boolean editI = 0;
boolean editS = 0;
boolean editD = 0;
boolean editM = 0;
boolean editY = 0;
int8_t h_Value = hour();
int8_t i_Value = minute();
int8_t s_Value = second();
int8_t d_Value = day();
int8_t m_Value = month();
int16_t y_Value = year() - 2000;
const char *monthArray[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
void loop()
{
  /////////////////////////

  /////////////////////////
  //  enc.tick(); // опрос происходит здесь

  if (millis() - currentTime > 1000)
  {
    currentTime = millis();
    digitalWrite(13, HIGH);
    if (editMode == 0)
    {
      //      drawClockValues();
      //      drawDateValues();

    }
    //        char strTime = h_Value + ":" + i_Value + ":" + s_Value;
//    lcd.LCDClear();
    lcd.LCDFont(LCDFont_Huge);
    lcd.LCDgotoXY(0, 0);
    if (hour() < 10)lcd.print(F("0"));
    lcd.print(hour());
    //    lcd.LCDgotoXY(40, 0);
    lcd.print(F(":"));

    if (minute() < 10)lcd.print(F("0"));
    lcd.print(minute());
    //    lcd.print(":");
    //    lcd.LCDgotoXY(60, 0);
    lcd.LCDFont(LCDFont_Huge);
    lcd.LCDgotoXY(25, 3);
    if (second() < 10)lcd.print(F("0"));
    lcd.print(second());
    lcd.LCDgotoXY(0, 2);
    //    lcd.print(sysD);


    ////////////////
    if (Serial.available() > 1)
    { // wait for at least two characters
      char c = Serial.read();
      Serial.println(c);
      if (c == TIME_HEADER)
      {
        processSyncMessage();
      }
    }
    ///////////////

    if (millis() - currentTime > 10)
      digitalWrite(13, LOW);
    //    delay(10);
    //    digitalWrite(13, LOW);


  }
  /*
    if (millis() - syncTime > 60000) {
    syncTime = millis();
    Serial.println("unix");
    rtc.gettime();
    //    const unsigned long unix = rtc.gettimeUnix();
    unixTime = rtc.gettimeUnix();
    //    Serial.println(rtc.Unix);
    Serial.println(unixTime);
    //    setTime(rtc.Unix);
    setTime(unixTime);
    }
  */
  //  if (millis() - backlightTimeout > 30000) {
  //    lcd.noBacklight();
  //    lcd.noDisplay();
  //  }
  //////////////////////////////////////////
  //////////////////////////////////////////
  //////////////////////////////////////////




}

void printDotsAndSpaces(void)
{
  lcd.LCDgotoXY(0, 0);
  lcd.write(3);
  lcd.write(2);
  lcd.write(1);
  lcd.write(0);
  lcd.LCDgotoXY(12, 0);
  lcd.write(0);
  lcd.write(1);
  lcd.write(2);
  lcd.write(3);
  //  lcd.LCDString("    ");
  lcd.LCDgotoXY(6, 0);
  lcd.write(2);
  //  lcd.LCDString(":");
  lcd.LCDgotoXY(9, 0);
  lcd.write(2);
  //  lcd.LCDString(":");
  //  lcd.LCDgotoXY(0, 1);
  //  lcd.write(0);
  //  lcd.LCDString("_");
  lcd.LCDgotoXY(3, 1);
  lcd.write(1);
  lcd.write(0);
  //  lcd.LCDString("_");
  lcd.LCDgotoXY(7, 1);
  lcd.write(0);
  //  lcd.LCDString("_");
  lcd.LCDgotoXY(11, 1);
  lcd.write(0);
  //  lcd.LCDString("_");
}
void drawClockValues(void)
{
  //********************
  //**    12:34:56    **
  //**                **
  //********************
  //
  //  Serial.println(rtc.Unix);
  //  lcd.LCDgotoXY(0, 0);
  //  lcd.write(second());
  // digital clock display of the time
  ////////////////////////////////////
  lcd.LCDgotoXY(4, 0);
  if (hour() < 10)
    lcd.LCDString("0");
  lcd.LCDString(hour());
  lcd.LCDgotoXY(7, 0);
  if (minute() < 10)
    lcd.LCDString("0");
  lcd.LCDString(minute());
  lcd.LCDgotoXY(10, 0);
  if (second() < 10)
    lcd.LCDString("0");
  lcd.LCDString(second());
}
//////////////////////////////////////
void drawDateValues(void)
{
  lcd.LCDgotoXY(0, 1);
  lcd.LCDString(dayShortStr(weekday()));
  lcd.LCDgotoXY(5, 1);
  if (day() < 10)
    lcd.LCDString("0");
  lcd.LCDString(day());
  lcd.LCDgotoXY(8, 1);
  lcd.LCDString(monthShortStr(month()));
  lcd.LCDgotoXY(12, 1);
  lcd.LCDString(year());
  //////////////////////////////////////
  //  lcd.LCDgotoXY(4, 0);
  //  if (rtc.gettime("H") < 10) lcd.LCDString('0');
  //  lcd.LCDString(rtc.gettime("H"));
  //  lcd.LCDString(':');
  //  if (rtc.minutes < 10) lcd.LCDString('0');
  //  lcd.LCDString(rtc.minutes);
  //  lcd.LCDString(':');
  //  if (rtc.seconds < 10) lcd.LCDString('0');
  //  lcd.LCDString(rtc.seconds);
  ///////////////////////////////////////
  //  lcd.LCDgotoXY(1, 1);
  //  //  lcd.LCDString(rtc.weekday);
  //  lcd.LCDString(rtc.gettime("D"));
  //  lcd.LCDString(' ');
  //  lcd.LCDString(rtc.day);
  //  lcd.LCDString(' ');
  //  lcd.LCDString(rtc.gettime("M"));
  //  lcd.LCDString(' ');
  //  lcd.LCDString(rtc.gettime("Y"));
  ///////////////////////////////////////
}

void processSyncMessage(void)
{
  unsigned long value;
  char c = Serial.read();
  Serial.println(c);
  switch (c)
  {
    case HOUR:
      value = Serial.parseInt();
      rtc.settime(-1, -1, value);
      // h_value = value;
      Serial.println("HOUR");
      Serial.println(value);
      h_Value = value;
      setTime(h_Value, -1, -1, -1, -1, -1);
      Serial.println(hour());

      break;
    case MIN:
      value = Serial.parseInt();
      //      if (value < 60)
      rtc.settime(-1, value);
      Serial.println("MIN");
      Serial.println(value);
      break;
    case SEC:
      value = Serial.parseInt();
      //      if (value < 60)
      rtc.settime(value);
      Serial.println("SEC");
      Serial.println(value);
      break;
    case DAY:
      value = Serial.parseInt();
      //      if (value < 32)
      rtc.settime(-1, -1, -1, value);
      Serial.println("DAY");
      Serial.println(value);
      break;
    case MONTH:
      value = Serial.parseInt();
      //      if (value < 13)
      rtc.settime(-1, -1, -1, -1, value);
      Serial.println("MONTH");
      Serial.println(value);
      break;
    case YEAR:
      value = Serial.parseInt();
      rtc.settime(-1, -1, -1, -1, -1, value);
      Serial.println("YEAR");
      Serial.println(value);
      break;
    default:
      // выполнить, если val ни 1 ни 2
      // default опционален
      break;
  }
  //  unsigned long pctime;
  //  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 - paul, perhaps we define in time.h?
  //
  //  pctime = Serial.parseInt();
  //  Serial.println(pctime);
  //  if ( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
  //    setTime(pctime); // Sync Arduino clock to the time received on the serial port
  //  }
  //  const unsigned long unix = rtc.gettimeUnix();
  unixTime = rtc.gettimeUnix();
  Serial.println(unixTime);
  setTime(unixTime);
}

time_t requestSync()
{
  return 0; // the time will be sent later in response to serial mesg
}
