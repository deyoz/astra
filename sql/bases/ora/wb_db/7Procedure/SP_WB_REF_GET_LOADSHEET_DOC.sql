create or replace PROCEDURE SP_WB_REF_GET_LOADSHEET_DOC
(cXML_in in clob, cXML_out out clob)
AS
  cXML_Data clob; cXML_LS clob;
  f1 char(3); f2 char(3); f3a char(8); f3b char(2); f4 char(6); f5 char(11); f6 char(7); f7 char(7); f8 char(4);
  f9a number; f9b varchar(3); f10 number; f11 varchar(1000); f11a char(32); f11b char(32); f12 number; f13 varchar(50); f14 varchar(50); f15 varchar(50);
  f16 varchar(50); f13_14_15_16 char(15); f17 number; f18 number; f19 char(3); f20 char(2); f21 char(11); f22 char(8); f23 char(8); f24 varchar(50);
  f25 varchar(50); f26 varchar(50); f27 varchar(50); f28 varchar(50); f29 varchar(50); f30 varchar(50); f31 varchar(50); f32 varchar(50); f33 varchar(50); f34a char(1);
  f34b char(1); f34c char(1); f35 number; f36a varchar(5); f36b varchar(5); f36c varchar(5); f36d varchar(5); f36e varchar(5); f36f varchar(5); f36g varchar(5);
  f36h varchar(5); f42 varchar(50); f47 char(12); f48 varchar(50);
  cLoadsheet_Text varchar(8000) := '';
  DT_LAST date;
  TABLEVAR varchar(40) := 'LoadSheet';
  P_ELEM_ID number;
begin
  SP_WB_GET_LOADSHEET(cXML_in, cXML_Data);

  -- Получить последнюю сохраненную запись в базе
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ELEM_ID
  from dual;

  select max(t1.DT)
  into DT_LAST
  from WB_CALCS_XML t1
  where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ELEM_ID;

  select t1.XML_VALUE
  into cXML_LS
  from WB_CALCS_XML t1
  where t1.DATA_NAME = TABLEVAR and t1.ELEM_ID = P_ELEM_ID and t1.DT = DT_LAST;

  -- Сравнить cXML_LS и cXML_Data
  select to_char(EXTRACTVALUE(value(b), '/loadsheet/@cFrom_1')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTo_2')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cFlight_3a')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cIdentifier_3b')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cA_C_Reg_4')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cVersion_5')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cCrew_6')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cDate_7')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTime_8')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cEd_No_9a')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cEd_No_Status_9b')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cWeight_10')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cLoad_in_compartments_11')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cPessenger_Cabin_bag_Weight_12')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cDistribution_13')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cAdults_14')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cChd_15')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cInf_16')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cPass_13_14_15_16')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTotal_No_17')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cCabin_Bag_18')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cPax_19')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cAct_class_serv_destinator_20')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTotal_number_seats_21')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cSOC_22')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBLKD_23')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTotal_Traffic_Load_24')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cDry_Operationg_Weight_25')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cZero_Fuel_Weight_26')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cZero_Fuel_Weight_MAX_27')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTake_Off_Fuel_28')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTake_Off_Weight_29')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cMAX_Take_Off_Weight_30')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cTrip_Fuel_31')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cLanding_Weight_32')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cMAX_Landing_Weight_33')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cIndctr_for_max_weight_34_1')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cIndctr_for_max_weight_34_2')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cIndctr_for_max_weight_34_3')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cUnderload_before_LMC_35')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_DOI')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_TOFI')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_LI_ZFW')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_LI_TOW')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_LI_LAW')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_MAC_ZFW')),
      to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_MAC_TOW')), to_char(EXTRACTVALUE(value(b), '/loadsheet/@cBlnce_and_Seat_Cnd_36_MAC_LAW'))
  into f1, f2, f3a, f3b, f4, f5, f6, f7, f8, f9a, f9b, f10, f11,
  f12, f13, f14, f15, f16, f13_14_15_16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31, f32, f33, f34a, f34b, f34c, f35, f36a, f36b, f36c, f36d, f36e, f36f, f36g, f36h
  from table(XMLSequence(Extract(xmltype(cXML_Data), '/root/loadsheet'))) b;

  -- Если разные, то сохранить
  insert into WB_CALCS_XML (ELEM_ID, DATA_NAME, XML_VALUE)
  values (P_ELEM_ID, TABLEVAR, cXML_Data);

  -- Нужна процедура, получающая из XML текст. Ее вызывать в SP_WB_GET_LOADSHEET_SAVED

cLoadsheet_Text :=
'L O A D S H E E T           CHECKED BY       APPROVED      EDNO
ALL WEIGHTS IN KILOGRAMS  ' || f47 || '                    ' || to_char(f9a, '00') || f9b || '

FROM/TO FLIGHT      A/C REG  VERSION      CREW     DATE    TIME
' || f1 || ' ' || f2 || ' ' || f3a || '/' || f3b || ' ' || f4 || '   ' || f5 || '  ' || f6 || '  ' || f7 || ' ' || f8 || '

                         WEIGHT           DISTRIBUTION
LOAD IN COMPARTMENTS   ' ||  to_char(f10, '9999999') || ' ' || f11a || '
                               ' || f11b || '
PASSENGER/CABIN BAG    ' || to_char(f12, '9999999') || ' ' || f13_14_15_16 || ' TTL ' || to_char(f17, '999') || ' CAB ' || to_char(f18, '9999') || '
                                ' || f19 || ' ' || f21 || ' SOC ' || f22 || '
TOTAL TRAFFIC LOAD     ' || to_char(f24, '9999999') || ' BLKD ' || f23 || '
DRY OPERATING WEIGHT   ' || to_char(f25, '9999999') || '
******************************
ZERO FUEL WEIGHT ACTUAL' || to_char(f26, '9999999') || ' MAX' || to_char(f27, '9999999') || '   ' || f34a || ' ADJ
******************************
TAKE OFF FUEL          ' || to_char(f28, '9999999') || '
******************************
TAKE OFF WEIGHT  ACTUAL' || to_char(f29, '9999999') || ' MAX' || to_char(f30, '9999999') || '   ' || f34b || ' ADJ
******************************
TRIP FUEL              ' || to_char(f31, '9999999') || '
LANDING WEIGHT   ACTUAL' || to_char(f32, '9999999') || ' MAX' || to_char(f33, '9999999') || '   ' || f34c || ' ADJ

BALANCE AND SEATING CONDITIONS        LAST MINUTE CHANGES
DOI      ' || f36a || '                    DEST  SPEC    CL/CPT + - WEIGHT
LIZFW    ' || f36c || ' MACZFW    ' || f36f || '
LITOW    ' || f36d || ' MACTOW    ' || f36g || '
LILAW    ' || f36e || ' MACLAW    ' || f36h || '



UNDERLOAD BEFORE LMC   ' || to_char(f35, '9999999') || '           LMC TOTAL + -
CAPTAINS INFORMATION / NOTES

LOADMESSAGE BEFORE LMC';

  -- корневые теги
  -- cXML_out := '<root name="get_loadsheet_doc" data="' || cLoadsheet_Text || '"</root>';
  cXML_out := '<root name="get_loadsheet_doc">' || cLoadsheet_Text || '</root>';
  cXML_out := '<?xml version="1.0" ?>' || cXML_out;

  commit;
END SP_WB_REF_GET_LOADSHEET_DOC;
/
