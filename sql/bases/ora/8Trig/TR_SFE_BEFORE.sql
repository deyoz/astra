create or replace trigger TR_SFE_BEFORE
BEFORE
INSERT OR UPDATE OR DELETE
ON SFE
BEGIN
  SFEAERPACK.SFES:=SFEAERPACK.empty;
END;





/