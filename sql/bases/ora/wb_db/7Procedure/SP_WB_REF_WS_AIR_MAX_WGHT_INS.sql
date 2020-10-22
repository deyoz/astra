create or replace procedure SP_WB_REF_WS_AIR_MAX_WGHT_INS
(cXML_in in clob,
   cXML_out out clob)
as
V_IDN number:=-1;
P_LANG varchar2(50):='';
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
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
            V_STR_MSG:='��� ⨯ �� 㤠���!';
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
                V_STR_MSG:='�� ������������ 㤠����!';
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
        V_IDN:=SEC_WB_REF_WS_AIR_MAX_WGHT_IDN.nextval();

        insert into WB_REF_WS_AIR_MAX_WGHT_IDN(ID,
	                                               ID_AC,
	                                                 ID_WS,
		                                                 ID_BORT,
                                                       TABLE_NAME,
	                                                       CONDITION,
	                                                         TYPE,
	                                                           TYPE_FROM,
	                                                             TYPE_TO,
	                                                               U_NAME,
		                                                               U_IP,
		                                                                 U_HOST_NAME,
		                                                                   DATE_WRITE)
        select V_IDN,
                 P_ID_AC,
                   P_ID_WS,
                     P_ID_BORT,
                       ' Table '||to_char(V_IDN),
                         'EMPTY_STRING',
                           'EMPTY_STRING',
                             NULL,
                               NULL,
                                 P_U_NAME,
	                                 P_U_IP,
	                                   P_U_HOST_NAME,
                                       sysdate()
        from dual;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list id="'||to_char(V_IDN)||'" str_msg="'||V_STR_MSG||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_MAX_WGHT_INS;
/
