create or replace trigger TRG_WB_REF_AIRCO_ADV_INFO_HIST
AFTER
INSERT OR UPDATE
ON WB_REF_AIRCOMPANY_ADV_INFO
FOR EACH ROW
BEGIN

  if inserting then
    insert into WB_REF_AIRCO_ADV_INFO_HISTORY (ID_,
	                                               U_NAME_,
	                                                 U_IP_,
	                                                   U_HOST_NAME_,
	                                                     DATE_WRITE_,
	                                                       ACTION,
	                                                         ID,
	                                                           ID_AC_OLD,
	                                                             DATE_FROM_OLD,
	                                                               ID_CITY_OLD,
	                                                                 IATA_CODE_OLD,
	                                                                   ICAO_CODE_OLD,
                                                                       OTHER_CODE_OLD,
                                                               	         NAME_RUS_SMALL_OLD,
	                                                                         NAME_RUS_FULL_OLD,
	                                                                           NAME_ENG_SMALL_OLD,
	                                                                             NAME_ENG_FULL_OLD,
	                                                                               U_NAME_OLD,
	                                                                                 U_IP_OLD,
	                                                                                   U_HOST_NAME_OLD,
	                                                                                     DATE_WRITE_OLD,
	                                                                                       ID_AC_NEW,
	                                                                                         DATE_FOM_NEW,
	                                                                                           ID_CITY_NEW,
	                                                                                             IATA_CODE_NEW,
	                                                                                               ICAO_CODE_NEW,
	                                                                                                 OTHER_CODE_NEW,
	                                                                                                   NAME_RUS_SMALL_NEW,
	                                                                                                     NAME_RUS_FULL_NEW,
	                                                                                                       NAME_ENG_SMALL_NEW,
	                                                                                                         NAME_ENG_FULL_NEW,
	                                                                                                           U_NAME_NEW,
	                                                                                                             U_IP_NEW,
	                                                                                                               U_HOST_NAME_NEW,
	                                                                                                                 DATE_WRITE_NEW,
                                                                                                                     remark_old,
                                                                                                                       REMARK_NEW)
        select SEC_WB_REF_AIRCO_ADV_INFO_HIST.nextval,
                 :new.U_NAME,
	                 :new.U_IP,
	                   :new.U_HOST_NAME,
                       SYSDATE,
                         'insert',
                           :new.id,
                             :new.ID_AC,
	                             :new.DATE_FROM,
	                               :new.ID_CITY,
	                                 :new.IATA_CODE,
	                                   :new.ICAO_CODE,
	                                     :new.OTHER_CODE,
	                                       :new.NAME_RUS_SMALL,
	                                         :new.NAME_RUS_FULL,
	                                           :new.NAME_ENG_SMALL,
	                                             :new.NAME_ENG_FULL,
	                                               :new.U_NAME,
	                                                 :new.U_IP,
	                                                   :new.U_HOST_NAME,
	                                                     :new.DATE_WRITE,
                                                         :new.ID_AC,
	                                                         :new.DATE_FROM,
	                                                           :new.ID_CITY,
	                                                             :new.IATA_CODE,
	                                                               :new.ICAO_CODE,
	                                                                 :new.OTHER_CODE,
	                                                                   :new.NAME_RUS_SMALL,
	                                                                     :new.NAME_RUS_FULL,
	                                                                       :new.NAME_ENG_SMALL,
	                                                                         :new.NAME_ENG_FULL,
	                                                                           :new.U_NAME,
	                                                                             :new.U_IP,
	                                                                               :new.U_HOST_NAME,
	                                                                                 :new.DATE_WRITE,
                                                                                     :new.remark,
                                                                                       :new.remark

        from dual;
  end if;

  if updating then
    insert into WB_REF_AIRCO_ADV_INFO_HISTORY (ID_,
	                                               U_NAME_,
	                                                 U_IP_,
	                                                   U_HOST_NAME_,
	                                                     DATE_WRITE_,
	                                                       ACTION,
	                                                         ID,
	                                                           ID_AC_OLD,
	                                                             DATE_FROM_OLD,
	                                                               ID_CITY_OLD,
	                                                                 IATA_CODE_OLD,
	                                                                   ICAO_CODE_OLD,
                                                                       OTHER_CODE_OLD,
                                                               	         NAME_RUS_SMALL_OLD,
	                                                                         NAME_RUS_FULL_OLD,
	                                                                           NAME_ENG_SMALL_OLD,
	                                                                             NAME_ENG_FULL_OLD,
	                                                                               U_NAME_OLD,
	                                                                                 U_IP_OLD,
	                                                                                   U_HOST_NAME_OLD,
	                                                                                     DATE_WRITE_OLD,
	                                                                                       ID_AC_NEW,
	                                                                                         DATE_FOM_NEW,
	                                                                                           ID_CITY_NEW,
	                                                                                             IATA_CODE_NEW,
	                                                                                               ICAO_CODE_NEW,
	                                                                                                 OTHER_CODE_NEW,
	                                                                                                   NAME_RUS_SMALL_NEW,
	                                                                                                     NAME_RUS_FULL_NEW,
	                                                                                                       NAME_ENG_SMALL_NEW,
	                                                                                                         NAME_ENG_FULL_NEW,
	                                                                                                           U_NAME_NEW,
	                                                                                                             U_IP_NEW,
	                                                                                                               U_HOST_NAME_NEW,
	                                                                                                                 DATE_WRITE_NEW,
                                                                                                                     remark_old,
                                                                                                                       REMARK_NEW)
        select SEC_WB_REF_AIRCO_ADV_INFO_HIST.nextval,
                 :new.U_NAME,
	                 :new.U_IP,
	                   :new.U_HOST_NAME,
                       SYSDATE,
                         'update',
                           :old.id,
                             :old.ID_AC,
	                             :old.DATE_FROM,
	                               :old.ID_CITY,
	                                 :old.IATA_CODE,
	                                   :old.ICAO_CODE,
	                                     :old.OTHER_CODE,
	                                       :old.NAME_RUS_SMALL,
	                                         :old.NAME_RUS_FULL,
	                                           :old.NAME_ENG_SMALL,
	                                             :old.NAME_ENG_FULL,
	                                               :old.U_NAME,
	                                                 :old.U_IP,
	                                                   :old.U_HOST_NAME,
	                                                     :old.DATE_WRITE,
                                                         :new.ID_AC,
	                                                         :new.DATE_FROM,
	                                                           :new.ID_CITY,
	                                                             :new.IATA_CODE,
	                                                               :new.ICAO_CODE,
	                                                                 :new.OTHER_CODE,
	                                                                   :new.NAME_RUS_SMALL,
	                                                                     :new.NAME_RUS_FULL,
	                                                                       :new.NAME_ENG_SMALL,
	                                                                         :new.NAME_ENG_FULL,
	                                                                           :new.U_NAME,
	                                                                             :new.U_IP,
	                                                                               :new.U_HOST_NAME,
	                                                                                 :new.DATE_WRITE,
                                                                                     :old.remark,
                                                                                       :new.remark

        from dual;
  end if;

END;
/
