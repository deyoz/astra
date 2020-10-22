create or replace trigger TRG_WB_REF_AC_C_DATA_DTM_HIST
AFTER
INSERT OR UPDATE
ON WB_REF_AIRCO_C_DATA_DTM
FOR EACH ROW
BEGIN

  if inserting then
    insert into WB_REF_AIRCO_C_DATA_DTM_HIST (ID_,
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
	                                                                                     DATE_WRITE_NEW,
	                                                                                       DTM_ID_OLD,
	                                                                                         DTM_ID_NEW)
        select SEC_WB_REF_AIRCO_C_DATA_2_HIST.nextval,
                 :new.U_NAME,
	                 :new.U_IP,
	                   :new.U_HOST_NAME,
                       SYSDATE(),
                         'insert',
                           :new.id,
                             :new.id_ac,
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
	                                                     :new.DATE_WRITE,
	                                                       :new.DTM_ID,
	                                                         :new.DTM_ID
        from dual;
  end if;

  if updating then
     insert into WB_REF_AIRCO_C_DATA_DTM_HIST (ID_,
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
	                                                                                     DATE_WRITE_NEW,
	                                                                                       DTM_ID_OLD,
	                                                                                         DTM_ID_NEW)
        select SEC_WB_REF_AIRCO_C_DATA_2_HIST.nextval,
                 :new.U_NAME,
	                 :new.U_IP,
	                   :new.U_HOST_NAME,
                       SYSDATE(),
                         'updatet',
                           :old.id,
                             :old.id_ac,
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
	                                                     :new.DATE_WRITE,
	                                                       :new.DTM_ID,
	                                                         :new.DTM_ID
        from dual;
  end if;

END;
/
