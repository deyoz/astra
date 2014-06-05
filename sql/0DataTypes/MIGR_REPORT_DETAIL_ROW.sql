create or replace TYPE "MIGR_REPORT_DETAIL_ROW"                                                                          AS OBJECT
 (CAPTURED_ID            NUMBER(38),
  CAPTURED_NAME          VARCHAR2(4000),
  CONVERTED_NAME          VARCHAR2(4000),
  CAPTURED_TYPE          VARCHAR2(4000),
  CONVERTED_TYPE          VARCHAR2(4000),
  CAPTURE_STATUS         VARCHAR2(20),
  CONVERT_STATUS         VARCHAR2(20),
  GENERATE_STATUS        VARCHAR2(20),
  LOGTEXT               VARCHAR2(4000)
 );

/
