create or replace procedure SP_WB_REF_WS_AIR_S_L_C_IDN_INS
(cXML_in in clob,
   cXML_out out clob)
as
V_IDN number:=-1;
P_LANG varchar2(50):='';
P_ADV_ID number:=-1;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT number:=-1;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ADV_ID[1]')) into P_ADV_ID from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_SEAT_LAY_IDN
    where id=P_ADV_ID;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='This record "Seating Layout" block deleted!';
          end;
        else
          begin
            V_STR_MSG:='Эта запись блока "Seating Layout" удалена!';
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        V_IDN:=SEC_WB_REF_WS_AIR_S_L_C_IDN.nextval();

        insert into WB_REF_WS_AIR_S_L_C_IDN (ID,
	                                             ID_AC,
	                                               ID_WS,
	                                                 ID_BORT,
                                                     ADV_ID,
                                                       table_name,
                                                         U_NAME,
	                                                         U_IP,
	                                                           U_HOST_NAME,
	                                                             DATE_WRITE)
        select V_IDN,
                 i.ID_AC,
                   i.ID_WS,
                     i.ID_BORT,
                       P_ADV_ID,
                         'Configuration '||to_char(nvl((select count(slc.id)+1 from WB_REF_WS_AIR_S_L_C_IDN slc where slc.adv_id=P_ADV_ID), 1)),
                           P_U_NAME,
	                           P_U_IP,
	                             P_U_HOST_NAME,
                                 sysdate()
        from WB_REF_WS_AIR_SEAT_LAY_IDN i
        where i.id=P_ADV_ID;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list id="'||to_char(V_IDN)||'" str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_S_L_C_IDN_INS;
/
