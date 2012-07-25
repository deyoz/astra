#include <stdlib.h>
#include "cent.h"
#include "basic.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "astra_misc.h"
#include "salons.h"
#include "salonform.h"
#include "develop_dbf.h"
#include "oralib.h"
#include "alarms.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace ASTRA;

const string ROGNOV_BALANCE_TYPE = "BR";


void getBalanceDBF( Develop_dbf &dbf, string dbf_file )
{
  if ( dbf_file.empty() ) {
    dbf.AddField( "NR", 'C', 8 ); //1 номер рейса 3 - Company + 5 flt_no + ??? suffix (ключ)
    dbf.AddField( "ID", 'N', 9 ); //2 идентификатор рейса (ключ)
    dbf.AddField( "FL_SPP", 'C', 1 ); //3 FL_SPP
    dbf.AddField( "FL_SPP_B", 'C', 1 ); //4 В исходных данных произведены изменения, не влияющие на результаты расчета центровки
    dbf.AddField( "FL_ACT", 'C', 1 ); //5 Резервное поле
    dbf.AddField( "FL_BAL", 'C', 1 ); //6 Со стороны системы "Центровка" в данных произведены изменения, предназначенные для передачи в ЛВС
    dbf.AddField( "FL_BAL_WRK", 'C', 1 ); //7 По рейсу произведен расчет и выпущена документация (СЗВ, LoadSheet, Схема Загрузки)
    dbf.AddField( "DOP", 'C', 1 ); //8 Метка "дополнительный"
    dbf.AddField( "TRANSIT", 'C', 1 ); //9 Метка "транзитный"
    dbf.AddField( "TYPE", 'C', 4 ); //10 Тип ВС
    dbf.AddField( "BORT", 'C', 9 ); //11 Номер борта
    dbf.AddField( "KR", 'N', 3 ); //12 Число пассажирских кресел
    dbf.AddField( "MODIF", 'C', 1 ); //13 Модификация
    dbf.AddField( "PLANDAT", 'D', 8 ); //14 Плановая дата вылета
    dbf.AddField( "FACTDAT", 'D', 8 ); //15 Фактическая дата вылета
    dbf.AddField( "TIME", 'C', 4 ); //16 Время вылета (ЧЧММ)
    dbf.AddField( "STAND", 'C', 5 ); //17 Номер стоянки
    dbf.AddField( "SEATLIM", 'N', 3 ); //18 Предел кресел
    dbf.AddField( "MAILLIM", 'N', 5 ); //19 Предел почты
    dbf.AddField( "CREW", 'N', 2 ); //20 Число членов экипажа
    dbf.AddField( "STEW", 'N', 2 ); //21 Число стюардесс
    dbf.AddField( "FEED", 'N', 3 ); //22 Масса продовольствия
    dbf.AddField( "FUEL_TO", 'N', 6 ); //23 Масса топлива на взлете
    dbf.AddField( "FUEL_USE", 'N', 6 ); //24 Масса топлива на полет
    dbf.AddField( "FUEL_TAXI", 'N', 6 ); //25 Масса топлива на руление
    dbf.AddField( "TOW_LIM", 'N', 6 ); //26 Масса допустимая взлетная
    dbf.AddField( "CAPT", 'C', 14 ); //27 Фамилия КВС
    dbf.AddField( "COMPANY", 'C', 3 ); //28 Авиакомпания
    dbf.AddField( "OWNER", 'C', 3 ); //29 Владелец ВС
    dbf.AddField( "NR_SM", 'N', 1 ); //30 Номер смены
    dbf.AddField( "WEEK", 'C', 7 ); //31 Линейка расписания
    dbf.AddField( "OPER_BASIC", 'N', 1 ); //32 Признак использования BW/BI
    dbf.AddField( "CREW_DOW", 'N', 2 ); //33 Число членов экипажа, вкл. в DOW
    dbf.AddField( "STEW_DOW", 'N', 2 ); //34 Число стюардесс, вкл. в DOW
    dbf.AddField( "FEED_DOW", 'N', 4 ); //35 Масса продовольствия, вкл. в DOW
    dbf.AddField( "CREW_XCR", 'N', 2 ); //36 Число сверх.экипажа на пасс.креслах
    dbf.AddField( "STEW_XCR", 'N', 2 ); //37 Число сверх.стюард. на пасс.креслах
    dbf.AddField( "PANTRY_C", 'C', 2 ); //38 Код PANTRY
    dbf.AddField( "CREW_C", 'C', 2 ); //39 Код CREW
    dbf.AddField( "CABIN_CONF", 'C', 3 ); //40 Конфигурация кабины
    
    dbf.AddField( "PORT1", 'C', 3 ); //41 Первый пункт посадки
    //ТРАНЗИТ ДО 1-ГО ПУНКТА
    dbf.AddField( "T_MAN1", 'N', 3 ); //42 Взрослые пассажиры / Мужчины
    dbf.AddField( "T_FAM1", 'N', 3 ); //43 Женщины
    dbf.AddField( "T_RB1", 'N', 3 ); //44 Ребята большие
    dbf.AddField( "T_RM1", 'N', 2 ); //45 Ребята малые
    dbf.AddField( "T_RK1", 'N', 4 ); //46 Ручная кладь
    dbf.AddField( "T_UNIT1", 'N', 3 ); //47 Единиц багажа
    dbf.AddField( "T_BAG1", 'N', 5 ); //48 Багаж
    dbf.AddField( "T_PBAG1", 'N', 4 ); //49 Платный багаж
    dbf.AddField( "T_CARGO1", 'N', 6 ); //50 Груз
    dbf.AddField( "T_MAIL1", 'N', 5 ); //51 Почта
    //ДОГРУЗКА ДО 1-ГО ПУНКТА
    dbf.AddField( "D_MAN1", 'N', 3 ); //52 Взрослые пассажиры / Мужчины
    dbf.AddField( "D_FAM1", 'N', 3 ); //53 Женщины
    dbf.AddField( "D_RB1", 'N', 3 ); //54 Ребята большие
    dbf.AddField( "D_RM1", 'N', 2 ); //55 Ребята малые
    dbf.AddField( "D_RK1", 'N', 4 ); //56 Ручная кладь
    dbf.AddField( "D_UNIT1", 'N', 3 ); //57 Единиц багажа
    dbf.AddField( "D_BAG1", 'N', 5 ); //58 Багаж
    dbf.AddField( "D_PBAG1", 'N', 4 ); //59 Платный багаж
    dbf.AddField( "D_CARGO1", 'N', 6 ); //60 Груз
    dbf.AddField( "D_MAIL1", 'N', 5 ); //61 Почта
    
    dbf.AddField( "PORT2", 'C', 3 ); //62 Второй пункт посадки
    //ТРАНЗИТ ДО 2-ГО ПУНКТА
    dbf.AddField( "T_MAN2", 'N', 3 ); //63 Взрослые пассажиры / Мужчины
    dbf.AddField( "T_FAM2", 'N', 3 ); //64 Женщины
    dbf.AddField( "T_RB2", 'N', 3 ); //65 Ребята большие
    dbf.AddField( "T_RM2", 'N', 2 ); //66 Ребята малые
    dbf.AddField( "T_RK2", 'N', 4 ); //67 Ручная кладь
    dbf.AddField( "T_UNIT2", 'N', 3 ); //68 Единиц багажа
    dbf.AddField( "T_BAG2", 'N', 5 ); //69 Багаж
    dbf.AddField( "T_PBAG2", 'N', 4 ); //70 Платный багаж
    dbf.AddField( "T_CARGO2", 'N', 6 ); //71 Груз
    dbf.AddField( "T_MAIL2", 'N', 5 ); //72 Почта
    //ДОГРУЗКА ДО 2-ГО ПУНКТА
    dbf.AddField( "D_MAN2", 'N', 3 ); //73 Взрослые пассажиры / Мужчины
    dbf.AddField( "D_FAM2", 'N', 3 ); //74 Женщины
    dbf.AddField( "D_RB2", 'N', 3 ); //75 Ребята большие
    dbf.AddField( "D_RM2", 'N', 2 ); //76 Ребята малые
    dbf.AddField( "D_RK2", 'N', 4 ); //77 Ручная кладь
    dbf.AddField( "D_UNIT2", 'N', 3 ); //78 Единиц багажа
    dbf.AddField( "D_BAG2", 'N', 5 ); //79 Багаж
    dbf.AddField( "D_PBAG2", 'N', 4 ); //80 Платный багаж
    dbf.AddField( "D_CARGO2", 'N', 6 ); //81 Груз
    dbf.AddField( "D_MAIL2", 'N', 5 ); //82 Почта
    
    dbf.AddField( "PORT3", 'C', 3 ); //83 Второй пункт посадки
    //ТРАНЗИТ ДО 3-ГО ПУНКТА
    dbf.AddField( "T_MAN3", 'N', 3 ); //84 Взрослые пассажиры / Мужчины
    dbf.AddField( "T_FAM3", 'N', 3 ); //85 Женщины
    dbf.AddField( "T_RB3", 'N', 3 ); //86 Ребята большие
    dbf.AddField( "T_RM3", 'N', 2 ); //87 Ребята малые
    dbf.AddField( "T_RK3", 'N', 4 ); //88 Ручная кладь
    dbf.AddField( "T_UNIT3", 'N', 3 ); //89 Единиц багажа
    dbf.AddField( "T_BAG3", 'N', 5 ); //90 Багаж
    dbf.AddField( "T_PBAG3", 'N', 4 ); //91 Платный багаж
    dbf.AddField( "T_CARGO3", 'N', 6 ); //92 Груз
    dbf.AddField( "T_MAIL3", 'N', 5 ); //93 Почта
    //ДОГРУЗКА ДО 3-ГО ПУНКТА
    dbf.AddField( "D_MAN3", 'N', 3 ); //94 Взрослые пассажиры / Мужчины
    dbf.AddField( "D_FAM3", 'N', 3 ); //95 Женщины
    dbf.AddField( "D_RB3", 'N', 3 ); //96 Ребята большие
    dbf.AddField( "D_RM3", 'N', 2 ); //97 Ребята малые
    dbf.AddField( "D_RK3", 'N', 4 ); //98 Ручная кладь
    dbf.AddField( "D_UNIT3", 'N', 3 ); //99 Единиц багажа
    dbf.AddField( "D_BAG3", 'N', 5 ); //100 Багаж
    dbf.AddField( "D_PBAG3", 'N', 4 ); //101 Платный багаж
    dbf.AddField( "D_CARGO3", 'N', 6 ); //102 Груз
    dbf.AddField( "D_MAIL3", 'N', 5 ); //103 Почта
    
    dbf.AddField( "PORT4", 'C', 3 ); //104 Второй пункт посадки
    //ТРАНЗИТ ДО 4-ГО ПУНКТА
    dbf.AddField( "T_MAN4", 'N', 3 ); //105 Взрослые пассажиры / Мужчины
    dbf.AddField( "T_FAM4", 'N', 3 ); //106 Женщины
    dbf.AddField( "T_RB4", 'N', 3 ); //107 Ребята большие
    dbf.AddField( "T_RM4", 'N', 2 ); //108 Ребята малые
    dbf.AddField( "T_RK4", 'N', 4 ); //109 Ручная кладь
    dbf.AddField( "T_UNIT4", 'N', 3 ); //110 Единиц багажа
    dbf.AddField( "T_BAG4", 'N', 5 ); //111 Багаж
    dbf.AddField( "T_PBAG4", 'N', 4 ); //112 Платный багаж
    dbf.AddField( "T_CARGO4", 'N', 6 ); //113 Груз
    dbf.AddField( "T_MAIL4", 'N', 5 ); //114 Почта
    //ДОГРУЗКА ДО 4-ГО ПУНКТА
    dbf.AddField( "D_MAN4", 'N', 3 ); //115 Взрослые пассажиры / Мужчины
    dbf.AddField( "D_FAM4", 'N', 3 ); //116 Женщины
    dbf.AddField( "D_RB4", 'N', 3 ); //117 Ребята большие
    dbf.AddField( "D_RM4", 'N', 2 ); //118 Ребята малые
    dbf.AddField( "D_RK4", 'N', 4 ); //119 Ручная кладь
    dbf.AddField( "D_UNIT4", 'N', 3 ); //120 Единиц багажа
    dbf.AddField( "D_BAG4", 'N', 5 ); //121 Багаж
    dbf.AddField( "D_PBAG4", 'N', 4 ); //122 Платный багаж
    dbf.AddField( "D_CARGO4", 'N', 6 ); //123 Груз
    dbf.AddField( "D_MAIL4", 'N', 5 ); //124 Почта
    
    //В ТОМ ЧИСЛЕ 1 КЛАСС
    dbf.AddField( "I_MAN", 'N', 3 ); //125 Взрослые пассажиры / Мужчины
    dbf.AddField( "I_FAM", 'N', 3 ); //126 Женщины
    dbf.AddField( "I_RB", 'N', 3 ); //127 Ребята большие
    dbf.AddField( "I_RM", 'N', 2 ); //128 Ребята малые
    dbf.AddField( "I_RK", 'N', 4 ); //129 Ручная кладь
    dbf.AddField( "I_UNIT", 'N', 3 ); //130 Единиц багажа
    dbf.AddField( "I_BAG", 'N', 5 ); //131 Багаж
    dbf.AddField( "I_PBAG", 'N', 4 ); //132 Платный багаж
    //В ТОМ ЧИСЛЕ БИЗНЕС-КЛАСС
    dbf.AddField( "B_MAN", 'N', 3 ); //133 Взрослые пассажиры / Мужчины
    dbf.AddField( "B_FAM", 'N', 3 ); //134 Женщины
    dbf.AddField( "B_RB", 'N', 3 ); //135 Ребята большие
    dbf.AddField( "B_RM", 'N', 2 ); //136 Ребята малые
    dbf.AddField( "B_RK", 'N', 4 ); //137 Ручная кладь
    dbf.AddField( "B_UNIT", 'N', 3 ); //138 Единиц багажа
    dbf.AddField( "B_BAG", 'N', 5 ); //139 Багаж
    dbf.AddField( "B_PBAG", 'N', 4 ); //140 Платный багаж
    //ЧИСЛО ЗАНЯТЫХ КРЕСЕЛ
    dbf.AddField( "K_MAN", 'N', 3 ); //141 Взрослые пассажиры
    dbf.AddField( "K_RB", 'N', 3 ); //142 Ребята большие
    dbf.AddField( "K_RM", 'N', 2 ); //143 Ребята малые
    //В ТОМ ЧИСЛЕ
    dbf.AddField( "PAD_F", 'N', 3 ); //144 Служебные пассажиры 1 кл.
    dbf.AddField( "PAD_C", 'N', 3 ); //145 Служебные пассажиры бзн.кл.
    dbf.AddField( "PAD_Y", 'N', 3 ); //146 Служебные пассажиры эк.кл.
    dbf.AddField( "XCR_F", 'N', 3 ); //147 Сверхнорм.экипаж и стюарды 1 кл.
    dbf.AddField( "XCR_C", 'N', 3 ); //148 Сверхнорм.экипаж и стюарды бзн.кл.
    dbf.AddField( "XCR_Y", 'N', 3 ); //149 Сверхнорм.экипаж и стюарды эк.кл.
    
    //ПАРАМЕТРЫ, ИСПОЛЬЗУЕМЫЕ ЦЕНТРОВКОЙ  (150-151)
    dbf.AddField( "DC_TOW", 'N', 3 ); //150 Оперативная МДВ
    dbf.AddField( "SERV", 'N', 3 ); //151 Используется опционально
    //ПАРАМЕТРЫ, ФОРМИРУЕМЫЕ В ПРОЦЕССЕ РАСЧЕТА ЦЕНТРОВКИ  (152-211)
    dbf.AddField( "LIM", 'N', 5 ); //152 Предельная коммерческая загрузка
    dbf.AddField( "SUMMA", 'N', 5 ); //153 Фактическая коммерческая загрузка
    dbf.AddField( "MAX_TOW", 'N', 6 ); //154 МДВ из БД ПАРК ВС
    //ИТОГО
    dbf.AddField( "PAS_VZR", 'N', 3 ); //155 Взрослых пассажиров
    dbf.AddField( "PAS_RB", 'N', 3 ); //156 Ребят больших
    dbf.AddField( "PAS_RM", 'N', 3 ); //157 Ребят малых
    dbf.AddField( "PAST", 'N', 3 ); //158 Транзитных пассажиров
    dbf.AddField( "ZAGR_K", 'N', 4 ); //159 Ручной клади
    dbf.AddField( "UNIT_VS", 'N', 4 ); //160 Единиц багажа
    dbf.AddField( "BAG_VS", 'N', 5 ); //161 Багажа
    dbf.AddField( "BAGPL", 'N', 5 ); //162 Платного багажа
    dbf.AddField( "GR_VS", 'N', 5 ); //163 Груза
    dbf.AddField( "GRT", 'N', 5 ); //164 Транзитного груза
    dbf.AddField( "POCT_VS", 'N', 5 ); //165 Почта
    dbf.AddField( "POCTT", 'N', 5 ); //167 Транзитной почты
    //РАЗМЕЩЕНИЕ ЗАГРУЗКИ ПО ГРУЗОБАГАЖНИКАМ
    dbf.AddField( "HOLD1", 'N', 5 ); //168 Загрузка г/б N  1
    dbf.AddField( "HOLD2", 'N', 5 ); //169 Загрузка г/б N  2
    dbf.AddField( "HOLD3", 'N', 5 ); //170 Загрузка г/б N  3
    dbf.AddField( "HOLD4", 'N', 5 ); //171 Загрузка г/б N  4
    dbf.AddField( "HOLD5", 'N', 5 ); //172 Загрузка г/б N  5
    dbf.AddField( "HOLD6", 'N', 5 ); //173 Загрузка г/б N  6
    dbf.AddField( "HOLD7", 'N', 5 ); //174 Загрузка г/б N  7
    dbf.AddField( "HOLD8", 'N', 5 ); //175 Загрузка г/б N  8
    dbf.AddField( "HOLD9", 'N', 5 ); //176 Загрузка г/б N  9
    dbf.AddField( "HOLD10", 'N', 5 ); //177 Загрузка г/б N  10
    dbf.AddField( "HOLD11", 'N', 5 ); //178 Загрузка г/б N  11
    dbf.AddField( "HOLD12", 'N', 5 ); //179 Загрузка г/б N  12
    dbf.AddField( "HOLD13", 'N', 5 ); //180 Загрузка г/б N  13
    dbf.AddField( "HOLD14", 'N', 5 ); //181 Загрузка г/б N  14
    dbf.AddField( "HOLD15", 'N', 5 ); //182 Загрузка г/б N  15
    dbf.AddField( "HOLD16", 'N', 5 ); //183 Загрузка г/б N  16
    //РАЗМЕЩЕНИЕ ЗАГРУЗКИ ПО СЕКЦИЯМ ВЕРХНЕЙ ПАЛУБЫ
    dbf.AddField( "HOLDUP1", 'N', 5 ); //184 Загрузка секции N  1
    dbf.AddField( "HOLDUP2", 'N', 5 ); //185 Загрузка секции N  2
    dbf.AddField( "HOLDUP3", 'N', 5 ); //186 Загрузка секции N  3
    dbf.AddField( "HOLDUP4", 'N', 5 ); //187 Загрузка секции N  4
    dbf.AddField( "HOLDUP5", 'N', 5 ); //188 Загрузка секции N  5
    dbf.AddField( "HOLDUP6", 'N', 5 ); //189 Загрузка секции N  6
    dbf.AddField( "HOLDUP7", 'N', 5 ); //190 Загрузка секции N  7
    dbf.AddField( "HOLDUP8", 'N', 5 ); //191 Загрузка секции N  8
    dbf.AddField( "HOLDUP9", 'N', 5 ); //192 Загрузка секции N  9
    dbf.AddField( "HOLDUP10", 'N', 5 ); //193 Загрузка секции N  10
    dbf.AddField( "HOLDUP11", 'N', 5 ); //194 Загрузка секции N  11
    dbf.AddField( "HOLDUP12", 'N', 5 ); //195 Загрузка секции N  12
    dbf.AddField( "HOLDUP13", 'N', 5 ); //196 Загрузка секции N  13
    dbf.AddField( "HOLDUP14", 'N', 5 ); //197 Загрузка секции N  14
    dbf.AddField( "HOLDUP15", 'N', 5 ); //198 Загрузка секции N  15
    dbf.AddField( "HOLDUP16", 'N', 5 ); //199 Загрузка секции N  16
    
    dbf.AddField( "PAS_ROW", 'C', 1 ); //200 * Метка "размещение пассажиров"
                                       //("C"- классы,  "Z"-зоны)
                                       //  При "зональном" размещении пасс.
                                       //используются поля с 212 по 231.
                                       //  При размещении "по классам"- как
                                       //показано ниже.
    //ПАССАЖИРЫ ЭКОНОМ-КЛАССА
    dbf.AddField( "PASS_1", 'N', 3 ); //201 1-й занятый ряд
    dbf.AddField( "PASS_2", 'N', 3 ); //202 Число пасс.в 1-м занятом ряду
    dbf.AddField( "PASS_3", 'N', 3 ); //203 Последний занятый ряд
    dbf.AddField( "PASS_4", 'N', 3 ); //204 Число пасс.в последнем занятом ряду
    //ПАССАЖИРЫ БИЗНЕС-КЛАССА
    dbf.AddField( "PASS_5", 'N', 3 ); //205 1-й занятый ряд
    dbf.AddField( "PASS_6", 'N', 3 ); //206 Число пасс.в 1-м занятом ряду
    dbf.AddField( "PASS_7", 'N', 3 ); //207 Последний занятый ряд
    dbf.AddField( "PASS_8", 'N', 3 ); //208 Число пасс.в последнем занятом ряду
    //ПАССАЖИРЫ ПЕРВОГО КЛАССА
    dbf.AddField( "PASS_9", 'N', 3 ); //209 1-й занятый ряд
    dbf.AddField( "PASS_10", 'N', 3 ); //210 Число пасс.в 1-м занятом ряду
    dbf.AddField( "PASS_11", 'N', 3 ); //211 Последний занятый ряд
    dbf.AddField( "PASS_12", 'N', 3 ); //212 Число пасс.в последнем занятом ряду
    //ПАССАЖИРЫ ПО ЗОНАМ
    dbf.AddField( "ZONENAME1", 'C', 3 ); //213 Наименование 1-й зоны
    dbf.AddField( "CAPACITY1", 'N', 3 ); //214 Кол-во кресел в 1-й зоне
    dbf.AddField( "OCCUPIED1", 'N', 3 ); //215 Кол-во занятых кресел в 1-й зоне для расчета
    dbf.AddField( "COUNTED1", 'N', 3 ); //216 Кол-во занятых кресел в 1-й зоне от Астры
    dbf.AddField( "ZONENAME2", 'C', 3 ); //217 Наименование 2-й зоны
    dbf.AddField( "CAPACITY2", 'N', 3 ); //218 Кол-во кресел в 2-й зоне
    dbf.AddField( "OCCUPIED2", 'N', 3 ); //219 Кол-во кресел в 2-й зоне
    dbf.AddField( "COUNTED2", 'N', 3 ); //220 Кол-во занятых кресел в 2-й зоне от Астры
    dbf.AddField( "ZONENAME3", 'C', 3 ); //221 Наименование 3-й зоны
    dbf.AddField( "CAPACITY3", 'N', 3 ); //222 Кол-во кресел в 3-й зоне
    dbf.AddField( "OCCUPIED3", 'N', 3 ); //223 Кол-во кресел в 3-й зоне
    dbf.AddField( "COUNTED3", 'N', 3 ); //224 Кол-во занятых кресел в 3-й зоне от Астры
    dbf.AddField( "ZONENAME4", 'C', 3 ); //225 Наименование 4-й зоны
    dbf.AddField( "CAPACITY4", 'N', 3 ); //226 Кол-во кресел в 4-й зоне
    dbf.AddField( "OCCUPIED4", 'N', 3 ); //227 Кол-во занятых кресел в 4-й зоне
    dbf.AddField( "COUNTED4", 'N', 3 ); //228 Кол-во занятых кресел в 4-й зоне от Астры
    dbf.AddField( "ZONENAME5", 'C', 3 ); //229 Наименование 5-й зоны
    dbf.AddField( "CAPACITY5", 'N', 3 ); //230 Кол-во кресел в 5-й зоне
    dbf.AddField( "OCCUPIED5", 'N', 3 ); //231 Кол-во занятых кресел в 5-й зоне
    dbf.AddField( "COUNTED5", 'N', 3 ); //232 Кол-во занятых кресел в 5-й зоне от Астры
    dbf.AddField( "ZONENAME6", 'C', 3 ); //233 Наименование 6-й зоны
    dbf.AddField( "CAPACITY6", 'N', 3 ); //234 Кол-во кресел в 6-й зоне
    dbf.AddField( "OCCUPIED6", 'N', 3 ); //235 Кол-во занятых кресел в 6-й зоне
    dbf.AddField( "COUNTED6", 'N', 3 ); //236 Кол-во занятых кресел в 6-й зоне от Астры
    dbf.AddField( "ZONENAME7", 'C', 3 ); //237 Наименование 7-й зоны
    dbf.AddField( "CAPACITY7", 'N', 3 ); //238 Кол-во кресел в 7-й зоне
    dbf.AddField( "OCCUPIED7", 'N', 3 ); //239 Кол-во занятых кресел в 7-й зоне
    dbf.AddField( "COUNTED7", 'N', 3 ); //240 Кол-во занятых кресел в 7-й зоне от Астры
    dbf.AddField( "ZONENAME8", 'C', 3 ); //241 Наименование 8-й зоны
    dbf.AddField( "CAPACITY8", 'N', 3 ); //242 Кол-во кресел в 8-й зоне
    dbf.AddField( "OCCUPIED8", 'N', 3 ); //243 Кол-во занятых кресел в 8-й зоне
    dbf.AddField( "COUNTED8", 'N', 3 ); //244 Кол-во занятых кресел в 3-й зоне от Астры
    dbf.AddField( "ZONENAME9", 'C', 3 ); //245 Наименование 9-й зоны
    dbf.AddField( "CAPACITY9", 'N', 3 ); //246 Кол-во кресел в 9-й зоне
    dbf.AddField( "OCCUPIED9", 'N', 3 ); //247 Кол-во занятых кресел в 9-й зоне
    dbf.AddField( "COUNTED9", 'N', 3 ); //248 Кол-во занятых кресел в 9-й зоне от Астры
    dbf.AddField( "ZONENAME10", 'C', 3 ); //249 Наименование 10-й зоны
    dbf.AddField( "CAPACITY10", 'N', 3 ); //250 Кол-во кресел в 10-й зоне
    dbf.AddField( "OCCUPIED10", 'N', 3 ); //251 Кол-во занятых кресел в 10-й зоне
    dbf.AddField( "COUNTED10", 'N', 3 ); //252 Кол-во занятых кресел в 10-й зоне от Астры
  }
  else {
    //dbf.Parse( dbf_file, "CP1251" ); //!!!убрать РОЖНОВ CP866
    dbf.Parse( dbf_file, "CP866" );
  }
}

