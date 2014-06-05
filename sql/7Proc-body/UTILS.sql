create or replace PACKAGE BODY utils
AS

procedure fill_tz_regions is
    cursor cur is select country, gmt_offset, min(id) id from date_time_zonespec where
        country IS NOT NULL group by country, gmt_offset;
    vtid NUMBER;
begin
    select tid__seq.nextval into vtid from dual;
    for c in cur loop
        update tz_regions set region=c.id,pr_del=0,tid=vtid
        where country=c.country and tz=to_number(substr(c.gmt_offset, 1, 3));
        if SQL%NOTFOUND then
          insert into tz_regions (country, tz, region, pr_del, tid) values
              (c.country, to_number(substr(c.gmt_offset, 1, 3)), c.id, 0, vtid);
        end if;
    end loop;
    commit;
end;

PROCEDURE add_sync_error(vtable_name IN code_sync_error.table_name%TYPE,
                    vida        IN code_sync_error.ida%TYPE,
                    vcode       IN code_sync_error.code%TYPE,
                    vmsg        IN code_sync_error.msg%TYPE)
IS
BEGIN
  UPDATE code_sync_error SET code=vcode, msg=vmsg
  WHERE table_name=vtable_name AND ida=vida;
  IF SQL%NOTFOUND THEN
    INSERT INTO code_sync_error(table_name,ida,code,msg)
    VALUES(vtable_name,vida,vcode,vmsg);
  END IF;
END add_sync_error;

PROCEDURE del_sync_error(vtable_name IN code_sync_error.table_name%TYPE,
                    vida        IN code_sync_error.ida%TYPE)
IS
BEGIN
  DELETE FROM code_sync_error WHERE table_name=vtable_name AND ida=vida;
END del_sync_error;

