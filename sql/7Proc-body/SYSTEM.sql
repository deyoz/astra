create or replace PACKAGE BODY system
AS

FUNCTION is_lat(str     IN VARCHAR2) RETURN BOOLEAN
IS
c       CHAR(1);
BEGIN
  IF str IS NULL THEN RETURN TRUE; END IF;
  FOR i IN 1..LENGTH(str) LOOP
    c:=SUBSTR(str,i,1);
    IF ASCII(c)>127 THEN
--    IF c BETWEEN 'А' AND 'Я' OR c='Ё' THEN
      RETURN FALSE;
    END IF;
  END LOOP;
  RETURN TRUE;
END is_lat;

FUNCTION is_upp_let(str 	IN VARCHAR2,
                    pr_lat 	IN INTEGER DEFAULT NULL) RETURN NUMBER
IS
c       CHAR(1);
BEGIN
  IF str IS NULL THEN RETURN 1; END IF;
  FOR i IN 1..LENGTH(str) LOOP
    c:=SUBSTR(str,i,1);
    IF pr_lat IS NULL OR pr_lat=0 THEN
      IF NOT c BETWEEN 'A' AND 'Z' AND
         NOT c BETWEEN 'А' AND 'Я' AND
         c<>'Ё' THEN RETURN 0; END IF;
    ELSE
      IF NOT c BETWEEN 'A' AND 'Z' THEN RETURN 0; END IF;
    END IF;
  END LOOP;
  RETURN 1;
END is_upp_let;

FUNCTION is_upp_let_dig(str 	IN VARCHAR2,
                        pr_lat 	IN INTEGER DEFAULT NULL) RETURN NUMBER
IS
c       CHAR(1);
BEGIN
  IF str IS NULL THEN RETURN 1; END IF;
  FOR i IN 1..LENGTH(str) LOOP
    c:=SUBSTR(str,i,1);
    IF pr_lat IS NULL OR pr_lat=0 THEN
      IF NOT c BETWEEN 'A' AND 'Z' AND
         NOT c BETWEEN 'А' AND 'Я' AND
         c<>'Ё' AND
         NOT c BETWEEN '0' AND '9' THEN RETURN 0; END IF;
    ELSE
      IF NOT c BETWEEN 'A' AND 'Z' AND
         NOT c BETWEEN '0' AND '9' THEN RETURN 0; END IF;
    END IF;
  END LOOP;
  RETURN 1;
END is_upp_let_dig;

FUNCTION is_dig(str 	IN VARCHAR2,
                        pr_lat 	IN INTEGER DEFAULT NULL) RETURN NUMBER
IS
c       CHAR(1);
BEGIN
  IF str IS NULL THEN RETURN 1; END IF;
  FOR i IN 1..LENGTH(str) LOOP
    c:=SUBSTR(str,i,1);
    IF NOT c BETWEEN '0' AND '9' THEN RETURN 0; END IF;
  END LOOP;
  RETURN 1;
END is_dig;

FUNCTION invalid_char_in_name(str	IN VARCHAR2,
                              pr_lat IN INTEGER DEFAULT NULL,
                              symbols IN VARCHAR2 DEFAULT ' -') RETURN CHAR
IS
c       CHAR(1);
BEGIN
  IF str IS NULL THEN RETURN NULL; END IF;
  FOR i IN 1..LENGTH(str) LOOP
    c:=SUBSTR(str,i,1);
    IF pr_lat IS NULL OR pr_lat=0 THEN
      IF NOT c BETWEEN 'A' AND 'Z' AND
         NOT c BETWEEN 'А' AND 'Я' AND
         c<>'Ё'AND
         NOT c BETWEEN '0' AND '9' AND
         (symbols IS NULL OR
          symbols IS NOT NULL AND INSTR(symbols,c)=0) THEN RETURN c; END IF;
    ELSE
      IF NOT c BETWEEN 'A' AND 'Z' AND
         NOT c BETWEEN '0' AND '9' AND
         (symbols IS NULL OR
          symbols IS NOT NULL AND INSTR(symbols,c)=0) THEN RETURN c; END IF;
    END IF;
  END LOOP;
  RETURN NULL;
END invalid_char_in_name;

FUNCTION is_name(str	IN VARCHAR2,
                 pr_lat IN INTEGER DEFAULT NULL,
                 symbols IN VARCHAR2 DEFAULT ' -') RETURN NUMBER
IS
BEGIN
  IF invalid_char_in_name(str, pr_lat, symbols) IS NULL THEN
    RETURN 1;
  ELSE
    RETURN 0;
  END IF;
END is_name;

FUNCTION is_airline_name(str	IN VARCHAR2,
                         pr_lat IN INTEGER DEFAULT NULL) RETURN NUMBER
IS
BEGIN
  RETURN is_name(str,pr_lat,' ,.+-/:;()"`''');
END is_airline_name;