void getBalanceTypePermit( vector<string> &airlines,
                           vector<string> &airps,
                           vector<int> &flt_nos )
{
  airlines.clear();
  airps.clear();
  flt_nos.clear();
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT airline,airp,flt_no FROM balance_sets "
    " WHERE balance_type=:balance_type AND pr_denial=0";
  Qry.CreateVariable( "balance_type", otString, ROGNOV_BALANCE_TYPE );
  Qry.Execute();
  bool pr_airline_empty = false;
  bool pr_airp_empty = false;
  bool pr_flt_no_empty = false;
  while ( !Qry.Eof ) {
    if ( !Qry.FieldIsNULL( "airline" ) )
      airlines.push_back( Qry.FieldAsString( "airline" ) );
    else
      pr_airline_empty = true;
    if ( !Qry.FieldIsNULL( "airp" ) )
      airps.push_back( Qry.FieldAsString( "airp" ) );
    else
      pr_airp_empty = true;
    if ( !Qry.FieldIsNULL( "flt_no" ) )
      flt_nos.push_back( Qry.FieldAsInteger( "flt_no" ) );
    else
      pr_flt_no_empty = true;
    Qry.Next();
  }
  if ( pr_airline_empty )
    airlines.clear();
  if ( pr_airp_empty )
    airps.clear();
  if ( pr_flt_no_empty )
    flt_nos.clear();
}

