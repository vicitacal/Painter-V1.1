
//#define SMOOTH_ALGORITHM
//#define DEBUG_EN
//#define NOT_SOLID_SNAKE

#include <Arduino.h>
#include "GyverTimers.h"
#include "GyverEncoder.h"
#include "GyverStepper.h"
#include "GyverButton.h"
#include "Keypad.h"
#include "U8g2lib.h"
#include <PinOut.h>
#include "Bitmap.h"
#include "PinOut.h"
#include <EEPROM.h>
 
char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
byte rowPins[ROWS] = {keypadRow1, keypadRow2, keypadRow3, keypadRow4}; //подключить к выводам строк клавиатуры
byte colPins[COLS] = {keypadCol1, keypadCol2, keypadCol3};             //подключить к выводам столбцов клавиатуры

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

Encoder encoder(enc1, enc2, encBtn, TYPE2);

U8G2_ST7920_128X64_F_SW_SPI screen(U8G2_R0, scr_en, scr_rw, scr_di, U8X8_PIN_NONE);

GStepper<STEPPER2WIRE> stepperX(stepPerRevoluteXY, stepX_pin, dirX_pin, enableX_pin);
GStepper<STEPPER2WIRE> stepperY(stepPerRevoluteXY, stepY_pin, dirY_pin, enableY_pin);
GStepper<STEPPER2WIRE> stepperZ(stepPerRevoluteZ, stepZ_pin, dirZ_pin, enableZ_pin);

GButton Xend(Xswith, HIGH_PULL, true);
GButton Yend(Yswith, HIGH_PULL, true);
GButton Zend(Zswith, HIGH_PULL, true);

bool calibrates, calibrated, keyPresset, runProgramm, doubleSnake, paintButt, error;
volatile bool numChange, servChange;
byte screenNum, firstCursPos, secondCursPos, thirdCursPos, lineNumberX, lineNumberY;
char key;
int scrapLen, scrapW, sprayWidth, sprayWidthStep, takeNumber, hui;
uint64_t drawTimer, autoOffTimer;
void DrowMain(byte scrNumber);
void DrawBogie(long cutX, long curY);
void DrawPreview();
void manualTics();
bool calibrate(bool mode);
void setupMotors();
void mainProgram(bool isRun);
void setPaintMode(byte mode);
bool changePaint(byte comand);

void setup()
{
#ifdef DEBUG_EN
  Serial.begin(9600);
#endif
  doubleSnake = EEPROM.read(0);
  paintButt = EEPROM.read(1);
  EEPROM.get(2, sprayWidthStep);
  sprayWidth=sprayWidthStep/stepPerMM;
  //Xend.setDebounce(110);
  //Yend.setDebounce(110);
  //Zend.setDebounce(110);
  Timer3.setPeriod(900);
  Timer3.enableISR(CHANNEL_B);
  pinMode(paintMosfet, OUTPUT);
  pinMode(airMosfet, OUTPUT);
  setupMotors();
  DrawPreview();
}

