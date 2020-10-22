create or replace procedure SP_WB_REF_AC_WS_BORT_UPDATE
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_LANG varchar2(50):='';
P_BORT_NUM varchar2(200):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_R_COUNT number:=0;
V_STR_MSG clob:=null;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BORT_NUM[1]') into P_BORT_NUM from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from  WB_REF_AIRCO_WS_BORTS
    where ID=P_ID;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='This record is removed!';
          end;
        else
          begin
            V_STR_MSG:='Выбранная запись удалена!';
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
         select count(id) into V_R_COUNT
         from WB_REF_AIRCO_WS_BORTS
         where id_ac=P_ID_AC and
               BORT_NUM=P_BORT_NUM and
               ID<>P_ID;

         if V_R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 V_STR_MSG:='This number of aircraft is already in use in the airline!';
               end;
             else
               begin
                 V_STR_MSG:='Такой бортовой номер воздушного судна уже используется в этой авиакомпании!';
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
        select count(id) into V_R_COUNT
        from WB_REF_RESERVED_PHRASE
        where phrase=P_BORT_NUM;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='Value '||P_BORT_NUM||' is a phrase reserved!';
              end;
            else
              begin
                V_STR_MSG:='Знаачение '||P_BORT_NUM||' является зарезервированной фразой!';
              end;
            end if;
          end;
        end if;

      end;
    end if;

    if (V_STR_MSG is null) then
      begin
       update WB_REF_AIRCO_WS_BORTS
       set BORT_NUM=P_BORT_NUM,
           U_NAME=P_U_NAME,
	         U_IP=P_U_IP,
	         U_HOST_NAME=P_U_HOST_NAME,
	         DATE_WRITE=sysdate()
        WHERE ID=P_ID;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

      cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';
    commit;
  end SP_WB_REF_AC_WS_BORT_UPDATE;
/
