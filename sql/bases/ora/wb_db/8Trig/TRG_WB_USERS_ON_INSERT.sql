create or replace trigger TRG_WB_USERS_ON_INSERT
BEFORE
INSERT
ON WB_USERS
FOR EACH ROW
BEGIN

  SELECT SEC_WB_USERS.NEXTVAL
  INTO :new.id
  FROM  dual;

END;
/
