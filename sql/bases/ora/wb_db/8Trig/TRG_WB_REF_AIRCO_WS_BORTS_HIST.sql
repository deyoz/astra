create or replace trigger TRG_WB_REF_AIRCO_WS_BORTS_HIST
AFTER
INSERT OR UPDATE
ON WB_REF_AIRCO_WS_BORTS
FOR EACH ROW
BEGIN
  if inserting then
    insert into WB_REF_AIRCO_WS_BORTS_HIST (ID_,
	                                            U_NAME_,
	                                              U_IP_,
	                                                U_HOST_NAME_,
	                                                  DATE_WRITE_,
	                                                    ACTION_,
	                                                      ID,
                                                          ID_AC_OLD,
	                                                          ID_WS_OLD,
                                                              BORT_NUM_OLD,
	                                                              U_NAME_OLD,
	                                                                U_IP_OLD,
                                                      	            U_HOST_NAME_OLD,
	                                                                    DATE_WRITE_OLD,
                                                                        ID_AC_NEW,
	                                                                        ID_WS_NEW,
                                                                            BORT_NUM_NEW,
	                                                                            U_NAME_NEW,
	                                                                              U_IP_NEW,
	                                                                                U_HOST_NAME_NEW,
	                                                                                  DATE_WRITE_NEW)
    select SEC_WB_REF_AIRCO_WS_BORTS_HIST.nextval,
             :new.U_NAME,
	             :new.U_IP,
	               :new.U_HOST_NAME,
                   SYSDATE(),
                     'insert',
                       :new.id,
                         :new.ID_AC,
                           :new.ID_WS,
                             :new.BORT_NUM,
                               :new.U_NAME,
	                               :new.U_IP,
                                   :new.U_HOST_NAME,
	                                   :new.DATE_WRITE,
                                       :new.ID_AC,
                                         :new.ID_WS,
                                           :new.BORT_NUM,
                                             :new.U_NAME,
	                                             :new.U_IP,
                                                 :new.U_HOST_NAME,
	                                                 :new.DATE_WRITE
    from dual;
  end if;

  if updating then
    insert into WB_REF_AIRCO_WS_BORTS_HIST (ID_,
	                                            U_NAME_,
	                                              U_IP_,
	                                                U_HOST_NAME_,
	                                                  DATE_WRITE_,
	                                                    ACTION_,
	                                                      ID,
                                                          ID_AC_OLD,
	                                                          ID_WS_OLD,
                                                              BORT_NUM_OLD,
	                                                              U_NAME_OLD,
	                                                                U_IP_OLD,
                                                      	            U_HOST_NAME_OLD,
	                                                                    DATE_WRITE_OLD,
                                                                        ID_AC_NEW,
	                                                                        ID_WS_NEW,
                                                                            BORT_NUM_NEW,
	                                                                            U_NAME_NEW,
	                                                                              U_IP_NEW,
	                                                                                U_HOST_NAME_NEW,
	                                                                                  DATE_WRITE_NEW)
    select SEC_WB_REF_AIRCO_WS_BORTS_HIST.nextval,
             :new.U_NAME,
	             :new.U_IP,
	               :new.U_HOST_NAME,
                   SYSDATE(),
                     'update',
                       :old.id,
                         :old.ID_AC,
                           :old.ID_WS,
                             :old.BORT_NUM,
                               :old.U_NAME,
	                               :old.U_IP,
                                   :old.U_HOST_NAME,
	                                   :old.DATE_WRITE,
                                       :new.ID_AC,
                                         :new.ID_WS,
                                           :new.BORT_NUM,
                                             :new.U_NAME,
	                                             :new.U_IP,
                                                 :new.U_HOST_NAME,
	                                                 :new.DATE_WRITE
    from dual;
  end if;
END;
/
