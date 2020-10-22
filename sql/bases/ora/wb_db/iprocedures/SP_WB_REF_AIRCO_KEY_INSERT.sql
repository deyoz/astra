create or replace procedure SP_WB_REF_AIRCO_KEY_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
id number:=-1;
id_adv number:=-1;
id_hist number:=-1;
lang varchar2(50):='';
AIRCO_NAME varchar2(100):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg varchar2(1000):=null;
r_count int:=0;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/lang[1]') into lang from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/AIRCO_NAME[1]') into AIRCO_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';
     ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if (str_msg is null) then
      begin
         select count(id) into r_count
         from WB_REF_RESERVED_PHRASE
         where phrase=AIRCO_NAME;

         if r_count>0 then
           begin
             if lang='ENG' then
               begin
                 str_msg:='Value field &quot;Title full/RUS&quot; is a phrase reserved!';
               end;
             else
               begin
                 str_msg:='Знаачение поля &quot;Назв.полное/RUS&quot; является зарезервированной фразой!';
               end;
             end if;
           end;
          end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

    if (str_msg is null) then
      begin
        id:=SEC_WB_REF_AIRCOMPANY_KEY.nextval();

        insert into WB_REF_AIRCOMPANY_KEY (ID,
	                                           U_NAME,
	                                             U_IP,
	                                               U_HOST_NAME,
	                                                 DATE_WRITE)
        select ID,
	               P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE
        from dual;

        id_hist:=SEC_WB_REF_AIRCOMPANY_KEY_HIST.nextval();

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
        select id_hist,
                 P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE,
                         'insert',
                           id,
                             P_U_NAME,
	                             P_U_IP,
	                               P_U_HOST_NAME,
                                   sysdate(),
                                     P_U_NAME,
	                                     P_U_IP,
	                                       P_U_HOST_NAME,
                                           sysdate()
        from dual;

        id_adv:=SEC_WB_REF_AIRCOMPANY_ADV_INFO.nextval();

        insert into WB_REF_AIRCOMPANY_ADV_INFO (ID,
                                                  ID_AC,
	                                                  DATE_FROM,
	                                                    ID_CITY,
	                                                      IATA_CODE,
	                                                        ICAO_CODE,
	                                                          OTHER_CODE,
	                                                            NAME_RUS_SMALL,
	                                                              NAME_RUS_FULL,
	                                                                NAME_ENG_SMALL,
	                                                                  NAME_ENG_FULL,
	                                                                    U_NAME,
	                                                                      U_IP,
	                                                                        U_HOST_NAME,
	                                                                          DATE_WRITE,
                                                                              remark)
        select id_adv,
                 id,
                   WB_EXTRACT_DATE(sysdate()),
                     -1,
                       'EMPTY_STRING',
                         'EMPTY_STRING',
                           'EMPTY_STRING',
                             'EMPTY_STRING',
                               case when lang='RUS' then AIRCO_NAME
                                    else 'EMPTY_STRING'
                               end,
                                 'EMPTY_STRING',
                                   case when lang='ENG' then AIRCO_NAME
                                        else 'EMPTY_STRING'
                                   end,
                                     P_U_NAME,
	                                     P_U_IP,
	                                       P_U_HOST_NAME,
                                           sysdate(),
                                             null

        from dual;

        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list id="'||to_char(id)||'" str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_AIRCO_KEY_INSERT;
/
