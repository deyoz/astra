create or replace procedure SP_WB_REF_WS_AIR_CFV_ADV_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_LANG varchar2(50):='';

P_TABLE_NAME varchar2(1000):='';
P_PROC_NAME varchar2(1000):='';

P_CH_WEIGHT number:=-1;
P_CH_VOLUME number:=-1;
P_CH_BALANCE_ARM number:=-1;
P_CH_INDEX_UNIT number:=-1;
P_CH_STANDART number:=-1;
P_CH_NON_STANDART number:=-1;
P_CH_USE_BY_DEFAULT number:=-1;


P_CH_INDEX number:=-1;
P_CH_PROC_MAC number:=-1;
P_CH_CERTIFIED number:=-1;
P_CH_CURTAILED number:=-1;

P_CONDITION varchar2(1000):='';
P_IS_CONDITION_EMPTY number;
P_TYPE varchar2(1000):='';
P_IS_TYPE_EMPTY number;

P_REMARK clob:='';
P_IS_REMARK_EMPTY number:=0;

P_DENSITY_INT_PART varchar2(1000):='';
P_DENSITY_DEC_PART varchar2(1000):='';
P_DENSITY number:=null;

P_MAX_VOLUME_INT_PART varchar2(1000):='';
P_MAX_VOLUME_DEC_PART varchar2(1000):='';
P_MAX_VOLUME number:=null;

P_MAX_WEIGHT_INT_PART varchar2(1000):='';
P_MAX_WEIGHT_DEC_PART varchar2(1000):='';
P_MAX_WEIGHT number:=null;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
R_COUNT int:=0;