FUNCTION IsLeapYear( vYear NUMBER ) RETURN NUMBER
IS
BEGIN
 IF ( vYear MOD 4 = 0 )AND( ( vYear MOD 100 <> 0 )OR( vYear MOD 400 = 0 ) )
  THEN RETURN 1;
  ELSE RETURN 0;
 END IF;
END IsLeapYear;

FUNCTION LastDayofMonth( vYear NUMBER, vMonth NUMBER ) RETURN NUMBER
IS
BEGIN
 IF vMonth IN (1,3,5,7,8,10,12)
  THEN RETURN 31;
  ELSE IF vMonth = 2
        THEN RETURN 28 + IsLeapYear( vYear );
        ELSE RETURN 30;
       END IF;
 END IF;
END LastDayofMonth;

FUNCTION UTCSYSDATE RETURN DATE
IS
vdate	DATE;
BEGIN
  SELECT CAST(SYS_EXTRACT_UTC(SYSTIMESTAMP) AS DATE) INTO vdate FROM dual;
  RETURN vdate;
END UTCSYSDATE;

FUNCTION LOCALSYSDATE RETURN DATE
IS
vdate	DATE;
BEGIN
  SELECT CAST(SYSTIMESTAMP AS DATE) INTO vdate FROM dual;
  RETURN vdate;
END LOCALSYSDATE;

FUNCTION transliter(str	IN VARCHAR2, fmt IN INTEGER) RETURN VARCHAR2
IS
CURSOR cur IS
  SELECT * FROM translit_dicts;
str2	VARCHAR2(4000);
c	CHAR(1);
c2	VARCHAR2(4);
BEGIN
  IF translit_dicts_t.FIRST IS NULL THEN
    -- пустой словарь: надо наполнить
    FOR curRow IN cur LOOP
      translit_dicts_t(curRow.letter):=curRow;
    END LOOP;
  END IF;

  str2:=NULL;
  FOR i IN 1..LENGTH(str) LOOP
    c:=SUBSTR(str,i,1);
    IF ASCII(c)>127 THEN
      BEGIN
        c2:=CASE fmt
              WHEN 2 THEN translit_dicts_t(UPPER(c)).lat2
                     ELSE translit_dicts_t(UPPER(c)).lat1
            END;
      EXCEPTION
        WHEN NO_DATA_FOUND THEN c2:='?';
      END;
      IF UPPER(c)<>c THEN c2:=LOWER(c2); END IF;
      str2:=str2||c2;
    ELSE
      str2:=str2||c;
    END IF;
  END LOOP;
  RETURN str2;
EXCEPTION
  WHEN VALUE_ERROR THEN RETURN str2;
END transliter;

FUNCTION transliter(str	        IN VARCHAR2,
                    fmt 	IN INTEGER,
                    pr_lat      IN INTEGER) RETURN VARCHAR2
IS
BEGIN
  IF pr_lat <> 0 AND NOT is_lat(str) THEN
    RETURN transliter(str,fmt);
  ELSE
    RETURN str;
  END IF;
END transliter;

FUNCTION transliter_equal(str1 IN VARCHAR2,
                          str2 IN VARCHAR2,
                          fmt  IN INTEGER DEFAULT NULL) RETURN NUMBER
IS
BEGIN
  IF str1 IS NULL OR str2 IS NULL THEN RETURN 0; END IF;
  FOR ifmt IN 1..2 LOOP
    IF fmt IS NULL OR
       fmt IS NOT NULL AND fmt=ifmt THEN
      IF transliter(str1,ifmt)=transliter(str2,ifmt) THEN RETURN 1; END IF;
    END IF;
  END LOOP;
  RETURN 0;
END transliter_equal;

PROCEDURE raise_user_exception(verror_code   IN NUMBER,
                               lexeme_id     IN locale_messages.id%TYPE,
                               lexeme_params IN TLexemeParams)
IS
xml VARCHAR2(2000);
i   VARCHAR2(20);
BEGIN
  xml:='<?xml version="1.0" encoding="UTF-8"?>'||CHR(10)||
       '<lexeme_data>'||CHR(10)||
       '  <id>'||lexeme_id||'</id>'||CHR(10)||
       '  <params>'||CHR(10);
  i:=lexeme_params.FIRST;
  WHILE i IS NOT NULL LOOP
    xml:=xml||
       '    <'||i||'>'||lexeme_params(i)||'</'||i||'>'||CHR(10);
    i:=lexeme_params.NEXT(i);
  END LOOP;
  xml:=xml||
       '  </params>'||CHR(10)||
       '</lexeme_data>'||CHR(10);
  raise_application_error(verror_code, xml);
END raise_user_exception;

PROCEDURE raise_user_exception(lexeme_id     IN locale_messages.id%TYPE,
                               lexeme_params IN TLexemeParams)
IS
BEGIN
  raise_user_exception(-20000, lexeme_id, lexeme_params);
END raise_user_exception;

PROCEDURE raise_user_exception(lexeme_id     IN locale_messages.id%TYPE)
IS
lexeme_params TLexemeParams;
BEGIN
  raise_user_exception(-20000, lexeme_id, lexeme_params);
END raise_user_exception;

END system;
/
