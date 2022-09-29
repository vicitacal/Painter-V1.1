//----------------------Физические параметры-------------------------
#define maxAcselXY 2200                     //макс ускорение по XY (1800)
#define maxSpdXY 6500                      //Макс скорость по XY 
#define maxAcselZ 800                      //Макс ускорение по Z 
#define maxSpdZ 1000                        //Макс скорость по Z 
#define calibr_speed 2500                   //Скорость движения на калибровке 
#define calibr_speedZ 300                   //Скорость движения на калибровке 
#define stepPerMM 9                        //Шагов на миллиметр по XY 
#define maxStepX 15300                      //Максимальное перемещение от нуля в шагах (1734)
#define maxStepY 14600                      //Максимальное перемещение от нуля в шагах (1670)
#define stepPerRevoluteXY 800              //Для прохождения полного оборота XY
#define stepPerRevoluteZ 3200               //Для прохождения полного оборота Z
#define workPlaseX 15200                    //Рабочая область в ширину в шагах
#define workPlaseY 14500                    //Рабочая область в длину в шагах
#define offsetX 50                         //Отступ рабочей области от домашней точки в шагах
#define offsetY 50                         //Отступ рабочей области от домашней точки в шагах
#define startOffsetZ 70                     //Начальное значение Z для устранения отклонения от концевика

//---------------------Пины(лучше не трогать)------------------------
#define dirX_pin A1                         //Dir пин драйвера X
#define stepX_pin A0                        //Step пин драйвера X
#define enableX_pin 38                      //Enable пин драйвера X
#define dirY_pin A7                         //Dir пин драйвера Y
#define stepY_pin A6                        //Step пин драйвера Y
#define enableY_pin A2                      //Enable пин драйвера Y
#define dirY2_pin 28                        //Dir пин драйвера Y со второй стороны
#define stepY2_pin 26                       //Step пин драйвера Y со второй стороны
#define enableY2_pin 24                     //Enable пин драйвера Y со второй стороны
#define dirZ_pin 48                         //Dir пин драйвера Z
#define stepZ_pin 46                        //Step пин драйвера Z
#define enableZ_pin A8                      //Enable пин драйвера Z
#define paintMosfet 8                       //Hiting bed mosfet
#define airMosfet 9                         //Fun mosfet
#define mosfet3_pin 10                      //Hot end 0 mosfet
#define mosfet4_pin 7                       //Hot end 1 mosfet
#define Xswith 4                            //Концевик X
#define Yswith 5                            //Концевик Y
#define Zswith 6                            //Концевик Z
#define enc1 33                             //Пины энкодера
#define enc2 31                             //Пины энкодера
#define encBtn 35                           //Пин кнопки энкодера
#define beeper 37                           //Пин пищалки на экране
#define scr_en 23                           //Контакты экрана
#define scr_rw 17                           //Контакты экрана
#define scr_di 16                           //Контакты экрана
#define drawPeriod 60                       //Период обновления экрана
#define keypadRow1 A5                       //Контакты клаватуры
#define keypadRow2 A12                      //Контакты клаватуры
#define keypadRow3 44                       //Контакты клаватуры
#define keypadRow4 A10                      //Контакты клаватуры
#define keypadCol1 42                       //Контакты клаватуры
#define keypadCol2 40                       //Контакты клаватуры
#define keypadCol3 A11                      //Контакты клаватуры
#define ROWS 4                              //Кол-во сток клавиатуры
#define COLS 3                              //Кол-во столбцов клавиатуры
#define setupCount 6                        //Кол-во строк в настройках !!! + 1 !!!
#define changeTime 30000                    //Время слива краски в мс
#define autoOffTime 300000                  //Время автоотключения в мс
#define UpDownTurn 0                        //Служебное для поворота головы по Z
#define LeftRightTurn stepPerRevoluteZ / 4  //Служебное для поворота головы по Z