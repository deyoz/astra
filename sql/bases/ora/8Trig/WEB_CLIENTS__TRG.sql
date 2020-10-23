create or replace trigger WEB_CLIENTS__TRG
BEFORE
INSERT OR UPDATE
ON WEB_CLIENTS
FOR EACH ROW
DECLARE n NUMBER;
BEGIN
  IF :new.client_type IN ('MOBIL','WEB','KIOSK') THEN
    BEGIN
      n:=TO_NUMBER(:new.client_id);
    EXCEPTION
      WHEN OTHERS THEN
        raise_application_error(-20000,'CLIENT_ID must be number for CLIENT_TYPE='''||:new.client_type||'''');
    END;
  END IF;
END;

/
