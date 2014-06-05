create or replace trigger CITIES__TRG
BEFORE
INSERT OR UPDATE
ON CITIES
FOR EACH ROW
DECLARE
    info	 adm.TCacheInfo;
    vfield_code  VARCHAR2(10);
    vfield_title cache_fields.title%TYPE;
    lparams      system.TLexemeParams;
  BEGIN
    info:=adm.get_cache_info('CITIES');
    BEGIN
      /* проверяем длину и допустимые символы в кодах и названиях */
      IF :new.code IS NULL OR
         LENGTH(:new.code)=3 AND
         system.is_upp_let(:new.code)<>0 THEN
        NULL;
      ELSE
        vfield_title:='CODE';
        raise_application_error(-20000,'');
      END IF;

      IF :new.code_lat IS NULL OR
         LENGTH(:new.code_lat)=3 AND
         system.is_upp_let(:new.code_lat,1)<>0 THEN
        NULL;
      ELSE
        vfield_title:='CODE_LAT';
        raise_application_error(-20000,'');
      END IF;

      IF system.is_name(:new.name,0,' -()./`')=0 THEN
        vfield_title:='NAME';
        raise_application_error(-20001,'');
      END IF;

      IF system.is_name(:new.name_lat,1,' -()./`')=0 THEN
        vfield_title:='NAME_LAT';
        raise_application_error(-20001,'');
      END IF;

      IF :new.tz BETWEEN -12 AND 12 THEN
        NULL;
      ELSE
        vfield_title:='TZ';
        raise_application_error(-20000,'');
      END IF;
    EXCEPTION
      WHEN OTHERS THEN
        lparams('field'):=info.field_title(vfield_title);
        lparams('city'):=:new.code;
        IF SQLCODE=-20001 THEN
          system.raise_user_exception(SQLCODE,'MSG.INVALID_FIELD_CHARS_FOR_CITY',lparams);
        ELSE
          system.raise_user_exception(SQLCODE,'MSG.INVALID_FIELD_VALUE_FOR_CITY',lparams);
        END IF;
    END;
  END;
/