void loop()
{
  manualTics();
  switch (screenNum)
  {
  case 0:
    switch (key)
    {
      case '*':
        calibrate(true);
      break;
      case '4':
        if (calibrate(false)) stepperX.setSpeed(-maxSpdXY, SMOOTH);
      break;
      case '6':
        if (calibrate(false)) stepperX.setSpeed(maxSpdXY, SMOOTH);
      break;
      case '5':
        stepperX.setSpeed(0);
        stepperY.setSpeed(0);
      break;
      case '2':
        if (calibrate(false)) stepperY.setSpeed(maxSpdXY, SMOOTH);
      break;
      case '8':
        if (calibrate(false)) stepperY.setSpeed(-maxSpdXY, SMOOTH);
      break;
      case '#':
        if(!runProgramm){
          calibrated=false;
          stepperX.disable();
          stepperY.disable();
          stepperZ.disable();
        }
      break;
    }
    
    if (encoder.isTurn())
    {
      firstCursPos++;
      firstCursPos %= 2;
    }

    if (encoder.isClick())
    { 
      switch (firstCursPos){
        case 0: 
          if(runProgramm){
            runProgramm=false;
          } else {
            screenNum = 1;
            encoder.resetStates();
          }
        break;
        case 1: 
          screenNum = 2; 
          encoder.resetStates();
        break;
      }
    }
  break;

  case 1:
    if (!keyPresset && numChange)
    {
      int curBut = key & 0x0F;
      switch (secondCursPos){
      case 0:
        lineNumberY = lineNumberY * 10 + curBut;
        if(lineNumberY > workPlaseY/sprayWidthStep) lineNumberY = workPlaseY/sprayWidthStep;      //workPlaseY/stepPerMM;
        scrapLen = lineNumberY * sprayWidth;
      break;
      case 1:
        lineNumberX = lineNumberX * 10 + curBut;
        if(lineNumberX > workPlaseX/sprayWidthStep) lineNumberX = workPlaseX/sprayWidthStep;
        scrapW = lineNumberX * sprayWidth;
      break;
      }
      keyPresset = true;
    }

    if (keyPresset && !numChange) keyPresset = false;

    switch (key)
    {
      case '*':
        secondCursPos++;
        secondCursPos %= 4;
      break;
      case '#':
        if (secondCursPos == 0) {
          lineNumberY /= 10;
          scrapLen = lineNumberY * sprayWidth;
        }
        else if (secondCursPos == 1) {
          lineNumberX /= 10;
          scrapW = lineNumberX * sprayWidth;
        }
      break;
    }
    
    if(encoder.isLeft()) {
      secondCursPos++;
      secondCursPos %= 4;
    }

    if(encoder.isRight()){
      secondCursPos--;
      secondCursPos %= 4;
    }

    if(encoder.isClick() || key=='*'){
      switch(secondCursPos){
        case 2: 
          screenNum=0; 
          encoder.resetStates();
        break;
        case 3: 
          if(!changePaint(0) && !error){
            screenNum=0;
            runProgramm=true;
            encoder.resetStates();
          }
        break;
      }
    }
    
  break;

  case 2:

    static bool wasTaked;

    if(encoder.isLeft()) {
      thirdCursPos++;
      thirdCursPos %= setupCount;
      takeNumber=false;
    }

    if(encoder.isRight()){
      thirdCursPos--;
      if(thirdCursPos>setupCount) thirdCursPos=(setupCount-1);
      takeNumber=false;
    }
    if(encoder.isClick()) {
      switch (thirdCursPos)
      {
      case 0:
        screenNum=0;
        changePaint(3);
        encoder.resetStates();
      break;
      case 1:
        if(!changePaint(0)) changePaint(1);
      break;
      case 2:
        if(!changePaint(0)) changePaint(2);
      break;
      case 3:
        doubleSnake=!doubleSnake;
        EEPROM.write(0, doubleSnake);
      break;
      case 4:
        paintButt=!paintButt;
        EEPROM.write(1, paintButt);
      break;
      case 5:
        if(takeNumber){
          takeNumber=false;
        } else {
          takeNumber=true;
          wasTaked=true;
        }
      break;
      }
    }

    if (wasTaked && !takeNumber) {
      sprayWidthStep=sprayWidth*stepPerMM;
      EEPROM.put(2, sprayWidthStep);
      scrapLen = lineNumberY * sprayWidth;
      scrapW = lineNumberX * sprayWidth;
      wasTaked=false;
    }

    if (!keyPresset && numChange && takeNumber)
    {
      int curBut = key & 0x0F;
      sprayWidth = sprayWidth * 10 + curBut;
      if(sprayWidth > 2000) sprayWidth = 2000;
      keyPresset = true;
    }

    if (keyPresset && !numChange && takeNumber) keyPresset = false;

    if(takeNumber && servChange) switch (key){
      case '*':
        takeNumber=false;
      break;
      case '#':
        sprayWidth /= 10;
      break;
    }

  break;
  }

  if(millis()-drawTimer>=drawPeriod){
    drawTimer=millis();
    DrowMain(screenNum);
  }
}

