create or replace procedure SP_WB_REF_AC_WS_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_LANG varchar2(50):='';
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
R_COUNT number:=0;
str_msg clob:=null;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    select count(id) into R_COUNT
    from WB_REF_WS_TYPES
    where ID=P_ID_WS;

    if R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            str_msg:='This aircraft is removed!';
          end;
        else
          begin
            str_msg:='Выбранный Тив ВС удален!';
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
         select count(id) into R_COUNT
         from WB_REF_AIRCOMPANY_KEY
         where id=P_ID_AC;

         if R_COUNT=0 then
           begin
             if P_LANG='ENG' then
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
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
         select count(id) into R_COUNT
         from WB_REF_AIRCO_WS_TYPES
         where id_ac=P_ID_AC and
               id_ws=P_ID_WS;

         if R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='This type of aircraft is already in use in the airline!';
               end;
             else
               begin
                 str_msg:='Такой тип воздушного судна уже используется в этой авиакомпании!';
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
        P_ID:=SEC_WB_REF_AIRCO_WS_TYPES.nextval();

        insert into WB_REF_AIRCO_WS_TYPES (ID,
	                                           ID_AC,
	                                             ID_WS,
	                                               U_NAME,
	                                                 U_IP,
	                                                   U_HOST_NAME,
	                                                     DATE_WRITE)
        select P_ID,
                 P_ID_AC,
	                 P_ID_WS,
	                   P_U_NAME,
	                     P_U_IP,
	                       P_U_HOST_NAME,
	                         SYSDATE()
        from dual;

        STR_MSG:='EMPTY_STRING';
      end;
    end if;

      cXML_out:=cXML_out||'<list id="'||to_char(P_ID)||'" str_msg="'||str_msg||'"/>'||'</root>';
    commit;
  end SP_WB_REF_AC_WS_INSERT;
/