PROCEDURE sync_countries
IS
  CURSOR cur IS
    SELECT ida,
           UPPER(TRIM(rcodeg)) AS rcode,
           TRANSLATE(UPPER(TRIM(lcodeg)),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS lcode,
           UPPER(TRIM(rname)) AS rname,
           TRANSLATE(UPPER(TRIM(lname)),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS lname,
           TRANSLATE(UPPER(TRIM(iso_codeg)),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS iso_code
    FROM gos WHERE close=0;
row	cur%ROWTYPE;
row2	countries%ROWTYPE;
row3    countries%ROWTYPE;
info	adm.TCacheInfo;
vtable_name     code_sync_error.table_name%TYPE;
BEGIN
  vtable_name:='COUNTRIES';
  info:=adm.get_cache_info('COUNTRIES');

  row2.tid:=NULL;
  FOR row IN cur LOOP
    row2.id:=NULL;
    row2.code:=NULL;
    row2.code_lat:=NULL;
    row2.code_iso:=NULL;
    row2.name:=NULL;
    row2.name_lat:=NULL;

    BEGIN
      /* проверяем длину и допустимые символы в кодах и названиях */
      IF row.rcode IS NOT NULL AND
         LENGTH(row.rcode)=2 AND
         system.is_upp_let(row.rcode)<>0 THEN row2.code:=row.rcode; END IF;
      IF row.lcode IS NOT NULL AND
         LENGTH(row.lcode)=2 AND
         system.is_upp_let(row.lcode,1)<>0 THEN row2.code_lat:=row.lcode; END IF;
      IF row.iso_code IS NOT NULL AND
         LENGTH(row.iso_code)=3 AND
         system.is_upp_let(row.iso_code,1)<>0 THEN row2.code_iso:=row.iso_code; END IF;
      IF row.rname IS NOT NULL AND
         system.is_name(row.rname)<>0 THEN row2.name:=row.rname; END IF;
      IF row.lname IS NOT NULL AND
         system.is_name(row.lname,1)<>0 THEN row2.name_lat:=row.lname; END IF;
      IF row.rname IS NULL THEN row2.name:=row2.name_lat; END IF;

      row3.id:=NULL;
      BEGIN
        -- берем строку из астровского кодификатора, соответствующую коду
        SELECT * INTO row3 FROM countries WHERE code=row2.code FOR UPDATE;
        row2.id:=row3.id;
        IF NVL(row2.code,' ') <> NVL(row3.code,' ') OR
           NVL(row2.code_lat,' ') <> NVL(row3.code_lat,' ') OR
           NVL(row2.code_iso,' ') <> NVL(row3.code_iso,' ') OR
           NVL(row2.name,' ') <> NVL(row3.name,' ') OR
           NVL(row2.name_lat,' ') <> NVL(row3.name_lat,' ') THEN
          -- изменяем строку
          IF row3.tid_sync IS NULL OR
             row3.tid_sync IS NOT NULL AND row3.tid<>row3.tid_sync THEN
            add_sync_error(vtable_name,row.ida,row2.code,'Строка была изменена вручную. Синхронизация не производится');
            GOTO end_loop;
          ELSE
            IF row2.tid IS NULL THEN SELECT tid__seq.nextval INTO row2.tid FROM dual; END IF;
            UPDATE countries
            SET code=row2.code,
                code_lat=row2.code_lat,
                code_iso=row2.code_iso,
                name=row2.name,
                name_lat=row2.name_lat,
                pr_del=0,
                tid=row2.tid,
                tid_sync=row2.tid
            WHERE id=row2.id;
            adm.check_country_codes(row2.id,row2.code,row2.code_lat,row2.code_iso,'RU');
          END IF;
        ELSE
          del_sync_error(vtable_name,row.ida);
          GOTO end_loop;
        END IF;
      EXCEPTION
        WHEN NO_DATA_FOUND THEN
          -- не нашли строку в Астре, соответствующую коду
          IF row2.tid IS NULL THEN SELECT tid__seq.nextval INTO row2.tid FROM dual; END IF;
          INSERT INTO countries(id,code,code_lat,code_iso,name,name_lat,pr_del,tid,tid_sync)
          VALUES(id__seq.nextval,row2.code,row2.code_lat,row2.code_iso,row2.name,row2.name_lat,0,row2.tid,row2.tid);
          SELECT id__seq.currval INTO row2.id FROM dual;
          adm.check_country_codes(row2.id,row2.code,row2.code_lat,row2.code_iso,'RU');
      END;
      del_sync_error(vtable_name,row.ida);
      system.MsgToLog(CASE WHEN row3.id IS NULL THEN 'Ввод' ELSE 'Изменение' END ||
                      ' строки в таблице '''||info.title||''': '||
                      info.field_title('CODE')    ||'='''||row2.code    ||''', '||
                      info.field_title('CODE_LAT')||'='''||row2.code_lat||''', '||
                      info.field_title('CODE_ISO')||'='''||row2.code_iso||''', '||
                      info.field_title('NAME')    ||'='''||row2.name    ||''', '||
                      info.field_title('NAME_LAT')||'='''||row2.name_lat||'''. '||
                      'Идентификатор: '||
                      info.field_title('ID')      ||'='''||row2.id||'''',
                      system.evtCodif);
    EXCEPTION
      WHEN OTHERS THEN
         ROLLBACK;
         add_sync_error(vtable_name,row.ida,row2.code,SUBSTR(SQLERRM,1,1000));
    END;
    <<end_loop>>
    COMMIT;
  END LOOP;
END sync_countries;

PROCEDURE sync_cities(pr_summer IN NUMBER)
IS
  CURSOR cur IS
    SELECT sfe.ida,
           UPPER(TRIM(sfe.rcodec)) AS rcode,
           TRANSLATE(UPPER(TRIM(sfe.lcodec)),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS lcode,
           UPPER(TRIM(sfe.rname)) AS rname,
           TRANSLATE(UPPER(TRIM(sfe.lname)),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS lname,
           UPPER(TRIM(gos.rcodeg)) AS codeg,
           sfe.offset,gos.nodst
    FROM sfe,gos WHERE sfe.codeg=gos.ida AND sfe.close=0;
row	cur%ROWTYPE;
row2	cities%ROWTYPE;
row3    cities%ROWTYPE;
info	adm.TCacheInfo;
vtable_name     code_sync_error.table_name%TYPE;
num     INTEGER;
BEGIN
  vtable_name:='CITIES';
  info:=adm.get_cache_info('CITIES');

  row2.tid:=NULL;
  FOR row IN cur LOOP
    row2.id:=NULL;
    row2.code:=NULL;
    row2.code_lat:=NULL;
    row2.name:=NULL;
    row2.name_lat:=NULL;
    row2.country:=NULL;
    row2.tz:=NULL;

    BEGIN
      /* проверяем длину и допустимые символы в кодах и названиях */
      IF row.rcode IS NOT NULL AND
         LENGTH(row.rcode)=3 AND
         system.is_upp_let(row.rcode)<>0 THEN row2.code:=row.rcode; END IF;
      IF row.lcode IS NOT NULL AND
         LENGTH(row.lcode)=3 AND
         system.is_upp_let(row.lcode,1)<>0 THEN row2.code_lat:=row.lcode; END IF;
      IF row.rname IS NOT NULL AND
         system.is_name(row.rname,0,' -()./`')<>0 THEN row2.name:=row.rname; END IF;
      IF row.lname IS NOT NULL AND
         system.is_name(row.lname,1,' -()./`')<>0 THEN row2.name_lat:=row.lname; END IF;
      IF row.rname IS NULL THEN row2.name:=row2.name_lat; END IF;

      SELECT COUNT(*) INTO num FROM countries WHERE code=row.codeg;
      IF num>0 THEN row2.country:=row.codeg; END IF;
      --кривость Сирены
      IF pr_summer=0 AND row.nodst<>0 THEN row.offset:=row.offset-1; END IF;
      row2.tz:=row.offset-12;

      row3.id:=NULL;
      BEGIN
        -- берем строку из астровского кодификатора, соответствующую коду
        SELECT * INTO row3 FROM cities WHERE code=row2.code FOR UPDATE;
        row2.id:=row3.id;
        IF NVL(row2.code,' ') <> NVL(row3.code,' ') OR
           NVL(row2.code_lat,' ') <> NVL(row3.code_lat,' ') OR
           NVL(row2.name,' ') <> NVL(row3.name,' ') OR
           NVL(row2.name_lat,' ') <> NVL(row3.name_lat,' ') OR
           NVL(row2.country,' ') <> NVL(row3.country,' ') OR
           NVL(row2.tz,100) <> NVL(row3.tz,100) THEN
           -- изменяем строку
          IF row3.tid_sync IS NULL OR
             row3.tid_sync IS NOT NULL AND row3.tid<>row3.tid_sync THEN
            add_sync_error(vtable_name,row.ida,row2.code,'Строка была изменена вручную. Синхронизация не производится');
            GOTO end_loop;
          ELSE
            IF row2.tid IS NULL THEN SELECT tid__seq.nextval INTO row2.tid FROM dual; END IF;
            UPDATE cities
            SET code=row2.code,
                code_lat=row2.code_lat,
                name=row2.name,
                name_lat=row2.name_lat,
                country=row2.country,
                tz=row2.tz,
                pr_del=0,
                tid=row2.tid,
                tid_sync=row2.tid
            WHERE id=row2.id;
            adm.check_city_codes(row2.id,row2.code,row2.code_lat,'RU');
          END IF;
        ELSE
          del_sync_error(vtable_name,row.ida);
          GOTO end_loop;
        END IF;
      EXCEPTION
        WHEN NO_DATA_FOUND THEN
          -- не нашли строку в Астре, соответствующую коду
          IF row2.tid IS NULL THEN SELECT tid__seq.nextval INTO row2.tid FROM dual; END IF;
          INSERT INTO cities(id,code,code_lat,name,name_lat,country,tz,pr_del,tid,tid_sync)
          VALUES(id__seq.nextval,row2.code,row2.code_lat,row2.name,row2.name_lat,
                 row2.country,row2.tz,0,row2.tid,row2.tid);
          SELECT id__seq.currval INTO row2.id FROM dual;
          adm.check_city_codes(row2.id,row2.code,row2.code_lat,'RU');
      END;
      del_sync_error(vtable_name,row.ida);
      system.MsgToLog(CASE WHEN row3.id IS NULL THEN 'Ввод' ELSE 'Изменение' END ||
                      ' строки в таблице '''||info.title||''': '||
                      info.field_title('CODE')    ||'='''||row2.code    ||''', '||
                      info.field_title('CODE_LAT')||'='''||row2.code_lat||''', '||
                      info.field_title('NAME')    ||'='''||row2.name    ||''', '||
                      info.field_title('NAME_LAT')||'='''||row2.name_lat||''', '||
                      info.field_title('COUNTRY') ||'='''||row2.country ||''', '||
                      info.field_title('TZ')      ||'='''||row2.tz      ||'''. '||
                      'Идентификатор: '||
                      info.field_title('ID')      ||'='''||row2.id||'''',
                      system.evtCodif);
    EXCEPTION
      WHEN OTHERS THEN
         ROLLBACK;
         add_sync_error(vtable_name,row.ida,row2.code,SUBSTR(SQLERRM,1,1000));
    END;
    <<end_loop>>
    COMMIT;
  END LOOP;
END;

PROCEDURE sync_airps
IS
  CURSOR cur IS
    SELECT sfe.ida,
           UPPER(TRIM(sfe.rcodec)) AS rcode,
           TRANSLATE(UPPER(TRIM(sfe.lcodec)),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS lcode,
           UPPER(TRIM(sfe.rcodec)) AS codec,
           UPPER(TRIM(sfe.rname)) AS rname,
           TRANSLATE(UPPER(TRIM(sfe.lname)),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS lname
    FROM sfe WHERE sfe.flprt=0 AND sfe.close=0
    UNION
    SELECT aer.ida,
           UPPER(TRIM(aer.rcodep)) AS rcode,
           TRANSLATE(UPPER(TRIM(aer.lcodep)),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS lcode,
           UPPER(TRIM(sfe.rcodec)) AS codec,
           UPPER(TRIM(aer.rname)) AS rname,
           TRANSLATE(UPPER(TRIM(aer.lname)),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS lname
    FROM aer,sfe WHERE aer.codec=sfe.ida AND aer.close=0;
row	cur%ROWTYPE;
row2	airps%ROWTYPE;
row3    airps%ROWTYPE;
info	adm.TCacheInfo;
vtable_name     code_sync_error.table_name%TYPE;
num	INTEGER;
BEGIN
  vtable_name:='AIRPS';
  info:=adm.get_cache_info('AIRPS');

  row2.tid:=NULL;
  FOR row IN cur LOOP
    row2.id:=NULL;
    row2.code:=NULL;
    row2.code_lat:=NULL;
    row2.name:=NULL;
    row2.name_lat:=NULL;
    row2.city:=NULL;

    BEGIN
      /* проверяем длину и допустимые символы в кодах и названиях */
      IF row.rcode IS NOT NULL AND
         LENGTH(row.rcode)=3 AND
         system.is_upp_let(row.rcode)<>0 THEN row2.code:=row.rcode; END IF;
      IF row.lcode IS NOT NULL AND
         LENGTH(row.lcode)=3 AND
         system.is_upp_let(row.lcode,1)<>0 THEN row2.code_lat:=row.lcode; END IF;
      IF row.rname IS NOT NULL AND
         system.is_name(row.rname,0,' -()./`')<>0 THEN row2.name:=row.rname; END IF;
      IF row.lname IS NOT NULL AND
         system.is_name(row.lname,1,' -()./`')<>0 THEN row2.name_lat:=row.lname; END IF;
      IF row.rname IS NULL THEN row2.name:=row2.name_lat; END IF;

      SELECT COUNT(*) INTO num FROM cities WHERE code=row.codec;
      IF num>0 THEN row2.city:=row.codec; END IF;

      row3.id:=NULL;
      BEGIN
        -- берем строку из астровского кодификатора, соответствующую коду
        SELECT * INTO row3 FROM airps WHERE code=row2.code FOR UPDATE;
        row2.id:=row3.id;
        IF NVL(row2.code,' ') <> NVL(row3.code,' ') OR
           NVL(row2.code_lat,' ') <> NVL(row3.code_lat,' ') OR
           NVL(row2.name,' ') <> NVL(row3.name,' ') OR
           NVL(row2.name_lat,' ') <> NVL(row3.name_lat,' ') OR
           NVL(row2.city,' ') <> NVL(row3.city,' ') THEN
          -- изменяем строку
          IF row3.tid_sync IS NULL OR
             row3.tid_sync IS NOT NULL AND row3.tid<>row3.tid_sync THEN
            add_sync_error(vtable_name,row.ida,row2.code,'Строка была изменена вручную. Синхронизация не производится');
            GOTO end_loop;
          ELSE
            IF row2.tid IS NULL THEN SELECT tid__seq.nextval INTO row2.tid FROM dual; END IF;
            UPDATE airps
            SET code=row2.code,
                code_lat=row2.code_lat,
                name=row2.name,
                name_lat=row2.name_lat,
                city=row2.city,
                pr_del=0,
                tid=row2.tid,
                tid_sync=row2.tid
            WHERE id=row2.id;
            adm.check_airp_codes(row2.id,row2.code,row2.code_lat,NULL,NULL,'RU');
          END IF;
        ELSE
          del_sync_error(vtable_name,row.ida);
          GOTO end_loop;
        END IF;
      EXCEPTION
        WHEN NO_DATA_FOUND THEN
          -- не нашли строку в Астре, соответствующую коду
          IF row2.tid IS NULL THEN SELECT tid__seq.nextval INTO row2.tid FROM dual; END IF;
          INSERT INTO airps(id,code,code_lat,name,name_lat,city,pr_del,tid,tid_sync)
          VALUES(id__seq.nextval,row2.code,row2.code_lat,row2.name,row2.name_lat,
                 row2.city,0,row2.tid,row2.tid);
          SELECT id__seq.currval INTO row2.id FROM dual;
          adm.check_airp_codes(row2.id,row2.code,row2.code_lat,NULL,NULL,'RU');
      END;
      del_sync_error(vtable_name,row.ida);
      system.MsgToLog(CASE WHEN row3.id IS NULL THEN 'Ввод' ELSE 'Изменение' END ||
                      ' строки в таблице '''||info.title||''': '||
                      info.field_title('CODE')    ||'='''||row2.code    ||''', '||
                      info.field_title('CODE_LAT')||'='''||row2.code_lat||''', '||
                      info.field_title('NAME')    ||'='''||row2.name    ||''', '||
                      info.field_title('NAME_LAT')||'='''||row2.name_lat||''', '||
                      info.field_title('CITY')    ||'='''||row2.city    ||'''. '||
                      'Идентификатор: '||
                      info.field_title('ID')      ||'='''||row2.id||'''',
                      system.evtCodif);
    EXCEPTION
      WHEN OTHERS THEN
         ROLLBACK;
         add_sync_error(vtable_name,row.ida,row2.code,SUBSTR(SQLERRM,1,1000));
    END;
    <<end_loop>>
    COMMIT;
  END LOOP;
END;

PROCEDURE sync_airlines
IS
  CURSOR cur IS
    SELECT awk.ida,
           UPPER(TRIM(awk.rcodea)) AS rcode,
           TRANSLATE(UPPER(TRIM(awk.lcodea)),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS lcode,
           UPPER(TRIM(awk.rname)) AS rname,
           TRANSLATE(UPPER(TRIM(awk.lname)),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS lname,
           UPPER(TRIM(awk.accode)) AS accode,
           UPPER(TRIM(sfe.rcodec)) AS codec
    FROM awk,sfe WHERE awk.codec=sfe.ida AND awk.close=0;
row	cur%ROWTYPE;
row2	airlines%ROWTYPE;
row3    airlines%ROWTYPE;
info	adm.TCacheInfo;
vtable_name     code_sync_error.table_name%TYPE;
num	INTEGER;
BEGIN
  vtable_name:='AIRLINES';
  info:=adm.get_cache_info('AIRLINES');

  row2.tid:=NULL;
  FOR row IN cur LOOP
    row2.id:=NULL;
    row2.code:=NULL;
    row2.code_lat:=NULL;
    row2.name:=NULL;
    row2.name_lat:=NULL;
    row2.aircode:=NULL;
    row2.city:=NULL;

    BEGIN
      /* проверяем длину и допустимые символы в кодах и названиях */
      IF row.rcode IS NOT NULL AND
         (LENGTH(row.rcode)=2 AND
          system.is_upp_let_dig(row.rcode)<>0 OR
          LENGTH(row.rcode)=3 AND
          system.is_upp_let(row.rcode)<>0) THEN row2.code:=row.rcode; END IF;
      IF row.lcode IS NOT NULL AND
         (LENGTH(row.lcode)=2 AND
          system.is_upp_let_dig(row.lcode,1)<>0 OR
          LENGTH(row.lcode)=3 AND
          system.is_upp_let(row.lcode,1)<>0) THEN row2.code_lat:=row.lcode; END IF;
      IF row.rname IS NOT NULL AND
         system.is_airline_name(row.rname)<>0 THEN row2.name:=row.rname; END IF;
      IF row.lname IS NOT NULL AND
         system.is_airline_name(row.lname,1)<>0 THEN row2.name_lat:=row.lname; END IF;
      IF row.accode IS NOT NULL AND
         LENGTH(row.accode) BETWEEN 2 AND 3 AND
         system.is_upp_let_dig(row.accode)<>0 THEN row2.aircode:=row.accode; END IF;
      IF row.rname IS NULL THEN row2.name:=row2.name_lat; END IF;

      SELECT COUNT(*) INTO num FROM cities WHERE code=row.codec;
      IF num>0 THEN row2.city:=row.codec; END IF;

      row3.id:=NULL;
      BEGIN
        -- берем строку из астровского кодификатора, соответствующую коду
        SELECT * INTO row3 FROM airlines WHERE code=row2.code FOR UPDATE;
        row2.id:=row3.id;
        IF NVL(row2.code,' ') <> NVL(row3.code,' ') OR
           NVL(row2.code_lat,' ') <> NVL(row3.code_lat,' ') OR
           NVL(row2.name,' ') <> NVL(row3.name,' ') OR
           NVL(row2.name_lat,' ') <> NVL(row3.name_lat,' ') OR
           NVL(row2.aircode,' ') <> NVL(row3.aircode,' ') OR
           NVL(row2.city,' ') <> NVL(row3.city,' ') THEN
          -- изменяем строку
          IF row3.tid_sync IS NULL OR
             row3.tid_sync IS NOT NULL AND row3.tid<>row3.tid_sync THEN
            add_sync_error(vtable_name,row.ida,row2.code,'Строка была изменена вручную. Синхронизация не производится');
            GOTO end_loop;
          ELSE
            IF row2.tid IS NULL THEN SELECT tid__seq.nextval INTO row2.tid FROM dual; END IF;
            UPDATE airlines
            SET code=row2.code,
                code_lat=row2.code_lat,
                name=row2.name,
                name_lat=row2.name_lat,
                aircode=row2.aircode,
                city=row2.city,
                pr_del=0,
                tid=row2.tid,
                tid_sync=row2.tid
            WHERE id=row2.id;
            adm.check_airline_codes(row2.id,row2.code,row2.code_lat,NULL,NULL,row2.aircode,'RU');
          END IF;
        ELSE
          del_sync_error(vtable_name,row.ida);
          GOTO end_loop;
        END IF;
      EXCEPTION
        WHEN NO_DATA_FOUND THEN
          -- не нашли строку в Астре, соответствующую коду
          IF row2.tid IS NULL THEN SELECT tid__seq.nextval INTO row2.tid FROM dual; END IF;
          INSERT INTO airlines(id,code,code_lat,name,name_lat,aircode,city,pr_del,tid,tid_sync)
          VALUES(id__seq.nextval,row2.code,row2.code_lat,row2.name,row2.name_lat,
                 row2.aircode,row2.city,0,row2.tid,row2.tid);
          SELECT id__seq.currval INTO row2.id FROM dual;
          adm.check_airline_codes(row2.id,row2.code,row2.code_lat,NULL,NULL,row2.aircode,'RU');
      END;
      del_sync_error(vtable_name,row.ida);
      system.MsgToLog(CASE WHEN row3.id IS NULL THEN 'Ввод' ELSE 'Изменение' END ||
                      ' строки в таблице '''||info.title||''': '||
                      info.field_title('CODE')    ||'='''||row2.code    ||''', '||
                      info.field_title('CODE_LAT')||'='''||row2.code_lat||''', '||
                      info.field_title('NAME')    ||'='''||row2.name    ||''', '||
                      info.field_title('NAME_LAT')||'='''||row2.name_lat||''', '||
                      info.field_title('AIRCODE') ||'='''||row2.aircode ||''', '||
                      info.field_title('CITY')    ||'='''||row2.city    ||'''. '||
                      'Идентификатор: '||
                      info.field_title('ID')      ||'='''||row2.id||'''',
                      system.evtCodif);
    EXCEPTION
      WHEN OTHERS THEN
         ROLLBACK;
         add_sync_error(vtable_name,row.ida,row2.code,SUBSTR(SQLERRM,1,1000));
    END;
    <<end_loop>>
    COMMIT;
  END LOOP;
END;

PROCEDURE sync_crafts
IS
  CURSOR cur IS
    SELECT ida,
           UPPER(TRIM(rcodet)) AS rcode,
           TRANSLATE(UPPER(TRIM(lcodet)),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS lcode,
           UPPER(TRIM(rname)) AS rname,
           TRANSLATE(UPPER(TRIM(lname)),'АВСЕНКМОРТХ','ABCEHKMOPTX') AS lname
    FROM tts WHERE close=0;
row	cur%ROWTYPE;
row2	crafts%ROWTYPE;
row3    crafts%ROWTYPE;
info	adm.TCacheInfo;
vtable_name     code_sync_error.table_name%TYPE;
BEGIN
  vtable_name:='CRAFTS';
  info:=adm.get_cache_info('CRAFTS');

  row2.tid:=NULL;
  FOR row IN cur LOOP
    row2.id:=NULL;
    row2.code:=NULL;
    row2.code_lat:=NULL;
    row2.name:=NULL;
    row2.name_lat:=NULL;

    BEGIN
      /* проверяем длину и допустимые символы в кодах и названиях */
      IF row.rcode IS NOT NULL AND
         LENGTH(row.rcode)=3 AND
         system.is_upp_let_dig(row.rcode)<>0 THEN row2.code:=row.rcode; END IF;
      IF row.lcode IS NOT NULL AND
         LENGTH(row.lcode)=3 AND
         system.is_upp_let_dig(row.lcode,1)<>0 THEN row2.code_lat:=row.lcode; END IF;
      IF row.rname IS NOT NULL AND
         system.is_name(row.rname)<>0 THEN row2.name:=row.rname; END IF;
      IF row.lname IS NOT NULL AND
         system.is_name(row.lname,1)<>0 THEN row2.name_lat:=row.lname; END IF;
      IF row.rname IS NULL THEN row2.name:=row2.name_lat; END IF;
      IF row2.name IS NULL THEN row2.name:=row2.code; END IF;

      row3.id:=NULL;
      BEGIN
        -- берем строку из астровского кодификатора, соответствующую коду
        SELECT * INTO row3 FROM crafts WHERE code=row2.code FOR UPDATE;
        row2.id:=row3.id;
        IF NVL(row2.code,' ') <> NVL(row3.code,' ') OR
           NVL(row2.code_lat,' ') <> NVL(row3.code_lat,' ') OR
           NVL(row2.name,' ') <> NVL(row3.name,' ') OR
           NVL(row2.name_lat,' ') <> NVL(row3.name_lat,' ') THEN
          -- изменяем строку
          IF row3.tid_sync IS NULL OR
             row3.tid_sync IS NOT NULL AND row3.tid<>row3.tid_sync THEN
            add_sync_error(vtable_name,row.ida,row2.code,'Строка была изменена вручную. Синхронизация не производится');
            GOTO end_loop;
          ELSE
            IF row2.tid IS NULL THEN SELECT tid__seq.nextval INTO row2.tid FROM dual; END IF;
            UPDATE crafts
            SET code=row2.code,
                code_lat=row2.code_lat,
                name=row2.name,
                name_lat=row2.name_lat,
                pr_del=0,
                tid=row2.tid,
                tid_sync=row2.tid
            WHERE id=row2.id;
            adm.check_craft_codes(row2.id,row2.code,row2.code_lat,NULL,NULL,'RU');
          END IF;
        ELSE
          del_sync_error(vtable_name,row.ida);
          GOTO end_loop;
        END IF;
      EXCEPTION
        WHEN NO_DATA_FOUND THEN
          -- не нашли строку в Астре, соответствующую коду
          IF row2.tid IS NULL THEN SELECT tid__seq.nextval INTO row2.tid FROM dual; END IF;
          INSERT INTO crafts(id,code,code_lat,name,name_lat,pr_del,tid,tid_sync)
          VALUES(id__seq.nextval,row2.code,row2.code_lat,row2.name,row2.name_lat,0,row2.tid,row2.tid);
          SELECT id__seq.currval INTO row2.id FROM dual;
          adm.check_craft_codes(row2.id,row2.code,row2.code_lat,NULL,NULL,'RU');
      END;
      del_sync_error(vtable_name,row.ida);
      system.MsgToLog(CASE WHEN row3.id IS NULL THEN 'Ввод' ELSE 'Изменение' END ||
                      ' строки в таблице '''||info.title||''': '||
                      info.field_title('CODE')    ||'='''||row2.code    ||''', '||
                      info.field_title('CODE_LAT')||'='''||row2.code_lat||''', '||
                      info.field_title('NAME')    ||'='''||row2.name    ||''', '||
                      info.field_title('NAME_LAT')||'='''||row2.name_lat||'''. '||
                      'Идентификатор: '||
                      info.field_title('ID')      ||'='''||row2.id||'''',
                      system.evtCodif);
    EXCEPTION
      WHEN OTHERS THEN
         ROLLBACK;
         add_sync_error(vtable_name,row.ida,row2.code,SUBSTR(SQLERRM,1,1000));
    END;
    <<end_loop>>
    COMMIT;
  END LOOP;
END;

PROCEDURE sync_sirena_codes(pr_summer IN NUMBER)
IS
BEGIN
  sync_countries;
  sync_cities(pr_summer);
  sync_airps;
  sync_airlines;
  sync_crafts;
END;

END utils;
/
