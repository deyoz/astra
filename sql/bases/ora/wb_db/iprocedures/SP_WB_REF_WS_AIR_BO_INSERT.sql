create or replace procedure SP_WB_REF_WS_AIR_BO_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;

P_LANG varchar2(50):='';
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;

P_DATE_FROM date;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');

    insert into WB_TEMP_XML_ID (ID,
                                  num,
                                    number_val)
    select f.item_id,
             f.det_col_id,
               f.is_checked
    from (select to_number(extractValue(value(t),'data/P_ITEM_ID[1]')) as item_id,
                   to_number(extractValue(value(t),'data/P_DET_COL_ID[1]')) as det_col_id,
                     to_number(extractValue(value(t),'data/P_IS_CHECKED[1]')) as is_checked
            from table(xmlsequence(xmltype(cXML_in).extract('//data'))) t) f;

    update WB_TEMP_XML_ID
    set string_val=(select extractValue(value(t),'data_remark/P_REMARK[1]')
                    from table(xmlsequence(xmltype(cXML_in).extract('//data_remark'))) t
                    where extractValue(value(t),'data_remark/P_ITEM_ID[1]')=WB_TEMP_XML_ID.id and
                          ROWNUM=1);

    cXML_out:='<?xml version="1.0" ?><root>';
    ----------------------------------------------------------------------------
    select count(id) into R_COUNT
    from WB_REF_WS_TYPES
    where id=P_ID_WS;

    if R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            str_msg:='This aircraft is removed!';
          end;
        else
          begin
            str_msg:='Этт тип ВС удален!';
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    if (str_msg is null) and
         (P_ID_AC<>-1) then
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
        select count(t.id) into R_COUNT
        from WB_TEMP_XML_ID t
        where not exists(select 1
                         from WB_REF_WS_AIR_BAL_OUTPUT_ITEM i
                         where i.id=t.id);

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                str_msg:='Part names removed from the directory!';
              end;
            else
              begin
                str_msg:='Часть наименований удалена из справочника!';
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
        select count(t.id) into R_COUNT
        from WB_TEMP_XML_ID t
        where not exists(select 1
                         from WB_REF_WS_AIR_BAL_OUT_DET_COL i
                         where i.id=t.num);

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                str_msg:='Some elements removed from the directory!';
              end;
            else
              begin
                str_msg:='Часть элементов удалена из справочника!';
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
         from WB_REF_WS_AIR_BAL_OUTPUT_ADV
         where ID_AC=P_ID_AC and
               ID_WS=P_ID_WS and
               ID_BORT=P_ID_BORT and
               DATE_FROM=P_DATE_FROM;

         if R_COUNT>0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='Record on this date already exists!';
               end;
             else
               begin
                 str_msg:='Запись с такой датой уже существует!';
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
        P_ID:=SEC_WB_REF_WS_AIR_BAL_OUT_ADV.nextval();

        insert into WB_REF_WS_AIR_BAL_OUTPUT_ADV (ID,
                                                    ID_AC,
	                                                    ID_WS,
	                                                      ID_BORT,
	                                                        DATE_FROM,
	                                                          REMARK,	
                                                              U_NAME,
	                                                              U_IP,
	                                                                U_HOST_NAME,
	                                                                  DATE_WRITE)
        select P_ID,
                 P_ID_AC,
	                 P_ID_WS,
	                   P_ID_BORT,
	                     P_DATE_FROM,
	                       'EMPTY_STRING',
	                         P_U_NAME,
	                           P_U_IP,
	                             P_U_HOST_NAME,
                                 sysdate()
        from dual;


        insert into WB_REF_WS_AIR_BAL_OUTPUT_ADV_I (ID,
                                                      ID_AC,
	                                                      ID_WS,
	                                                        ID_BORT,
	                                                          DATE_FROM,	
                                                              ADV_ID,
	                                                              ITEM_ID,
                                                                  REMARK,
                                                                    U_NAME,
	                                                                    U_IP,
	                                                                      U_HOST_NAME,
	                                                                        DATE_WRITE)
        select SEC_WB_REF_WS_AIR_BO_ADV_I.nextval,
                 P_ID_AC,
	                 P_ID_WS,
	                   P_ID_BORT,
	                     P_DATE_FROM,
                         P_ID,
                           t.id,
	                           t.string_val,
	                             P_U_NAME,
	                               P_U_IP,
	                                 P_U_HOST_NAME,
                                     sysdate()

        from (select distinct id,
                                string_val
              from WB_TEMP_XML_ID) t;

        insert into WB_REF_WS_AIR_BAL_OUTPUT_DATA (ID,
                                                     ID_AC,
	                                                     ID_WS,
	                                                       ID_BORT,
	                                                         DATE_FROM,	
                                                             ITEM_ID,
	                                                             DET_COL_ID,
	                                                               IS_CHECK,
                                                                   REMARK,
                                                                     U_NAME,
	                                                                     U_IP,
	                                                                       U_HOST_NAME,
	                                                                         DATE_WRITE,
                                                                             adv_id)
         select SEC_WB_REF_WS_AIR_BAL_OUT_DATA.nextval,
                  P_ID_AC,
	                  P_ID_WS,
	                    P_ID_BORT,
	                      P_DATE_FROM,
                          id,
                            num,
                              number_val,
                                'EMPTY_STRING',
                                  P_U_NAME,
	                                  P_U_IP,
	                                    P_U_HOST_NAME,
                                        sysdate(),
                                          P_ID
         from WB_TEMP_XML_ID;

        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list id="'||to_char(P_ID)||'" str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_BO_INSERT;
/
