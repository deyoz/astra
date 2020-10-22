create or replace trigger TRG_WB_REF_AC_AT_MISC_ADV_HIST
AFTER
INSERT OR UPDATE
ON WB_REF_AIRCO_MISC_ADV
FOR EACH ROW
BEGIN
  if inserting then
    insert into WB_REF_AIRCO_MISC_ADV_HIST (ID_,
	                                            U_NAME_,
	                                              U_IP_,
	                                                U_HOST_NAME_,
	                                                  DATE_WRITE_,
	                                                    ACTION_,
	                                                      ID,
	                                                        ID_AC_OLD,
	                                                          DATE_FROM_OLD,
                                                      	      REMARK_OLD,
	                                                              U_NAME_OLD,
	                                                                U_IP_OLD,
                                                      	            U_HOST_NAME_OLD,
	                                                                    DATE_WRITE_OLD,
	                                                                      ID_AC_NEW,
	                                                                        DATE_FROM_NEW,
	                                                                          REMARK_NEW,
	                                                                            U_NAME_NEW,
	                                                                              U_IP_NEW,
	                                                                                U_HOST_NAME_NEW,
	                                                                                  DATE_WRITE_NEW)
    select SEC_WB_REF_AIRC_MISC_ADV_HST.nextval,
             :new.U_NAME,
	             :new.U_IP,
	               :new.U_HOST_NAME,
                   SYSDATE,
                     'insert',
                       :new.id,
                         :new.ID_AC,
	                         :new.DATE_FROM,
                             :new.REMARK,
	                             :new.U_NAME,
	                               :new.U_IP,
	                                 :new.U_HOST_NAME,
	                                   :new.DATE_WRITE,
                                       :new.ID_AC,
	                                       :new.DATE_FROM,
                                           :new.REMARK,
	                                           :new.U_NAME,
	                                             :new.U_IP,
	                                               :new.U_HOST_NAME,
	                                                 :new.DATE_WRITE
    from dual;
  end if;

  if updating then
    insert into WB_REF_AIRCO_MISC_ADV_HIST (ID_,
	                                            U_NAME_,
	                                              U_IP_,
	                                                U_HOST_NAME_,
	                                                  DATE_WRITE_,
	                                                    ACTION_,
	                                                      ID,
	                                                        ID_AC_OLD,
	                                                          DATE_FROM_OLD,
                                                      	      REMARK_OLD,
	                                                              U_NAME_OLD,
	                                                                U_IP_OLD,
                                                      	            U_HOST_NAME_OLD,
	                                                                    DATE_WRITE_OLD,
	                                                                      ID_AC_NEW,
	                                                                        DATE_FROM_NEW,
	                                                                          REMARK_NEW,
	                                                                            U_NAME_NEW,
	                                                                              U_IP_NEW,
	                                                                                U_HOST_NAME_NEW,
	                                                                                  DATE_WRITE_NEW)
    select SEC_WB_REF_AIRC_MISC_ADV_HST.nextval,
             :new.U_NAME,
	             :new.U_IP,
	               :new.U_HOST_NAME,
                   SYSDATE,
                     'update',
                       :old.id,
                         :old.ID_AC,
	                         :old.DATE_FROM,
                             :old.REMARK,
	                             :old.U_NAME,
	                               :old.U_IP,
	                                 :old.U_HOST_NAME,
	                                   :old.DATE_WRITE,
                                       :new.ID_AC,
	                                       :new.DATE_FROM,
                                           :new.REMARK,
	                                           :new.U_NAME,
	                                             :new.U_IP,
	                                               :new.U_HOST_NAME,
	                                                 :new.DATE_WRITE
    from dual;
  end if;
END;
/
