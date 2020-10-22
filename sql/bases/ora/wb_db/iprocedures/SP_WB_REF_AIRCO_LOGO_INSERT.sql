create or replace procedure SP_WB_REF_AIRCO_LOGO_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
P_lang varchar2(50):='';
P_ID_AC number:=-1;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
P_LOGO clob:=null;
P_LOGO_TYPE varchar2(50);
str_msg varchar2(1000):=null;
r_count int:=0;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_lang[1]') into P_lang from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]') into P_ID_AC from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LOGO_TYPE[1]') into P_LOGO_TYPE from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    select value(t).extract('/P_LOGO/text()').getclobval() into P_LOGO
    from table(xmlsequence(xmltype(cXML_in).extract('//list/*'))) t
    where value(t).extract('/P_LOGO/text()').getclobval() is not null;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into r_count
    from WB_REF_AIRCOMPANY_KEY
    where id=P_ID_AC;

    if r_count=0 then
      begin
        if P_lang='ENG' then
          begin
            str_msg:='This airline is removed!';
          end;
        else
          begin
            str_msg:='Эта авиакомпания удалена!';
          end;
        end if;

      end;
    end if;

    if (str_msg is null) then
      begin
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

        from WB_REF_AIRCOMPANY_LOGO d
        where d.id_ac=P_ID_AC;

        delete from WB_REF_AIRCOMPANY_LOGO
        where id_ac=P_ID_AC;


        insert into WB_REF_AIRCOMPANY_LOGO (ID,
	                                           ID_AC,
	                                             DATE_FROM,
	                                               LOGO,
	                                                 U_NAME,
	                                                   U_IP,
	                                                     U_HOST_NAME,
                                                         DATE_WRITE,
                                                           LOGO_TYPE)
        select SEC_WB_REF_AIRCO_LOGO.nextval,
                 P_ID_AC,
                   null,
                     P_LOGO,
                       P_U_NAME,
	                       P_U_IP,
	                         P_U_HOST_NAME,
                             sysdate(),
                               P_LOGO_TYPE
        from dual;

        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_AIRCO_LOGO_INSERT;
/