void DrowMain(byte scrNumber)
{
  screen.clearBuffer();
  switch (scrNumber)
  {
  case 0:
    DrawBogie(stepperX.getCurrent(), stepperY.getCurrent());
    screen.setDrawColor(1);
    screen.drawFrame(2, 2, 80, 58); //рама

    screen.drawLine(2, 2, 5, 6); //Ножки
    screen.drawLine(2, 60, 5, 64);
    screen.drawLine(82, 2, 84, 6);
    screen.drawLine(82, 60, 84, 64);

    screen.setCursor(91, 8); //Статус и его значения
    screen.print("Статус:");
    if(error){
      screen.setCursor(91, 16);
      screen.print("Ошибка");
    } else if(calibrates){
      screen.setCursor(86, 16);
      screen.print("Калибровка");
    } else if(runProgramm){
      screen.setCursor(91, 16);
      screen.print("Работа");
    } else if(calibrated){
      screen.setCursor(91, 16);
      screen.print("Готов!");
    }else{
      screen.setCursor(91, 16);
      screen.print("Не дома");
    }
    screen.setDrawColor(2);
    screen.drawFrame(83, 19, 44, 26); //Обводка значений координат

    if (Xend.state())
    {
      screen.drawBox(84, 20, 5, 8);
      //Serial.println("X prest");
    }
    if (Yend.state())
    {
      
      screen.drawBox(84, 28, 5, 8);
      //Serial.println("Y prest");
    }
    if (Zend.state())
    {
      screen.drawBox(84, 36, 5, 8);
    }

    screen.setCursor(85, 26);
    screen.print("X=");
    #ifdef DEBUG_EN
      screen.print(stepperX.getCurrent());
    #else
      if (calibrated) screen.print(stepperX.getCurrent()); else screen.print("?");
    #endif
    
    
    screen.setCursor(85, 34);
    screen.print("Y=");
    #ifdef DEBUG_EN
      screen.print(stepperY.getCurrent());
    #else
      if (calibrated) screen.print(stepperY.getCurrent()); else screen.print("?");
    #endif
    

    screen.setCursor(85, 42);
    screen.print("Z=");
    #ifdef DEBUG_EN
      screen.print(stepperZ.getCurrent());
    #else
      if (calibrated) screen.print(stepperZ.getCurrent()); else screen.print("?");
    #endif
    
    if (firstCursPos == 1)
    { 
      screen.drawFrame(83, 50, 19, 10); //закраска кнопки пуск
      screen.drawBox(103, 50, 24, 10);
    }
    else
    {
      screen.drawBox(83, 50, 19, 10);
      screen.drawFrame(103, 50, 24, 10);
    }

    screen.setCursor(85, 57);
    if(runProgramm) screen.print("Стоп"); else screen.print("Пуск");

    screen.setCursor(105, 57);
    screen.print("Настр");
    break;
  case 1:
    screen.setDrawColor(1);
    screen.drawFrame(2, 2, 80, 58); //рама

    screen.drawLine(2, 2, 5, 6); //Ножки
    screen.drawLine(2, 60, 5, 64);
    screen.drawLine(82, 2, 84, 6);
    screen.drawLine(82, 60, 84, 64);

    /*
    screen.drawFrame(21, 35, 58, 22); //Дверь
    screen.drawRFrame(25, 38, 23, 7, 1);
    screen.drawRFrame(25, 47, 23, 7, 1);
    screen.drawRFrame(51, 38, 7, 7, 1);
    screen.drawRFrame(51, 47, 7, 7, 1);
    screen.drawRFrame(61, 38, 14, 7, 1);
    screen.drawRFrame(61, 47, 14, 7, 1);
    */

    switch (secondCursPos)
    {
    case 0:
      screen.drawBox(96, 13, 21, 9);
    break;
    case 1:
      screen.drawBox(96, 37, 21, 9);
    break;
    case 2:
      screen.drawBox(83, 50, 19, 10);
    break;
    case 3:
      screen.drawBox(103, 50, 24, 10);
    break;
    }
    screen.drawFrame(96, 13, 21, 9);
    screen.drawFrame(96, 37, 21, 9);
    screen.drawFrame(83, 50, 19, 10);
    screen.drawFrame(103, 50, 24, 10);

    screen.drawBox(80-map(scrapLen*stepPerMM, 0, workPlaseY, 0, 76),58-map(scrapW*stepPerMM, 0, workPlaseX, 0, 54),map(scrapLen*stepPerMM, 0, workPlaseY, 0, 76),map(scrapW*stepPerMM, 0, workPlaseX, 0, 54));

    //screen.drawFrame(93, 2, 23, 10);
    //screen.drawFrame(92, 26, 27, 10);
    
    /*
    screen.drawLine(16, 35, 16, 36);
    screen.drawLine(16, 55, 16, 56);
    screen.drawLine(21, 30, 39, 30);
    screen.drawLine(78, 30, 59, 30);
    screen.drawLine(21, 35, 21, 30);
    screen.drawLine(21, 35, 16, 35);
    screen.drawLine(78, 35, 78, 30);
    screen.drawLine(21, 56, 16, 56);
    */

    screen.setDrawColor(2);
    screen.setCursor(95, 9);
    screen.print("Длина:");
    screen.setCursor(94, 34);
    screen.print("Ширина:");
    screen.setCursor(105, 57);
    screen.print("Старт");
    screen.setCursor(85, 57);
    screen.print("Назд");
    
    screen.setCursor(98, 20);
    screen.print(lineNumberY);
    screen.setCursor(98, 44);
    screen.print(lineNumberX);
  
  break;
  case 2:

    screen.setDrawColor(1);

    screen.drawLine(0,8,128,8);         //линии сверху и сниху + кнопка
    screen.drawFrame(2,53,23,10);         //назад
    screen.drawFrame(36,53,54,10);        //Замена краски
    screen.drawFrame(92,53,35,10);        //Продувка
    screen.drawLine(0,51,128,51);  

   // screen.drawFrame(1,10,126,10);     //Элементы меню
   // screen.drawFrame(1,21,126,10);
   // screen.drawFrame(1,33,126,10);
    
    
    switch (thirdCursPos)
    {
    case 0:
      screen.drawBox(2,53,23,10);          //назад
    break;
    case 1:
      screen.drawBox(36,53,54,10);        //Замена краски
    break;
    case 2:
      screen.drawBox(92,53,35,10);        //Продувка
    break;
    case 3:
      screen.drawBox(1,10,126,10);
    break;
    case 4:
      screen.drawBox(1,21,126,10);
    break;
    case 5:
      if (!takeNumber) screen.drawBox(1,33,126,10);
    break;
    }
  

    screen.setDrawColor(2);

    screen.setCursor(46,6);             //надписи
    screen.print("Настройки");
    screen.setCursor(4,60);
    screen.print("Назад");
    screen.setCursor(4,17);
    screen.print("Двойная змейка");
    screen.setCursor(4,28);
    screen.print("Красить торцы");
    screen.setCursor(4,40);
    screen.print("Шинина распыла");
    screen.setCursor(110,40);
    screen.print(sprayWidth);
    screen.setCursor(38,60);
    screen.print("Замена краски");
    screen.setCursor(94,60);
    screen.print("Продувка");

    if(doubleSnake) {
      screen.setCursor(117,17);             //надписи
      screen.print("Да");
    } else {
      screen.setCursor(113,17);             //надписи
      screen.print("Нет");
    }
    if(paintButt) {
      screen.setCursor(117,28);             //надписи
      screen.print("Да");
    } else {
      screen.setCursor(113,28);             //надписи
      screen.print("Нет");
    }

    if (takeNumber) {
      screen.drawBox(108, 34, 16, 8);
    }
    
  break;
  }
  screen.sendBuffer();
}

