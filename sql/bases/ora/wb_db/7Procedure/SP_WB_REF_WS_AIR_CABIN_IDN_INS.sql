create or replace procedure SP_WB_REF_WS_AIR_CABIN_IDN_INS
(cXML_in in clob,
   cXML_out out clob)
as
V_IDN number:=-1;
P_LANG varchar2(50):='';
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
V_ID number:=-1;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT number:=-1;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_TYPES
    where id=P_ID_WS;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='This aircraft is removed!';
          end;
        else
          begin
            V_STR_MSG:='Этот тип ВС удален!';
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    if (V_STR_MSG is null) and
         (P_ID_AC<>-1) then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_AIRCOMPANY_KEY
        where id=P_ID_AC;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='This airline is removed!';
              end;
            else
              begin
                V_STR_MSG:='Эта авиакомпания удалена!';
              end;
            end if;

          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        V_IDN:=SEC_WB_REF_WS_AIR_CABIN_IDN.nextval();

        select count(id) into V_ID
        from WB_REF_WS_AIR_CABIN_IDN
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT;

        insert into WB_REF_WS_AIR_CABIN_IDN (ID,
	                                             ID_AC,
	                                               ID_WS,
	                                                 ID_BORT,
                                                     TABLE_NAME,
                                                       U_NAME,
	                                                       U_IP,
	                                                         U_HOST_NAME,
	                                                           DATE_WRITE)
        select V_IDN,
                 P_ID_AC,
                   P_ID_WS,
                     P_ID_BORT,
                       'Cabin '||to_char(V_ID+1),
                         P_U_NAME,
	                         P_U_IP,
	                           P_U_HOST_NAME,
                               sysdate()
        from dual;

        insert into WB_REF_WS_AIR_CABIN_ADV (ID,
    	                                         ID_AC,
	                                               ID_WS,
                                                 	 ID_BORT,
                                             	       IDN,
                                             	         CD_BALANCE_ARM,
	                                                       CD_INDEX_UNIT,
	                                                         FDL_BALANCE_ARM,
	                                                           FDL_INDEX_UNIT,
	                                                             CCL_BALANCE_ARM,
	                                                               CCL_INDEX_UNIT,
	                                                                 U_NAME,
	                                                                   U_IP,
	                                                                     U_HOST_NAME,
	                                                                       DATE_WRITE,
                                                                           DATE_FROM)
        select SEC_WB_REF_WS_AIR_CABIN_ADV.nextval,
                 P_ID_AC,
                   P_ID_WS,
                     P_ID_BORT,
                       V_IDN,
                        1,
                          0,
                            1,
                              0,
                                1,
                                  0,
                                    P_U_NAME,
	                                    P_U_IP,
	                                      P_U_HOST_NAME,
                                          sysdate(),
                                            null
        from dual;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list id="'||to_char(V_IDN)||'" str_msg="'||V_STR_MSG||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_CABIN_IDN_INS;
/