bool getBalanceFlightPermit( TQuery &FlightPermitQry,
                             int point_id,
                             const string &airline,
                             const string &airp,
                             int flt_no,
                             const string &bort,
                             const string &balance_type )
{
  FlightPermitQry.SetVariable( "airline", airline );
  FlightPermitQry.SetVariable( "airp", airp );
  FlightPermitQry.SetVariable( "flt_no", flt_no );
  FlightPermitQry.SetVariable( "bort", bort );
  FlightPermitQry.Execute();
  ProgTrace( TRACE5, "point_id=%d, FlightPermitQry.Eof=%d, incomming balance_type=%s",
             point_id, FlightPermitQry.Eof, balance_type.c_str() );
  return ( !FlightPermitQry.Eof &&
            balance_type == string(FlightPermitQry.FieldAsString( "balance_type" )) &&
            FlightPermitQry.FieldAsInteger( "pr_denial" ) == 0 );
}

string getConstraintRowValue( Develop_dbf &dbf, int irow )
{
  string res;
  res += dbf.GetFieldValue( irow, "COMPANY" );
  res += dbf.GetFieldValue( irow, "TYPE" );
  res += dbf.GetFieldValue( irow, "BORT" );
  res += dbf.GetFieldValue( irow, "CREW" );
  res += dbf.GetFieldValue( irow, "STEW" );
  res += dbf.GetFieldValue( irow, "K_MAN" );
  res += dbf.GetFieldValue( irow, "K_RB" );
  res += dbf.GetFieldValue( irow, "K_RM" );
  res += dbf.GetFieldValue( irow, "K_RM" );
  //1
  res += dbf.GetFieldValue( irow, "PORT1" );
  res += dbf.GetFieldValue( irow, "T_MAN1" );
  res += dbf.GetFieldValue( irow, "T_FAM1" );
  res += dbf.GetFieldValue( irow, "T_RB1" );
  res += dbf.GetFieldValue( irow, "T_RK1" );
  res += dbf.GetFieldValue( irow, "T_BAG1" );
  res += dbf.GetFieldValue( irow, "T_CARGO1" );
  res += dbf.GetFieldValue( irow, "T_MAIL1" );
  res += dbf.GetFieldValue( irow, "D_MAN1" );
  res += dbf.GetFieldValue( irow, "D_FAM1" );
  res += dbf.GetFieldValue( irow, "D_RB1" );
  res += dbf.GetFieldValue( irow, "D_RM1" );
  res += dbf.GetFieldValue( irow, "D_RK1" );
  res += dbf.GetFieldValue( irow, "D_BAG1" );
  res += dbf.GetFieldValue( irow, "D_CARGO1" );
  res += dbf.GetFieldValue( irow, "D_MAIL1" );
  //2
  res += dbf.GetFieldValue( irow, "PORT2" );
  res += dbf.GetFieldValue( irow, "T_MAN2" );
  res += dbf.GetFieldValue( irow, "T_FAM2" );
  res += dbf.GetFieldValue( irow, "T_RB2" );
  res += dbf.GetFieldValue( irow, "T_RK2" );
  res += dbf.GetFieldValue( irow, "T_BAG2" );
  res += dbf.GetFieldValue( irow, "T_CARGO2" );
  res += dbf.GetFieldValue( irow, "T_MAIL2" );
  res += dbf.GetFieldValue( irow, "D_MAN2" );
  res += dbf.GetFieldValue( irow, "D_FAM2" );
  res += dbf.GetFieldValue( irow, "D_RB2" );
  res += dbf.GetFieldValue( irow, "D_RM2" );
  res += dbf.GetFieldValue( irow, "D_RK2" );
  res += dbf.GetFieldValue( irow, "D_BAG2" );
  res += dbf.GetFieldValue( irow, "D_CARGO2" );
  res += dbf.GetFieldValue( irow, "D_MAIL2" );
  //3
  res += dbf.GetFieldValue( irow, "PORT3" );
  res += dbf.GetFieldValue( irow, "T_MAN3" );
  res += dbf.GetFieldValue( irow, "T_FAM3" );
  res += dbf.GetFieldValue( irow, "T_RB3" );
  res += dbf.GetFieldValue( irow, "T_RK3" );
  res += dbf.GetFieldValue( irow, "T_BAG3" );
  res += dbf.GetFieldValue( irow, "T_CARGO3" );
  res += dbf.GetFieldValue( irow, "T_MAIL3" );
  res += dbf.GetFieldValue( irow, "D_MAN3" );
  res += dbf.GetFieldValue( irow, "D_FAM3" );
  res += dbf.GetFieldValue( irow, "D_RB3" );
  res += dbf.GetFieldValue( irow, "D_RM3" );
  res += dbf.GetFieldValue( irow, "D_RK3" );
  res += dbf.GetFieldValue( irow, "D_BAG3" );
  res += dbf.GetFieldValue( irow, "D_CARGO3" );
  res += dbf.GetFieldValue( irow, "D_MAIL3" );
  //4
  res += dbf.GetFieldValue( irow, "PORT4" );
  res += dbf.GetFieldValue( irow, "T_MAN4" );
  res += dbf.GetFieldValue( irow, "T_FAM4" );
  res += dbf.GetFieldValue( irow, "T_RB4" );
  res += dbf.GetFieldValue( irow, "T_RK4" );
  res += dbf.GetFieldValue( irow, "T_BAG4" );
  res += dbf.GetFieldValue( irow, "T_CARGO4" );
  res += dbf.GetFieldValue( irow, "T_MAIL4" );
  res += dbf.GetFieldValue( irow, "D_MAN4" );
  res += dbf.GetFieldValue( irow, "D_FAM4" );
  res += dbf.GetFieldValue( irow, "D_RB4" );
  res += dbf.GetFieldValue( irow, "D_RM4" );
  res += dbf.GetFieldValue( irow, "D_RK4" );
  res += dbf.GetFieldValue( irow, "D_BAG4" );
  res += dbf.GetFieldValue( irow, "D_CARGO4" );
  res += dbf.GetFieldValue( irow, "D_MAIL4" );
  res += dbf.GetFieldValue( irow, "ZONENAME1" );
  res += dbf.GetFieldValue( irow, "CAPACITY1" );
  res += dbf.GetFieldValue( irow, "OCCUPIED1" );
  res += dbf.GetFieldValue( irow, "ZONENAME2" );
  res += dbf.GetFieldValue( irow, "CAPACITY2" );
  res += dbf.GetFieldValue( irow, "OCCUPIED2" );
  res += dbf.GetFieldValue( irow, "ZONENAME3" );
  res += dbf.GetFieldValue( irow, "CAPACITY3" );
  res += dbf.GetFieldValue( irow, "OCCUPIED3" );
  res += dbf.GetFieldValue( irow, "ZONENAME4" );
  res += dbf.GetFieldValue( irow, "CAPACITY4" );
  res += dbf.GetFieldValue( irow, "OCCUPIED4" );
  res += dbf.GetFieldValue( irow, "ZONENAME5" );
  res += dbf.GetFieldValue( irow, "CAPACITY5" );
  res += dbf.GetFieldValue( irow, "OCCUPIED5" );
  res += dbf.GetFieldValue( irow, "ZONENAME6" );
  res += dbf.GetFieldValue( irow, "CAPACITY6" );
  res += dbf.GetFieldValue( irow, "OCCUPIED6" );
  res += dbf.GetFieldValue( irow, "ZONENAME7" );
  res += dbf.GetFieldValue( irow, "CAPACITY7" );
  res += dbf.GetFieldValue( irow, "OCCUPIED7" );
  res += dbf.GetFieldValue( irow, "ZONENAME8" );
  res += dbf.GetFieldValue( irow, "CAPACITY8" );
  res += dbf.GetFieldValue( irow, "OCCUPIED8" );
  res += dbf.GetFieldValue( irow, "ZONENAME9" );
  res += dbf.GetFieldValue( irow, "CAPACITY9" );
  res += dbf.GetFieldValue( irow, "OCCUPIED9" );
  res += dbf.GetFieldValue( irow, "ZONENAME10" );
  res += dbf.GetFieldValue( irow, "CAPACITY10" );
  res += dbf.GetFieldValue( irow, "OCCUPIED10" );
  return res;
}