void DrawBogie(long curX, long curY)
{
  screen.setDrawColor(1);
  curX = map(curX, 0, maxStepX, 0, 53);
  curY = map(curY, 0, maxStepY, 0, 74);
  screen.drawFrame(6 + curY, 3 + curX, 5, 5);
  screen.drawBox(3 + curY, 3, 3, 56);
  screen.setDrawColor(0);
  for (int i = 0; i < 54; i += 2)
  {
    screen.drawPixel(4 + curY, 4 + i);
  }
}

void DrawPreview()
{
  screen.begin();
  screen.enableUTF8Print();
  screen.clearBuffer();
  screen.setFont(u8g2_font_4x6_t_cyrillic);
  screen.setFontMode(true);
  screen.setDrawColor(1);
  screen.drawXBMP(34, 0, 60, 64, start_bmp);
  screen.sendBuffer();
  delay(1000);
}

void manualTics(){
  mainProgram(runProgramm);
  calibrate(false);
  if(stepperX.getState() || stepperY.getState() || stepperZ.getState()){
    autoOffTimer=millis();
  }
  if(!runProgramm) {
    changePaint(0);
    key = keypad.getKey();
    if (key != NO_KEY)
    {
      if (key == '*' || key == '#')
      {
        servChange = true;
        numChange = false;
      }
      else
      {
        numChange = true;
        servChange = false;
      }
    }
    else
    {
      servChange = false;
      numChange = false;
    }
    if(millis()-autoOffTimer>=autoOffTime){
      autoOffTimer=millis();
      stepperX.disable();
      stepperY.disable();
      stepperZ.disable();
      calibrated=false;
    }
  }

  if(stepperX.getMode()) {
    if((stepperX.getCurrent() > maxStepX && stepperX.getSpeed()>0) || (Xend.state() && stepperX.getSpeed()<0)){
      stepperX.brake();
    }
  } else {
    if(stepperX.getTarget()>maxStepX) stepperX.setTarget(maxStepX);
  }

  if(stepperY.getMode()) {
    if((stepperY.getCurrent() > maxStepY && stepperY.getSpeed()>0) || (Yend.state() && stepperY.getSpeed()<0)){
      stepperY.brake();
    }
  } else {
    if(stepperY.getTarget()>maxStepY) stepperY.setTarget(maxStepY);
  }
}

