create or replace procedure SP_WB_REF_WS_AIR_SEC_B_A_DATA
(cXML_in in clob,
   cXML_out out clob)
as
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
cXML_data clob;
V_R_COUNT number:=0;

V_IS_AC number:=1;
V_IS_WS number:=1;
V_IS_BORT number:=1;
V_ID number:=-1;
begin
    cXML_out:='';

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_SEC_BAY_A
    where ID_AC=P_ID_AC and
          ID_WS=P_ID_WS and
          ID_BORT=P_ID_BORT;

    if V_R_COUNT=0 then
      begin
        if P_ID_AC>0 then
          begin
            select count(id) into V_IS_AC
            from WB_REF_AIRCOMPANY_KEY
            where id=P_ID_AC;
          end;
        end if;

        if P_ID_WS>0 then
          begin
            select count(id) into V_IS_WS
            from WB_REF_WS_TYPES
            where id=P_ID_WS;
          end;
        end if;

        if (V_IS_AC>0) and
             (V_IS_WS>0) and
               (V_IS_BORT>0) then
          begin
            insert into WB_REF_WS_AIR_SEC_BAY_A (ID,
	                                                 ID_AC,
	                                                   ID_WS,
	                                                     ID_BORT,
	                                                       CH_BALANCE_ARM,
	                                                         CH_INDEX_UNIT,
	                                                           REMARKS,
	                                                             U_NAME,
	                                                               U_IP,
	                                                                 U_HOST_NAME,
	                                                                   DATE_WRITE)
            select SEC_WB_REF_WS_AIR_SEC_BAY_A.nextval,
                     P_ID_AC,
                       P_ID_WS,
                         P_ID_BORT,
                           0,
                             1,
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
    from WB_REF_WS_AIR_SEC_BAY_A
    where ID_AC=P_ID_AC and
          ID_WS=P_ID_WS and
          ID_BORT=P_ID_BORT;

    if V_R_COUNT>0 then
      begin
        select '<adv_data CH_BALANCE_ARM="'||to_char(i.CH_BALANCE_ARM)||'" '||
                         'CH_INDEX_UNIT="'||to_char(i.CH_INDEX_UNIT)||'" '||
                         'REMARKS="'||WB_CLEAR_XML(i.REMARKS)||'" '||
                         'U_NAME="'||WB_CLEAR_XML(i.U_NAME)||'" '||
                         'U_IP="'||WB_CLEAR_XML(i.U_IP)||'" '||
                         'U_HOST_NAME="'||WB_CLEAR_XML(i.U_HOST_NAME)||'" '||
                         'DATE_WRITE="'||to_char(i.DATE_WRITE, 'dd.mm.yyyy hh24:mm:ss')||'"/>'
        INTO cXML_data
        from WB_REF_WS_AIR_SEC_BAY_A i
        where i.ID_AC=P_ID_AC and
              i.ID_WS=P_ID_WS and
              i.ID_BORT=P_ID_BORT;

        cXML_out:=cXML_out||cXML_data;
      end;
    end if;


  cXML_out:=cXML_out||'</root>';
end SP_WB_REF_WS_AIR_SEC_B_A_DATA;
/
