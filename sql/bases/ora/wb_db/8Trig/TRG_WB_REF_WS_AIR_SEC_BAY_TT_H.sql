create or replace trigger TRG_WB_REF_WS_AIR_SEC_BAY_TT_H
AFTER
INSERT OR UPDATE
ON WB_REF_WS_AIR_SEC_BAY_TT
FOR EACH ROW
BEGIN
  if inserting then
    insert into WB_REF_WS_AIR_SEC_BAY_TT_HST(ID_,
	                                            U_NAME_,
	                                              U_IP_,
	                                                U_HOST_NAME_,
	                                                  DATE_WRITE_,
                                                      OPERATION_,
	                                                      ACTION_,
	                                                        ID,
	                                                          ID_AC,
	                                                            ID_WS,
		                                                            ID_BORT,
                                                                  T_ID,
	                                                                  ULD_IATA_ID,
	                                                                    U_NAME,
		                                                                    U_IP,
		                                                                      U_HOST_NAME,
		                                                                        DATE_WRITE)
    select SEC_WB_REF_WS_AIR_SEC_BAY_TT_H.nextval,
             :new.U_NAME,
	             :new.U_IP,
	               :new.U_HOST_NAME,
                   SYSDATE(),
                     'insert',
                       'insert',
                         :new.id,
                           :new.ID_AC,
	                           :new.ID_WS,
		                           :new.ID_BORT,		
	                               :new.T_ID,
	                                 :new.ULD_IATA_ID,
	                                   :new.U_NAME,
		                               	   :new.U_IP,
		                                	   :new.U_HOST_NAME,
		                                 	     :new.DATE_WRITE
    from dual;
  end if;

  if updating then
    insert into WB_REF_WS_AIR_SEC_BAY_TT_HST(ID_,
	                                            U_NAME_,
	                                              U_IP_,
	                                                U_HOST_NAME_,
	                                                  DATE_WRITE_,
                                                      OPERATION_,
	                                                      ACTION_,
	                                                        ID,
	                                                          ID_AC,
	                                                            ID_WS,
		                                                            ID_BORT,
                                                                  T_ID,
	                                                                  ULD_IATA_ID,
	                                                                    U_NAME,
		                                                                    U_IP,
		                                                                      U_HOST_NAME,
		                                                                        DATE_WRITE)
    select SEC_WB_REF_WS_AIR_SEC_BAY_TT_H.nextval,
             :new.U_NAME,
	             :new.U_IP,
	               :new.U_HOST_NAME,
                   SYSDATE(),
                     'update',
                       'delete',
                         :old.id,
                           :old.ID_AC,
	                           :old.ID_WS,
		                           :old.ID_BORT,		
	                               :old.T_ID,
	                                 :old.ULD_IATA_ID,
	                                   :old.U_NAME,
		                               	   :old.U_IP,
		                                	   :old.U_HOST_NAME,
		                                 	     :old.DATE_WRITE
    from dual;

    insert into WB_REF_WS_AIR_SEC_BAY_TT_HST(ID_,
	                                            U_NAME_,
	                                              U_IP_,
	                                                U_HOST_NAME_,
	                                                  DATE_WRITE_,
                                                      OPERATION_,
	                                                      ACTION_,
	                                                        ID,
	                                                          ID_AC,
	                                                            ID_WS,
		                                                            ID_BORT,
                                                                  T_ID,
	                                                                  ULD_IATA_ID,
	                                                                    U_NAME,
		                                                                    U_IP,
		                                                                      U_HOST_NAME,
		                                                                        DATE_WRITE)
    select SEC_WB_REF_WS_AIR_SEC_BAY_TT_H.nextval,
             :new.U_NAME,
	             :new.U_IP,
	               :new.U_HOST_NAME,
                   SYSDATE(),
                     'update',
                       'insert',
                         :new.id,
                           :new.ID_AC,
	                           :new.ID_WS,
		                           :new.ID_BORT,		
	                               :new.T_ID,
	                                 :new.ULD_IATA_ID,
	                                   :new.U_NAME,
		                               	   :new.U_IP,
		                                	   :new.U_HOST_NAME,
		                                 	     :new.DATE_WRITE
    from dual;
  end if;
END;
/
