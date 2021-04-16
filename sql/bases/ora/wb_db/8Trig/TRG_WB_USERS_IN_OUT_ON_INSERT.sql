create or replace trigger TRG_WB_USERS_IN_OUT_ON_INSERT
BEFORE
INSERT
ON WB_USERS_IN_OUT
FOR EACH ROW
BEGIN

  SELECT SEC_WB_USERS_IN_OUT.NEXTVAL
  INTO :new.id
  FROM  dual;

END;
/