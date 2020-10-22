create or replace trigger TRG_WB_REF_WS_AIR_WCC_AC_HIST
AFTER
INSERT OR UPDATE
ON WB_REF_WS_AIR_WCC_AC
FOR EACH ROW
BEGIN
  if inserting then
    insert into WB_REF_WS_AIR_WCC_AC_HIST(ID_,
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
                                                               WCC_ID,
	                                                               ADJ_CODE_ID,
	                                                                 U_NAME,
		                                                                 U_IP,
		                                                                   U_HOST_NAME,
		                                                                     DATE_WRITE)
    select SEC_WB_REF_WS_AIR_WCC_AC_HIST.nextval,
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
	                               :new.WCC_ID,
	                                 :new.ADJ_CODE_ID,
	                                   :new.U_NAME,
		                               	   :new.U_IP,
		                                	   :new.U_HOST_NAME,
		                                 	     :new.DATE_WRITE
    from dual;
  end if;

  if updating then
    insert into WB_REF_WS_AIR_WCC_AC_HIST(ID_,
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
                                                               WCC_ID,
	                                                               ADJ_CODE_ID,
	                                                                 U_NAME,
		                                                                 U_IP,
		                                                                   U_HOST_NAME,
		                                                                     DATE_WRITE)
    select SEC_WB_REF_WS_AIR_WCC_AC_HIST.nextval,
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
	                               :old.WCC_ID,
	                                 :old.ADJ_CODE_ID,
	                                   :old.U_NAME,
		                               	   :old.U_IP,
		                                	   :old.U_HOST_NAME,
		                                 	     :old.DATE_WRITE
    from dual;

    insert into WB_REF_WS_AIR_WCC_AC_HIST(ID_,
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
                                                               WCC_ID,
	                                                               ADJ_CODE_ID,
	                                                                 U_NAME,
		                                                                 U_IP,
		                                                                   U_HOST_NAME,
		                                                                     DATE_WRITE)
    select SEC_WB_REF_WS_AIR_WCC_AC_HIST.nextval,
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
	                               :new.WCC_ID,
	                                 :new.ADJ_CODE_ID,
	                                   :new.U_NAME,
		                               	   :new.U_IP,
		                                	   :new.U_HOST_NAME,
		                                 	     :new.DATE_WRITE
    from dual;
  end if;
END;
/