V_ACTION_NAME varchar2(100):='SP_WB_REF_WS_AIR_CFV_ADV_UPD';
V_ACTION_DATE date:=sysdate();
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID[1]')) into P_ID from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_WEIGHT[1]')) into P_CH_WEIGHT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_VOLUME[1]')) into P_CH_VOLUME from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_BALANCE_ARM[1]')) into P_CH_BALANCE_ARM from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_INDEX_UNIT[1]')) into P_CH_INDEX_UNIT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_STANDART[1]')) into P_CH_STANDART from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_NON_STANDART[1]')) into P_CH_NON_STANDART from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CH_USE_BY_DEFAULT[1]')) into P_CH_USE_BY_DEFAULT from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_TABLE_NAME[1]') into P_TABLE_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_PROC_NAME[1]') into P_PROC_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_REMARKS[1]') into P_REMARK from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_IS_REMARKS_EMPTY[1]')) into P_IS_REMARK_EMPTY from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_DENSITY_INT_PART[1]') into P_DENSITY_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_DENSITY_DEC_PART[1]') into P_DENSITY_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_MAX_VOLUME_INT_PART[1]') into P_MAX_VOLUME_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_MAX_VOLUME_DEC_PART[1]') into P_MAX_VOLUME_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_MAX_WEIGHT_INT_PART[1]') into P_MAX_WEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_MAX_WEIGHT_DEC_PART[1]') into P_MAX_WEIGHT_DEC_PART from dual;

    if P_DENSITY_INT_PART<>'NULL' then
      begin
        P_DENSITY:=to_number(P_DENSITY_INT_PART||'.'||P_DENSITY_DEC_PART);
      end;
    end if;

    if P_MAX_VOLUME_INT_PART<>'NULL' then
      begin
        P_MAX_VOLUME:=to_number(P_MAX_VOLUME_INT_PART||'.'||P_MAX_VOLUME_DEC_PART);
      end;
    end if;

    if P_MAX_WEIGHT_INT_PART<>'NULL' then
      begin
        P_MAX_WEIGHT:=to_number(P_MAX_WEIGHT_INT_PART||'.'||P_MAX_WEIGHT_DEC_PART);
      end;
    end if;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';
    ----------------------------------------------------------------------------
    select count(id) into R_COUNT
    from WB_REF_WS_AIR_CFV_ADV
    where idn=P_ID;

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
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
     if P_CH_USE_BY_DEFAULT=1 then
      begin
        if str_msg is null then
          begin
            select count(a.id) into R_COUNT
            from WB_REF_WS_AIR_CFV_ADV a join WB_REF_WS_AIR_CFV_IDN i
            on i.id=P_ID and
               a.id_ac=i.id_ac and
               a.id_ws=i.id_ws and
               a.id_bort=i.id_bort and
               a.idn<>i.id and
               a.CH_USE_BY_DEFAULT=1;

           if R_COUNT>0 then
             begin
               if P_LANG='ENG' then
                 begin
                   str_msg:='Recording with a sign "Use By Default" already exists!';
                 end;
               else
                 begin
                   str_msg:='Запись с признаком "Use By Default" уже существует!';
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
        if p_IS_REMARK_EMPTY=1 then
          begin

            P_REMARK:='EMPTY_STRING';
          end;
        end if;

        if p_IS_REMARK_EMPTY=0 then
          begin
            if P_REMARK='EMPTY_STRING' then
              begin
                if P_LANG='ENG' then
                  begin
                     str_msg:='Value '||P_REMARK||' is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Значение '||P_REMARK||' является зарезервированной фразой!';
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
        select count(id) into R_COUNT
        from WB_REF_RESERVED_PHRASE
        where PHRASE=P_TABLE_NAME;

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 str_msg:='Value '||P_TABLE_NAME||' is a phrase reserved!';
              end;
            else
              begin
                str_msg:='Значение '||P_TABLE_NAME||' является зарезервированной фразой!';
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
        where PHRASE=P_PROC_NAME;

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 str_msg:='Value '||P_PROC_NAME||' is a phrase reserved!';
              end;
            else
              begin
                str_msg:='Значение '||P_PROC_NAME||' является зарезервированной фразой!';
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
        update WB_REF_WS_AIR_CFV_IDN
        set TABLE_NAME=P_TABLE_NAME,
            U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where id=P_ID;


        UPDATE WB_REF_WS_AIR_CFV_ADV
        set PROC_NAME=P_PROC_NAME,
            DENSITY=P_DENSITY,
	          MAX_VOLUME=P_MAX_VOLUME,
	          MAX_WEIGHT=P_MAX_WEIGHT,
            CH_WEIGHT=P_CH_WEIGHT,
	          CH_VOLUME=P_CH_VOLUME,
	          CH_BALANCE_ARM=P_CH_BALANCE_ARM,
	          CH_INDEX_UNIT=P_CH_INDEX_UNIT,
	          CH_STANDART=P_CH_STANDART,
	          CH_NON_STANDART=P_CH_NON_STANDART,
	          CH_USE_BY_DEFAULT=P_CH_USE_BY_DEFAULT,
	          REMARKS=P_REMARK,
            U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where idn=P_ID;
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------
        insert into WB_REF_WS_AIR_CFV_TBL_HST(ID_,
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
                                                                    WEIGHT,
	                                                                    VOLUME,
	                                                                      ARM,
	                                                                        INDEX_UNIT,
		                                                                        U_NAME,
		                                                                          U_IP,
		                                                                            U_HOST_NAME,
		                                                                              DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CFV_TBL_HST.nextval,
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
	                                   i.WEIGHT,
	                                     i.VOLUME,
	                                       i.ARM,
	                                         i.INDEX_UNIT,
		                                         i.U_NAME,
		                                           i.U_IP,
		                                             i.U_HOST_NAME,
                                                   i.DATE_WRITE
    from WB_REF_WS_AIR_CFV_TBL i
    where i.idn=P_ID;

    delete from WB_REF_WS_AIR_CFV_TBL
    where idn=P_ID;

    insert into WB_REF_WS_AIR_CFV_TBL(ID,
	                                      ID_AC,
	                                        ID_WS,
		                                        ID_BORT,
		                                          IDN,
                                                ADV_ID,
                                                  WEIGHT,
	                                                  VOLUME,
	                                                    ARM,
	                                                      INDEX_UNIT,
		                                                      U_NAME,
		                                                        U_IP,
		                                                          U_HOST_NAME,
		                                                            DATE_WRITE)

    select SEC_WB_REF_WS_AIR_CFV_TBL.nextval,
             a.ID_AC,
	             a.ID_WS,
	               a.ID_BORT,
                   a.IDN,
                     a.id,

                       case when f.P_WEIGHT_INT_PART='NULL'
                            then NULL
                            else to_number(f.P_WEIGHT_INT_PART||'.'||f.P_WEIGHT_DEC_PART)
                       end,

                         case when f.P_VOLUME_INT_PART='NULL'
                              then NULL
                              else to_number(f.P_VOLUME_INT_PART||'.'||f.P_VOLUME_DEC_PART)
                         end,

                          case when f.P_ARM_INT_PART='NULL'
                               then NULL
                               else to_number(f.P_ARM_INT_PART||'.'||f.P_ARM_DEC_PART)
                          end,

                            case when f.P_INDEX_INT_PART='NULL'
                                 then NULL
                                 else to_number(f.P_INDEX_INT_PART||'.'||f.P_INDEX_DEC_PART)
                            end,

                              P_U_NAME,
	                              P_U_IP,
	                                P_U_HOST_NAME,
                                    SYSDATE()
    from WB_REF_WS_AIR_CFV_ADV a join (select extractValue(value(t),'tbl_data/P_WEIGHT_INT_PART[1]') as P_WEIGHT_INT_PART,
                                                extractValue(value(t),'tbl_data/P_WEIGHT_DEC_PART[1]') as P_WEIGHT_DEC_PART,

                                                  extractValue(value(t),'tbl_data/P_VOLUME_INT_PART[1]') as P_VOLUME_INT_PART,
                                                    extractValue(value(t),'tbl_data/P_VOLUME_DEC_PART[1]') as P_VOLUME_DEC_PART,

                                                      extractValue(value(t),'tbl_data/P_ARM_INT_PART[1]') as P_ARM_INT_PART,
                                                        extractValue(value(t),'tbl_data/P_ARM_DEC_PART[1]') as P_ARM_DEC_PART,

                                                          extractValue(value(t),'tbl_data/P_INDEX_INT_PART[1]') as P_INDEX_INT_PART,
                                                            extractValue(value(t),'tbl_data/P_INDEX_DEC_PART[1]') as P_INDEX_DEC_PART
                                       from table(xmlsequence(xmltype(cXML_in).extract('//tbl_data'))) t) f
     on a.idn=P_ID;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

   -----------------------------------------------------------------------------
   -----------------------------------------------------------------------------
       insert into WB_REF_WS_AIR_CFV_BORT_HST(ID_,
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
		                                                                    U_NAME,
		                                                                      U_IP,
		                                                                        U_HOST_NAME,
		                                                                          DATE_WRITE)
        select SEC_WB_REF_WS_AIR_CFV_BORT_HST.nextval,
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
		                                     i.U_NAME,
		                                       i.U_IP,
		                                         i.U_HOST_NAME,
                                               i.DATE_WRITE
        from WB_REF_WS_AIR_CFV_BORT i
        where i.idn=P_ID;

        delete from WB_REF_WS_AIR_CFV_BORT
        where idn=P_ID;

        insert into WB_REF_WS_AIR_CFV_BORT(ID,
	                                           ID_AC,
	                                             ID_WS,
		                                             ID_BORT,
		                                               IDN,
                                                     ADV_ID,
		                                                   U_NAME,
		                                                     U_IP,
		                                                       U_HOST_NAME,
		                                                         DATE_WRITE)

        select SEC_WB_REF_WS_AIR_CFV_BORT.nextval,
                 a.ID_AC,
	                 a.ID_WS,
	                   f.ID_BORT,
                       a.IDN,
                         a.id,
                           P_U_NAME,
	                           P_U_IP,
	                             P_U_HOST_NAME,
                                 SYSDATE()
         from WB_REF_WS_AIR_CFV_ADV a join (select to_number(extractValue(value(t),'bort_data/P_ID_BORT[1]')) as ID_BORT
                                            from table(xmlsequence(xmltype(cXML_in).extract('//bort_data'))) t) f
         on a.idn=P_ID join WB_REF_AIRCO_WS_BORTS b
            on b.id=f.ID_BORT and
               b.id_ac=a.ID_AC and
               b.id_ws=a.ID_WS;
        ------------------------------------------------------------------------
        ------------------------------------------------------------------------

        str_msg:='EMPTY_STRING';
      end;
    end if;

    commit;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(str_msg)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_CFV_ADV_UPD;
/
