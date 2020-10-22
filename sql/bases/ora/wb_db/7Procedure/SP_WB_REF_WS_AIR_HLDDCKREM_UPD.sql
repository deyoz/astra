create or replace procedure SP_WB_REF_WS_AIR_HLDDCKREM_UPD
(cXML_in in clob,
   cXML_out out clob)
as

P_LANG varchar2(50):='';
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
P_REMARKS clob:='';
P_IS_REMARKS_EMPTY number:=0;


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

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_REMARKS[1]') into P_REMARKS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_REMARKS_EMPTY[1]')) into P_IS_REMARKS_EMPTY from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;
    --------------------------------------------------------------------------------------------------------------------------------------------------------
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
   -----------------------------------------------------------------------------
   -----------------------------------------------------------------------------
   if V_STR_MSG is null then
      begin
        if p_IS_REMARKS_EMPTY=1 then
          begin

            P_REMARKS:='EMPTY_STRING';
          end;
        end if;

        if p_IS_REMARKS_EMPTY=0 then
          begin
            if P_REMARKS='EMPTY_STRING' then
              begin
                if P_LANG='ENG' then
                  begin
                    V_STR_MSG:='Value field "Remarks" is a phrase reserved!';
                  end;
                else
                  begin
                    V_STR_MSG:='Значение поля "Примечание" является зарезервированной фразой!';
                  end;
                end if;
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
        from WB_REF_WS_AIR_HLD_DECK_REM
        where id_ac=P_ID_AC and
              id_ws=P_ID_WS and
              id_bort=P_ID_BORT;

        if V_R_COUNT=0 then
          begin
            insert into WB_REF_WS_AIR_HLD_DECK_REM (ID,
	                                                    ID_AC,
	                                                      ID_WS,
	                                                        ID_BORT,
	                                                          REMARKS,
	                                                            U_NAME,
	                                                              U_IP,
	                                                                U_HOST_NAME,
	                                                                  DATE_WRITE)
            select SEC_WB_REF_WS_AIR_HLD_DECK_REM.nextval,
                     P_ID_AC,
                       P_ID_WS,
                         P_ID_BORT,
                           P_REMARKS,
                             P_U_NAME,
	                             P_U_IP,
	                               P_U_HOST_NAME,
                                   sysdate()
            from dual;
          end;
      else
        begin
          update WB_REF_WS_AIR_HLD_DECK_REM
          set REMARKS=P_REMARKS,
	            U_NAME=P_U_NAME,
	            U_IP=P_U_IP,
	            U_HOST_NAME=P_U_HOST_NAME,
	            DATE_WRITE=sysdate()
          where id_ac=P_ID_AC and
                id_ws=P_ID_WS and
                id_bort=P_ID_BORT;
        end;
      end if;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_HLDDCKREM_UPD;
/
