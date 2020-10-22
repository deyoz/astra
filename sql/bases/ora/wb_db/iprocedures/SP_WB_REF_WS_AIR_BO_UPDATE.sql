create or replace procedure SP_WB_REF_WS_AIR_BO_UPDATE
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
    select extractValue(xmltype(cXML_in),'/root[1]/id[1]/P_ID[1]') into P_ID from dual;
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
        select count(id) into R_COUNT
        from WB_REF_WS_AIR_BAL_OUTPUT_ADV
        where id=P_ID;

        if R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                str_msg:='This record is removed!';
              end;
            else
              begin
                str_msg:='Эта запись удалена!';
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
         where ID<>P_ID and
               ID_AC=P_ID_AC and
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
        update WB_REF_WS_AIR_BAL_OUTPUT_ADV
        set date_from=P_DATE_FROM,
            u_name=P_U_NAME,
            u_ip=P_U_IP,
            u_host_name=P_U_HOST_NAME,
            date_write=sysdate()
        where id=P_ID;

        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
        insert into WB_REF_WS_AIR_BAL_OUT_AD_I_HST(ID_,
	                                                   U_NAME_,
	                                                     U_IP_,
    	                                                   U_HOST_NAME_,
	                                                         DATE_WRITE_,
                                                             OPERATION_,
	                                                             ACTION_,
	                                                               ID,
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
        select SEC_WB_REF_WS_AIR_BO_ADV_I_HST.nextval,
                P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE(),
                         'delete',
                           'delete',
                             i.id,
                               i.ID_AC,
	                               i.ID_WS,
		                               i.ID_BORT,
		                                 i.DATE_FROM,
                                       i.ADV_ID,
	                                       i.ITEM_ID,
		                                       i.REMARK,
		                                         i.U_NAME,
		                                           i.U_IP,
		                                             i.U_HOST_NAME,
		                                               i.DATE_WRITE
        from WB_REF_WS_AIR_BAL_OUTPUT_ADV_I i
        where i.adv_id=P_ID;

        delete from WB_REF_WS_AIR_BAL_OUTPUT_ADV_I
        where adv_id=P_ID;

        insert into WB_REF_WS_AIR_BAL_OUT_DATA_HST (ID_,
	                                                    U_NAME_,
    	                                                  U_IP_,
	                                                        U_HOST_NAME_,
	                                                          DATE_WRITE_,
	                                                            ACTION_,
	                                                              ID,
	                                                                ID_AC_OLD,
	                                                                  ID_WS_OLD,
		                                                                  ID_BORT_OLD,
		                                                                    DATE_FROM_OLD,
		                                                                      ITEM_ID_OLD,
	                                                                          DET_COL_ID_OLD,
	                                                                            IS_CHECK_OLD,
                                                                                REMARK_OLD,
		                                                                              U_NAME_OLD,
		                                                                                U_IP_OLD,
		                                                                                  U_HOST_NAME_OLD,
		                                                                                    DATE_WRITE_OLD,
		                                                                                      ID_AC_NEW,
		                                                                                        ID_WS_NEW,
		                                                                                          ID_BORT_NEW,
		                                                                                            DATE_FROM_NEW,
		                                                                                              ITEM_ID_NEW,
	                                                                                                  DET_COL_ID_NEW,
	                                                                                                    IS_CHECK_NEW,
		                                                                                                    REMARK_NEW,
		                                                                                                      U_NAME_NEW,
		                                                                                                        U_IP_NEW,
    		                                                                                                      U_HOST_NAME_NEW,
		                                                                                                            DATE_WRITE_NEW,
                                                                                                                  ADV_ID_OLD,
                                                                                                                    ADV_ID_NEW)
        select SEC_WB_REF_WS_AIR_BL_OT_DT_HST.nextval,
                 P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE(),
                         'delete',
                           i.ID,
	                           i.ID_AC,
	                             i.ID_WS,
		                             i.ID_BORT,
		                               i.DATE_FROM,
		                                 i.ITEM_ID,
	                                     i.DET_COL_ID,
	                                       i.IS_CHECK,
                                           i.REMARK,
		                                         i.U_NAME,
		                                           i.U_IP,
    		                                         i.U_HOST_NAME,
		                                               i.DATE_WRITE,
		                                                 i.ID_AC,
		                                                   i.ID_WS,
		                                                     i.ID_BORT,
		                                                       i.DATE_FROM,
		                                                         i.ITEM_ID,
	                                                             i.DET_COL_ID,
	                                                               i.IS_CHECK,
		                                                               i.REMARK,
		                                                                 i.U_NAME,
		                                                                   i.U_IP,
		                                                                     i.U_HOST_NAME,
		                                                                       i.DATE_WRITE,
                                                                             i.ADV_ID,
                                                                               i.ADV_ID
        from WB_REF_WS_AIR_BAL_OUTPUT_DATA i
        where i.adv_id=P_ID;

        delete
        from WB_REF_WS_AIR_BAL_OUTPUT_DATA
        where adv_id=P_ID;
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
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

    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_BO_UPDATE;
/
