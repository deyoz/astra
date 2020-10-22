create or replace trigger TRG_WB_REF_WS_AIR_EQP_GOL_HST
AFTER
INSERT OR UPDATE
ON WB_REF_WS_AIR_EQUIP_GOL
FOR EACH ROW
BEGIN
  if inserting then
    insert into WB_REF_WS_AIR_EQUIP_GOL_HST(ID_,
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
                                                                    ADV_ID,
                                                                      TYPE_ID,
	                                                                      DECK_ID,
	                                                                        DESCRIPTION,
	                                                                          MAX_WEIGHT,
	                                                                            LA_CENTROID,
	                                                                              LA_FROM,
	                                                                                LA_TO,
	                                                                                  BA_CENTROID,
	                                                                                    BA_FWD,
	                                                                                      BA_AFT,
	                                                                                        INDEX_PER_WT_UNIT,
	                                                                                          SHOW_ON_PLAN,
		                                                                                          U_NAME,
		                                                                                            U_IP,
		                                                                                              U_HOST_NAME,
		                                                                                                DATE_WRITE)
    select SEC_WB_REF_WS_AIR_EQP_GOL_HST.nextval,
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
	                                 :new.ADV_ID,
                                     :new.TYPE_ID,
	                                     :new.DECK_ID,
	                                       :new.DESCRIPTION,
	                                         :new.MAX_WEIGHT,
	                                           :new.LA_CENTROID,
	                                             :new.LA_FROM,
	                                               :new.LA_TO,
	                                                 :new.BA_CENTROID,
	                                                   :new.BA_FWD,
	                                                     :new.BA_AFT,
	                                                       :new.INDEX_PER_WT_UNIT,
	                                                         :new.SHOW_ON_PLAN,
		                         	                               :new.U_NAME,
		                               	                           :new.U_IP,
		                                	                           :new.U_HOST_NAME,
		                                 	                             :new.DATE_WRITE
    from dual;
  end if;

  if updating then
    insert into WB_REF_WS_AIR_EQUIP_GOL_HST(ID_,
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
                                                                    ADV_ID,
                                                                      TYPE_ID,
	                                                                      DECK_ID,
	                                                                        DESCRIPTION,
	                                                                          MAX_WEIGHT,
	                                                                            LA_CENTROID,
	                                                                              LA_FROM,
	                                                                                LA_TO,
	                                                                                  BA_CENTROID,
	                                                                                    BA_FWD,
	                                                                                      BA_AFT,
	                                                                                        INDEX_PER_WT_UNIT,
	                                                                                          SHOW_ON_PLAN,
		                                                                                          U_NAME,
		                                                                                            U_IP,
		                                                                                              U_HOST_NAME,
		                                                                                                DATE_WRITE)
    select SEC_WB_REF_WS_AIR_EQP_GOL_HST.nextval,
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
	                                 :old.ADV_ID,
                                     :old.TYPE_ID,
	                                     :old.DECK_ID,
	                                       :old.DESCRIPTION,
	                                         :old.MAX_WEIGHT,
	                                           :old.LA_CENTROID,
	                                             :old.LA_FROM,
	                                               :old.LA_TO,
	                                                 :old.BA_CENTROID,
	                                                   :old.BA_FWD,
	                                                     :old.BA_AFT,
	                                                       :old.INDEX_PER_WT_UNIT,
	                                                         :old.SHOW_ON_PLAN,
		                         	                               :old.U_NAME,
		                               	                           :old.U_IP,
		                                	                           :old.U_HOST_NAME,
		                                 	                             :old.DATE_WRITE
    from dual;

    insert into WB_REF_WS_AIR_EQUIP_GOL_HST(ID_,
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
                                                                    ADV_ID,
                                                                      TYPE_ID,
	                                                                      DECK_ID,
	                                                                        DESCRIPTION,
	                                                                          MAX_WEIGHT,
	                                                                            LA_CENTROID,
	                                                                              LA_FROM,
	                                                                                LA_TO,
	                                                                                  BA_CENTROID,
	                                                                                    BA_FWD,
	                                                                                      BA_AFT,
	                                                                                        INDEX_PER_WT_UNIT,
	                                                                                          SHOW_ON_PLAN,
		                                                                                          U_NAME,
		                                                                                            U_IP,
		                                                                                              U_HOST_NAME,
		                                                                                                DATE_WRITE)
    select SEC_WB_REF_WS_AIR_EQP_GOL_HST.nextval,
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
	                                 :new.ADV_ID,
                                     :new.TYPE_ID,
	                                     :new.DECK_ID,
	                                       :new.DESCRIPTION,
	                                         :new.MAX_WEIGHT,
	                                           :new.LA_CENTROID,
	                                             :new.LA_FROM,
	                                               :new.LA_TO,
	                                                 :new.BA_CENTROID,
	                                                   :new.BA_FWD,
	                                                     :new.BA_AFT,
	                                                       :new.INDEX_PER_WT_UNIT,
	                                                         :new.SHOW_ON_PLAN,
		                         	                               :new.U_NAME,
		                               	                           :new.U_IP,
		                                	                           :new.U_HOST_NAME,
		                                 	                             :new.DATE_WRITE
    from dual;
  end if;
END;
/