bool calibrate(bool mode)
{
  if (calibrated) return true;
  if (mode && !error) calibrates = true;

  static bool Xcal, Ycal, Zcal;
  static byte calibStates;

  if (calibrates)
  {
    switch(calibStates){
      case 0:
        stepperX.enable();
        stepperX.setRunMode(KEEP_SPEED);
        stepperX.setSpeed(-calibr_speed);
        stepperY.enable();
        stepperY.setRunMode(KEEP_SPEED);
        stepperY.setSpeed(-calibr_speed);
        stepperZ.enable();
        if (Zend.state()){
          stepperZ.reset();
          stepperZ.setRunMode(FOLLOW_POS);
          stepperZ.setTarget(300);
        } else {
          stepperZ.setRunMode(KEEP_SPEED);
          stepperZ.setSpeed(-calibr_speedZ);
        }
        calibStates++;
      break;
      case 1:
        if (Xend.state()){
          stepperX.reset();
          Xcal = true;
        }
        if (Yend.state()){
          stepperY.reset();
          Ycal = true;
        }
        if (stepperZ.getMode()){
          if(Zend.state()){
            stepperZ.reset();
            stepperZ.setCurrent(startOffsetZ);
            Zcal = true;
          }
        } else if(!stepperZ.tick()){
          stepperZ.setRunMode(KEEP_SPEED);
          stepperZ.setSpeed(-calibr_speedZ);
        }
        if (Xcal && Ycal && Zcal){
          stepperZ.setRunMode(FOLLOW_POS);
          stepperZ.setTarget(0);
          calibStates++;
        }
      break;
      case 2:
        if(!stepperZ.tick()){
          stepperX.setRunMode(FOLLOW_POS);
          stepperY.setRunMode(FOLLOW_POS);
          stepperZ.setRunMode(FOLLOW_POS);
          stepperX.setTarget(200);
          stepperY.setTarget(200);
          stepperZ.setTarget(200);
          calibStates++;
        }
      break;
      case 3:
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){
          if(!Xend.state() && !Yend.state() && !Zend.state()){
            stepperX.setTarget(0);
            stepperY.setTarget(10);
            stepperZ.setTarget(0);
            calibStates++;
          } else {
            calibrates = false;
            error=true;
            stepperX.disable();
            stepperY.disable();
            stepperZ.disable();
          }
        }
      break;
      case 4:
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){
          calibrated = true;
          calibrates = false;
          Xcal = false;
          Ycal = false;
          Zcal = false;
          calibStates=0;
          stepperY.reset();
          stepperX.setRunMode(KEEP_SPEED);
          stepperY.setRunMode(KEEP_SPEED);
          stepperZ.setRunMode(KEEP_SPEED);
        }
      break;
    }
  }
  return calibrated;
}

void setupMotors()
{
  stepperX.setMaxSpeed(maxSpdXY);
  stepperY.setMaxSpeed(maxSpdXY);
  stepperZ.setMaxSpeed(maxSpdZ);
  stepperX.setAcceleration(maxAcselXY);
  stepperY.setAcceleration(maxAcselXY);
  stepperZ.setAcceleration(maxAcselZ);
  stepperX.disable();
  stepperY.disable();
  stepperZ.disable();
}

ISR(TIMER3_B)
{
  encoder.tick();
  stepperX.tick();
  stepperY.tick();
  stepperZ.tick();
  Xend.tick();
  Yend.tick();
  Zend.tick();
}