inline void setBalanceValue( Develop_dbf &dbf, int irow, const string &strnum, TBalance &bal, const string &type )
{
  dbf.SetFieldValue( irow, type + "_MAN" + strnum, IntToString( bal.male ) );
  dbf.SetFieldValue( irow, type + "_FAM" + strnum, IntToString( bal.female ) );
  dbf.SetFieldValue( irow, type + "_RB" + strnum, IntToString( bal.chd ) );
  dbf.SetFieldValue( irow, type + "_RM" + strnum, IntToString( bal.inf ) );
  dbf.SetFieldValue( irow, type + "_RK" + strnum, IntToString( bal.rk_weight ) );
  dbf.SetFieldValue( irow, type + "_UNIT" + strnum, IntToString( bal.bag_amount ) );
  dbf.SetFieldValue( irow, type + "_BAG" + strnum, IntToString( bal.bag_weight ) );
  dbf.SetFieldValue( irow, type + "_PBAG" + strnum, IntToString( bal.paybag_weight ) );
  if ( !strnum.empty() ) {
    dbf.SetFieldValue( irow, type + "_CARGO" + strnum, IntToString( bal.cargo ) );
    dbf.SetFieldValue( irow, type + "_MAIL" + strnum, IntToString( bal.mail ) );
  }
}

