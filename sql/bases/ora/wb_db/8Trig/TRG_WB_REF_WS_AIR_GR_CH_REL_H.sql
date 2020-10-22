create or replace trigger TRG_WB_REF_WS_AIR_GR_CH_REL_H
AFTER
INSERT OR UPDATE
ON WB_REF_WS_AIR_GR_CH_REL
FOR EACH ROW
BEGIN
  if inserting then
    insert into WB_REF_WS_AIR_GR_CH_REL_H (ID_,
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
		                                                             IDN,
	                                                                 IDN_REL,
	                                                                   U_NAME,
	                                                                     U_IP,
	                                                                       U_HOST_NAME,
	                                                                         DATE_WRITE)
    select SEC_WB_REF_WS_AIR_GR_CH_REL_H.nextval,
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
		                             :new.IDN,
	                                 :new.IDN_REL,	
	                                   :new.U_NAME,
		                               	   :new.U_IP,
		                                	   :new.U_HOST_NAME,
		                                 	     :new.DATE_WRITE
    from dual;
  end if;

  if updating then
    insert into WB_REF_WS_AIR_GR_CH_REL_H (ID_,
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
		                                                             IDN,
	                                                                 IDN_REL,
	                                                                   U_NAME,
	                                                                     U_IP,
	                                                                       U_HOST_NAME,
	                                                                         DATE_WRITE)
    select SEC_WB_REF_WS_AIR_GR_CH_REL_H.nextval,
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
		                             :old.IDN,
	                                 :old.IDN_REL,
	                                   :old.U_NAME,
		                               	   :old.U_IP,
		                                	   :old.U_HOST_NAME,
		                                 	     :old.DATE_WRITE
    from dual;

    insert into WB_REF_WS_AIR_GR_CH_REL_H (ID_,
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
		                                                             IDN,
	                                                                 IDN_REL,
	                                                                   U_NAME,
	                                                                     U_IP,
	                                                                       U_HOST_NAME,
	                                                                         DATE_WRITE)
    select SEC_WB_REF_WS_AIR_GR_CH_REL_H.nextval,
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
		                             :new.IDN,
	                                 :new.IDN_REL,	
	                                   :new.U_NAME,
		                               	   :new.U_IP,
		                                	   :new.U_HOST_NAME,
		                                 	     :new.DATE_WRITE
    from dual;
  end if;
END;
/