void mainProgram(bool isRun){
  static bool notFirstRun, ProgrammRuned;
  static bool directionY, directionX;
  static int  numLinesY, addSizeY, numLinesX, addSizeX, curSprayWithStep;
  static long scrapLenSteps, scrapWSteps;
  static byte state;

  if(isRun && calibrate(isRun)){

    if(!notFirstRun){
      ProgrammRuned = true;
      curSprayWithStep = sprayWidthStep;
      scrapWSteps = scrapW * stepPerMM;
      scrapLenSteps = scrapLen * stepPerMM;
      numLinesY = scrapWSteps / curSprayWithStep;
      addSizeY = scrapWSteps - numLinesY * curSprayWithStep;
      numLinesX = scrapLenSteps / curSprayWithStep;
      addSizeX = scrapLenSteps - numLinesX * curSprayWithStep;
      state = 0;
      directionY = false;
      directionX = false;
      notFirstRun = true;
      stepperX.setRunMode(FOLLOW_POS);
      stepperY.setRunMode(FOLLOW_POS);
      stepperZ.setRunMode(FOLLOW_POS);
    }

    switch(state){
      case 0:                                                               /*Встать на левый верхний угол    \/                 */
        stepperX.setTarget(offsetX+workPlaseX-scrapWSteps);                 //                                    
        stepperY.setTarget(offsetY+workPlaseY-scrapLenSteps);
        if (paintButt) stepperZ.setTarget(UpDownTurn); else stepperZ.setTarget(LeftRightTurn); 
        state++;
      break;
      
      case 1:   
        if (paintButt) {                                                          //Покрасить торец   | 
          if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){           //                  \/* 
            setPaintMode(1);
            stepperX.setTarget(directionX?-scrapWSteps:scrapWSteps, RELATIVE);                
            directionX=!directionX;
            state++;
          } 
        } else state=7;
      break;

      case 2:
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){           // Повернуться
          setPaintMode(0);                                                      //               >                                
          stepperZ.setTarget(LeftRightTurn);
          state++;
        }
      break;

      case 3:
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){            //Покрасить нижний торец
          setPaintMode(1);                                                       //                         ->*
          stepperY.setTarget(directionY?-scrapLenSteps:scrapLenSteps, RELATIVE);
          directionY=!directionY;
          state++;
        }
      break;

      case 4:
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){             //Повернуться
          setPaintMode(0);                                                        /*                        /\        */
          stepperZ.setTarget(UpDownTurn);
          state++;
        }
      break;

      case 5:
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){    //Покрасить другой торец        ^*
          setPaintMode(1);                                               //                              |
          stepperX.setTarget(directionX?-scrapWSteps:scrapWSteps, RELATIVE);                
          directionX=!directionX;
          state++;
        }
      break;

      case 6:                                                             //Повернуться           <
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){     //                          
          setPaintMode(0);
          stepperZ.setTarget(LeftRightTurn);
          state++;
        }  
      break;

      case 7:                                                             //Переехать на противоположную сторону    *<-->*    
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){     //                                              
          setPaintMode(1);
          stepperY.setTarget(directionY?-scrapLenSteps:scrapLenSteps, RELATIVE);
          directionY=!directionY;
          state++;
        }
      break;

      case 8:                                                             //Спуститься на ширину распыла   |
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){     //                               \/
          #ifdef NOT_SOLID_SNAKE
            setPaintMode(0);
          #endif
          if(numLinesY>0) {
            stepperX.setTarget(curSprayWithStep, RELATIVE);
          }
          state++;
        }
      break;

      case 9:
        if(numLinesY>0){                                                   //Повторить если не закончились линии
          numLinesY--;
          state = 7;
        } else {
          directionX=!directionX;
          state++;
        }
      break;

      case 10:                                                                      
        if(addSizeY>0){                                                           //Спуститься на оставшуюся ширину 
          if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){           //                                |      
            stepperX.setTarget(addSizeY, RELATIVE);                               //                                \/
            state++;
          }
        } else state = 12;
      break;

      case 11:
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){             //Покрасить край 
          setPaintMode(1);                                                        //  
          stepperY.setTarget(directionY?-scrapLenSteps:scrapLenSteps, RELATIVE);  //                ->*
          directionY=!directionY;
          state++;
        }
      break;
      
      case 12:
        if (doubleSnake) {
          if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){           //Развернуться                                               
            setPaintMode(0);                                                      /*                          /\                     */                                     
            stepperZ.setTarget(UpDownTurn);
            state++;
          } 
        } else state=18;
      break;

      case 13:
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){           //Провести линию вертекально   ^*
          setPaintMode(1);                                                      //                             |
          stepperX.setTarget(directionX?-scrapWSteps:scrapWSteps, RELATIVE); 
          directionX=!directionX;
          state++;
        }
      break;

      case 14:
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){           //Переместиться чуть дальше   ->
          #ifdef NOT_SOLID_SNAKE
            setPaintMode(0);
          #endif
          if(numLinesX>0){
            stepperY.setTarget(directionY?-curSprayWithStep:curSprayWithStep, RELATIVE);
          }
          state++;
        }
      break;

      case 15:
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){            //Повторять пока не закончатся линии.
          if (numLinesX>0){
            numLinesX--;
            state=13;
          } else {
            directionY=!directionY;
            state++;
          }
        }
      break;

      case 16:
        if(addSizeX>0){
          if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){           //Доехать до конца детали           
            stepperY.setTarget(directionY?-addSizeX:addSizeX, RELATIVE);                                
            state++;
          }
        } else state=18;
      break;

      case 17:
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){           //Провести линию вертекально   ^*
          setPaintMode(1);                                                      //                             |
          stepperX.setTarget(directionX?-scrapWSteps:scrapWSteps, RELATIVE); 
          directionX=!directionX;
          state++;
        }
      break;

      case 18:
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){         //Уехать на базу
          setPaintMode(0);
          state++;
          stepperX.setTarget(0);
          stepperY.setTarget(0);
          stepperZ.setTarget(0);
        }
      break;

      case 19:
        if(!stepperX.tick() && !stepperY.tick() && !stepperZ.tick()){
          runProgramm = false;
        }
      break;
    }
  } else if(ProgrammRuned){
    ProgrammRuned = false;
    notFirstRun = false;
    setPaintMode(0);
    stepperX.brake();
    stepperY.brake();
    stepperZ.brake();
    stepperX.setRunMode(KEEP_SPEED);
    stepperY.setRunMode(KEEP_SPEED);
    stepperZ.setRunMode(KEEP_SPEED);
  }
}

