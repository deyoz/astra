create or replace PROCEDURE test_translit IS
  CURSOR cur IS
    SELECT TRIM(SUBSTR(scan_data, 1, INSTR(scan_data, '/')-1)) AS scan_surname,
           TRIM(SUBSTR(scan_data, INSTR(scan_data, '/')+1)) AS scan_name,
           surname, name, rowid, reason
    FROM drop_test_translit WHERE pax_id IS NOT NULL FOR UPDATE;
surname_equal_len NUMBER;
name_equal_len NUMBER;
surname_equal_old BOOLEAN;
surname_equal_new BOOLEAN;
name_equal_old BOOLEAN;
name_equal_new BOOLEAN;
equal_old BOOLEAN;
equal_new BOOLEAN;
pos NUMBER;
BEGIN
  FOR curRow IN cur LOOP
    surname_equal_len:=NULL;
    name_equal_len:=NULL;
    IF LENGTH(curRow.scan_surname)+LENGTH(curRow.scan_name)+1>=20 THEN
      surname_equal_len:=LENGTH(curRow.scan_surname);
      name_equal_len:=LENGTH(curRow.scan_name);
    END IF;
    IF surname_equal_len IS NOT NULL THEN
      surname_equal_old:=system.transliter_equal(SUBSTR(curRow.surname, 1, surname_equal_len),
                                                 SUBSTR(curRow.scan_surname, 1, surname_equal_len))<>0;
      surname_equal_new:=system.transliter_equal_begin(curRow.surname, curRow.scan_surname)<>0;
    ELSE
      surname_equal_old:=system.transliter_equal(curRow.surname, curRow.scan_surname)<>0;
      surname_equal_new:=system.transliter_equal(curRow.surname, curRow.scan_surname)<>0;
    END IF;

    pos:=INSTR(curRow.name, ' ');
    IF pos>0 THEN curRow.name:=SUBSTR(curRow.name,1,pos-1); END IF;
    pos:=INSTR(curRow.scan_name, ' ');
    IF pos>0 THEN curRow.scan_name:=SUBSTR(curRow.scan_name,1,pos-1); END IF;

    IF name_equal_len IS NOT NULL THEN
      name_equal_old:=system.transliter_equal(SUBSTR(curRow.name, 1, name_equal_len),
                                              SUBSTR(curRow.scan_name, 1, name_equal_len))<>0;
      name_equal_new:=system.transliter_equal_begin(curRow.name, curRow.scan_name)<>0;
    ELSE
      name_equal_old:=system.transliter_equal(curRow.name, curRow.scan_name)<>0;
      name_equal_new:=system.transliter_equal(curRow.name, curRow.scan_name)<>0;
    END IF;

--    IF LENGTH(curRow.surname)<LENGTH(curRow.scan_surname) THEN surname_equal_new:=FALSE; END IF;
--    IF LENGTH(curRow.name)<LENGTH(curRow.scan_name) THEN name_equal_new:=FALSE; END IF;

    equal_old:=surname_equal_old AND name_equal_old;
    equal_new:=surname_equal_new AND name_equal_new;

    curRow.reason:=NULL;
    IF equal_old AND equal_new THEN curRow.reason:='both equal'; END IF;
    IF NOT(equal_old) AND equal_new THEN curRow.reason:='+'; END IF;
    IF equal_old AND NOT(equal_new) THEN curRow.reason:='-'; END IF;
    IF NOT(equal_old) AND NOT(equal_new) THEN curRow.reason:='both not equal'; END IF;

    UPDATE drop_test_translit SET reason=curRow.reason WHERE rowid=curRow.rowid;
  END LOOP;
  COMMIT;
END;
/
