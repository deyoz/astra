create or replace FUNCTION check_locale_row(vid      locale_messages.id%TYPE,
                          vlang    locale_messages.lang%TYPE,
                          vtext    locale_messages.text%TYPE,
                          vpr_term locale_messages.pr_term%TYPE) RETURN VARCHAR2
IS
i BINARY_INTEGER;
BEGIN
  IF system.is_lat(vid) THEN
    --идентификатор латинский
    IF vlang='RU' THEN
      --проверим пару
      SELECT COUNT(*) INTO i FROM locale_messages WHERE id=vid AND lang='EN';
      IF i=0 THEN
        RETURN 'Нет пары на английском';
      END IF;

      IF system.is_name(vid,NULL,'._')=0 THEN
        RETURN 'Странные символы в ид.';
      END IF;
    END IF;
    IF (vlang='EN') AND UPPER(vid)!=vid THEN
      RETURN 'Латинский текст для латинского ид.';
    END IF;
  ELSE
    --идентификатор русский (это перевод терминала)
    IF vlang='RU' THEN
      RETURN 'Русский текст для русского ид.';
    END IF;
    /*
    IF vpr_term=0 OR vpr_term IS NULL THEN
      SELECT COUNT(*) INTO i FROM
        (SELECT title FROM cache_tables WHERE title=vid
         UNION
         SELECT title FROM cache_fields WHERE title=vid);
      IF i=0 THEN
        RETURN 'Русский ид. для серверной части';
      END IF;
    END IF;
    */
  END IF;
  IF system.is_lat(vtext) THEN
    IF vlang='RU' THEN
      RETURN 'Латинский текст для RU';
    END IF;
  ELSE
    IF vlang='EN' THEN
      RETURN 'Русский текст для EN';
    END IF;
  END IF;
  SELECT COUNT(*) INTO i FROM (SELECT DISTINCT pr_term FROM locale_messages WHERE id=vid);
  IF i>1 THEN
    RETURN 'Разный pr_term для одного ид.';
  END IF;
  RETURN NULL;
END;
/
