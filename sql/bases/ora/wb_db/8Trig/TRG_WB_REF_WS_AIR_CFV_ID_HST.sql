create or replace trigger TRG_WB_REF_WS_AIR_CFV_ID_HST
AFTER
INSERT OR UPDATE
ON WB_REF_WS_AIR_CFV_IDN
FOR EACH ROW
BEGIN null;
  if inserting then
    insert into WB_REF_WS_AIR_CFV_IDN_HST(ID_,
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
		                                                            TABLE_NAME,
		                                                              U_NAME,
		                                                                U_IP,
		                                                                  U_HOST_NAME,
		                                                                    DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CFV_IDN_HIST.nextval,
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
		                             :new.TABLE_NAME,
		                               :new.U_NAME,
		                                 :new.U_IP,
		                                   :new.U_HOST_NAME,
		                                     :new.DATE_WRITE
    from dual;
  end if;

  if updating then
    insert into WB_REF_WS_AIR_CFV_IDN_HST(ID_,
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
		                                                            TABLE_NAME,
		                                                              U_NAME,
		                                                                U_IP,
		                                                                  U_HOST_NAME,
		                                                                    DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CFV_IDN_HIST.nextval,
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
		                             :old.TABLE_NAME,
		                               :old.U_NAME,
		                                 :old.U_IP,
		                                   :old.U_HOST_NAME,
		                                     :old.DATE_WRITE
    from dual;

    insert into WB_REF_WS_AIR_CFV_IDN_HST(ID_,
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
		                                                            TABLE_NAME,
		                                                              U_NAME,
		                                                                U_IP,
		                                                                  U_HOST_NAME,
		                                                                    DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CFV_IDN_HIST.nextval,
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
		                             :new.TABLE_NAME,
		                               :new.U_NAME,
		                                 :new.U_IP,
		                                   :new.U_HOST_NAME,
		                                     :new.DATE_WRITE
    from dual;
  end if;
END;
/
