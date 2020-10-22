create or replace procedure SP_WB_REF_WS_AIR_EQUIP_ADV_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_LANG varchar2(50):='';

P_TABLE_NAME varchar2(1000):='';
P_PWL_BALANCE_ARM int:=0;
P_PWL_INDEX_UNIT int:=0;
P_GOL_BALANCE_ARM int:=0;
P_GOL_INDEX_UNIT int:=0;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
R_COUNT int:=0;

V_ACTION_NAME varchar2(100):='SP_WB_REF_WS_AIR_CABIN_ADV_UPD';
V_ACTION_DATE date:=sysdate();
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_ID[1]')) into P_ID from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_TABLE_NAME[1]') into P_TABLE_NAME from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_PWL_BALANCE_ARM[1]')) into P_PWL_BALANCE_ARM from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_PWL_INDEX_UNIT[1]')) into P_PWL_INDEX_UNIT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_GOL_BALANCE_ARM[1]')) into P_GOL_BALANCE_ARM from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_GOL_INDEX_UNIT[1]')) into P_GOL_INDEX_UNIT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';
    ----------------------------------------------------------------------------
    select count(id) into R_COUNT
    from WB_REF_WS_AIR_EQUIP_ADV
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
        delete WB_TEMP_XML_ID_EX
        where ACTION_NAME=V_ACTION_NAME and
              ACTION_DATE=V_ACTION_DATE;

        insert into WB_TEMP_XML_ID_EX (ID,
                                         F_STR_1,
                                           ACTION_NAME,
                                             ACTION_DATE)
        select distinct P_ID,
                          f.p_tank_name,
                            V_ACTION_NAME,
                              V_ACTION_DATE
        from (select extractValue(value(t),'pwl_data/P_TANK_NAME[1]') as P_TANK_NAME
              from table(xmlsequence(xmltype(cXML_in).extract('//pwl_data'))) t) f;


        select count(t.id) into R_COUNT
        from WB_TEMP_XML_ID_EX t
        where t.ACTION_NAME=V_ACTION_NAME and
              t.ACTION_DATE=V_ACTION_DATE and
              exists(select d.id
                     from WB_REF_RESERVED_PHRASE d
                     where d.PHRASE=t.F_STR_1);

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 str_msg:='Block "Portable Water Locations" - some of the imposed "Tank Name" names is reserved phrase!';
              end;
            else
              begin
                str_msg:='Блок "Portable Water Locations" - некоторые из введенных названий "Tank Name" является зарезервированной фразой!';
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
        delete WB_TEMP_XML_ID_EX
        where ACTION_NAME=V_ACTION_NAME and
              ACTION_DATE=V_ACTION_DATE;

        insert into WB_TEMP_XML_ID_EX (ID,
                                         F_FLT_1,
                                           F_STR_1,
                                             ACTION_NAME,
                                               ACTION_DATE)
        select distinct f.p_deck_id,
                          f.p_type_id,
                            f.p_description,
                              V_ACTION_NAME,
                                V_ACTION_DATE
        from (select extractValue(value(t),'gol_data/P_DECK_ID[1]') as P_DECK_ID,
                       extractValue(value(t),'gol_data/P_TYPE_ID[1]') as P_TYPE_ID,
                         extractValue(value(t),'gol_data/P_DESCRIPTION[1]') as P_DESCRIPTION
              from table(xmlsequence(xmltype(cXML_in).extract('//gol_data'))) t) f;

        select count(t.id) into R_COUNT
        from WB_TEMP_XML_ID_EX t
        where t.ACTION_NAME=V_ACTION_NAME and
              t.ACTION_DATE=V_ACTION_DATE and
               not exists(select d.id
                         from WB_REF_WS_DECK d
                         where d.id=t.id);

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 str_msg:='Block "Galleys and Other Locations" - some of the names entered "Deck" removed from the directory!';
              end;
            else
              begin
                str_msg:='Блок "Galleys and Other Locations" - некоторые из введенных названий "Deck" удалены из справочника!';
              end;
            end if;
          end;
        end if;

        select count(t.id) into R_COUNT
        from WB_TEMP_XML_ID_EX t
        where t.ACTION_NAME=V_ACTION_NAME and
              t.ACTION_DATE=V_ACTION_DATE and
               not exists(select d.id
                         from WB_REF_WS_GOL_LOCATIONS d
                         where d.id=t.F_FLT_1);

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 str_msg:='Block "Galleys and Other Locations" - some of the names entered "Type" removed from the directory!';
              end;
            else
              begin
                str_msg:='Блок "Galleys and Other Locations" - некоторые из введенных названий "Type" удалены из справочника!';
              end;
            end if;
          end;
        end if;


        select count(t.id) into R_COUNT
        from WB_TEMP_XML_ID_EX t
        where t.ACTION_NAME=V_ACTION_NAME and
              t.ACTION_DATE=V_ACTION_DATE and
              exists(select d.id
                     from WB_REF_RESERVED_PHRASE d
                     where d.PHRASE=t.F_STR_1);

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 str_msg:='Block "Galleys and Other Locations" - some of the imposed "Description" names is reserved phrase!';
              end;
            else
              begin
                str_msg:='Блок "Galleys and Other Locations" - некоторые из введенных названий "Description" являются зарезервированной фразой!';
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
        update WB_REF_WS_AIR_EQUIP_IDN
        set TABLE_NAME=P_TABLE_NAME,
            U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where id=P_ID;

        UPDATE WB_REF_WS_AIR_EQUIP_ADV
        set PWL_BALANCE_ARM=P_PWL_BALANCE_ARM,
	          PWL_INDEX_UNIT=P_PWL_INDEX_UNIT,
	          GOL_BALANCE_ARM=P_GOL_BALANCE_ARM,
	          GOL_INDEX_UNIT=P_GOL_INDEX_UNIT,
            U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where idn=P_ID;


    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_EQUIP_PWL_HST(ID_,
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
                                                                      TANK_NAME,
	                                                                      MAX_WEIGHT,
	                                                                        L_CENTROID,
	                                                                          BA_CENTROID,
	                                                                            BA_FWD,
	                                                                              BA_AFT,
	                                                                                INDEX_PER_WT_UNIT,
	                                                                                  SHOW_ON_PLAN,
		                                                                                  U_NAME,
		                                                                                    U_IP,
		                                                                                      U_HOST_NAME,
		                                                                                        DATE_WRITE)
    select SEC_WB_REF_WS_AIR_EQP_PWL_HST.nextval,
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
	                                   i.TANK_NAME,
	                                     i.MAX_WEIGHT,
	                                       i.L_CENTROID,
	                                         i.BA_CENTROID,
	                                           i.BA_FWD,
	                                             i.BA_AFT,
	                                               i.INDEX_PER_WT_UNIT,
	                                                 i.SHOW_ON_PLAN,	
		                         	                       i.U_NAME,
		                               	                   i.U_IP,
		                                	                   i.U_HOST_NAME,
		                                 	                     i.DATE_WRITE
     from WB_REF_WS_AIR_EQUIP_PWL i
     where i.IDN=P_ID;

     delete from WB_REF_WS_AIR_EQUIP_PWL
     where IDN=P_ID;

     insert into WB_REF_WS_AIR_EQUIP_PWL (ID,
	                                         ID_AC,
	                                           ID_WS,
	                                             ID_BORT,
                                         	       IDN,
                                         	         ADV_ID,
	                                                   TANK_NAME,
	                                                     MAX_WEIGHT,
	                                                       L_CENTROID,
	                                                         BA_CENTROID,
	                                                           BA_FWD,
	                                                             BA_AFT,
	                                                               INDEX_PER_WT_UNIT,
	                                                                 SHOW_ON_PLAN,
                                                                     U_NAME,
	                                                                     U_IP,
	                                                                       U_HOST_NAME,
	                                                                         DATE_WRITE)
     select SEC_WB_REF_WS_AIR_EQUIP_PWL.nextval,
              a.ID_AC,
	              a.ID_WS,
	                a.ID_BORT,
                    a.IDN,
                      a.id,
                        f.P_TANK_NAME,

                          case when f.P_MAX_WEIGHT_INT_PART='NULL'
                               then NULL
                               else to_number(f.P_MAX_WEIGHT_INT_PART||'.'||f.P_MAX_WEIGHT_DEC_PART)
                          end,

                            case when f.P_L_CENTROID_INT_PART='NULL'
                                 then NULL
                                 else to_number(f.P_L_CENTROID_INT_PART||'.'||f.P_L_CENTROID_DEC_PART)
                            end,

                             case when f.P_BA_CENTROID_INT_PART='NULL'
                                  then NULL
                                  else to_number(f.P_BA_CENTROID_INT_PART||'.'||f.P_BA_CENTROID_DEC_PART)
                             end,

                               case when f.P_BA_FWD_INT_PART='NULL'
                                    then NULL
                                    else to_number(f.P_BA_FWD_INT_PART||'.'||f.P_BA_FWD_DEC_PART)
                               end,

                                 case when f.P_BA_AFT_INT_PART='NULL'
                                      then NULL
                                      else to_number(f.P_BA_AFT_INT_PART||'.'||f.P_BA_AFT_DEC_PART)
                                 end,

                                   case when f.P_INDEX_PER_WT_UNIT_INT_PART='NULL'
                                        then NULL
                                        else to_number(f.P_INDEX_PER_WT_UNIT_INT_PART||'.'||f.P_INDEX_PER_WT_UNIT_DEC_PART)
                                   end,

                                     to_number(f.P_SHOW_ON_PLAN),

                                       P_U_NAME,
	                                       P_U_IP,
	                                         P_U_HOST_NAME,
                                             SYSDATE()
     from WB_REF_WS_AIR_EQUIP_ADV a join (select extractValue(value(t),'pwl_data/P_TANK_NAME[1]') as P_TANK_NAME,

                                                   extractValue(value(t),'pwl_data/P_MAX_WEIGHT_INT_PART[1]') as P_MAX_WEIGHT_INT_PART,
                                                     extractValue(value(t),'pwl_data/P_MAX_WEIGHT_DEC_PART[1]') as P_MAX_WEIGHT_DEC_PART,

                                                       extractValue(value(t),'pwl_data/P_L_CENTROID_INT_PART[1]') as P_L_CENTROID_INT_PART,
                                                         extractValue(value(t),'pwl_data/P_L_CENTROID_DEC_PART[1]') as P_L_CENTROID_DEC_PART,

                                                           extractValue(value(t),'pwl_data/P_BA_CENTROID_INT_PART[1]') as P_BA_CENTROID_INT_PART,
                                                             extractValue(value(t),'pwl_data/P_BA_CENTROID_DEC_PART[1]') as P_BA_CENTROID_DEC_PART,

                                                               extractValue(value(t),'pwl_data/P_BA_FWD_INT_PART[1]') as P_BA_FWD_INT_PART,
                                                                 extractValue(value(t),'pwl_data/P_BA_FWD_DEC_PART[1]') as P_BA_FWD_DEC_PART,

                                                                   extractValue(value(t),'pwl_data/P_BA_AFT_INT_PART[1]') as P_BA_AFT_INT_PART,
                                                                     extractValue(value(t),'pwl_data/P_BA_AFT_DEC_PART[1]') as P_BA_AFT_DEC_PART,

                                                                       extractValue(value(t),'pwl_data/P_INDEX_PER_WT_UNIT_INT_PART[1]') as P_INDEX_PER_WT_UNIT_INT_PART,
                                                                         extractValue(value(t),'pwl_data/P_INDEX_PER_WT_UNIT_DEC_PART[1]') as P_INDEX_PER_WT_UNIT_DEC_PART,

                                                                           extractValue(value(t),'pwl_data/P_SHOW_ON_PLAN[1]') as P_SHOW_ON_PLAN
                                          from table(xmlsequence(xmltype(cXML_in).extract('//pwl_data'))) t) f
     on a.idn=P_ID;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_EQUIP_BORT_HS(ID_,
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
    select SEC_WB_REF_WS_AIR_EQP_BORT_HST.nextval,
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
     from WB_REF_WS_AIR_EQUIP_BORT i
     where i.IDN=P_ID;

     delete from WB_REF_WS_AIR_EQUIP_BORT
     where IDN=P_ID;

     insert into WB_REF_WS_AIR_EQUIP_BORT(ID,
	                                         ID_AC,
	                                           ID_WS,
	                                             ID_BORT,
                                         	       IDN,
                                         	         ADV_ID,
                                                     U_NAME,
	                                                     U_IP,
	                                                       U_HOST_NAME,
	                                                         DATE_WRITE)
     select SEC_WB_REF_WS_AIR_EQP_BORT.nextval,
              a.ID_AC,
	              a.ID_WS,
	                f.ID_BORT,
                    a.IDN,
                      a.id,
                        P_U_NAME,
	                        P_U_IP,
	                          P_U_HOST_NAME,
                              SYSDATE()
     from WB_REF_WS_AIR_EQUIP_ADV a join (select to_number(extractValue(value(t),'bort_data/P_ID_BORT[1]')) as ID_BORT
                                          from table(xmlsequence(xmltype(cXML_in).extract('//bort_data'))) t) f
     on a.idn=P_ID join WB_REF_AIRCO_WS_BORTS b
        on b.id=f.ID_BORT and
           b.id_ac=a.ID_AC and
           b.id_ws=a.ID_WS;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_EQUIP_GOL_HST(ID_,
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
                                                                      TYPE_ID,
	                                                                      DECK_ID,
	                                                                        DESCRIPTION,
	                                                                          MAX_WEIGHT,
	                                                                            LA_CENTROID,
	                                                                              LA_FROM,
	                                                                                LA_TO,
	                                                                                  BA_CENTROID,
	                                                                                    BA_FWD,
	                                                                                      BA_AFT,
	                                                                                        INDEX_PER_WT_UNIT,
	                                                                                          SHOW_ON_PLAN,
		                                                                                          U_NAME,
		                                                                                            U_IP,
		                                                                                              U_HOST_NAME,
		                                                                                                DATE_WRITE)
    select SEC_WB_REF_WS_AIR_EQP_GOL_HST.nextval,
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
	                                   i.TYPE_ID,
	                                     i.DECK_ID,
	                                       i.DESCRIPTION,
	                                         i.MAX_WEIGHT,
	                                           i.LA_CENTROID,
	                                             i.LA_FROM,
	                                               i.LA_TO,
	                                                 i.BA_CENTROID,
	                                                   i.BA_FWD,
	                                                     i.BA_AFT,
	                                                       i.INDEX_PER_WT_UNIT,
	                                                         i.SHOW_ON_PLAN,	
		                         	                               i.U_NAME,
		                               	                           i.U_IP,
		                                	                           i.U_HOST_NAME,
		                                 	                             i.DATE_WRITE
     from WB_REF_WS_AIR_EQUIP_GOL i
     where i.IDN=P_ID;

     delete from WB_REF_WS_AIR_EQUIP_GOL
     where IDN=P_ID;

     insert into WB_REF_WS_AIR_EQUIP_GOL (ID,
	                                         ID_AC,
	                                           ID_WS,
	                                             ID_BORT,
                                         	       IDN,
                                         	         ADV_ID,
	                                                   TYPE_ID,
	                                                     DECK_ID,
	                                                       DESCRIPTION,
	                                                         MAX_WEIGHT,
	                                                           LA_CENTROID,
	                                                             LA_FROM,
	                                                               LA_TO,
	                                                                 BA_CENTROID,
	                                                                   BA_FWD,
	                                                                     BA_AFT,
	                                                                       INDEX_PER_WT_UNIT,
	                                                                         SHOW_ON_PLAN,
                                                                             U_NAME,
	                                                                             U_IP,
	                                                                               U_HOST_NAME,
	                                                                                 DATE_WRITE)
     select SEC_WB_REF_WS_AIR_EQUIP_PWL.nextval,
              a.ID_AC,
	              a.ID_WS,
	                a.ID_BORT,
                    a.IDN,
                      a.id,
                        f.P_TYPE_ID,
                          f.P_DECK_ID,
                            f.P_DESCRIPTION,

                              case when f.P_MAX_WEIGHT_INT_PART='NULL'
                                   then NULL
                                   else to_number(f.P_MAX_WEIGHT_INT_PART||'.'||f.P_MAX_WEIGHT_DEC_PART)
                              end,

                                case when f.P_LA_CENTROID_INT_PART='NULL'
                                     then NULL
                                     else to_number(f.P_LA_CENTROID_INT_PART||'.'||f.P_LA_CENTROID_DEC_PART)
                                end,

                                 case when f.P_LA_FROM_INT_PART='NULL'
                                      then NULL
                                      else to_number(f.P_LA_FROM_INT_PART||'.'||f.P_LA_FROM_DEC_PART)
                                 end,

                                   case when f.P_LA_TO_INT_PART='NULL'
                                        then NULL
                                        else to_number(f.P_LA_TO_INT_PART||'.'||f.P_LA_TO_DEC_PART)
                                   end,

                                     case when f.P_BA_CENTROID_INT_PART='NULL'
                                          then NULL
                                          else to_number(f.P_BA_CENTROID_INT_PART||'.'||f.P_BA_CENTROID_DEC_PART)
                                     end,

                                       case when f.P_BA_FWD_INT_PART='NULL'
                                            then NULL
                                            else to_number(f.P_BA_FWD_INT_PART||'.'||f.P_BA_FWD_DEC_PART)
                                       end,

                                         case when f.P_BA_AFT_INT_PART='NULL'
                                              then NULL
                                              else to_number(f.P_BA_AFT_INT_PART||'.'||f.P_BA_AFT_DEC_PART)
                                         end,

                                           case when f.P_INDEX_PER_WT_UNIT_INT_PART='NULL'
                                                then NULL
                                                else to_number(f.P_INDEX_PER_WT_UNIT_INT_PART||'.'||f.P_INDEX_PER_WT_UNIT_DEC_PART)
                                           end,

                                             to_number(f.P_SHOW_ON_PLAN),

                                               P_U_NAME,
	                                               P_U_IP,
	                                                 P_U_HOST_NAME,
                                                     SYSDATE()
     from WB_REF_WS_AIR_EQUIP_ADV a join (select extractValue(value(t),'gol_data/P_DECK_ID[1]') as P_DECK_ID,
                                                   extractValue(value(t),'gol_data/P_TYPE_ID[1]') as P_TYPE_ID,
                                                     extractValue(value(t),'gol_data/P_DESCRIPTION[1]') as P_DESCRIPTION,

                                                       extractValue(value(t),'gol_data/P_MAX_WEIGHT_INT_PART[1]') as P_MAX_WEIGHT_INT_PART,
                                                         extractValue(value(t),'gol_data/P_MAX_WEIGHT_DEC_PART[1]') as P_MAX_WEIGHT_DEC_PART,

                                                           extractValue(value(t),'gol_data/P_LA_CENTROID_INT_PART[1]') as P_LA_CENTROID_INT_PART,
                                                             extractValue(value(t),'gol_data/P_LA_CENTROID_DEC_PART[1]') as P_LA_CENTROID_DEC_PART,

                                                               extractValue(value(t),'gol_data/P_LA_FROM_INT_PART[1]') as P_LA_FROM_INT_PART,
                                                                 extractValue(value(t),'gol_data/P_LA_FROM_DEC_PART[1]') as P_LA_FROM_DEC_PART,

                                                                   extractValue(value(t),'gol_data/P_LA_TO_INT_PART[1]') as P_LA_TO_INT_PART,
                                                                     extractValue(value(t),'gol_data/P_LA_TO_DEC_PART[1]') as P_LA_TO_DEC_PART,

                                                                       extractValue(value(t),'gol_data/P_BA_CENTROID_INT_PART[1]') as P_BA_CENTROID_INT_PART,
                                                                         extractValue(value(t),'gol_data/P_BA_CENTROID_DEC_PART[1]') as P_BA_CENTROID_DEC_PART,

                                                                           extractValue(value(t),'gol_data/P_BA_FWD_INT_PART[1]') as P_BA_FWD_INT_PART,
                                                                             extractValue(value(t),'gol_data/P_BA_FWD_DEC_PART[1]') as P_BA_FWD_DEC_PART,

                                                                               extractValue(value(t),'gol_data/P_BA_AFT_INT_PART[1]') as P_BA_AFT_INT_PART,
                                                                                 extractValue(value(t),'gol_data/P_BA_AFT_DEC_PART[1]') as P_BA_AFT_DEC_PART,

                                                                                   extractValue(value(t),'gol_data/P_INDEX_PER_WT_UNIT_INT_PART[1]') as P_INDEX_PER_WT_UNIT_INT_PART,
                                                                                     extractValue(value(t),'gol_data/P_INDEX_PER_WT_UNIT_DEC_PART[1]') as P_INDEX_PER_WT_UNIT_DEC_PART,

                                                                                       extractValue(value(t),'gol_data/P_SHOW_ON_PLAN[1]') as P_SHOW_ON_PLAN
                                          from table(xmlsequence(xmltype(cXML_in).extract('//gol_data'))) t) f
     on a.idn=P_ID;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

        str_msg:='EMPTY_STRING';
      end;
    end if;

    commit;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(str_msg)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_EQUIP_ADV_UPD;
/