void exportDBF( int external_point_id, const string &dbf_file )
{
  Develop_dbf dbf;
  ProgTrace( TRACE5, "dbf_file.size()=%d", dbf_file.size() );
  getBalanceDBF( dbf, dbf_file );
}

void TBalanceData::getPassBalance( bool pr_tranzit_pass, int point_id, const TTripRoute &routesB, const TTripRoute &routesA, bool isLimitDest4 )
{
  balances.clear();
  unsigned int maxdestnum;
  if ( isLimitDest4 )
    maxdestnum = 4;
  else
    maxdestnum = routesA.size();
    
  TQuery *PassQry, *BagQry, *ExcessBagQry;
  
  ProgTrace( TRACE5, "pr_tranzit_pass=%d", pr_tranzit_pass );
  if ( pr_tranzit_pass ) {
    PassQry = qPassQry;
    BagQry = qBagQry;
    ExcessBagQry = qExcessBagQry;
  }
  else {
    PassQry = qPassTQry;
    BagQry = qBagTQry;
    ExcessBagQry = qExcessBagTQry;
  }

  int male;
  int female;
  int chd;
  int inf;
  int seats;
  string strclass;
    
  for ( unsigned int num=1; num<=maxdestnum; num++ ) {
    TDestBalance dest_bal;
    dest_bal.num = num;
    if ( num <= routesA.size() ) {
      dest_bal.point_id = routesA[ num - 1 ].point_id;
      dest_bal.airp = routesA[ num - 1 ].airp;
      if ( dataFlags.isFlag( tdPass ) ) {
        PassQry->SetVariable( "point_dep", point_id );
        PassQry->SetVariable( "point_arv", routesA[ num - 1 ].point_id );
        ProgTrace( TRACE5, "point_dep=%d, point_arv=%d", point_id, routesA[ num - 1 ].point_id );
        PassQry->Execute();
        tst();
        int idx_pax_id = PassQry->FieldIndex( "pax_id" );
        int idx_grp_id = PassQry->FieldIndex( "grp_id" );
        int idx_parent_pax_id = PassQry->FieldIndex( "parent_pax_id" );
        int idx_reg_no = PassQry->FieldIndex( "reg_no" );
        int idx_point_dep = PassQry->FieldIndex( "point_dep" );
        int idx_seats = PassQry->FieldIndex( "seats" );
        int idx_pers_type = PassQry->FieldIndex( "pers_type" );
        int idx_surname = PassQry->FieldIndex( "surname" );
        int idx_gender = PassQry->FieldIndex( "gender" );
        int idx_class = PassQry->FieldIndex( "class" );
        int idx_pr_tranzit = PassQry->FieldIndex( "pr_tranzit" );
        for ( ;!PassQry->Eof; PassQry->Next() ) {
          int point_dep = PassQry->FieldAsInteger( idx_point_dep );
          TTripRoute::const_iterator i = routesB.begin();
          for ( ; i!=routesB.end(); i++ ) {
            if ( point_dep == i->point_id )
              break;
          }
          if ( i == routesB.end() ) // это не транзит через наш пункт, это регистрация пассажиров за нашим пунктом
            continue;
          TBalance bal;
          std::map<std::string,TBalance> classbal;
          if ( PassQry->FieldAsInteger( idx_pr_tranzit ) == 0 ) {
            bal = dest_bal.goshow;
            classbal = dest_bal.goshow_classbal;
          }
          else {
            bal = dest_bal.tranzit;
            classbal = dest_bal.tranzit_classbal;
          }
          TPassenger p;
          p.pax_id = PassQry->FieldAsInteger( idx_pax_id );
          p.grp_id = PassQry->FieldAsInteger( idx_grp_id );
          p.reg_no = PassQry->FieldAsInteger( idx_reg_no );
          if ( !PassQry->FieldIsNULL( idx_parent_pax_id ) ) {
            p.parent_pax_id = PassQry->FieldAsInteger( idx_parent_pax_id );
          }
          p.point_dep = PassQry->FieldAsInteger( idx_point_dep );
          p.point_arv = routesA[ num - 1 ].point_id;
          p.pers_type = PassQry->FieldAsString( idx_pers_type );
          p.surname = PassQry->FieldAsString( idx_surname );
          seats = PassQry->FieldAsInteger( idx_seats );
          p.seats = seats;
          p.gender = PassQry->FieldAsString( idx_gender );
          passengers[ p.pax_id ] = p;
          strclass = PassQry->FieldAsString( idx_class );
          male = ( p.pers_type == "ВЗ" )&&(( p.gender.empty() || (p.gender.substr( 0, 1 ) == "M")  ));
          female = ( p.pers_type == "ВЗ" )&&( !p.gender.empty() && (p.gender.substr( 0, 1 ) == "F") );
          chd = ( p.pers_type == "РБ" );
          inf = ( p.pers_type == "РМ" );
          ProgTrace( TRACE5, "pax_id=%d, gender=%s, male=%d, female=%d, chd=%d, inf=%d",
                     p.pax_id, p.gender.c_str(), male, female, chd, inf );
          bal.male += male;
          bal.male_seats += seats*male;
          dest_bal.total_classbal[ strclass ].male +=male;
          dest_bal.total_classbal[ strclass ].male_seats += seats*male;
          classbal[ strclass ].male += male;
          classbal[ strclass ].male_seats += seats*male;
          bal.female += female;
          bal.female_seats += seats*female;
          dest_bal.total_classbal[ strclass ].female += female;
          dest_bal.total_classbal[ strclass ].female_seats += seats*female;
          classbal[ strclass ].female += female;
          classbal[ strclass ].female_seats += seats*female;
          bal.chd += chd;
          bal.chd_seats += seats*chd;
          dest_bal.total_classbal[ strclass ].chd += chd;
          dest_bal.total_classbal[ strclass ].chd_seats += seats*chd;
          classbal[ strclass ].chd += chd;
          classbal[ strclass ].chd_seats += seats*chd;
          bal.inf += inf;
          bal.inf_seats += seats*inf;
          dest_bal.total_classbal[ strclass ].inf += inf;
          dest_bal.total_classbal[ strclass ].inf_seats += seats*inf;
          classbal[ strclass ].inf += inf;
          classbal[ strclass ].inf_seats += seats*inf;
          if ( PassQry->FieldAsInteger( idx_pr_tranzit ) == 0 ) {
            dest_bal.goshow = bal;
            dest_bal.goshow_classbal = classbal;
          }
          else {
            dest_bal.tranzit = bal;
            dest_bal.tranzit_classbal = classbal;
          }
        }
      }
      if ( dataFlags.isFlag( tdBag ) ) {
        BagQry->SetVariable( "point_dep", point_id );
        BagQry->SetVariable( "point_arv", routesA[ num - 1 ].point_id );
        tst();
        BagQry->Execute();
        tst();
        for ( ;!BagQry->Eof; BagQry->Next() ) {
          int point_dep = BagQry->FieldAsInteger( "point_dep" );
          TTripRoute::const_iterator i = routesB.begin();
          for ( ; i!=routesB.end(); i++ ) {
            if ( point_dep == i->point_id )
              break;
          }
          if ( i == routesB.end() ) // это не транзит через наш пункт, это регистрация пассажиров за нашим пунктом
            continue;
          TBalance bal;
          std::map<std::string,TBalance> classbal;
          if ( BagQry->FieldAsInteger( "pr_tranzit" ) == 0 ) {
            bal = dest_bal.goshow;
            classbal = dest_bal.goshow_classbal;
          }
          else {
            bal = dest_bal.tranzit;
            classbal = dest_bal.tranzit_classbal;
          }
          bal.rk_weight += BagQry->FieldAsInteger( "rk_weight" );
          dest_bal.total_classbal[ BagQry->FieldAsString( "class" ) ].rk_weight += BagQry->FieldAsInteger( "rk_weight" );
          classbal[ BagQry->FieldAsString( "class" ) ].rk_weight += BagQry->FieldAsInteger( "rk_weight" );
          bal.bag_amount += BagQry->FieldAsInteger( "bag_amount" );
          dest_bal.total_classbal[ BagQry->FieldAsString( "class" ) ].bag_amount += BagQry->FieldAsInteger( "bag_amount" );
          classbal[ BagQry->FieldAsString( "class" ) ].bag_amount += BagQry->FieldAsInteger( "bag_amount" );
          bal.bag_weight += BagQry->FieldAsInteger( "bag_weight" );
          dest_bal.total_classbal[ BagQry->FieldAsString( "class" ) ].bag_weight += BagQry->FieldAsInteger( "bag_weight" );
          classbal[ BagQry->FieldAsString( "class" ) ].bag_weight += BagQry->FieldAsInteger( "bag_weight" );
          if ( BagQry->FieldAsInteger( "pr_tranzit" ) == 0 ) {
            dest_bal.goshow = bal;
            dest_bal.goshow_classbal = classbal;
          }
          else {
            dest_bal.tranzit = bal;
            dest_bal.tranzit_classbal = classbal;
          }
          ProgTrace( TRACE5, "class=%s, point_arv=%d, rk_weight=%d, bag_amount=%d, bag_weight=%d",
                     BagQry->FieldAsString( "class" ),
                     routesA[ num - 1 ].point_id,
                     BagQry->FieldAsInteger( "rk_weight" ),
                     BagQry->FieldAsInteger( "bag_amount" ),
                     BagQry->FieldAsInteger( "bag_weight" ) );
        }
      }
      if ( dataFlags.isFlag( tdExcess ) ) {
        ExcessBagQry->SetVariable( "point_dep", point_id );
        ExcessBagQry->SetVariable( "point_arv", routesA[ num - 1 ].point_id );
        tst();
        ExcessBagQry->Execute();
        tst();
        for ( ;!ExcessBagQry->Eof; ExcessBagQry->Next() ) {
          int point_dep = ExcessBagQry->FieldAsInteger( "point_dep" );
          TTripRoute::const_iterator i = routesB.begin();
          for ( ; i!=routesB.end(); i++ ) {
            if ( point_dep == i->point_id )
              break;
          }
          if ( i == routesB.end() ) // это не транзит через наш пункт, это регистрация пассажиров за нашим пунктом
            continue;
          TBalance bal;
          std::map<std::string,TBalance> classbal;
          if ( ExcessBagQry->FieldAsInteger( "pr_tranzit" ) == 0 ) {
            bal = dest_bal.goshow;
            classbal = dest_bal.goshow_classbal;
          }
          else {
            bal = dest_bal.tranzit;
            classbal = dest_bal.tranzit_classbal;
          }
          bal.paybag_weight += ExcessBagQry->FieldAsInteger( "paybag_weight" );
          dest_bal.total_classbal[ ExcessBagQry->FieldAsString( "class" ) ].paybag_weight += ExcessBagQry->FieldAsInteger( "paybag_weight" );
          classbal[ ExcessBagQry->FieldAsString( "class" ) ].paybag_weight += ExcessBagQry->FieldAsInteger( "paybag_weight" );
          if ( ExcessBagQry->FieldAsInteger( "pr_tranzit" ) == 0 ) {
            dest_bal.goshow = bal;
            dest_bal.goshow_classbal = classbal;
          }
          else {
            dest_bal.tranzit = bal;
            dest_bal.tranzit_classbal = classbal;
          }
          ProgTrace( TRACE5, "class=%s, point_arv=%d, paybag_weight=%d",
                     ExcessBagQry->FieldAsString( "class" ),
                     routesA[ num - 1 ].point_id,
                     ExcessBagQry->FieldAsInteger( "paybag_weight" ) );
        }
      }
      if ( dataFlags.isFlag( tdCargo ) ) {
        qCargoQry->SetVariable( "point_dep", point_id );
        qCargoQry->SetVariable( "point_arv", routesA[ num - 1 ].point_id );
        tst();
        qCargoQry->Execute();
        tst();
        for ( ;!qCargoQry->Eof; qCargoQry->Next() ) {
          int point_dep = qCargoQry->FieldAsInteger( "point_dep" );
          TTripRoute::const_iterator i = routesB.begin();
          for ( ; i!=routesB.end(); i++ ) {
            if ( point_dep == i->point_id )
              break;
          }
          if ( i == routesB.end() ) // это не транзит через наш пункт, это регистрация пассажиров за нашим пунктом
            continue;
          TBalance bal;
          if ( qCargoQry->FieldAsInteger( "pr_tranzit" ) == 0 )
            bal = dest_bal.goshow;
          else
            bal = dest_bal.tranzit;
          bal.cargo += qCargoQry->FieldAsInteger( "cargo" );
          bal.mail += qCargoQry->FieldAsInteger( "mail" );
          if ( qCargoQry->FieldAsInteger( "pr_tranzit" ) == 0 )
            dest_bal.goshow = bal;
          else
            dest_bal.tranzit = bal;
        }
      }
      if ( dataFlags.isFlag( tdPad ) ) {
        qPADQry->SetVariable( "point_dep", point_id );
        qPADQry->SetVariable( "point_arv", routesA[ num - 1 ].point_id );
        tst();
        qPADQry->Execute();
        tst();
        for ( ;!qPADQry->Eof; qPADQry->Next() ) {
          int point_dep = qPADQry->FieldAsInteger( "point_dep" );
          TTripRoute::const_iterator i = routesB.begin();
          for ( ; i!=routesB.end(); i++ ) {
            if ( point_dep == i->point_id )
              break;
          }
          if ( i == routesB.end() ) // это не транзит через наш пункт, это регистрация пассажиров за нашим пунктом
            continue;
          TBalance bal;
          dest_bal.total_classbal[ qPADQry->FieldAsString( "class" ) ].pad_seats += qPADQry->FieldAsInteger( "seats" );
          dest_bal.total_classbal[ qPADQry->FieldAsString( "class" ) ].pad++;
          if ( passengers.find( qPADQry->FieldAsInteger( "pax_id" ) ) == passengers.end() )
            throw EXCEPTIONS::Exception( "passengers not found, pax_id=%d", qPADQry->FieldAsInteger( "pax_id" ) );
          passengers[ qPADQry->FieldAsInteger( "pax_id" ) ].pr_pad = true;
        }
      }
    }  //end for num
    balances.push_back( dest_bal );
  }
}

