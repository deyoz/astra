create or replace trigger TRG_WB_REF_WS_AIR_S_L_C_ADV_HS
AFTER
INSERT OR UPDATE
ON WB_REF_WS_AIR_S_L_C_ADV
FOR EACH ROW
BEGIN null;
  if inserting then
    insert into WB_REF_WS_AIR_S_L_C_ADV_HST(ID_,
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
                                                                  ADV_ID,
                                                                    IDN,
                                                                      DATE_FROM,
   	                                                                    CH_CAI_BALANCE_ARM,
   	                                                                      CH_CAI_INDEX_UNIT,
   	                                                                        CH_CI_BALANCE_ARM,
   	                                                                          CH_CI_INDEX_UNIT,
   	                                                                            CH_USE_AS_DEFAULT,
		                                                                              U_NAME,
		                                                                                U_IP,
		                                                                                  U_HOST_NAME,
		                                                                                    DATE_WRITE)
    select SEC_WB_REF_WS_AIR_S_L_C_ADV_HS.nextval,
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
                                 :new.ADV_ID,
                                   :new.IDN,
                                     :new.DATE_FROM,
   	                                   :new.CH_CAI_BALANCE_ARM,
   	                                     :new.CH_CAI_INDEX_UNIT,
   	                                       :new.CH_CI_BALANCE_ARM,
   	                                         :new.CH_CI_INDEX_UNIT,
   	                                           :new.CH_USE_AS_DEFAULT,
		                                             :new.U_NAME,
		                                               :new.U_IP,
		                                                 :new.U_HOST_NAME,
		                                                   :new.DATE_WRITE
    from dual;
  end if;

  if updating then
    insert into WB_REF_WS_AIR_S_L_C_ADV_HST(ID_,
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
                                                                  ADV_ID,
                                                                    IDN,
                                                                      DATE_FROM,
   	                                                                    CH_CAI_BALANCE_ARM,
   	                                                                      CH_CAI_INDEX_UNIT,
   	                                                                        CH_CI_BALANCE_ARM,
   	                                                                          CH_CI_INDEX_UNIT,
   	                                                                            CH_USE_AS_DEFAULT,
		                                                                              U_NAME,
		                                                                                U_IP,
		                                                                                  U_HOST_NAME,
		                                                                                    DATE_WRITE)
    select SEC_WB_REF_WS_AIR_S_L_C_ADV_HS.nextval,
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
		                             :old.ADV_ID,
                                   :old.IDN,
                                     :old.DATE_FROM,
   	                                   :old.CH_CAI_BALANCE_ARM,
   	                                     :old.CH_CAI_INDEX_UNIT,
   	                                       :old.CH_CI_BALANCE_ARM,
   	                                         :old.CH_CI_INDEX_UNIT,
   	                                           :old.CH_USE_AS_DEFAULT,
		                                             :old.U_NAME,
		                                               :old.U_IP,
		                                                 :old.U_HOST_NAME,
		                                                   :old.DATE_WRITE
    from dual;

    insert into WB_REF_WS_AIR_S_L_C_ADV_HST(ID_,
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
                                                                  ADV_ID,
                                                                    IDN,
                                                                      DATE_FROM,
   	                                                                    CH_CAI_BALANCE_ARM,
   	                                                                      CH_CAI_INDEX_UNIT,
   	                                                                        CH_CI_BALANCE_ARM,
   	                                                                          CH_CI_INDEX_UNIT,
   	                                                                            CH_USE_AS_DEFAULT,
		                                                                              U_NAME,
		                                                                                U_IP,
		                                                                                  U_HOST_NAME,
		                                                                                    DATE_WRITE)
    select SEC_WB_REF_WS_AIR_S_L_C_ADV_HS.nextval,
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
                                 :new.ADV_ID,
                                   :new.IDN,
                                     :new.DATE_FROM,
   	                                   :new.CH_CAI_BALANCE_ARM,
   	                                     :new.CH_CAI_INDEX_UNIT,
   	                                       :new.CH_CI_BALANCE_ARM,
   	                                         :new.CH_CI_INDEX_UNIT,
   	                                           :new.CH_USE_AS_DEFAULT,
		                                             :new.U_NAME,
		                                               :new.U_IP,
		                                                 :new.U_HOST_NAME,
		                                                   :new.DATE_WRITE
    from dual;
  end if;
END;
/