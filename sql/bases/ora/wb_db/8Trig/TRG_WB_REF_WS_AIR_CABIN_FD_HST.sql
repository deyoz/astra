create or replace trigger TRG_WB_REF_WS_AIR_CABIN_FD_HST
AFTER
INSERT OR UPDATE
ON WB_REF_WS_AIR_CABIN_FD
FOR EACH ROW
BEGIN
  if inserting then
    insert into WB_REF_WS_AIR_CABIN_FD_HST(ID_,
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
                                                                     FCL_ID,
	                                                                     MAX_NUM_SEATS,
	                                                                       LA_CENTROID,
	                                                                         BA_CENTROID,
	                                                                           INDEX_PER_WT_UNIT,
		                                                                           U_NAME,
		                                                                             U_IP,
		                                                                               U_HOST_NAME,
		                                                                                 DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CABIN_FD_HST.nextval,
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
                                     :new.FCL_ID,
	                                     :new.MAX_NUM_SEATS,
	                                       :new.LA_CENTROID,
	                                         :new.BA_CENTROID,
	                                           :new.INDEX_PER_WT_UNIT,
		                         	                 :new.U_NAME,
		                               	             :new.U_IP,
		                                	             :new.U_HOST_NAME,
		                                 	               :new.DATE_WRITE
    from dual;
  end if;

  if updating then
    insert into WB_REF_WS_AIR_CABIN_FD_HST(ID_,
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
                                                                     FCL_ID,
	                                                                     MAX_NUM_SEATS,
	                                                                       LA_CENTROID,
	                                                                         BA_CENTROID,
	                                                                           INDEX_PER_WT_UNIT,
		                                                                           U_NAME,
		                                                                             U_IP,
		                                                                               U_HOST_NAME,
		                                                                                 DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CABIN_FD_HST.nextval,
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
                                     :new.FCL_ID,
	                                     :new.MAX_NUM_SEATS,
	                                       :new.LA_CENTROID,
	                                         :new.BA_CENTROID,
	                                           :new.INDEX_PER_WT_UNIT,
		                         	                 :old.U_NAME,
		                               	             :old.U_IP,
		                                	             :old.U_HOST_NAME,
		                                 	               :old.DATE_WRITE
    from dual;

    insert into WB_REF_WS_AIR_CABIN_FD_HST(ID_,
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
                                                                     FCL_ID,
	                                                                     MAX_NUM_SEATS,
	                                                                       LA_CENTROID,
	                                                                         BA_CENTROID,
	                                                                           INDEX_PER_WT_UNIT,
		                                                                           U_NAME,
		                                                                             U_IP,
		                                                                               U_HOST_NAME,
		                                                                                 DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CABIN_FD_HST.nextval,
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
                                     :new.FCL_ID,
	                                     :new.MAX_NUM_SEATS,
	                                       :new.LA_CENTROID,
	                                         :new.BA_CENTROID,
	                                           :new.INDEX_PER_WT_UNIT,
		                         	                 :new.U_NAME,
		                               	             :new.U_IP,
		                                	             :new.U_HOST_NAME,
		                                 	               :new.DATE_WRITE
    from dual;
  end if;
END;
/