create or replace procedure SP_WB_REF_AIRCO_KEY_DELETE
(cXML_in in clob,
   cXML_out out clob)
as
id number:=-1;
id_adv number:=-1;
id_hist number:=-1;
lang varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg varchar2(1000):=null;

ac_ws_XML clob:=null;
TYPE EXIST_REF_REC IS REF CURSOR;
CUR_EXIST_REF_REC EXIST_REF_REC;
EXIST_REF_ID_AC number;
EXIST_REF_ID_WS number;
EMPTY_PAR clob;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/lang[1]') into lang from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    if (str_msg is null) then
      begin
        insert into WB_TEMP_XML_ID (ID, num)
        select f.id,
                 f.id
        from (select to_number(extractValue(value(t),'list/id[1]')) as id
              from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

       -------------------------------------------------------------------------
       ------------------------ìÑÄãÖçàÖ àçîõ èé Çë------------------------------
       ac_ws_XML:='<?xml version="1.0" ?><root>';

    ac_ws_XML:=ac_ws_XML||'<user>';
    ac_ws_XML:=ac_ws_XML||'<P_U_NAME>'||WB_CLEAR_XML(P_U_NAME)||'</P_U_NAME>'||
                          '<P_U_IP>'||WB_CLEAR_XML(P_U_IP)||'</P_U_IP>'||
                          '<P_U_COMP_NAME>'||WB_CLEAR_XML(P_U_COMP_NAME)||'</P_U_COMP_NAME>'||
                          '<P_U_HOST_NAME>'||WB_CLEAR_XML(P_U_HOST_NAME)||'</P_U_HOST_NAME>';
    ac_ws_XML:=ac_ws_XML||'</user>';

    ac_ws_XML:=ac_ws_XML||'<optons>';
    ac_ws_XML:=ac_ws_XML||'<P_INT_TRAN>1</P_INT_TRAN>';
    ac_ws_XML:=ac_ws_XML||'</optons>';


    open CUR_EXIST_REF_REC
    for 'select distinct ac.id_ac,'||
                        'ac.id_ws '||
        'from WB_TEMP_XML_ID t join WB_REF_AIRCO_WS_TYPES ac '||
        'on t.id=ac.id_ac';

    LOOP
      FETCH CUR_EXIST_REF_REC INTO EXIST_REF_ID_AC,
                                     EXIST_REF_ID_WS;
      EXIT WHEN CUR_EXIST_REF_REC%NOTFOUND;

      ac_ws_XML:=ac_ws_XML||'<list>'||
                            '<ID_AC>'||to_char(EXIST_REF_ID_AC)||'</ID_AC>'||
                            '<ID_WS>'||to_char(EXIST_REF_ID_WS)||'</ID_WS>'||
                            '</list>';
    END LOOP;
    CLOSE CUR_EXIST_REF_REC;

    ac_ws_XML:=ac_ws_XML||'</root>';

    SP_WB_REF_AC_WS_DELETE(ac_ws_XML,
                             EMPTY_PAR);
       ------------------------ìÑÄãÖçàÖ àçîõ èé Çë------------------------------
       -------------------------------------------------------------------------

        insert into WB_REF_AIRCOMPANY_KEY_HISTORY (ID_,
	                                                    U_NAME_,
	                                                      U_IP_,
	                                                        U_HOST_NAME_,
	                                                          DATE_WRITE_,
	                                                            ACTION,
	                                                              ID,
	                                                                U_NAME_OLD,
	                                                                  U_IP_OLD,
	                                                                    U_HOST_NAME_OLD,
	                                                                      DATE_WRITE_OLD,
	                                                                        U_NAME_NEW,
	                                                                          U_IP_NEW,
	                                                                            U_HOST_NAME_NEW,
	                                                                              DATE_WRITE_NEW)
        select SEC_WB_REF_AIRCOMPANY_KEY_HIST.nextval,
                   P_U_NAME,
	                   P_U_IP,
	                     P_U_HOST_NAME,
                         SYSDATE,
                           'delete',
                             k.id,
                               U_NAME,
	                               U_IP,
	                                 U_HOST_NAME,
                                     DATE_WRITE,
                                       U_NAME,
	                                       U_IP,
	                                         U_HOST_NAME,
                                             DATE_WRITE
        from WB_TEMP_XML_ID t join WB_REF_AIRCOMPANY_KEY k
        on t.id=k.id;

        delete from WB_REF_AIRCOMPANY_KEY
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_AIRCOMPANY_KEY.id);

        ------------------------------------------------------------------------
        -----------------óàëíàå ëÇüáÄççõÖ íÄÅãàñõ-------------------------------
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
                                                                                                                           remark_new)
        select SEC_WB_REF_AIRCO_ADV_INFO_HIST.nextval,
                 P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE,
                         'delete',
                           d.id,
                             d.ID_AC,
	                             d.DATE_FROM,
	                               d.ID_CITY,
	                                 d.IATA_CODE,
	                                   d.ICAO_CODE,
	                                     d.OTHER_CODE,
	                                       d.NAME_RUS_SMALL,
	                                         d.NAME_RUS_FULL,
	                                           d.NAME_ENG_SMALL,
	                                             d.NAME_ENG_FULL,
	                                               d.U_NAME,
	                                                 d.U_IP,
	                                                   d.U_HOST_NAME,
	                                                     d.DATE_WRITE,
                                                         d.ID_AC,
	                                                         d.DATE_FROM,
	                                                           d.ID_CITY,
	                                                             d.IATA_CODE,
	                                                               d.ICAO_CODE,
	                                                                 d.OTHER_CODE,
	                                                                   d.NAME_RUS_SMALL,
	                                                                     d.NAME_RUS_FULL,
	                                                                       d.NAME_ENG_SMALL,
	                                                                         d.NAME_ENG_FULL,
	                                                                           d.U_NAME,
	                                                                             d.U_IP,
	                                                                               d.U_HOST_NAME,
	                                                                                 d.DATE_WRITE,
                                                                                     d.remark,
                                                                                       d.remark

        from WB_TEMP_XML_ID t join WB_REF_AIRCOMPANY_ADV_INFO d
        on d.id_ac=t.id;

        delete from WB_REF_AIRCOMPANY_ADV_INFO
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_AIRCOMPANY_ADV_INFO.id_ac);

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
                 P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE,
                         'delete',
                           d.id,
                             d.ID_AC,
	                             d.DATE_FROM,
	                               d.LOGO,	
	                                 d.U_NAME,
	                                   d.U_IP,
	                                     d.U_HOST_NAME,
	                                       d.DATE_WRITE,
                                           d.ID_AC,
	                                           d.DATE_FROM,
	                                             d.LOGO,
                                                 d.U_NAME,
	                                                 d.U_IP,
	                                                   d.U_HOST_NAME,
	                                                     d.DATE_WRITE,
                                                         d.LOGO_TYPE,
                                                           d.LOGO_TYPE

        from WB_TEMP_XML_ID t join WB_REF_AIRCOMPANY_LOGO d
        on d.id_ac=t.id;

        delete from WB_REF_AIRCOMPANY_LOGO
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_AIRCOMPANY_LOGO.id_ac);

       insert into WB_REF_AIRCO_C_DATA_1_HISTORY (ID_,
                                                   U_NAME_,
	                                                   U_IP_,
	                                                     U_HOST_NAME_,
	                                                       DATE_WRITE_,
	                                                         ACTION,
	                                                           ID,
	                                                             ID_AC_OLD,
	                                                               DATE_FROM_OLD,
                                                                   H_ADRESS_OLD,
	                                                                   C_PERSONE_OLD,
	                                                                     E_MAIL_ADRESS_OLD,
	                                                                       TELETYPE_ADRESS_OLD,
	                                                                         PHONE_NUMBER_OLD,
	                                                                         	 FAX_NUMBER_OLD,
		                                                                           REMARK_OLD,
		                                                                             U_NAME_OLD,
		                                                                               U_IP_OLD,
		                                                                                 U_HOST_NAME_OLD,
		                                                                                   DATE_WRITE_OLD,
		                                                                                     ID_AC_NEW,
	                                                                                   	     DATE_FROM_NEW,
	                                                                     	                     H_ADRESS_NEW,
	                                                                     	                       C_PERSONE_NEW,
	                                                                     	                         E_MAIL_ADRESS_NEW,
	                                                                     	                           TELETYPE_ADRESS_NEW,
	                                                                     	                             PHONE_NUMBER_NEW,
	                                                                     	                               FAX_NUMBER_NEW,
	                                                                     	                                 REMARK_NEW,
	                                                                     	                                   U_NAME_NEW,
	                                                                     	                                     U_IP_NEW,
	                                                                     	                                       U_HOST_NAME_NEW,
	                                                                     	                                         DATE_WRITE_NEW)
        select SEC_WB_REF_AIRCO_C_DATA_1_HIST.nextval,
                 P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE(),
                         'delete',
                           d.id,
                             d.id_ac,
	                             d.DATE_FROM,
                                 d.H_ADRESS,
	                                 d.C_PERSONE,
	                                   d.E_MAIL_ADRESS,
	                                     d.TELETYPE_ADRESS,
	                                       d.PHONE_NUMBER,
	                                         d.FAX_NUMBER,
	                                           d.REMARK,	
	                                             d.U_NAME,
	                                               d.U_IP,
	                                                 d.U_HOST_NAME,
	                                                   d.DATE_WRITE,
                                                       d.id_ac,
                                                         d.DATE_FROM,
                                                           d.H_ADRESS,
                             	                               d.C_PERSONE,
	                                                             d.E_MAIL_ADRESS,
	                                                               d.TELETYPE_ADRESS,
	                                                                 d.PHONE_NUMBER,
	                                                                   d.FAX_NUMBER,
	                                                                     d.REMARK,	
	                                                                       d.U_NAME,
	                                                                         d.U_IP,
	                                                                           d.U_HOST_NAME,
	                                                                             d.DATE_WRITE
        from WB_TEMP_XML_ID t join WB_REF_AIRCO_C_DATA_1 d
        on d.id_ac=t.id;

        delete from WB_REF_AIRCO_C_DATA_1
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_AIRCO_C_DATA_1.id_ac);

    insert into WB_REF_AIRCO_C_DATA_2_HISTORY (ID_,
                                                 U_NAME_,
	                                                 U_IP_,
	                                                   U_HOST_NAME_,
	                                                     DATE_WRITE_,
	                                                       ACTION,
	                                                         ID,
	                                                           ID_AC_OLD,
	                                                             DATE_FROM_OLD,
                                                                 AR_OLD,
	                                                                 E_MAIL_ADRESS_OLD,
	                                                                   TELETYPE_ADRESS_OLD,
	                                                                     PHONE_NUMBER_OLD,
	                                                                       FAX_NUMBER_OLD,
	                                                                         REMARK_OLD,
	                                                                           U_NAME_OLD,
	                                                                             U_IP_OLD,
	                                                                               U_HOST_NAME_OLD,
	                                                                                 DATE_WRITE_OLD,
	                                                                                   ID_AC_NEW,
	                                                                                     DATE_FROM_NEW,
	                                                                                       AR_NEW,
	                                                                                         E_MAIL_ADRESS_NEW,
	                                                                                           TELETYPE_ADRESS_NEW,
	                                                                                             PHONE_NUMBER_NEW,
	                                                                                               FAX_NUMBER_NEW,
	                                                                                                 REMARK_NEW,
	                                                                                                   U_NAME_NEW,
	                                                                                                     U_IP_NEW,
	                                                                                                       U_HOST_NAME_NEW,
	                                                                                                         DATE_WRITE_NEW)
        select SEC_WB_REF_AIRCO_C_DATA_2_HIST.nextval,
                P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE(),
                         'delete',
                           d.id,
                             d.id_ac,
	                             d.DATE_FROM,
                                 d.AR,
	                                 d.E_MAIL_ADRESS,
	                                   d.TELETYPE_ADRESS,
	                                     d.PHONE_NUMBER,
	                                       d.FAX_NUMBER,
	                                         d.REMARK,
	                                           d.U_NAME,
	                                             d.U_IP,
	                                               d.U_HOST_NAME,
	                                                 d.DATE_WRITE,
                                                     d.id_ac,
	                                                     d.DATE_FROM,
                                                         d.AR,
	                                                         d.E_MAIL_ADRESS,
	                                                           d.TELETYPE_ADRESS,
	                                                             d.PHONE_NUMBER,
	                                                               d.FAX_NUMBER,
	                                                                 d.REMARK,
	                                                                   d.U_NAME,
	                                                                     d.U_IP,
	                                                                       d.U_HOST_NAME,
	                                                                         d.DATE_WRITE
        from WB_TEMP_XML_ID t join WB_REF_AIRCO_C_DATA_2 d
        on d.id_ac=t.id;

        delete from WB_REF_AIRCO_C_DATA_2
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_AIRCO_C_DATA_2.id_ac);

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
        select SEC_WB_REF_AC_C_DATA_DTM_HIST.nextval,
                 P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE(),
                         'delete',
                           i.id,
                             i.id_ac,
	                             i.DATE_FROM,
                                 i.REMARK,
	                                 i.U_NAME,
	                                   i.U_IP,
                                       i.U_HOST_NAME,
                                         i.DATE_WRITE,
	                                         i.ID_AC,
	                                           i.DATE_FROM,
	                                             i.REMARK,
	                                               i.U_NAME,
	                                                 i.U_IP,
	                                                   i.U_HOST_NAME,
	                                                     i.DATE_WRITE,
	                                                       i.DTM_ID,
	                                                         i.DTM_ID
        from WB_TEMP_XML_ID t join WB_REF_AIRCO_C_DATA_DTM i
        on i.id_ac=t.id;

        delete from WB_REF_AIRCO_C_DATA_DTM
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_AIRCO_C_DATA_DTM.id_ac);

        insert into WB_REF_AIRCO_AUTO_DOC_ADV_HIST (ID_,
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
        select SEC_WB_REF_AIRC_AUT_DC_ADV_HST.nextval,
                 P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE(),
                         'delete',
                           i.id,
                             i.ID_AC,
	                             i.DATE_FROM,
                                 i.REMARK,
	                                 i.U_NAME,
	                                   i.U_IP,
	                                     i.U_HOST_NAME,
	                                       i.DATE_WRITE,
                                           i.ID_AC,
	                                           i.DATE_FROM,
                                               i.REMARK,
	                                               i.U_NAME,
	                                                 i.U_IP,
	                                                   i.U_HOST_NAME,
	                                                     i.DATE_WRITE
        from WB_TEMP_XML_ID t join WB_REF_AIRCO_AUTO_DOCS_ADV i
        on i.id_ac=t.id;

        delete from WB_REF_AIRCO_AUTO_DOCS_ADV
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_AIRCO_AUTO_DOCS_ADV.id_ac);

        insert into WB_REF_AIRCO_AUTO_DOC_DET_HIST (ID_,
	                                                    U_NAME_,
	                                                      U_IP_,
	                                                        U_HOST_NAME_,
	                                                          DATE_WRITE_,
	                                                            ACTION_,
	                                                              ID,
	                                                                ID_AC_OLD,
	                                                                  ADV_ID_OLD,
	                                                                    DOC_ID_OLD,
	                                                                      U_NAME_OLD,
	                                                                        U_IP_OLD,
	                                                                          U_HOST_NAME_OLD,
	                                                                            DATE_WRITE_OLD,
	                                                                              COPY_COUNT_OLD,
	                                                                                ID_AC_NEW,
	                                                                                  ADV_ID_NEW,
	                                                                                    DOC_ID_NEW,
	                                                                                      U_NAME_NEW,
	                                                                                        U_IP_NEW,
	                                                                                          U_HOST_NAME_NEW,
	                                                                                            DATE_WRITE_NEW,
	                                                                                              COPY_COUNT_NEW)
    select SEC_WB_REF_AIRC_AUT_DC_DET_HST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
	                         i.ADV_ID,
	                           i.DOC_ID,
	                             i.U_NAME,
	                               i.U_IP,
	                                 i.U_HOST_NAME,
	                                   i.DATE_WRITE,
	                                     i.COPY_COUNT,
                                         i.ID_AC,
	                                         i.ADV_ID,
	                                           i.DOC_ID,
	                                             i.U_NAME,
	                                               i.U_IP,
	                                                 i.U_HOST_NAME,
	                                                   i.DATE_WRITE,
	                                                     i.COPY_COUNT
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_AUTO_DOCS_DET i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_AUTO_DOCS_DET
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_AUTO_DOCS_DET.id_ac);

    insert into WB_REF_AIRCO_AUTO_MES_ADV_HIST (ID_,
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
    select SEC_WB_REF_AIRC_AUT_MS_ADV_HST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
	                         i.DATE_FROM,
                             i.REMARK,
	                             i.U_NAME,
	                               i.U_IP,
	                                 i.U_HOST_NAME,
	                                   i.DATE_WRITE,
                                       i.ID_AC,
	                                       i.DATE_FROM,
                                           i.REMARK,
	                                           i.U_NAME,
	                                             i.U_IP,
	                                               i.U_HOST_NAME,
	                                                 i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_AUTO_MES_ADV i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_AUTO_MES_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_AUTO_MES_ADV.id_ac);

    insert into WB_REF_AIRCO_AUTO_MES_DET_HIST (ID_,
	                                                U_NAME_,
	                                                  U_IP_,
	                                                    U_HOST_NAME_,
	                                                      DATE_WRITE_,
	                                                        ACTION_,
	                                                          ID,
	                                                            ID_AC_OLD,
	                                                              ADV_ID_OLD,
	                                                                MES_ID_OLD,
	                                                                  U_NAME_OLD,
	                                                                    U_IP_OLD,
	                                                                      U_HOST_NAME_OLD,
	                                                                        DATE_WRITE_OLD,	
	                                                                          ID_AC_NEW,
	                                                                            ADV_ID_NEW,
	                                                                              MES_ID_NEW,
	                                                                                U_NAME_NEW,
	                                                                                  U_IP_NEW,
	                                                                                    U_HOST_NAME_NEW,
	                                                                                      DATE_WRITE_NEW)
    select SEC_WB_REF_AIRC_AUT_MS_DET_HST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
	                         i.ADV_ID,
	                           i.MES_ID,
	                             i.U_NAME,
	                               i.U_IP,
	                                 i.U_HOST_NAME,
	                                   i.DATE_WRITE,	
                                       i.ID_AC,
	                                       i.ADV_ID,
	                                         i.MES_ID,
	                                           i.U_NAME,
	                                             i.U_IP,
	                                               i.U_HOST_NAME,
	                                                 i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_AUTO_MES_DET i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_AUTO_MES_DET
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_AUTO_MES_DET.id_ac);

    insert into WB_REF_AIRCO_DOCS_H_F_HIST (ID_,
	                                            U_NAME_,
	                                              U_IP_,
	                                                U_HOST_NAME_,
	                                                  DATE_WRITE_,
	                                                    ACTION_,
	                                                      ID,
	                                                        ID_AC_OLD,
	                                                          DOC_ID_OLD,
	                                                            DATE_FROM_OLD,
	                                                              HEADER_OLD,
	                                                                FOOTER_OLD,
	                                                                  U_NAME_OLD,
	                                                                    U_IP_OLD,
	                                                                      U_HOST_NAME_OLD,
	                                                                        DATE_WRITE_OLD,
	                                                                          ID_AC_NEW,
	                                                                            DOC_ID_NEW,
	                                                                              DATE_FROM_NEW,
	                                                                                HEADER_NEW,
	                                                                                  FOOTER_NEW,
	                                                                                    U_NAME_NEW,
	                                                                                      U_IP_NEW,
	                                                                                        U_HOST_NAME_NEW,
	                                                                                          DATE_WRITE_NEW)
    select SEC_WB_REF_AC_DOCS_H_F_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
                           i.DOC_ID,
	                           i.DATE_FROM,
	                             i.HEADER,
	                               i.FOOTER,
	                                 i.U_NAME,
	                                   i.U_IP,
	                                     i.U_HOST_NAME,
	                                       i.DATE_WRITE,
                                           i.ID_AC,
                                             i.DOC_ID,
	                                             i.DATE_FROM,
	                                               i.HEADER,
	                                                 i.FOOTER,
	                                                   i.U_NAME,
	                                                     i.U_IP,
	                                                       i.U_HOST_NAME,
	                                                         i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_DOCS_HEAD_FOOT i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_DOCS_HEAD_FOOT
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_DOCS_HEAD_FOOT.id_ac);
    ----------------------------------------------------------------------------
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
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
	                         i.DATE_FROM,
                             i.REMARK,
	                             i.U_NAME,
	                               i.U_IP,
	                                 i.U_HOST_NAME,
	                                   i.DATE_WRITE,
                                       i.ID_AC,
	                                       i.DATE_FROM,
                                           i.REMARK,
	                                           i.U_NAME,
	                                             i.U_IP,
	                                               i.U_HOST_NAME,
	                                                 i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_MISC_ADV i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_MISC_ADV
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_MISC_ADV.id_ac);

    insert into WB_REF_AIRCO_MISC_DET_HIST (ID_,
	                                            U_NAME_,
	                                              U_IP_,
	                                                U_HOST_NAME_,
	                                                  DATE_WRITE_,
	                                                    ACTION_,
	                                                      ID,
	                                                        ID_AC_OLD,
	                                                          ADV_ID_OLD,
	                                                            MISC_ID_OLD,
	                                                              U_NAME_OLD,
	                                                                U_IP_OLD,
	                                                                  U_HOST_NAME_OLD,
	                                                                    DATE_WRITE_OLD,	
	                                                                      ID_AC_NEW,
	                                                                        ADV_ID_NEW,
	                                                                          MISC_ID_NEW,
	                                                                            U_NAME_NEW,
	                                                                              U_IP_NEW,
	                                                                                U_HOST_NAME_NEW,
	                                                                                  DATE_WRITE_NEW)
    select SEC_WB_REF_AIRC_MISC_DET_HST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
	                         i.ADV_ID,
	                           i.MISC_ID,
	                             i.U_NAME,
	                               i.U_IP,
	                                 i.U_HOST_NAME,
	                                   i.DATE_WRITE,	
                                       i.ID_AC,
	                                       i.ADV_ID,
	                                         i.MISC_ID,
	                                           i.U_NAME,
	                                             i.U_IP,
	                                               i.U_HOST_NAME,
	                                                 i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_MISC_DET i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_MISC_DET
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_MISC_DET.id_ac);
    ----------------------------------------------------------------------------
    insert into WB_REF_AIRCO_PORT_FLIGHT_HIST (ID_,
	                                               U_NAME_,
	                                                 U_IP_,
	                                                   U_HOST_NAME_,
	                                                     DATE_WRITE_,
	                                                       ACTION_,
	                                                         ID,
	                                                           ID_AC_OLD,
	                                                             ID_PORT_OLD,
	 	                                                             DATE_FROM_OLD,
	 	                                                               UTC_DIFF_OLD,
	 	                                                                 IS_CHECK_IN_OLD,
	 	                                                                   IS_LOAD_CONTROL_OLD,
	 	                                                                     U_NAME_OLD,
	 	                                                                       U_IP_OLD,
		                                                                         U_HOST_NAME_OLD,
		                                                                           DATE_WRITE_OLD,
		                                                                             ID_AC_NEW,
		                                                                               ID_PORT_NEW,
		                                                                                 DATE_FROM_NEW,
		                                                                                   UTC_DIFF_NEW,
		                                                                                     IS_CHECK_IN_NEW,
		                                                                                       IS_LOAD_CONTROL_NEW,
		                                                                                         U_NAME_NEW,
		                                                                                           U_IP_NEW,
		                                                                                             U_HOST_NAME_NEW,
		                                                                                               DATE_WRITE_NEW)
    select SEC_WB_REF_AC_PORT_FLIGHT_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
	                         i.ID_PORT,
	                           i.DATE_FROM,
	                             i.UTC_DIFF,
	                               i.IS_CHECK_IN,
	                                 i.IS_LOAD_CONTROL,
	                                   i.U_NAME,
	                                     i.U_IP,
	                                       i.U_HOST_NAME,
	                                         i.DATE_WRITE,
                                             i.ID_AC,
	                                             i.ID_PORT,
	                                               i.DATE_FROM,
	                                                 i.UTC_DIFF,
	                                                   i.IS_CHECK_IN,
	                                                     i.IS_LOAD_CONTROL,
	                                                       i.U_NAME,
	                                                         i.U_IP,
	                                                           i.U_HOST_NAME,
	                                                             i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_PORT_FLIGHT i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_PORT_FLIGHT
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_PORT_FLIGHT.id_ac);
    ----------------------------------------------------------------------------
    insert into WB_REF_AIRCO_MEAS_HIST (ID_,
	                                        U_NAME_,
	                                          U_IP_,
	                                            U_HOST_NAME_,
	                                              DATE_WRITE_,
	                                                ACTION_,
	                                                  ID,
	                                                    ID_AC_OLD,
	                                                      DATE_FROM_OLD,
                                                      	  WEIGHT_ID_OLD,
	                                                          VOLUME_ID_OLD,
	                                                            LENGTH_ID_OLD,
	                                                              DENSITY_ID_OLD,
	                                                                U_NAME_OLD,
	                                                                  U_IP_OLD,
	                                                                    U_HOST_NAME_OLD,
	                                                                      DATE_WRITE_OLD,
	                                                                        ID_AC_NEW,
	                                                                          DATE_FROM_NEW,
	                                                                            WEIGHT_ID_NEW,
	                                                                              VOLUME_ID_NEW,
	                                                                                LENGTH_ID_NEW,
	                                                                                  DENSITY_ID_NEW,
	                                                                                    U_NAME_NEW,
	                                                                                      U_IP_NEW,
	                                                                                        U_HOST_NAME_NEW,
	                                                                                          DATE_WRITE_NEW)
    select SEC_WB_REF_AIRCO_MEAS_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
                           i.date_from,
                             i.WEIGHT_ID,
	                             i.VOLUME_ID,
	                               i.LENGTH_ID,
	                                 i.DENSITY_ID,
                                     i.U_NAME,
	                                     i.U_IP,
	                                       i.U_HOST_NAME,
	                                         i.DATE_WRITE,	
                                             i.ID_AC,
	                                             i.DATE_FROM,
                                                 i.WEIGHT_ID,
	                                                 i.VOLUME_ID,
	                                                   i.LENGTH_ID,
	                                                     i.DENSITY_ID,
	                                                       i.U_NAME,
	                                                         i.U_IP,
	                                                           i.U_HOST_NAME,
	                                                             i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_MEAS i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_MEAS
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_MEAS.id_ac);
    ----------------------------------------------------------------------------
    insert into WB_REF_AIRCO_CLASS_CODES_HIST (ID_,
	                                               U_NAME_,
	                                                 U_IP_,
	                                                   U_HOST_NAME_,
	                                                     DATE_WRITE_,
	                                                       ACTION_,
	                                                         ID,
	                                                           ID_AC_OLD,
                                                               CLASS_CODE_OLD,
	                                                               DATE_FROM_OLD,
                                                                   DATE_TO_OLD,
                                                                     PRIORITY_CODE_OLD,
                                                      	               DESCRIPTION_OLD,
	                                                                       U_NAME_OLD,
	                                                                         U_IP_OLD,
                                                      	                     U_HOST_NAME_OLD,
	                                                                             DATE_WRITE_OLD,
	                                                                               ID_AC_NEW,
                                                                                   CLASS_CODE_NEW,
	                                                                                   DATE_FROM_NEW,
                                                                                       DATE_TO_NEW,
                                                                                         PRIORITY_CODE_NEW,
	                                                                                         DESCRIPTION_NEW,
	                                                                                           U_NAME_NEW,
	                                                                                             U_IP_NEW,
	                                                                                               U_HOST_NAME_NEW,
	                                                                                                 DATE_WRITE_NEW)
    select SEC_WB_REF_AC_CLASS_CODES_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
                           i.CLASS_CODE,
	                           i.DATE_FROM,
                               i.DATE_TO,
                                 i.priority_code,
                                   i.DESCRIPTION,
	                                   i.U_NAME,
	                                     i.U_IP,
	                                       i.U_HOST_NAME,
	                                         i.DATE_WRITE,
                                             i.ID_AC,
                                               i.CLASS_CODE,
	                                               i.DATE_FROM,
                                                   i.DATE_TO,
                                                     i.priority_code,
                                                       i.DESCRIPTION,
	                                                       i.U_NAME,
	                                                         i.U_IP,
	                                                           i.U_HOST_NAME,
	                                                             i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_CLASS_CODES i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_CLASS_CODES
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_CLASS_CODES.id_ac);
    ----------------------------------------------------------------------------
    insert into WB_REF_AIRCO_LOAD_CODES_HIST (ID_,
	                                              U_NAME_,
	                                                U_IP_,
	                                                  U_HOST_NAME_,
	                                                    DATE_WRITE_,
	                                                      ACTION_,
	                                                        ID,
	                                                          ID_AC_OLD,
                                                              CODE_NAME_OLD,
                                                                IATA_CODE_ID_OLD,
                                                                  DENSITY_OLD,
                                                                    PRIORITY_OLD,
	                                                                    DATE_FROM_OLD,
                                                                        DATE_TO_OLD,
                                                      	                  DESCRIPTION_OLD,
	                                                                          U_NAME_OLD,
	                                                                            U_IP_OLD,
                                                      	                        U_HOST_NAME_OLD,
	                                                                                DATE_WRITE_OLD,
	                                                                                  ID_AC_NEW,
                                                                                      CODE_NAME_NEW,
                                                                                        IATA_CODE_ID_NEW,
                                                                                          DENSITY_NEW,
                                                                                            PRIORITY_NEW,
	                                                                                            DATE_FROM_NEW,
                                                                                                DATE_TO_NEW,
	                                                                                                DESCRIPTION_NEW,
	                                                                                                  U_NAME_NEW,
	                                                                                                    U_IP_NEW,
	                                                                                                      U_HOST_NAME_NEW,
	                                                                                                        DATE_WRITE_NEW)
    select SEC_WB_REF_AC_LOAD_CODES_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
                           i.CODE_NAME,
                             i.IATA_CODE_ID,
                               i.DENSITY,
                                 i.PRIORITY,
	                                 i.DATE_FROM,
                                     i.DATE_TO,
                                       i.DESCRIPTION,
	                                       i.U_NAME,
	                                         i.U_IP,
	                                           i.U_HOST_NAME,
	                                             i.DATE_WRITE,
                                                 i.ID_AC,
                                                   i.CODE_NAME,
                                                     i.IATA_CODE_ID,
                                                       i.DENSITY,
                                                         i.PRIORITY,
	                                                         i.DATE_FROM,
                                                             i.DATE_TO,
                                                               i.DESCRIPTION,
	                                                               i.U_NAME,
	                                                                 i.U_IP,
	                                                                   i.U_HOST_NAME,
	                                                                     i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_LOAD_CODES i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_LOAD_CODES
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_LOAD_CODES.id_ac);
    ----------------------------------------------------------------------------
    insert into WB_REF_AIRCO_FLIGHT_CODES_HIST (ID_,
	                                                U_NAME_,
	                                                  U_IP_,
	                                                    U_HOST_NAME_,
	                                                      DATE_WRITE_,
	                                                        ACTION_,
	                                                          ID,
	                                                            ID_AC_OLD,
                                                                CODE_NAME_OLD,
	                                                                DATE_FROM_OLD,
                                                                    DATE_TO_OLD,
                                                      	              DESCRIPTION_OLD,
	                                                                      U_NAME_OLD,
	                                                                        U_IP_OLD,
                                                      	                    U_HOST_NAME_OLD,
	                                                                            DATE_WRITE_OLD,
	                                                                              ID_AC_NEW,
                                                                                  CODE_NAME_NEW,
                                                                                    DATE_FROM_NEW,
                                                                                      DATE_TO_NEW,
	                                                                                      DESCRIPTION_NEW,
	                                                                                        U_NAME_NEW,
	                                                                                          U_IP_NEW,
	                                                                                            U_HOST_NAME_NEW,
	                                                                                              DATE_WRITE_NEW)
    select SEC_WB_REF_AC_FLIGHT_CODE_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
                           i.CODE_NAME,
                             i.DATE_FROM,
                               i.DATE_TO,
                                 i.DESCRIPTION,
	                                 i.U_NAME,
	                                   i.U_IP,
	                                     i.U_HOST_NAME,
	                                       i.DATE_WRITE,
                                           i.ID_AC,
                                             i.CODE_NAME,
                                               i.DATE_FROM,
                                                 i.DATE_TO,
                                                   i.DESCRIPTION,
	                                                   i.U_NAME,
	                                                     i.U_IP,
	                                                       i.U_HOST_NAME,
	                                                         i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_FLIGHT_CODES i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_FLIGHT_CODES
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_FLIGHT_CODES.id_ac);
    ----------------------------------------------------------------------------
    insert into WB_REF_AIRCO_CREW_WEIGHTS_HIST (ID_,
	                                                U_NAME_,
	                                                  U_IP_,
	                                                    U_HOST_NAME_,
	                                                      DATE_WRITE_,
	                                                        ACTION_,
	                                                          ID,
	                                                            ID_AC_OLD,
                                                                DESCRIPTION_OLD,
	                                                                DATE_FROM_OLD,
	                                                                  FC_STANDART_OLD,
	                                                                    FC_MALE_OLD,
                                                                     	  FC_FEMALE_OLD,
                                                                	     	  FC_HAND_BAG_OLD,
                                                                	     	    FC_HAND_BAG_INCLUDE_OLD,
                                                                	   	        CC_STANDART_OLD,
                                                                	   	          CC_MALE_OLD,
                                                                   	  	          CC_FEMALE_OLD,
	                                                                 	                CC_HAND_BAG_OLD,
	                                                                 	                  CC_HAND_BAG_INCLUDE_OLD,
	                                                                	                    FC_BAGGAGE_WEIGHT_OLD,
	                                                                	                      CC_BAGGAGE_WEIGHT_OLD,
	                                                                	                        BY_DEFAULT_OLD,
	                                                                	                          U_NAME_OLD,
	                                                               	                              U_IP_OLD,
	                                                               	                                U_HOST_NAME_OLD,
	                                                              	                                  DATE_WRITE_OLD,
	                                                              	                                    ID_AC_NEW,
	                                                              	                                      DESCRIPTION_NEW,
	                                                              	                                        DATE_FROM_NEW,
	                                                              	                                          FC_STANDART_NEW,
	                                                              	                                            FC_MALE_NEW,
	                                                              	                                              FC_FEMALE_NEW,
	                                                              	                                                FC_HAND_BAG_NEW,
	                                                              	                                                  FC_HAND_BAG_INCLUDE_NEW,
	                                                              	                                                    CC_STANDART_NEW,
	                                                               	                                                      CC_MALE_NEW,
	                                                                	                                                      CC_FEMALE_NEW,
	                                                               	                                                          CC_HAND_BAG_NEW,
	                                                               	                                                            CC_HAND_BAG_INCLUDE_NEW,
	                                                                	                                                            FC_BAGGAGE_WEIGHT_NEW,
	                                                                	                                                              CC_BAGGAGE_WEIGHT_NEW,
	                                                                	                                                                BY_DEFAULT_NEW,
	                                                               	                                                                    U_NAME_NEW,
	                                                               	                                                                      U_IP_NEW,
	                                                                	                                                                      U_HOST_NAME_NEW,
	                                                                	                                                                        DATE_WRITE_NEW)
    select SEC_WB_REF_AC_CR_WEIGHTS_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
                           i.DESCRIPTION,
	                           i.DATE_FROM,
	                             i.FC_STANDART,
                             	   i.FC_MALE,
	                                 i.FC_FEMALE,
	                                   i.FC_HAND_BAG,
	                                     i.FC_HAND_BAG_INCLUDE,
	                                       i.CC_STANDART,
	                                         i.CC_MALE,
	                                           i.CC_FEMALE,
	                                             i.CC_HAND_BAG,
	                                               i.CC_HAND_BAG_INCLUDE,
	                                                 i.FC_BAGGAGE_WEIGHT,
	                                                   i.CC_BAGGAGE_WEIGHT,
	                                                     i.BY_DEFAULT,
	                                                       i.U_NAME,
                                             	             i.U_IP,
                                             	               i.U_HOST_NAME,
	                                                             i.DATE_WRITE,
                                                                 i.ID_AC,
                                                                   i.DESCRIPTION,
	                                                                   i.DATE_FROM,
	                                                                     i.FC_STANDART,
	                                                                       i.FC_MALE,
	                                                                         i.FC_FEMALE,
	                                                                           i.FC_HAND_BAG,
	                                                                             i.FC_HAND_BAG_INCLUDE,
	                                                                               i.CC_STANDART,
	                                                                                 i.CC_MALE,
	                                                                                   i.CC_FEMALE,
	                                                                                     i.CC_HAND_BAG,
	                                                                                       i.CC_HAND_BAG_INCLUDE,
	                                                                                         i.FC_BAGGAGE_WEIGHT,
	                                                                                           i.CC_BAGGAGE_WEIGHT,
	                                                                                             i.BY_DEFAULT,
	                                                                                               i.U_NAME,
	                                                                                                 i.U_IP,
                                             	                                                       i.U_HOST_NAME,
	                                                                                                     i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_CREW_WEIGHTS i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_CREW_WEIGHTS
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_CREW_WEIGHTS.id_ac);
    ----------------------------------------------------------------------------
    insert into WB_REF_AIRCO_PAX_WEIGHTS_HIST (ID_,
	                                               U_NAME_,
	                                                 U_IP_,
	                                                   U_HOST_NAME_,
	                                                     DATE_WRITE_,
	                                                       ACTION_,
	                                                         ID,
	                                                           ID_AC_OLD,
                                                               DESCRIPTION_OLD,
	                                                               DATE_FROM_OLD,
	                                                                 ADULT_OLD,
	                                                                   MALE_OLD,
	                                                                     FEMALE_OLD,
	                                                                       CHILD_OLD,
	                                                                         INFANT_OLD,
	                                                                           HAND_BAG_OLD,
	                                                                             HAND_BAG_INCLUDE_OLD,
	                                                                               U_NAME_OLD,
	                                                                                 U_IP_OLD,
	                                                                                   U_HOST_NAME_OLD,
	                                                                                     DATE_WRITE_OLD,
	                                                                                       ID_CLASS_OLD,
	                                                                                         BY_DEFAULT_OLD,
	                                                                                           ID_AC_NEW,
	                                                                                             DESCRIPTION_NEW,
	                                                                                               DATE_FROM_NEW,
	                                                                                                 ADULT_NEW,
	                                                                                                   MALE_NEW,
	                                                                                                     FEMALE_NEW,
	                                                                                                       CHILD_NEW,
	                                                                                                         INFANT_NEW,
	                                                                                                           HAND_BAG_NEW,
	                                                                                                             HAND_BAG_INCLUDE_NEW,
	                                                                                                               U_NAME_NEW,
	                                                                                                                 U_IP_NEW,
	                                                                                                                   U_HOST_NAME_NEW,
	                                                                                                                     DATE_WRITE_NEW,
	                                                                                                                       ID_CLASS_NEW,
	                                                                                                                         BY_DEFAULT_NEW)
    select SEC_WB_REF_AC_PAX_WEIGHTS_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
                           i.DESCRIPTION,
	                           i.DATE_FROM,
	                             i.ADULT,
	                               i.MALE,
	                                 i.FEMALE,
	                                   i.CHILD,
	                                     i.INFANT,
	                                       i.HAND_BAG,
	                                         i.HAND_BAG_INCLUDE,
	                                           i.U_NAME,
	                                             i.U_IP,
	                                               i.U_HOST_NAME,
	                                                 i.DATE_WRITE,
	                                                   i.ID_CLASS,
	                                                     i.BY_DEFAULT,
                                                         i.ID_AC,
                                                           i.DESCRIPTION,
	                                                           i.DATE_FROM,
	                                                             i.ADULT,
	                                                               i.MALE,
	                                                                 i.FEMALE,
	                                                                   i.CHILD,
	                                                                     i.INFANT,
	                                                                       i.HAND_BAG,
	                                                                         i.HAND_BAG_INCLUDE,
	                                                                           i.U_NAME,
	                                                                             i.U_IP,
	                                                                               i.U_HOST_NAME,
	                                                                                 i.DATE_WRITE,
	                                                                                   i.ID_CLASS,
	                                                                                     i.BY_DEFAULT
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_PAX_WEIGHTS i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_PAX_WEIGHTS
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_PAX_WEIGHTS.id_ac);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_AIRCO_ULD_REM_HIST(ID_,
	                                          U_NAME_,
	                                            U_IP_,
	                                              U_HOST_NAME_,
	                                                DATE_WRITE_,
                                                    OPERATION_,
	                                                    ACTION_,
	                                                      ID,
	                                                        ID_AC,	
	                                                          REMARKS,
	                                                            U_NAME,
		                                                            U_IP,
		                                                              U_HOST_NAME,
		                                                                DATE_WRITE)
    select SEC_WB_REF_AIRCO_ULD_REM_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       'delete',
                         i.id,
                           i.ID_AC,
                             i.remarks,
                               i.U_NAME,
	                               i.U_IP,
	                                 i.U_HOST_NAME,
	                                   i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_ULD_REM i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_ULD_REM
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_ULD_REM.id_ac);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
     insert into WB_REF_AIRCO_ULD_HIST(ID_,
	                                      U_NAME_,
	                                        U_IP_,
	                                          U_HOST_NAME_,
	                                            DATE_WRITE_,
                                                OPERATION_,
	                                                ACTION_,
	                                                  ID,
	                                                    ID_AC,	
	                                                      ULD_TYPE_ID,
	                                                        ULD_ATA_ID,
	                                                          ULD_IATA_ID,
	                                                            BEG_SER_NUM,
	                                                              END_SER_NUM,
	                                                                OWNER_CODE,
	                                                                  TARE_WEIGHT,
	                                                                    MAX_WEIGHT,
	                                                                      MAX_VOLUME,
	                                                                        WIDTH,
	                                                                          LENGTH,
	                                                                            HEIGHT,
	                                                                              BY_DEFAULT,
	                                                                                U_NAME,
		                                                                                U_IP,
		                                                                                  U_HOST_NAME,
		                                                                                    DATE_WRITE)
    select SEC_WB_REF_AIRCO_ULD_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       'delete',
                         i.id,
                           i.ID_AC,
                             i.ULD_TYPE_ID,
	                             i.ULD_ATA_ID,
	                               i.ULD_IATA_ID,
	                                 i.BEG_SER_NUM,
	                                   i.END_SER_NUM,
	                                     i.OWNER_CODE,
	                                       i.TARE_WEIGHT,
	                                         i.MAX_WEIGHT,
	                                           i.MAX_VOLUME,
	                                             i.WIDTH,
	                                               i.LENGTH,
	                                                 i.HEIGHT,
	                                                   i.BY_DEFAULT,
                                                       i.U_NAME,
	                                                       i.U_IP,
	                                                         i.U_HOST_NAME,
	                                                           i.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_ULD i
    on i.id_ac=t.id;

    delete from WB_REF_AIRCO_ULD
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_ULD.id_ac);
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_AIRCO_KEY_DELETE;
/