void TBalanceData::get( int point_id, int pr_tranzit,
                        const TTripRoute &routesB, const TTripRoute &routesA,
                        bool isLimitDest4 )
{
  bool pr_tranzit_pass = ( pr_tranzit != 0 );
  if  ( pr_tranzit_pass ) {
    qTripSetsQry->SetVariable( "point_id", point_id );
    qTripSetsQry->Execute();
    pr_tranzit_pass = ( qTripSetsQry->Eof || qTripSetsQry->FieldAsInteger( "pr_tranz_reg" ) == 0 ); //нет перерегистрации транзита
  }
  getPassBalance( pr_tranzit_pass, point_id, routesB, routesA, isLimitDest4 );
}

void importDBF( int external_point_id, string &dbf_file )
{
  Develop_dbf dbf;
  ProgTrace( TRACE5, "dbf_file.size()=%d", dbf_file.size() );
  getBalanceDBF( dbf, dbf_file );
  vector<string> airlines;
  vector<string> airps;
  vector<int> flt_nos;
  getBalanceTypePermit( airlines, airps, flt_nos );
  TQuery FlightPermitQry( &OraSession );
  FlightPermitQry.SQLText =
    "SELECT balance_type, "
    "       DECODE( airp, NULL, 0, 8 ) + "
    "       DECODE( bort, NULL, 0, 4 ) + "
    "       DECODE( airline, NULL, 0, 2 ) + "
    "       DECODE( flt_no, NULL, 0, 1 ) AS priority, "
    "       pr_denial "
    " FROM balance_sets "
    " WHERE airp=:airp AND "
	  "       ( bort IS NULL OR bort=:bort ) AND "
	  "       ( airline IS NULL OR airline=:airline ) AND "
	  "       ( flt_no IS NULL OR flt_no=:flt_no ) "
	  " ORDER BY priority DESC";
  FlightPermitQry.DeclareVariable( "airp", otString );
  FlightPermitQry.DeclareVariable( "bort", otString );
  FlightPermitQry.DeclareVariable( "airline", otString );
  FlightPermitQry.DeclareVariable( "flt_no", otInteger );
  //начитываем данные по СПП для центровки
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "SELECT system.UTCSYSDATE currdate FROM dual";
  Qry.Execute();
  tst();
  TDateTime currdate = Qry.FieldAsDateTime( "currdate" );
  Qry.Clear();
  string sql;
  sql =
    "SELECT point_id,point_num,first_point,pr_tranzit,airline,flt_no,suffix,"
    "       craft,bort,airp,trip_type,SUBSTR( park_out, 1, 5 ) park_out,"
    "       scd_out,est_out,act_out,time_out FROM points "
    " WHERE time_out in (:day1,:day2) AND pr_del=0 AND pr_reg=1 ";
  if ( external_point_id != NoExists )
    sql += " AND point_id=:point_id ";
  if ( !airps.empty() ) {
    sql += " AND airp IN (";
    int idx=0;
    for ( vector<string>::const_iterator istr=airps.begin(); istr!=airps.end(); istr++ ) {
      if ( istr != airps.begin() )
        sql += ",";
      sql += ":airp"+IntToString(idx);
      Qry.CreateVariable( "airp"+IntToString(idx), otString, *istr );
      idx++;
    }
    sql += ")";
  }
  if ( !airlines.empty() ) {
    sql += " AND airline IN (";
    int idx=0;
    for ( vector<string>::const_iterator istr=airlines.begin(); istr!=airlines.end(); istr++ ) {
      if ( istr != airlines.begin() )
        sql += ",";
      sql += ":airline"+IntToString(idx);
      Qry.CreateVariable( "airline"+IntToString(idx), otString, *istr );
      idx++;
    }
    sql += ")";
  }
  if ( !flt_nos.empty() ) {
    sql += " AND flt_no IN (";
    int idx=0;
    for ( vector<int>::const_iterator iflt_no=flt_nos.begin(); iflt_no!=flt_nos.end(); iflt_no++ ) {
      if ( iflt_no != flt_nos.begin() )
        sql += ",";
      sql += ":flt_no"+IntToString(idx);
      Qry.CreateVariable( "flt_no"+IntToString(idx), otInteger, *iflt_no );
      idx++;
    }
    sql += ")";
  }
  Qry.SQLText = sql;
  TDateTime vdate;
  modf( currdate, &vdate );
  Qry.CreateVariable( "day1", otDate, vdate );
  Qry.CreateVariable( "day2", otDate, vdate + 1 );
  if ( external_point_id != NoExists )
    Qry.CreateVariable( "point_id", otInteger, external_point_id );
  Qry.Execute();
  TTripRoute routesB, routesA;
  //экипаж
  TQuery CrewsQry( &OraSession );
  CrewsQry.SQLText =
    "SELECT commander,cockpit,cabin FROM trip_crew WHERE point_id=:point_id";
  CrewsQry.DeclareVariable( "point_id", otInteger );

  string value;
  string FLAG_SET = "*";
  string prior_constraint_balance_value;


  map<string,bool> points;
  string strexternal_point_id;
  if ( external_point_id != NoExists )
    strexternal_point_id = IntToString( external_point_id );
  for ( int irow = 0; irow<dbf.getRowCount(); irow++ ) {
    if ( points.find( dbf.GetFieldValue( irow, "ID" ) ) != points.end() ) {
      tst();
      continue;
    }
    ProgTrace( TRACE5, "insert into points(%s,%d)", dbf.GetFieldValue( irow, "ID" ).c_str(), external_point_id != NoExists && strexternal_point_id != dbf.GetFieldValue( irow, "ID" ) );
    points.insert( make_pair( dbf.GetFieldValue( irow, "ID" ), external_point_id != NoExists && strexternal_point_id != dbf.GetFieldValue( irow, "ID" ) ) );
  }

  for ( ;!Qry.Eof; Qry.Next() ) {
    int point_id = Qry.FieldAsInteger( "point_id" );
    if ( fabs( Qry.FieldAsDateTime( "time_out" ) - currdate ) > 1.0 )
      continue;
    if ( !getBalanceFlightPermit( FlightPermitQry,
                                  Qry.FieldAsInteger( "point_id" ),
                                  Qry.FieldAsString( "airline" ),
                                  Qry.FieldAsString( "airp" ),
                                  Qry.FieldAsInteger( "flt_no" ),
                                  Qry.FieldAsString( "bort" ),
                                  ROGNOV_BALANCE_TYPE ) )
      continue;
    prior_constraint_balance_value.clear();
    routesA.GetRouteAfter( NoExists,
                           point_id,
                           Qry.FieldAsInteger( "point_num" ),
                           Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point"),
                           Qry.FieldAsInteger( "pr_tranzit" ),
                           trtNotCurrent,
                           trtNotCancelled );
    ProgTrace( TRACE5, "point_id=%d, routes.size()=%d", point_id, routesA.size() );
    if ( routesA.empty() ) {
      continue;
    }
    routesB.GetRouteBefore( NoExists,
                            point_id,
                            Qry.FieldAsInteger( "point_num" ),
                            Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point"),
                            Qry.FieldAsInteger( "pr_tranzit" ),
                            trtWithCurrent,
                            trtNotCancelled );
    int irow = 0;
    for ( ; irow<dbf.getRowCount(); irow++ ) {
      if ( dbf.GetFieldValue( irow, "ID" ) == IntToString( point_id ) ) {
        points[ IntToString( point_id ) ] = true;
        tst();
        break;
      }
    }
    bool pr_insert = ( irow == dbf.getRowCount() );
    if ( pr_insert ) {
      dbf.NewRow();
      ProgTrace( TRACE5, "NewRow, irow=%d, dbf.getRowCount()=%d, point_id=%d", irow, dbf.getRowCount(), point_id );
    }
    prior_constraint_balance_value = getConstraintRowValue( dbf, irow );

    try {
      vector<SALONS2::TCompSectionLayers> CompSectionsLayers;
      vector<TZoneOccupiedSeats> zones;
      ZoneLoads( point_id, true, true, true, zones, CompSectionsLayers );
      if ( CompSectionsLayers.empty() ) { //нет информации по зонам
        ProgTrace( TRACE5, "CompSectionsLayers.empty(), point_id=%d", point_id );
        throw Exception( "CompSectionsLayers is empty" );
      }
      string bort = Qry.FieldAsString( "bort" );
      if ( !( bort.size() == 5 && bort.find( "-" ) == string::npos || bort.size() == 6 && bort.find( "-" ) == 2 ) ) {
        ProgTrace( TRACE5, "point_id=%d,bort=%s, bort.find=%d", point_id, bort.c_str(), bort.find( "-" ) );
        throw Exception( "Invalid bort" );
      }

      //надо будет заполнить на основе данных
      dbf.SetFieldValue( irow, "ID", IntToString( point_id ) );
      value = string(Qry.FieldAsString( "airline" )) + Qry.FieldAsString( "flt_no" ) + Qry.FieldAsString( "suffix" );
      dbf.SetFieldValue( irow, "NR", value );
      dbf.SetFieldValue( irow, "TRANSIT", Qry.FieldAsInteger( "pr_tranzit" )==0?string(" "):FLAG_SET );
      dbf.SetFieldValue( irow, "TYPE", Qry.FieldAsString( "craft" ) );
      dbf.SetFieldValue( irow, "BORT", Qry.FieldAsString( "bort" ) );
      tst();
      string region = AirpTZRegion( Qry.FieldAsString( "airp" ) );
      TDateTime scd_out =  UTCToLocal( Qry.FieldAsDateTime( "scd_out" ), region );
      dbf.SetFieldValue( irow, "PLANDAT", DateTimeToStr( scd_out, "yyyymmdd" ) );
      if ( Qry.FieldIsNULL( "est_out" ) )
        dbf.SetFieldValue( irow, "TIME", DateTimeToStr( scd_out, "hhnn" ) );
      else {
        scd_out =  UTCToLocal( Qry.FieldAsDateTime( "est_out" ), region );
        dbf.SetFieldValue( irow, "TIME", DateTimeToStr( scd_out, "hhnn" ) );
      }
      tst();
      dbf.SetFieldValue( irow, "STAND", Qry.FieldAsString( "park_out" ) );
      CrewsQry.SetVariable( "point_id", point_id );
      CrewsQry.Execute();
      if ( !CrewsQry.Eof ) {
        if ( !CrewsQry.FieldIsNULL( "cockpit" ) ) {
          dbf.SetFieldValue( irow, "CREW", CrewsQry.FieldAsString( "cockpit" ) );
        }
        if ( !CrewsQry.FieldIsNULL( "cockpit" ) ) {
          dbf.SetFieldValue( irow, "STEW", CrewsQry.FieldAsString( "cabin" ) );
        }
        if ( !CrewsQry.FieldIsNULL( "commander" ) )
          dbf.SetFieldValue( irow, "CAPT", upperc( CrewsQry.FieldAsString( "commander" ) ) );
      }

      dbf.SetFieldValue( irow, "COMPANY", Qry.FieldAsString( "airline" ) );

      TBalanceData balanceData;
      balanceData.get( point_id, Qry.FieldAsInteger( "pr_tranzit" ), routesB, routesA, true );

      dbf.SetFieldValue( irow, "PAS_ROW", "Z" );
      string strnum;
      int total_tranzit_man_seats = 0, total_tranzit_chd_seats = 0, total_tranzit_inf_seats = 0;
      int total_goshow_man_seats = 0, total_goshow_chd_seats = 0, total_goshow_inf_seats = 0;
      map<string,TBalance> classbal;
      for ( vector<TDestBalance>::iterator ibal=balanceData.balances.begin(); ibal!=balanceData.balances.end(); ibal++ ) {
        strnum = IntToString( ibal->num );
        dbf.SetFieldValue( irow, string("PORT") + strnum, ibal->airp );
        setBalanceValue( dbf, irow, strnum, ibal->tranzit, "T" );
        classbal[ "П" ] += ibal->total_classbal[ "П" ];
        classbal[ "Б" ] += ibal->total_classbal[ "Б" ];
        classbal[ "Э" ] += ibal->total_classbal[ "Э" ];
        total_tranzit_man_seats += ibal->tranzit.male_seats + ibal->tranzit.female_seats;
        ProgTrace( TRACE5, "airp=%s, tranzit.male_seats=%d, tranzit.female_seats=%d, tranzit.chd_seats=%d, tranzit.inf_seats=%d",
                   ibal->airp.c_str(), ibal->tranzit.male_seats, ibal->tranzit.female_seats, ibal->tranzit.chd_seats, ibal->tranzit.inf_seats );
        total_tranzit_chd_seats += ibal->tranzit.chd_seats;
        total_tranzit_inf_seats += ibal->tranzit.inf_seats;
        setBalanceValue( dbf, irow, strnum, ibal->goshow, "D" );
        total_goshow_man_seats += ibal->goshow.male_seats + ibal->goshow.female_seats;
        ProgTrace( TRACE5, "airp=%s, goshow.male_seats=%d, goshow.female_seats=%d, goshow.chd_seats=%d, goshow.inf_seats=%d",
                   ibal->airp.c_str(), ibal->goshow.male_seats, ibal->goshow.female_seats, ibal->goshow.chd_seats, ibal->goshow.inf_seats );
        total_goshow_chd_seats += ibal->goshow.chd_seats;
        total_goshow_inf_seats += ibal->goshow.inf_seats;
      }
      setBalanceValue( dbf, irow, "", classbal[ "П" ], "I" );
      setBalanceValue( dbf, irow, "", classbal[ "Б" ], "B" );
      dbf.SetFieldValue( irow, "K_MAN", IntToString( total_tranzit_man_seats + total_goshow_man_seats ) );
      dbf.SetFieldValue( irow, "K_RB", IntToString( total_tranzit_chd_seats + total_goshow_chd_seats ) );
      dbf.SetFieldValue( irow, "K_RM", IntToString( total_tranzit_inf_seats + total_goshow_inf_seats ) );
      dbf.SetFieldValue( irow, "PAD_F", IntToString( classbal[ "П" ].pad_seats ) );
      dbf.SetFieldValue( irow, "PAD_C", IntToString( classbal[ "Б" ].pad_seats ) );
      dbf.SetFieldValue( irow, "PAD_Y", IntToString( classbal[ "Э" ].pad_seats ) );

      int idx=1;
      total_tranzit_man_seats += total_tranzit_chd_seats + total_tranzit_inf_seats; // кол-во транзитных мест
      total_goshow_man_seats += total_goshow_chd_seats + total_goshow_inf_seats; // кол-во зарегистрированных мест
      for ( vector<SALONS2::TCompSectionLayers>::iterator i=CompSectionsLayers.begin(); i!=CompSectionsLayers.end(); i++ ) {
        if ( i->layersSeats.find( cltBlockTrzt ) != i->layersSeats.end() )
          total_tranzit_man_seats -= i->layersSeats[ cltBlockTrzt ].size();
        if ( i->layersSeats.find( cltSOMTrzt ) != i->layersSeats.end() )
          total_tranzit_man_seats -= i->layersSeats[ cltSOMTrzt ].size();
        if ( i->layersSeats.find( cltPRLTrzt ) != i->layersSeats.end() )
          total_tranzit_man_seats -= i->layersSeats[ cltPRLTrzt ].size();
        if ( i->layersSeats.find( cltTranzit ) != i->layersSeats.end() )
          total_goshow_man_seats -= i->layersSeats[ cltTranzit ].size();
        if ( i->layersSeats.find( cltCheckin ) != i->layersSeats.end() )
          total_goshow_man_seats -= i->layersSeats[ cltCheckin ].size();
        if ( i->layersSeats.find( cltTCheckin ) != i->layersSeats.end() )
          total_goshow_man_seats -= i->layersSeats[ cltTCheckin ].size();
        if ( i->layersSeats.find( cltGoShow ) != i->layersSeats.end() )
          total_goshow_man_seats -= i->layersSeats[ cltGoShow ].size();
        dbf.SetFieldValue( irow, string("ZONENAME")+IntToString( idx ), i->compSection.name );
        dbf.SetFieldValue( irow, string("CAPACITY")+IntToString( idx ), IntToString( i->compSection.seats ) );
        int occupy=0;
        for ( std::map<ASTRA::TCompLayerType,SALONS2::TPlaces>::iterator ilayer=i->layersSeats.begin(); ilayer!=i->layersSeats.end(); ilayer++ ) {
          occupy += ilayer->second.size();
        };
        dbf.SetFieldValue( irow, string("OCCUPIED")+IntToString( idx ), IntToString( occupy ) );
        ProgTrace( TRACE5, "i->compSection.name=%s, i->compSection.seats=%d, zones[ i->compSection.name ]=%d",
                   i->compSection.name.c_str(), i->compSection.seats, occupy );
        tst();
        idx++;
      }
    
      if ( get_alarm( point_id, atWaitlist ) ) {
        ProgTrace( TRACE5, "waitlist exists, point_id=%d", point_id );
        throw Exception( "waitlist exists" );
      }
      if ( total_tranzit_man_seats + total_goshow_man_seats != 0 ) {
        ProgTrace( TRACE5, "point_id=%d, total_tranzit_man_seats=%d, total_goshow_man_seats=%d",
                            point_id, total_tranzit_man_seats, total_goshow_man_seats );
        throw Exception( "total seats not equal layers" );
      }
    
      if ( prior_constraint_balance_value != getConstraintRowValue( dbf, irow ) ) {
        dbf.SetFieldValue( irow, "FL_SPP_B", FLAG_SET ); // есть изменения влияющие на результат центровки
        ProgTrace( TRACE5, "FL_SPP_B" );
      }
      else {
        if ( dbf.isModifyRow( irow ) ) {
          dbf.SetFieldValue( irow, "FL_SPP", FLAG_SET ); // есть изменения не влияющие на результат центровки
          ProgTrace( TRACE5, "FLAG_SET" );
        }
      }
    }
    catch( Exception &e ) {
      dbf.RollBackRow( irow );
      ProgTrace( TRACE5, "Rollback: exception.what()=%s, point_id=%d", e.what(), point_id );
    }
  }
  for ( map<string,bool>::iterator i=points.begin(); i!=points.end(); i++ ) {
    ProgTrace( TRACE5, "points: first=%s, second=%d", i->first.c_str(), i->second );
    if ( i->second )
      continue;
    for ( int irow = 0; irow<dbf.getRowCount(); irow++ ) {
      if ( i->first == dbf.GetFieldValue( irow, "ID" ) ) {
        ProgTrace( TRACE5, "dbf.DeleteRow, irow=%d", irow );
        dbf.DeleteRow( irow, false );
        break;
      }
    }
  }
  dbf.Build( "CP866" );
  dbf_file = dbf.Result();
  tst();
  
}

