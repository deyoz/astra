create or replace procedure SP_WB_REF_AIRCO_ULD_REM_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
cXML_data clob;
V_R_COUNT number:=0;

V_IS_AC number:=1;
V_ID number:=-1;
begin
    cXML_out:='';

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into V_R_COUNT
    from WB_REF_AIRCO_ULD_REM
    where ID_AC=P_ID_AC;

    if V_R_COUNT=0 then
      begin
        if P_ID_AC>0 then
          begin
            select count(id) into V_IS_AC
            from WB_REF_AIRCOMPANY_KEY
            where id=P_ID_AC;
          end;
        end if;

        if (V_IS_AC>0) then
          begin
            insert into WB_REF_AIRCO_ULD_REM (ID,
	                                              ID_AC,	
	                                                REMARKS,
	                                                  U_NAME,
	                                                    U_IP,
	                                                      U_HOST_NAME,
	                                                        DATE_WRITE)
            select SEC_WB_REF_AIRCO_ULD_REM.nextval,
                     P_ID_AC,
                       'EMPTY_STRING',
                         P_U_NAME,
	                         P_U_IP,
	                           P_U_HOST_NAME,
                               SYSDATE()
            from dual;

            commit;
          end;
        end if;
      end;
    end if;


    select count(id) into V_R_COUNT
    from WB_REF_AIRCO_ULD_REM
    where ID_AC=P_ID_AC;

    if V_R_COUNT>0 then
      begin
        select '<adv_data REMARKS="'||WB_CLEAR_XML(i.REMARKS)||'" '||
                         'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                         'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                         'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                         'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
        INTO cXML_data
        from WB_REF_AIRCO_ULD_REM i
        where i.ID_AC=P_ID_AC;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;


  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_AIRCO_ULD_REM_DATA;
/
