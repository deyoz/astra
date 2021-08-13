create or replace PACKAGE system
AS

SUBTYPE TEventType IS events_bilingual.type%TYPE;
evtSeason       CONSTANT TEventType := '���';
evtDisp         CONSTANT TEventType := '���';
evtFlt          CONSTANT TEventType := '���';
evtGraph        CONSTANT TEventType := '���';
evtFltTask      CONSTANT TEventType := '���';
evtPax          CONSTANT TEventType := '���';
evtPay          CONSTANT TEventType := '���';
evtComp         CONSTANT TEventType := '���';
evtTlg          CONSTANT TEventType := '���';
evtAccess       CONSTANT TEventType := '���';
evtSystem       CONSTANT TEventType := '���';
evtCodif        CONSTANT TEventType := '���';
evtPeriod       CONSTANT TEventType := '���';
evtPrn          CONSTANT TEventType := '���';
evtProgError    CONSTANT TEventType := '!';
evtUnknown      CONSTANT TEventType := '?';

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

FUNCTION UTCSYSDATE RETURN DATE;
FUNCTION LOCALSYSDATE RETURN DATE;

PRAGMA RESTRICT_REFERENCES (UTCSYSDATE, WNDS, WNPS, RNPS);
PRAGMA RESTRICT_REFERENCES (LOCALSYSDATE, WNDS, WNPS, RNPS);

TYPE TLexemeParams IS TABLE OF VARCHAR2(50) INDEX BY VARCHAR2(20);

PROCEDURE raise_user_exception(verror_code   IN NUMBER,
                               lexeme_id     IN VARCHAR2,
                               lexeme_params IN TLexemeParams);
PROCEDURE raise_user_exception(lexeme_id     IN VARCHAR2,
                               lexeme_params IN TLexemeParams);
PROCEDURE raise_user_exception(lexeme_id     IN VARCHAR2);

END system;
/