void setPaintMode(byte mode){

  switch(mode){

    case 0:                                   //Выкл
      digitalWrite(paintMosfet, LOW);
      digitalWrite(airMosfet, LOW);
    break;

    case 1:                                   //Красить
      digitalWrite(paintMosfet, HIGH);
      digitalWrite(airMosfet, HIGH);
    break;

    case 2:                                   //Слив краски
      digitalWrite(paintMosfet, HIGH);
      digitalWrite(airMosfet, LOW);
    break;
    case 3:                                   //Продувка
      digitalWrite(paintMosfet, LOW);
      digitalWrite(airMosfet, HIGH);
    break;
  }
}

bool changePaint(byte comand){
  static bool wasChenget, state, runChenge;
  static byte stateChange;
  static uint32_t changeTimer;

  if(comand == 1 && !runChenge && !runProgramm && !error) {
    runChenge=true;
    state=false;    //Слив краски 
  }
  if(comand == 2 && !runChenge && !runProgramm && !error) {
    runChenge=true;
    state=true;     //Продувка
  }
  if(comand == 3) runChenge=false;
  
  if(runChenge && calibrate(true)){
    switch (stateChange)
    {
      case 0:
        wasChenget=true;
        stepperX.setRunMode(FOLLOW_POS);
        stepperY.setRunMode(FOLLOW_POS);
        stepperX.setTarget(maxStepX);
        stepperY.setTarget(maxStepY);
        stateChange++;
      break;
      case 1:
        if(!stepperX.tick() && !stepperY.tick()){
          if(state) setPaintMode(3); else setPaintMode(2); 
          changeTimer=millis();
          stateChange++;
        }
      break;
      case 2:
        if(millis()-changeTimer>=changeTime){
          setPaintMode(0);
          stepperX.setTarget(0);
          stepperY.setTarget(0);
          stateChange++;
        }
      break;
      case 3:
        if(!stepperX.tick() && !stepperY.tick()){
          runChenge=false;
        }
      break;
    }
  } else if(wasChenget){
    setPaintMode(0);
    stepperX.setRunMode(KEEP_SPEED);
    stepperY.setRunMode(KEEP_SPEED);
    stepperX.brake();
    stepperY.brake();
    stateChange=0;
    wasChenget=false;
  }

  return runChenge;
}