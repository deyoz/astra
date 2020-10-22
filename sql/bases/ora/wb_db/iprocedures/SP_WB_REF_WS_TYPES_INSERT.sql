create or replace procedure SP_WB_REF_WS_TYPES_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_LANG varchar2(50):='';
P_NAME_RUS_SMALL varchar2(100):='';
P_NAME_RUS_FULL varchar2(200):='';
P_NAME_ENG_SMALL varchar2(100):='';
P_NAME_ENG_FULL varchar2(200):='';
P_IATA varchar2(50):='';
P_ICAO varchar2(50):='';
P_DOP_IDENT varchar2(50):='';
P_DOP_IDENT_IS_EMPTY number:=0;
P_REMARK clob:='';
P_REMARK_IS_EMPTY number:=0;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
STR_MSG varchar2(1000):=null;
R_COUNT int:=0;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_NAME_RUS_SMALL[1]') into p_NAME_RUS_SMALL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_NAME_RUS_FULL[1]') into p_NAME_RUS_FULL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_NAME_ENG_SMALL[1]') into p_NAME_ENG_SMALL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_NAME_ENG_FULL[1]') into p_NAME_ENG_FULL from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IATA[1]') into P_IATA from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ICAO[1]') into P_ICAO from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DOP_IDENT[1]') into P_DOP_IDENT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DOP_IDENT_IS_EMPTY[1]')) into P_DOP_IDENT_IS_EMPTY from dual;

    select value(t).extract('/P_REMARK/text()').getclobval() into P_REMARK
    from table(xmlsequence(xmltype(cXML_in).extract('//list/*'))) t
    where value(t).extract('/P_REMARK/text()').getclobval() is not null;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_REMARK_IS_EMPTY[1]')) into P_REMARK_IS_EMPTY from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if P_DOP_IDENT_IS_EMPTY=1 then
      begin

        P_DOP_IDENT:='EMPTY_STRING';
      end;
    end if;

    if P_DOP_IDENT_IS_EMPTY=0 then
      begin
        select count(id) into R_COUNT
        from WB_REF_RESERVED_PHRASE
        where phrase=P_DOP_IDENT;

        if r_count>0 then
          begin
            if P_LANG='ENG' then
              begin
                str_msg:='Value field &quot;Suffix&quot; is a phrase reserved!';
              end;
            else
              begin
                str_msg:='����祭�� ���� &quot;���䨪�&quot; ���� ��१�ࢨ஢����� �ࠧ��!';
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
        from WB_REF_WS_TYPES
        where P_IATA=IATA and
              P_ICAO=ICAO and
              P_DOP_IDENT=DOP_IDENT;

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                str_msg:='Record with this value fields &quot;ICAO code/IATA Code/Add Code&quot; already exists!';
              end;
            else
              begin
                str_msg:='������ � ⠪�� ���祭��� ����� &quot;��� ����/��� ����/���.���&quot; 㦥 �������!';
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
        if P_REMARK_IS_EMPTY=1 then
          begin

            P_REMARK:='EMPTY_STRING';
          end;
        end if;

        if P_REMARK_IS_EMPTY=0 then
          begin
            select count(id) into R_COUNT
            from WB_REF_RESERVED_PHRASE
            where phrase=to_char(P_REMARK);

            if r_count>0 then
              begin
                if P_LANG='ENG' then
                  begin
                    str_msg:='Value field &quot;Remark&quot; is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='����祭�� ���� &quot;�ਬ�砭��&quot; ���� ��१�ࢨ஢����� �ࠧ��!';
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
    if str_msg is null then
      begin
         select count(id) into r_count
         from WB_REF_RESERVED_PHRASE
         where phrase=P_NAME_RUS_SMALL;

         if R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='Value field &quot;Title short/RUS&quot; is a phrase reserved!';
               end;
             else
               begin
                 str_msg:='����祭�� ���� &quot;����.��⪮�/RUS&quot; ���� ��१�ࢨ஢����� �ࠧ��!';
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
         from WB_REF_RESERVED_PHRASE
         where phrase=P_NAME_ENG_SMALL;

         if R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='Value field &quot;Title short/ENG&quot; is a phrase reserved!';
               end;
             else
               begin
                 str_msg:='����祭�� ���� &quot;����.��⪮�/ENG&quot; ���� ��१�ࢨ஢����� �ࠧ��!';
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
         from WB_REF_RESERVED_PHRASE
         where phrase=P_NAME_RUS_FULL;

         if R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='Value field &quot;Title full/RUS&quot; is a phrase reserved!';
               end;
             else
               begin
                 str_msg:='����祭�� ���� &quot;����.������/RUS&quot; ���� ��१�ࢨ஢����� �ࠧ��!';
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
         from WB_REF_RESERVED_PHRASE
         where phrase=P_NAME_ENG_FULL;

         if R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='Value field &quot;Title full/ENG&quot; is a phrase reserved!';
               end;
             else
               begin
                 str_msg:='����祭�� ���� &quot;����.������/ENG&quot; ���� ��१�ࢨ஢����� �ࠧ��!';
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
        P_ID:=SEC_WB_REF_WS_TYPES.nextval();

        insert into WB_REF_WS_TYPES (ID,
	                                     U_NAME,
	                                       U_IP,
	                                         U_HOST_NAME,
	                                           DATE_WRITE,
	                                             IATA,
	                                               ICAO,
	                                                 DOP_IDENT,
	                                                   NAME_RUS_SMALL,
	                                                     NAME_RUS_FULL,
	                                                       NAME_ENG_SMALL,
	                                                         NAME_ENG_FULL,
	                                                           REMARK)
        select P_ID,
	               P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
	                     SYSDATE(),
	                       P_IATA,
	                         P_ICAO,
	                           P_DOP_IDENT,
	                             P_NAME_RUS_SMALL,
	                               P_NAME_RUS_FULL,
	                                 P_NAME_ENG_SMALL,
	                                   P_NAME_ENG_FULL,
	                                     P_REMARK
        from dual;

        STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list id="'||to_char(P_ID)||'" str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_TYPES_INSERT;
/