void CentInterface::synchBalance(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	string command_line_params;
	xmlNodePtr node = GetNode( "commandline_params", reqNode );
	if ( node ) {
	  command_line_params = NodeAsString( node );
	  command_line_params = TrimString( command_line_params );
  }
  if ( command_line_params.empty() )
    throw AstraLocale::UserException( "param 'commandline_params' not found" );
  ProgTrace( TRACE5, "command_line_params=%s", command_line_params.c_str() );
  int external_point_id = NoExists;
  std::string::size_type idx = command_line_params.find( " IN" );
  bool pr_import = ( idx != std::string::npos );
  ProgTrace( TRACE5, "idx=%d, npos=%d, pr_import=%d", idx, std::string::npos, pr_import );
  if ( !pr_import ) {
    tst();
    idx = command_line_params.find( " OUT" );
  }
  ProgTrace( TRACE5, "idx=%d, npos=%d, pr_import=%d", idx, std::string::npos, pr_import );
  if ( idx == std::string::npos ) {
    tst();
    throw AstraLocale::UserException( "param 'commandline_params' is invalid" );
  }
  command_line_params = command_line_params.substr( idx + 1 + (pr_import?2:3) );
  command_line_params = TrimString( command_line_params );
  tst();
  ProgTrace( TRACE5, "idx=%d, commandline_params=|%s|", idx, command_line_params.c_str() );
  if ( !command_line_params.empty() ) {
     if ( StrToInt( command_line_params.c_str(), external_point_id ) == EOF )
       throw AstraLocale::UserException( "param 'commandline_params' is invalid" );
  }
  string indbf_file, outdbf_file;
  HexToString( NodeAsString( "data", reqNode ), indbf_file );
  ofstream f;
  string filename = string( "balance/" ) + TReqInfo::Instance()->desk.code + "_in.dbf";
  f.open( filename.c_str() );
  if (!f.is_open()) throw Exception( "Can't open file '%s'", filename.c_str() );
  try {
    f << indbf_file;
    f << "\r\n";
    f.close();
  }
  catch(...)
  {
    try { f.close(); } catch( ... ) { };
    try
    {
      //в случае ошибки запишем пустой файл
      f.open( filename.c_str() );
      if ( f.is_open() ) f.close();
    }
    catch( ... ) { };
    throw;
  };
  
  if ( pr_import ) {
    outdbf_file = indbf_file;
    importDBF( external_point_id, outdbf_file );
    string hs;
    if ( indbf_file == outdbf_file )
      NewTextChild( resNode, "nochange", hs );
    else {
      StringToHex( outdbf_file, hs );
      NewTextChild( resNode, "data", hs );
    }
    //запись в файл
    string filename = string( "balance/" ) + TReqInfo::Instance()->desk.code + "_out.dbf";
    f.open( filename.c_str() );
    if (!f.is_open()) throw Exception( "Can't open file '%s'", filename.c_str() );
    try {
      f << outdbf_file;
      f << "\r\n";
      f.close();
    }
    catch(...)
    {
      try { f.close(); } catch( ... ) { };
      try
      {
        //в случае ошибки запишем пустой файл
        f.open( filename.c_str() );
        if ( f.is_open() ) f.close();
      }
      catch( ... ) { };
      throw;
    };
  }
  else {
    exportDBF( external_point_id, indbf_file );
    NewTextChild( resNode, "nochange" );
  }
}

void CentInterface::getDBFBalance(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    tst();
    ifstream f;
    ostringstream st;
    string desk = NodeAsString( "canon_name", reqNode );
    string filename = string( "balance/" ) + desk + "_out.dbf";
    f.open( filename.c_str() );
    if (!f.is_open()) throw Exception( "Can't open file '%s'", filename.c_str() );
    try {
      f.seekg(0);
      st << f.rdbuf();
      ProgTrace( TRACE5, "st.str().size()=%d", st.str().size() );
      string str;
      StringToHex( st.str(), str );
      NewTextChild( resNode, "data", str );
      f.close();
    }
    catch(...)
    {
      try { f.close(); } catch( ... ) { };
      try
      {
        //в случае ошибки запишем пустой файл
        f.open( filename.c_str() );
        if ( f.is_open() ) f.close();
      }
      catch( ... ) { };
      throw;
    };
}
