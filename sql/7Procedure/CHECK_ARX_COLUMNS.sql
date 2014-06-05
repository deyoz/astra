create or replace PROCEDURE check_arx_columns
IS
  CURSOR cur IS
    SELECT SUBSTR(table_name,5) AS table_name FROM user_tables WHERE SUBSTR(table_name,1,4)='ARX_' ORDER BY table_name;
  CURSOR cur2(vtable_name user_tables.table_name%TYPE) IS
    SELECT column_name
    FROM user_tab_columns
    WHERE table_name=vtable_name
    MINUS
    SELECT column_name
    FROM user_tab_columns
    WHERE table_name='ARX_'||vtable_name;
  CURSOR cur3(vtable_name user_tables.table_name%TYPE) IS
    SELECT a.column_name
    FROM user_tab_columns a, user_tab_columns b
    WHERE a.column_name=b.column_name AND a.table_name=vtable_name AND b.table_name='ARX_'||vtable_name
    MINUS
    SELECT a.column_name
    FROM user_tab_columns a, user_tab_columns b
    WHERE a.column_name=b.column_name AND a.table_name=vtable_name AND b.table_name='ARX_'||vtable_name AND
          (a.data_type IS NULL AND b.data_type IS NULL OR a.data_type=b.data_type) AND
          (a.data_length IS NULL AND b.data_length IS NULL OR a.data_length=b.data_length) AND
          (a.data_precision IS NULL AND b.data_precision IS NULL OR a.data_precision=b.data_precision) AND
          (a.data_scale IS NULL AND b.data_scale IS NULL OR a.data_scale=b.data_scale) AND
          (a.nullable IS NULL AND b.nullable IS NULL OR a.nullable=b.nullable);
  CURSOR cur4(vtable_name user_tables.table_name%TYPE) IS
    SELECT column_name
    FROM user_tab_columns
    WHERE table_name='ARX_'||vtable_name AND column_name<>'PART_KEY'
    MINUS
    SELECT column_name
    FROM user_tab_columns
    WHERE table_name=vtable_name;
BEGIN
  FOR curRow IN cur LOOP
    FOR cur2Row IN cur2(curRow.table_name) LOOP
      IF curRow.table_name||'.'||cur2Row.column_name IN ('PAX_GRP.TRFER_CONFIRM',
                                                         'POINTS.TIME_IN',
                                                         'POINTS.TIME_OUT',
                                                         'TCKIN_SEGMENTS.POINT_ID_TRFER',
                                                         'TLGS_IN.TIME_RECEIVE_NOT_PARSE',
                                                         'TRANSFER.POINT_ID_TRFER',
                                                         'TRIP_SETS.AUTO_COMP_CHG',
                                                         'TRIP_SETS.BRD_ALARM',
                                                         'TRIP_SETS.ET_FINAL_ATTEMPT',
                                                         'TRIP_SETS.OVERLOAD_ALARM',
                                                         'TRIP_SETS.PR_BLOCK_TRZT',
                                                         'TRIP_SETS.PR_CHECK_LOAD',
                                                         'TRIP_SETS.PR_CHECK_PAY',
                                                         'TRIP_SETS.PR_EXAM',
                                                         'TRIP_SETS.PR_EXAM_CHECK_PAY',
                                                         'TRIP_SETS.PR_LAT_SEAT',
                                                         'TRIP_SETS.PR_OVERLOAD_REG',
                                                         'TRIP_SETS.PR_REG_WITH_DOC',
                                                         'TRIP_SETS.PR_REG_WITH_TKN',
                                                         'TRIP_SETS.WAITLIST_ALARM',
                                                         'TRIP_STAGES.TIME_AUTO_NOT_ACT',
                                                         'TRIP_STAGES.IGNORE_AUTO') THEN
        NULL;
      ELSE
        DBMS_OUTPUT.PUT_LINE('+ '||curRow.table_name||'.'||cur2Row.column_name);
      END IF;
    END LOOP;
    FOR cur3Row IN cur3(curRow.table_name) LOOP
      IF curRow.table_name||'.'||cur3Row.column_name IN ('TLGS_IN.BODY',
                                                         'BAG_RECEIPTS.POINT_ID') THEN
        NULL;
      ELSE
        DBMS_OUTPUT.PUT_LINE('M '||curRow.table_name||'.'||cur3Row.column_name);
      END IF;
    END LOOP;
    FOR cur4Row IN cur4(curRow.table_name) LOOP
      IF curRow.table_name||'.'||cur4Row.column_name IN ('PAX.SEAT_NO',
                                                         'TCKIN_SEGMENTS.AIRLINE',
                                                         'TCKIN_SEGMENTS.AIRP_DEP',
                                                         'TCKIN_SEGMENTS.FLT_NO',
                                                         'TCKIN_SEGMENTS.SCD',
                                                         'TCKIN_SEGMENTS.SUFFIX',
                                                         'TLGS_IN.PAGE_NO',
                                                         'TRANSFER.AIRLINE',
                                                         'TRANSFER.AIRP_DEP',
                                                         'TRANSFER.FLT_NO',
                                                         'TRANSFER.SCD',
                                                         'TRANSFER.SUFFIX',
                                                         'AGENT_STAT.POINT_PART_KEY') THEN
        NULL;
      ELSE
        DBMS_OUTPUT.PUT_LINE('- '||curRow.table_name||'.'||cur4Row.column_name);
      END IF;
    END LOOP;
  END LOOP;
END check_arx_columns;
/
