create or replace PROCEDURE check_arx_partitions(vfirst_date IN DATE DEFAULT NULL,
                                                 vlast_date IN DATE DEFAULT NULL)
IS
  CURSOR cur IS
    SELECT table_name FROM user_tables WHERE SUBSTR(table_name,1,4)='ARX_' ORDER BY table_name;
first_date DATE;
last_date DATE;
d DATE;
months BINARY_INTEGER;
use_month_pt BOOLEAN;
pt_name user_tab_partitions.partition_name%TYPE;
ts_name user_tablespaces.tablespace_name%TYPE;
h_value user_tab_partitions.high_value%TYPE;
pt_name1 user_tab_partitions.partition_name%TYPE;
pt_name2 user_tab_partitions.partition_name%TYPE;
ts_name1 user_tablespaces.tablespace_name%TYPE;
ts_name2 user_tablespaces.tablespace_name%TYPE;
pos BINARY_INTEGER;
str VARCHAR2(6);
BEGIN
  IF vfirst_date IS NOT NULL THEN
    first_date:=vfirst_date;
  ELSE
    first_date:=TO_DATE('01.10.2006','DD.MM.YYYY');
  END IF;
  IF vlast_date IS NOT NULL THEN
    last_date:=vlast_date;
  ELSE
    SELECT TO_DATE('01.01'||TO_CHAR(SYSDATE,'YYYY'),'DD.MM.YYYY') INTO last_date FROM dual;
    last_date:=ADD_MONTHS(last_date,12);
  END IF;
  FOR curRow IN cur LOOP
    use_month_pt := NOT(curRow.table_name IN ('ARX_BAG_NORMS',
                                              'ARX_BAG_RATES',
                                              'ARX_CRS_DISPLACE2',
                                              'ARX_EXCHANGE_RATES',
                                              'ARX_MOVE_REF',
                                              'ARX_POINTS',
                                              'ARX_STAT',
                                              'ARX_TRFER_STAT',
                                              'ARX_KIOSK_STAT',
                                              'ARX_AGENT_STAT',
                                              'ARX_TRIP_CLASSES',
                                              'ARX_TRIP_DELAYS',
                                              'ARX_TRIP_LOAD',
                                              'ARX_TRIP_SETS',
                                              'ARX_TRIP_STAGES',
                                              'ARX_VALUE_BAG_TAXES',
                                              'ARX_MARK_TRIPS'));

    /* Первый проход: Ищем названия partition и tablespace */
    /* Второй проход: Выводим строку ALTER */
    pt_name1:=NULL;
    pt_name2:=NULL;
    ts_name1:=NULL;
    ts_name2:=NULL;
    FOR pass IN 1..2 LOOP
      IF use_month_pt THEN
        d:=TRUNC(first_date,'q');
      ELSE
        d:=TRUNC(first_date,'year');
      END IF;
      WHILE d<last_date LOOP
        IF use_month_pt THEN
          months:=3;
          str:=TO_CHAR(d,'YYYYMM');
        ELSE
          months:=12;
          str:=TO_CHAR(d,'YYYY');
        END IF;
        BEGIN
          SELECT partition_name,tablespace_name,high_value
          INTO pt_name, ts_name, h_value
          FROM user_tab_partitions WHERE table_name=curRow.table_name AND partition_name LIKE '%'||str||'%';
          IF pass=1 THEN
            --первый проход
            pos:=INSTR(pt_name,str);
            IF pos>0 THEN
              pt_name2:=SUBSTR(pt_name,pos+LENGTH(str));
              pt_name1:=SUBSTR(pt_name,1,pos-1);
            END IF;
            pos:=INSTR(ts_name,str);
            IF pos>0 THEN
              ts_name2:=SUBSTR(ts_name,pos+LENGTH(str));
              ts_name1:=SUBSTR(ts_name,1,pos-1);
            END IF;
          ELSE
            --второй проход
            IF pt_name<>pt_name1||str||pt_name2 OR
               ts_name<>ts_name1||str||ts_name2 OR
               INSTR(h_value,TO_CHAR(ADD_MONTHS(d,months),'SYYYY-MM-DD HH24:MI:SS'))=0 THEN
              RAISE NO_DATA_FOUND;
            END IF;
          END IF;
        EXCEPTION
          WHEN NO_DATA_FOUND THEN
            IF pass=2 THEN
              DBMS_OUTPUT.PUT_LINE('ALTER TABLE '||curRow.table_name||' ADD PARTITION '||pt_name1||str||pt_name2||
                                   ' VALUES LESS THAN (TO_DATE('''||TO_CHAR(ADD_MONTHS(d,months),'DD.MM.YYYY')||
                                   ''',''DD.MM.YYYY'')) TABLESPACE '||ts_name1||str||ts_name2||';');
            END IF;
        END;
        d:=ADD_MONTHS(d,months);
      END LOOP; /*first_date<last_date*/
    END LOOP; /*pass*/
  END LOOP;
END;
/
