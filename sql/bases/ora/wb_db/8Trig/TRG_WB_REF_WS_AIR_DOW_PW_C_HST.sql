create or replace trigger TRG_WB_REF_WS_AIR_DOW_PW_C_HST
AFTER
INSERT OR UPDATE
ON WB_REF_WS_AIR_DOW_PW_CODES
FOR EACH ROW
BEGIN null;
  if inserting then
    insert into WB_REF_WS_AIR_DOW_PW_CODES_HST(ID_,
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
		                                                                 PW_CODE_NAME,
	                                                                     TOTAL_WEIGHT,
	                                                                       WEIHGT_DIFF,
	                                                                         ARM_DIFF,
	                                                                           INDEX_DIFF,
	                                                                             CODE_TYPE,
                                                                                 BY_DEFAULT,
	                                                                                 REMARKS,
		                                                                                 U_NAME,
		                                                                                   U_IP,
		                                                                                     U_HOST_NAME,
		                                                                                       DATE_WRITE)
    select SEC_WB_REF_WS_AIR_DOW_PW_C_HST.nextval,
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
		                             :new.PW_CODE_NAME,
	                                 :new.TOTAL_WEIGHT,
	                                   :new.WEIHGT_DIFF,
	                                     :new.ARM_DIFF,
	                                       :new.INDEX_DIFF,
	                                         :new.CODE_TYPE,
                                             :new.BY_DEFAULT,
	                                             :new.REMARKS,
		                                             :new.U_NAME,
		                                               :new.U_IP,
		                                                 :new.U_HOST_NAME,
		                                                   :new.DATE_WRITE
    from dual;
  end if;

  if updating then
    insert into WB_REF_WS_AIR_DOW_PW_CODES_HST(ID_,
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
		                                                                 PW_CODE_NAME,
	                                                                     TOTAL_WEIGHT,
	                                                                       WEIHGT_DIFF,
	                                                                         ARM_DIFF,
	                                                                           INDEX_DIFF,
	                                                                             CODE_TYPE,
                                                                                 BY_DEFAULT,
	                                                                                 REMARKS,
		                                                                                 U_NAME,
		                                                                                   U_IP,
		                                                                                     U_HOST_NAME,
		                                                                                       DATE_WRITE)
    select SEC_WB_REF_WS_AIR_DOW_PW_C_HST.nextval,
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
		                             :old.PW_CODE_NAME,
	                                 :old.TOTAL_WEIGHT,
	                                   :old.WEIHGT_DIFF,
	                                     :old.ARM_DIFF,
	                                       :old.INDEX_DIFF,
	                                         :old.CODE_TYPE,
                                             :old.BY_DEFAULT,
	                                             :old.REMARKS,
		                                             :old.U_NAME,
		                                               :old.U_IP,
		                                                 :old.U_HOST_NAME,
		                                                   :old.DATE_WRITE
    from dual;

    insert into WB_REF_WS_AIR_DOW_PW_CODES_HST(ID_,
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
		                                                                 PW_CODE_NAME,
	                                                                     TOTAL_WEIGHT,
	                                                                       WEIHGT_DIFF,
	                                                                         ARM_DIFF,
	                                                                           INDEX_DIFF,
	                                                                             CODE_TYPE,
                                                                                 BY_DEFAULT,
	                                                                                 REMARKS,
		                                                                                 U_NAME,
		                                                                                   U_IP,
		                                                                                     U_HOST_NAME,
		                                                                                       DATE_WRITE)
    select SEC_WB_REF_WS_AIR_DOW_PW_C_HST.nextval,
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
		                             :new.PW_CODE_NAME,
	                                 :new.TOTAL_WEIGHT,
	                                   :new.WEIHGT_DIFF,
	                                     :new.ARM_DIFF,
	                                       :new.INDEX_DIFF,
	                                         :new.CODE_TYPE,
                                             :new.BY_DEFAULT,
	                                             :new.REMARKS,
		                                             :new.U_NAME,
		                                               :new.U_IP,
		                                                 :new.U_HOST_NAME,
		                                                   :new.DATE_WRITE
    from dual;
  end if;
END;
/