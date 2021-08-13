create or replace PACKAGE BODY system
AS

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
         NOT c BETWEEN '€' AND 'Ÿ' AND
         c<>'ð' THEN RETURN 0; END IF;
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
         NOT c BETWEEN '€' AND 'Ÿ' AND
         c<>'ð' AND
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
         NOT c BETWEEN '€' AND 'Ÿ' AND
         c<>'ð'AND
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

PROCEDURE raise_user_exception(verror_code   IN NUMBER,
                               lexeme_id     IN VARCHAR2,
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

PROCEDURE raise_user_exception(lexeme_id     IN VARCHAR2,
                               lexeme_params IN TLexemeParams)
IS
BEGIN
  raise_user_exception(-20000, lexeme_id, lexeme_params);
END raise_user_exception;

PROCEDURE raise_user_exception(lexeme_id     IN VARCHAR2)
IS
lexeme_params TLexemeParams;
BEGIN
  raise_user_exception(-20000, lexeme_id, lexeme_params);
END raise_user_exception;

END system;
/
