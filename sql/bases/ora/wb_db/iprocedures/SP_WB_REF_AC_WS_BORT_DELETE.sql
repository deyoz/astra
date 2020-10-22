create or replace procedure SP_WB_REF_AC_WS_BORT_DELETE
(cXML_in in clob,
   cXML_out out clob)
as
P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_LANG[1]') into P_LANG from dual;


    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    insert into WB_TEMP_XML_ID (ID,
                                  num)
    select distinct f.id,
                      f.id
    from (select to_number(extractValue(value(t),'list/P_ID[1]')) as id
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    --ВСТАВИТЬ ПРОВЕРКИ!!!!
    select count(i.id) into V_R_COUNT
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_REG_WGT i
    on i.id_bort=t.id;

    if V_R_COUNT>0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='In the value field "Bort" is referenced in the "Registration Weights" block. The operation is prohibited!';
          end;
        else
          begin
            V_STR_MSG:='На значения поля "Bort" имеются ссылки в блоке "Registration Weights". Операция запрещена!';
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(i.id) into V_R_COUNT
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_MAX_WGHT_T i
        on i.id_bort=t.id;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='In the value field "Bort" is referenced in the "Maximum Weights" block. The operation is prohibited!';
              end;
            else
              begin
                V_STR_MSG:='На значения поля "Bort" имеются ссылки в блоке "Maximum Weights". Операция запрещена!';
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
        select count(i.id) into V_R_COUNT
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_MIN_WGHT_T i
        on i.id_bort=t.id;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='In the value field "Bort" is referenced in the "Minimum Weights" block. The operation is prohibited!';
              end;
            else
              begin
                V_STR_MSG:='На значения поля "Bort" имеются ссылки в блоке "Minimum Weights". Операция запрещена!';
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
        select count(i.id) into V_R_COUNT
        from WB_TEMP_XML_ID t join WB_SHED i
        on i.id_bort=t.id;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='There is a link in block "Schedule"!';
              end;
            else
              begin
                V_STR_MSG:='Имеются ссылки в блоке "Расписание"!';
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
        select count(i.id) into V_R_COUNT
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_CFV_BORT i
        on i.id_bort=t.id;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='There is a link in block "Combined Fuel Vector"!';
              end;
            else
              begin
                V_STR_MSG:='Имеются ссылки в блоке "Combined Fuel Vector"!';
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
        select count(i.id) into V_R_COUNT
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_GR_CH_BORT i
        on i.id_bort=t.id;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='There is a link in block "Centre of Gravity Charts"!';
              end;
            else
              begin
                V_STR_MSG:='Имеются ссылки в блоке "Centre of Gravity Charts"!';
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
        select count(i.id) into V_R_COUNT
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_EQUIP_BORT i
        on i.id_bort=t.id;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='There is a link in block "Equipment"!';
              end;
            else
              begin
                V_STR_MSG:='Имеются ссылки в блоке "Equipment"!';
              end;
            end if;
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

    if (V_STR_MSG is null) then
      begin
        insert into WB_REF_AIRCO_WS_BORTS_HIST (ID_,
	                                                U_NAME_,
	                                                  U_IP_,
	                                                    U_HOST_NAME_,
	                                                      DATE_WRITE_,
	                                                        ACTION_,
	                                                          ID,
                                                              ID_AC_OLD,
	                                                              ID_WS_OLD,
                                                                  BORT_NUM_OLD,
	                                                                  U_NAME_OLD,
	                                                                    U_IP_OLD,
                                                          	            U_HOST_NAME_OLD,
	                                                                        DATE_WRITE_OLD,
                                                                            ID_AC_NEW,
	                                                                            ID_WS_NEW,
                                                                                BORT_NUM_NEW,
	                                                                                U_NAME_NEW,
	                                                                                  U_IP_NEW,
	                                                                                    U_HOST_NAME_NEW,
	                                                                                      DATE_WRITE_NEW)
        select SEC_WB_REF_AIRCO_WS_BORTS_HIST.nextval,
                 P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE(),
                         'delete',
                           i.id,
                             i.ID_AC,
	                             i.ID_WS,
                                 i.BORT_NUM,
	                                 i.U_NAME,
	                                   i.U_IP,
	                                     i.U_HOST_NAME,
	                                       i.DATE_WRITE,
                                           i.ID_AC,
	                                           i.ID_WS,
                                               i.BORT_NUM,
	                                               i.U_NAME,
	                                                 i.U_IP,
	                                                   i.U_HOST_NAME,
	                                                     i.DATE_WRITE
        from WB_TEMP_XML_ID t join WB_REF_AIRCO_WS_BORTS i
        on i.id=t.id;

        delete from WB_REF_AIRCO_WS_BORTS
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_AIRCO_WS_BORTS.id);

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_AC_WS_BORT_DELETE;
/
