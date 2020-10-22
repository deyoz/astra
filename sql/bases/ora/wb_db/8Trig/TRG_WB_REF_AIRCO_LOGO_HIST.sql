create or replace trigger TRG_WB_REF_AIRCO_LOGO_HIST
AFTER
INSERT OR UPDATE
ON WB_REF_AIRCOMPANY_LOGO
FOR EACH ROW
BEGIN

  if inserting then
    insert into WB_REF_AIRCOMPANY_LOGO_HISTORY (ID_,
	                                               U_NAME_,
	                                                 U_IP_,
	                                                   U_HOST_NAME_,
	                                                     U_DATE_WRITE_,
	                                                       ACTION,
	                                                         ID,
	                                                           ID_AC_OLD,
	                                                             DATE_FROM_OLD,
	                                                               LOGO_OLD,
	                                                                 U_NAME_OLD,
	                                                                   U_IP_OLD,
	                                                                     U_HOST_NAME_OLD,
	                                                                       DATE_WRITE_OLD,
	                                                                         ID_AC_NEW,
	                                                                           DATE_FROM_NEW,
	                                                                             LOGO_NEW,
	                                                                               U_NAME_NEW,
	                                                                                 U_IP_NEW,
	                                                                                   U_HOST_NAME_NEW,
	                                                                                     DATE_WRITE_NEW,
                                                                                         LOGO_TYPE_OLD,
                                                                                           LOGO_TYPE_NEW)
        select SEC_WB_REF_AIRCO_LOGO_HIST.nextval,
                 :new.U_NAME,
	                 :new.U_IP,
	                   :new.U_HOST_NAME,
                       SYSDATE,
                         'insert',
                           :new.id,
                             :new.ID_AC,
	                             :new.DATE_FROM,
	                               :new.LOGO,	
	                                 :new.U_NAME,
	                                   :new.U_IP,
	                                     :new.U_HOST_NAME,
	                                       :new.DATE_WRITE,
                                           :new.ID_AC,
	                                           :new.DATE_FROM,
	                                             :new.LOGO,
                                                 :new.U_NAME,
	                                                 :new.U_IP,
	                                                   :new.U_HOST_NAME,
	                                                     :new.DATE_WRITE,
                                                         :new.LOGO_TYPE,
                                                           :new.LOGO_TYPE

        from dual;
  end if;

  if updating then
    insert into WB_REF_AIRCOMPANY_LOGO_HISTORY (ID_,
	                                               U_NAME_,
	                                                 U_IP_,
	                                                   U_HOST_NAME_,
	                                                     U_DATE_WRITE_,
	                                                       ACTION,
	                                                         ID,
	                                                           ID_AC_OLD,
	                                                             DATE_FROM_OLD,
	                                                               LOGO_OLD,
	                                                                 U_NAME_OLD,
	                                                                   U_IP_OLD,
	                                                                     U_HOST_NAME_OLD,
	                                                                       DATE_WRITE_OLD,
	                                                                         ID_AC_NEW,
	                                                                           DATE_FROM_NEW,
	                                                                             LOGO_NEW,
	                                                                               U_NAME_NEW,
	                                                                                 U_IP_NEW,
	                                                                                   U_HOST_NAME_NEW,
	                                                                                     DATE_WRITE_NEW,
                                                                                         LOGO_TYPE_OLD,
                                                                                           LOGO_TYPE_NEW)
        select SEC_WB_REF_AIRCO_LOGO_HIST.nextval,
                 :new.U_NAME,
	                 :new.U_IP,
	                   :new.U_HOST_NAME,
                       SYSDATE,
                         'insert',
                           :new.id,
                             :old.ID_AC,
	                             :old.DATE_FROM,
	                               :old.LOGO,	
	                                 :old.U_NAME,
	                                   :old.U_IP,
	                                     :old.U_HOST_NAME,
	                                       :old.DATE_WRITE,
                                           :new.ID_AC,
	                                           :new.DATE_FROM,
	                                             :new.LOGO,
                                                 :new.U_NAME,
	                                                 :new.U_IP,
	                                                   :new.U_HOST_NAME,
	                                                     :new.DATE_WRITE,
                                                         :old.LOGO_TYPE,
                                                           :new.LOGO_TYPE

        from dual;
  end if;

END;
/
