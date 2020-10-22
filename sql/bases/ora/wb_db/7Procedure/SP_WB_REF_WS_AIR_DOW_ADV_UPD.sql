create or replace procedure SP_WB_REF_WS_AIR_DOW_ADV_UPD
(cXML_in in clob,
   cXML_out out clob)
as

P_LANG varchar2(50):='';
P_ID_AC number:=-1;
P_ID_WS number:=-1;
P_ID_BORT number:=-1;
P_REMARKS clob:='';
P_IS_REMARKS_EMPTY number:=0;
P_BASIC_WEIGHT number:=0;
P_DOW number:=0;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT number:=-1;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID_AC[1]')) into P_ID_AC from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID_WS[1]')) into P_ID_WS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID_BORT[1]')) into P_ID_BORT from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_BASIC_WEIGHT[1]')) into P_BASIC_WEIGHT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_DOW[1]')) into P_DOW from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_REMARKS[1]') into P_REMARKS from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_IS_REMARKS_EMPTY[1]')) into P_IS_REMARKS_EMPTY from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;


    cXML_out:='<?xml version="1.0" ?><root>';
    --------------------------------------------------------------------------------------------------------------------------------------------------------
    select count(id) into V_R_COUNT
    from WB_REF_WS_AIR_DOW_ADV
    where ID_AC=P_ID_AC and
          ID_WS=P_ID_WS and
          ID_BORT=P_ID_BORT;

    if V_R_COUNT=0 then
      begin
        if P_LANG='ENG' then
          begin
            V_STR_MSG:='This record is removed!';
          end;
        else
          begin
            V_STR_MSG:='Эта запись удалена!';
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
        update WB_REF_WS_AIR_DOW_ADV
        set CH_BASIC_WEIGHT=P_BASIC_WEIGHT,
	          CH_DOW=P_DOW,
	          REMARK=P_REMARKS,
		        U_NAME=P_U_NAME,
		        U_IP=P_U_IP,
		        U_HOST_NAME=P_U_HOST_NAME,
		        DATE_WRITE=sysdate()
        where ID_AC=P_ID_AC and
              ID_WS=P_ID_WS and
              ID_BORT=P_ID_BORT;


        insert into WB_REF_WS_AIR_DOW_ITM_HST(ID_,
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
                                                                    IDN,
	                                                                    ADV_ID,
	                                                                      ITEM_ID,
	                                                                        IS_INCLUDED,
	                                                                          IS_EXCLUDED,
	                                                                            REMARK,
		                                                                            U_NAME,
		                                                                              U_IP,
		                                                                                U_HOST_NAME,
		                                                                                  DATE_WRITE)
        select SEC_WB_REF_WS_AIR_DOW_ITM_HST.nextval,
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
                                     i.IDN,
	                                     i.ADV_ID,
                                         i.ITEM_ID,
	                                         i.IS_INCLUDED,
	                                           i.IS_EXCLUDED,
	                                             i.REMARK,
		                                             i.U_NAME,
		                                               i.U_IP,
		                                                 i.U_HOST_NAME,
		                                                   i.DATE_WRITE
        from WB_REF_WS_AIR_DOW_ADV a join WB_REF_WS_AIR_DOW_ITM i
        on a.ID_AC=P_ID_AC and
           a.ID_WS=P_ID_WS and
           a.ID_BORT=P_ID_BORT and
           a.id=i.adv_id;


       delete from WB_REF_WS_AIR_DOW_ITM
       where exists(select 1
                    from WB_REF_WS_AIR_DOW_ADV t
                    where t.ID_AC=P_ID_AC and
                          t.ID_WS=P_ID_WS and
                          t.ID_BORT=P_ID_BORT and
                          t.id=WB_REF_WS_AIR_DOW_ITM.adv_id);


       insert into WB_REF_WS_AIR_DOW_ITM (ID,
	                                          ID_AC,
	                                            ID_WS,
	                                              ID_BORT,
	                                                IDN,
	                                                  ADV_ID,
	                                                    ITEM_ID,
	                                                      IS_INCLUDED,
	                                                        IS_EXCLUDED,
	                                                          REMARK,
	                                                            U_NAME,
	                                                              U_IP,
	                                                                U_HOST_NAME,
	                                                                  DATE_WRITE)
       select SEC_WB_REF_WS_AIR_DOW_ITM.nextval,
                f.ID_AC,
	                f.ID_WS,
	                  f.ID_BORT,
	                    f.IDN,
	                      f.ADV_ID,
                          to_number(f.P_ID),
                            to_number(f.P_IS_INCLUDED),
                              to_number(f.P_IS_EXCLUDED),
                                f.P_REMARK,
                                  P_U_NAME,
	                                  P_U_IP,
	                                    P_U_HOST_NAME,
                                        SYSDATE()
       from (select distinct extractValue(value(t),'table_data/P_ID[1]') as P_ID,
                               extractValue(value(t),'table_data/P_IS_INCLUDED[1]') as P_IS_INCLUDED,
                                 extractValue(value(t),'table_data/P_IS_EXCLUDED[1]') as P_IS_EXCLUDED,
                                   extractValue(value(t),'table_data/P_IS_REMARK_EMPTY[1]') as P_IS_REMARK_EMPTY,
                                     extractValue(value(t),'table_data/P_REMARK[1]') as P_REMARK,
                                       a.id as ADV_ID,
                                         a.id_ac,
                                           a.id_ws,
                                            a.id_bort,
                                              a.idn
             from table(xmlsequence(xmltype(cXML_in).extract('//table_data'))) t join WB_REF_WS_AIR_DOW_ADV a
             on a.ID_AC=P_ID_AC and
                a.ID_WS=P_ID_WS and
                a.ID_BORT=P_ID_BORT) f;



        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_DOW_ADV_UPD;
/
