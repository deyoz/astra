create or replace PACKAGE system
AS

SUBTYPE TEventType IS events.type%TYPE;
evtSeason       CONSTANT TEventType := 'ëÖá';
evtDisp         CONSTANT TEventType := 'Ñàë';
evtFlt          CONSTANT TEventType := 'êÖâ';
evtGraph        CONSTANT TEventType := 'Éêî';
evtPax          CONSTANT TEventType := 'èÄë';
evtPay          CONSTANT TEventType := 'éèã';
evtComp         CONSTANT TEventType := 'äåè';
evtTlg          CONSTANT TEventType := 'íãÉ';
evtAccess       CONSTANT TEventType := 'Ñëí';
evtSystem       CONSTANT TEventType := 'ëàë';
evtCodif        CONSTANT TEventType := 'äéÑ';
evtPeriod       CONSTANT TEventType := 'èêÑ';
evtProgError    CONSTANT TEventType := '!';
evtUnknown      CONSTANT TEventType := '?';

FUNCTION is_lat(str     IN VARCHAR2) RETURN BOOLEAN;

FUNCTION is_upp_let(str 	IN VARCHAR2,
                    pr_lat 	IN INTEGER DEFAULT NULL) RETURN NUMBER;
FUNCTION is_upp_let_dig(str 	IN VARCHAR2,
                        pr_lat 	IN INTEGER DEFAULT NULL) RETURN NUMBER;
FUNCTION is_dig(str 	IN VARCHAR2,
                        pr_lat 	IN INTEGER DEFAULT NULL) RETURN NUMBER;
FUNCTION invalid_char_in_name(str	IN VARCHAR2,
                              pr_lat IN INTEGER DEFAULT NULL,
                              symbols IN VARCHAR2 DEFAULT ' -') RETURN CHAR;
FUNCTION is_name(str	IN VARCHAR2,
                 pr_lat IN INTEGER DEFAULT NULL,
                 symbols IN VARCHAR2 DEFAULT ' -') RETURN NUMBER;
FUNCTION is_airline_name(str	IN VARCHAR2,
                         pr_lat IN INTEGER DEFAULT NULL) RETURN NUMBER;

FUNCTION IsLeapYear( vYear NUMBER ) RETURN NUMBER;
FUNCTION LastDayofMonth( vYear NUMBER, vMonth NUMBER ) RETURN NUMBER;
FUNCTION UTCSYSDATE RETURN DATE;
FUNCTION LOCALSYSDATE RETURN DATE;
PRAGMA RESTRICT_REFERENCES (IsLeapYear, WNDS, WNPS, RNPS);
PRAGMA RESTRICT_REFERENCES (LastDayofMonth, WNDS, WNPS, RNPS);
PRAGMA RESTRICT_REFERENCES (UTCSYSDATE, WNDS, WNPS, RNPS);
PRAGMA RESTRICT_REFERENCES (LOCALSYSDATE, WNDS, WNPS, RNPS);
PROCEDURE MsgToLog(vmsg         VARCHAR2,
                   vtype        TEventType,
                   vid1         events.id1%TYPE DEFAULT NULL,
                   vid2         events.id2%TYPE DEFAULT NULL,
                   vid3         events.id3%TYPE DEFAULT NULL);
PROCEDURE ErrorToLog(vmsg       VARCHAR2,
                   vid1         events.id1%TYPE DEFAULT NULL,
                   vid2         events.id2%TYPE DEFAULT NULL,
                   vid3         events.id3%TYPE DEFAULT NULL);

TYPE TTranslitDicts IS TABLE OF translit_dicts%ROWTYPE INDEX BY translit_dicts.letter%TYPE;
translit_dicts_t TTranslitDicts;

FUNCTION transliter(str	        IN VARCHAR2,
                    fmt 	IN INTEGER,
                    pr_lat      IN INTEGER) RETURN VARCHAR2;
FUNCTION transliter(str	IN VARCHAR2, fmt IN INTEGER) RETURN VARCHAR2;
FUNCTION transliter_equal(str1 IN VARCHAR2,
                          str2 IN VARCHAR2,
                          fmt  IN INTEGER DEFAULT NULL) RETURN NUMBER;

TYPE TLexemeParams IS TABLE OF VARCHAR2(50) INDEX BY VARCHAR2(20);

PROCEDURE raise_user_exception(verror_code   IN NUMBER,
                               lexeme_id     IN locale_messages.id%TYPE,
                               lexeme_params IN TLexemeParams);
PROCEDURE raise_user_exception(lexeme_id     IN locale_messages.id%TYPE,
                               lexeme_params IN TLexemeParams);
PROCEDURE raise_user_exception(lexeme_id     IN locale_messages.id%TYPE);

END system;
/
