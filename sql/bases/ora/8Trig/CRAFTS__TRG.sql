create or replace trigger CRAFTS__TRG
BEFORE
INSERT OR UPDATE
ON CRAFTS
FOR EACH ROW
DECLARE
    info	 adm.TCacheInfo;
    vfield_code  VARCHAR2(10);
    vfield_title cache_fields.title%TYPE;
    lparams      system.TLexemeParams;
  BEGIN
    info:=adm.get_cache_info('CRAFTS');
    BEGIN
      /* проверяем длину и допустимые символы в кодах и названиях */
      IF :new.code IS NULL OR
         LENGTH(:new.code) BETWEEN 2 AND 3 AND
         system.is_upp_let_dig(:new.code)<>0 THEN
        NULL;
      ELSE
        vfield_title:='CODE';
        raise_application_error(-20000,'');
      END IF;

      IF :new.code_lat IS NULL OR
         LENGTH(:new.code_lat)=3 AND
         system.is_upp_let_dig(:new.code_lat,1)<>0 THEN
        NULL;
      ELSE
        vfield_title:='CODE_LAT';
        raise_application_error(-20000,'');
      END IF;

      IF :new.code_icao IS NULL OR
         LENGTH(:new.code_icao)=4 AND
         system.is_upp_let_dig(:new.code_icao)<>0 THEN
        NULL;
      ELSE
        vfield_title:='CODE_ICAO';
        raise_application_error(-20000,'');
      END IF;

      IF :new.code_icao_lat IS NULL OR
         LENGTH(:new.code_icao_lat)=4 AND
         system.is_upp_let_dig(:new.code_icao_lat,1)<>0 THEN
        NULL;
      ELSE
        vfield_title:='CODE_ICAO_LAT';
        raise_application_error(-20000,'');
      END IF;

      IF system.is_airline_name(:new.name)=0 THEN
        vfield_title:='NAME';
        raise_application_error(-20001,'');
      END IF;

      IF system.is_airline_name(:new.name_lat,1)=0 THEN
        vfield_title:='NAME_LAT';
        raise_application_error(-20001,'');
      END IF;

    EXCEPTION
      WHEN OTHERS THEN
        lparams('field'):=info.field_title(vfield_title);
        lparams('craft'):=:new.code;
        IF SQLCODE=-20001 THEN
          system.raise_user_exception(SQLCODE,'MSG.INVALID_FIELD_CHARS_FOR_AIRCRAFT',lparams);
        ELSE
          system.raise_user_exception(SQLCODE,'MSG.INVALID_FIELD_VALUE_FOR_AIRCRAFT',lparams);
        END IF;
    END;
  END;
/
