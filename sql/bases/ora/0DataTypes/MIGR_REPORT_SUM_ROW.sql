create or replace TYPE        "MIGR_REPORT_SUM_ROW"                                          AS OBJECT
       (LABEL           VARCHAR2(50),
	    SCHEMA_SUM      NUMBER,
        TABLE_SUM       NUMBER,
		INDEX_SUM       NUMBER,
		CONSTRAINT_SUM  NUMBER,
		VIEW_SUM        NUMBER,
		TRIGGER_SUM     NUMBER,
		SP_SUM          NUMBER		
        );
/
