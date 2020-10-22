create or replace procedure SP_WB_REF_WS_AIR_CABIN_ADV_UPD
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_LANG varchar2(50):='';

P_TABLE_NAME varchar2(1000):='';
P_CD_BALANCE_ARM int:=0;
P_CD_INDEX_UNIT int:=0;
P_FDL_BALANCE_ARM int:=0;
P_FDL_INDEX_UNIT int:=0;
P_CCL_BALANCE_ARM int:=0;
P_CCL_INDEX_UNIT int:=0;
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

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CD_BALANCE_ARM[1]')) into P_CD_BALANCE_ARM from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CD_INDEX_UNIT[1]')) into P_CD_INDEX_UNIT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_FDL_BALANCE_ARM[1]')) into P_FDL_BALANCE_ARM from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_FDL_INDEX_UNIT[1]')) into P_FDL_INDEX_UNIT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CCL_BALANCE_ARM[1]')) into P_CCL_BALANCE_ARM from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_CCL_INDEX_UNIT[1]')) into P_CCL_INDEX_UNIT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/adv_data[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';
    ----------------------------------------------------------------------------
    select count(id) into R_COUNT
    from WB_REF_WS_AIR_CABIN_ADV
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
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
        delete WB_TEMP_XML_ID_EX
        where ACTION_NAME=V_ACTION_NAME and
              ACTION_DATE=V_ACTION_DATE;

        insert into WB_TEMP_XML_ID_EX (ID,
                                         F_STR_1,
                                           F_FLT_1,
                                             F_FLT_2,
                                               ACTION_NAME,
                                                 ACTION_DATE)
        select distinct f.p_deck_id,
                          f.p_section,
                            to_number(f.P_ROWS_FROM_INT_PART||'.'||P_ROWS_FROM_DEC_PART),
                              to_number(f.P_ROWS_TO_INT_PART||'.'||P_ROWS_TO_DEC_PART),
                                V_ACTION_NAME,
                                  V_ACTION_DATE
        from (select extractValue(value(t),'cd_data/P_DECK_ID[1]') as P_DECK_ID,
                       extractValue(value(t),'cd_data/P_SECTION[1]') as P_SECTION,

                         extractValue(value(t),'cd_data/P_ROWS_FROM_INT_PART[1]') as P_ROWS_FROM_INT_PART,
                           extractValue(value(t),'cd_data/P_ROWS_FROM_DEC_PART[1]') as P_ROWS_FROM_DEC_PART,

                             extractValue(value(t),'cd_data/P_ROWS_TO_INT_PART[1]') as P_ROWS_TO_INT_PART,
                               extractValue(value(t),'cd_data/P_ROWS_TO_DEC_PART[1]') as P_ROWS_TO_DEC_PART
              from table(xmlsequence(xmltype(cXML_in).extract('//cd_data'))) t) f;

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
                 str_msg:='Block "Cabin Definitions" - some of the names entered "Deck" removed from the directory!';
              end;
            else
              begin
                str_msg:='Блок "Cabin Definitions" - некоторые из введенных названий "Deck" удалены из справочника!';
              end;
            end if;
          end;
        end if;

        if str_msg is null then
          begin
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
                    str_msg:='Block "Cabin Definitions" - some of the imposed "Section" names is reserved phrase!';
                  end;
                else
                  begin
                    str_msg:='Блок "Cabin Definitions" - некоторые из введенных названий "Section" является зарезервированной фразой!';
                  end;
                end if;
              end;
            end if;
          end;
        end if;

        if str_msg is null then
          begin
            select count(slp.id) into R_COUNT
            from WB_REF_WS_AIR_SEAT_LAY_ADV sla join WB_REF_WS_AIR_S_L_PLAN slp
            on sla.cabin_id=P_ID and
               sla.id=slp.adv_id and
               not exists(select t.F_STR_1
                          from WB_TEMP_XML_ID_EX t
                          where t.ACTION_NAME=V_ACTION_NAME and
                                t.ACTION_DATE=V_ACTION_DATE and
                                t.F_STR_1=slp.CABIN_SECTION);

            if R_COUNT>0 then
               begin
                 if P_LANG='ENG' then STR_MSG:='Referential integrity violations in the field "Section" with the block "Seating Layout"!'; end if;
                 if P_LANG='RUS' then STR_MSG:='Нарушение ссылочной целостности по полю "Section" с блоком "Seating Layout"!'; end if;
               end;
            end if;
          end;
        end if;

        if str_msg is null then
          begin
            select count(slp.id) into R_COUNT
            from WB_REF_WS_AIR_SEAT_LAY_ADV sla join WB_REF_WS_AIR_S_L_PLAN slp
            on sla.cabin_id=P_ID and
               sla.id=slp.adv_id
            where not exists(select t.F_STR_1
                             from WB_TEMP_XML_ID_EX t
                             where t.ACTION_NAME=V_ACTION_NAME and
                                   t.ACTION_DATE=V_ACTION_DATE and
                                   t.F_STR_1=slp.CABIN_SECTION and
                                   slp.ROW_NUMBER between t.F_FLT_1 and t.F_FLT_2);

            if R_COUNT>0 then
               begin
                 if P_LANG='ENG' then STR_MSG:='Violation of the range of values in the fields "Section"-"Rows From"-"Rows To" with the block "Seating Layout"!'; end if;
                 if P_LANG='RUS' then STR_MSG:='Нарушение диапазона значений по полям "Section"-"Rows From"-"Rows To" с блоком "Seating Layout"!'; end if;
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
        select count(p.id) into R_COUNT
        from WB_REF_WS_AIR_S_L_PLAN p join WB_REF_WS_AIR_SEAT_LAY_ADV a
        on p.adv_id=a.id and
           a.CABIN_ID=P_ID and
           not exists(select f.P_SECTION
                      from (select extractValue(value(t),'cd_data/P_SECTION[1]') as P_SECTION
                            from table(xmlsequence(xmltype(cXML_in).extract('//cd_data'))) t) f
                      where f.p_section=p.CABIN_SECTION);

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then STR_MSG:='On the value of the field "Section" references are available in the block "Seating Layout"! The operation is not performed.'; end if;
            if P_LANG='RUS' then STR_MSG:='На значение поля "Section" имеются ссылки в блоке "Seating Layout"! Операция НЕ проведена.'; end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
        select count(t.id) into R_COUNT
        from WB_REF_WS_AIR_SEAT_LAY_ADV a join  WB_REF_WS_AIR_S_L_C_ADV aa
        on a.CABIN_ID=P_ID and
           a.id=aa.adv_id join WB_REF_WS_AIR_SL_CAI_T t
           on t.adv_id=aa.id and
              not exists(select f.P_SECTION
                         from (select extractValue(value(t),'cd_data/P_SECTION[1]') as P_SECTION
                               from table(xmlsequence(xmltype(cXML_in).extract('//cd_data'))) t) f
                         where f.p_section=t.CABIN_SECTION);
        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then STR_MSG:='On the value of the field "Section" references are available in the blocks "Seating Layout"->"Configuration"->"Cabin Area Information"! The operation is not performed.'; end if;
            if P_LANG='RUS' then STR_MSG:='На значение поля "Section" имеются ссылки в блоках "Seating Layout"->"Configuration"->"Cabin Area Information"! Операция НЕ проведена.'; end if;
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
                                         ACTION_NAME,
                                           ACTION_DATE)
        select distinct f.p_fcl_id,
                         V_ACTION_NAME,
                           V_ACTION_DATE
        from (select extractValue(value(t),'fd_data/P_FCL_ID[1]') as P_FCL_ID
              from table(xmlsequence(xmltype(cXML_in).extract('//fd_data'))) t) f;

        select count(t.id) into R_COUNT
        from WB_TEMP_XML_ID_EX t
        where t.ACTION_NAME=V_ACTION_NAME and
              t.ACTION_DATE=V_ACTION_DATE and
               not exists(select d.id
                         from WB_REF_WS_FL_CREW_LOCATION d
                         where d.id=t.id);

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                 str_msg:='Block "Flight Deck Locations" - some of the names entered "Location" removed from the directory!';
              end;
            else
              begin
                str_msg:='Блок "Flight Deck Locations" - некоторые из введенных названий "Location" удалены из справочника!';
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
        select distinct f.p_deck_id,
                          f.p_section,
                            V_ACTION_NAME,
                              V_ACTION_DATE
        from (select extractValue(value(t),'ccl_data/P_DECK_ID[1]') as P_DECK_ID,
                       extractValue(value(t),'ccl_data/P_LOCATION[1]') as P_SECTION
              from table(xmlsequence(xmltype(cXML_in).extract('//ccl_data'))) t) f;

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
                 str_msg:='Block "Cabin Crew Locations" - some of the names entered "Deck" removed from the directory!';
              end;
            else
              begin
                str_msg:='Блок "Cabin Crew Locations" - некоторые из введенных названий "Deck" удалены из справочника!';
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
                 str_msg:='Block "Cabin Crew Locations" - some of the imposed "Location" names is reserved phrase!';
              end;
            else
              begin
                str_msg:='Блок "Cabin Crew Locations" - некоторые из введенных названий "Location" является зарезервированной фразой!';
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
        update WB_REF_WS_AIR_CABIN_IDN
        set TABLE_NAME=P_TABLE_NAME,
            U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where id=P_ID;

        UPDATE WB_REF_WS_AIR_CABIN_ADV
        set CD_BALANCE_ARM=P_CD_BALANCE_ARM,
	          CD_INDEX_UNIT=P_CD_INDEX_UNIT,
	          FDL_BALANCE_ARM=P_FDL_BALANCE_ARM,
	          FDL_INDEX_UNIT=P_FDL_INDEX_UNIT,
	          CCL_BALANCE_ARM=P_CCL_BALANCE_ARM,
	          CCL_INDEX_UNIT=P_CCL_INDEX_UNIT,
            U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where idn=P_ID;

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_CABIN_CD_HST(ID_,
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
                                                                     DECK_ID,
	                                                                     SECTION,
	                                                                       ROWS_FROM,
	                                                                         ROWS_TO,
	                                                                           LA_FROM,
	                                                                             LA_TO,
	                                                                               BA_CENTROID,
	                                                                                 BA_FWD,
	                                                                                   BA_AFT,
	                                                                                     INDEX_PER_WT_UNIT,
		                                                                                     U_NAME,
		                                                                                       U_IP,
		                                                                                         U_HOST_NAME,
		                                                                                           DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CABIN_CD_HST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       'delete',
                         id,
                           ID_AC,
	                           ID_WS,
		                           ID_BORT,
		                             IDN,
	                                 ADV_ID,
                                     DECK_ID,
	                                     SECTION,
	                                       ROWS_FROM,
	                                         ROWS_TO,
	                                           LA_FROM,
	                                             LA_TO,
	                                               BA_CENTROID,
	                                                 BA_FWD,
	                                                   BA_AFT,
	                                                     INDEX_PER_WT_UNIT,
		                         	                           U_NAME,
		                               	                       U_IP,
		                                	                       U_HOST_NAME,
		                                 	                         DATE_WRITE
     from WB_REF_WS_AIR_CABIN_CD
     where IDN=P_ID;

     delete from WB_REF_WS_AIR_CABIN_CD
     where IDN=P_ID;

     insert into WB_REF_WS_AIR_CABIN_CD (ID,
	                                         ID_AC,
	                                           ID_WS,
	                                             ID_BORT,
                                         	       IDN,
                                         	         ADV_ID,
	                                                   DECK_ID,
	                                                     SECTION,
	                                                       ROWS_FROM,
	                                                         ROWS_TO,
	                                                           LA_FROM,
	                                                             LA_TO,
	                                                               BA_CENTROID,
	                                                                 BA_FWD,
                                         	                           BA_AFT,
                                         	                             INDEX_PER_WT_UNIT,
                                                                      	 U_NAME,
	                                                                         U_IP,
	                                                                           U_HOST_NAME,
	                                                                             DATE_WRITE)
     select SEC_WB_REF_WS_AIR_CABIN_CD.nextval,
              a.ID_AC,
	              a.ID_WS,
	                a.ID_BORT,
                    a.IDN,
                      a.id,
                        f.P_DECK_ID,
                          f.P_SECTION,

                            case when f.P_ROWS_FROM_INT_PART='NULL'
                                 then NULL
                                 else to_number(f.P_ROWS_FROM_INT_PART||'.'||f.P_ROWS_FROM_DEC_PART)
                            end,

                              case when f.P_ROWS_TO_INT_PART='NULL'
                                   then NULL
                                   else to_number(f.P_ROWS_TO_INT_PART||'.'||f.P_ROWS_TO_DEC_PART)
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

                                           P_U_NAME,
	                                           P_U_IP,
	                                             P_U_HOST_NAME,
                                                 SYSDATE()
     from WB_REF_WS_AIR_CABIN_ADV a join (select extractValue(value(t),'cd_data/P_DECK_ID[1]') as P_DECK_ID,
                                                   extractValue(value(t),'cd_data/P_SECTION[1]') as P_SECTION,

                                                     extractValue(value(t),'cd_data/P_ROWS_FROM_INT_PART[1]') as P_ROWS_FROM_INT_PART,
                                                       extractValue(value(t),'cd_data/P_ROWS_FROM_DEC_PART[1]') as P_ROWS_FROM_DEC_PART,

                                                         extractValue(value(t),'cd_data/P_ROWS_TO_INT_PART[1]') as P_ROWS_TO_INT_PART,
                                                           extractValue(value(t),'cd_data/P_ROWS_TO_DEC_PART[1]') as P_ROWS_TO_DEC_PART,

                                                             extractValue(value(t),'cd_data/P_LA_FROM_INT_PART[1]') as P_LA_FROM_INT_PART,
                                                               extractValue(value(t),'cd_data/P_LA_FROM_DEC_PART[1]') as P_LA_FROM_DEC_PART,

                                                                 extractValue(value(t),'cd_data/P_LA_TO_INT_PART[1]') as P_LA_TO_INT_PART,
                                                                   extractValue(value(t),'cd_data/P_LA_TO_DEC_PART[1]') as P_LA_TO_DEC_PART,

                                                                     extractValue(value(t),'cd_data/P_BA_CENTROID_INT_PART[1]') as P_BA_CENTROID_INT_PART,
                                                                       extractValue(value(t),'cd_data/P_BA_CENTROID_DEC_PART[1]') as P_BA_CENTROID_DEC_PART,

                                                                         extractValue(value(t),'cd_data/P_BA_FWD_INT_PART[1]') as P_BA_FWD_INT_PART,
                                                                           extractValue(value(t),'cd_data/P_BA_FWD_DEC_PART[1]') as P_BA_FWD_DEC_PART,

                                                                             extractValue(value(t),'cd_data/P_BA_AFT_INT_PART[1]') as P_BA_AFT_INT_PART,
                                                                               extractValue(value(t),'cd_data/P_BA_AFT_DEC_PART[1]') as P_BA_AFT_DEC_PART,

                                                                                 extractValue(value(t),'cd_data/P_INDEX_PER_WT_UNIT_INT_PART[1]') as P_INDEX_PER_WT_UNIT_INT_PART,
                                                                                   extractValue(value(t),'cd_data/P_INDEX_PER_WT_UNIT_DEC_PART[1]') as P_INDEX_PER_WT_UNIT_DEC_PART
                                          from table(xmlsequence(xmltype(cXML_in).extract('//cd_data'))) t) f
     on a.idn=P_ID;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_CABIN_FD_HST(ID_,
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
                                                                     FCL_ID,
	                                                                     MAX_NUM_SEATS,
	                                                                       LA_CENTROID,
	                                                                         BA_CENTROID,
	                                                                           INDEX_PER_WT_UNIT,
		                                                                           U_NAME,
		                                                                             U_IP,
		                                                                               U_HOST_NAME,
		                                                                                 DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CABIN_FD_HST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       'delete',
                         id,
                           ID_AC,
	                           ID_WS,
		                           ID_BORT,
		                             IDN,
	                                 ADV_ID,
                                     FCL_ID,
	                                     MAX_NUM_SEATS,
	                                       LA_CENTROID,
	                                         BA_CENTROID,
	                                           INDEX_PER_WT_UNIT,
		                         	                 U_NAME,
		                               	             U_IP,
		                                	             U_HOST_NAME,
		                                 	               DATE_WRITE
     from WB_REF_WS_AIR_CABIN_FD
     where IDN=P_ID;

     delete from WB_REF_WS_AIR_CABIN_FD
     where IDN=P_ID;

     insert into WB_REF_WS_AIR_CABIN_FD (ID,
	                                         ID_AC,
	                                           ID_WS,
	                                             ID_BORT,
                                         	       IDN,
                                         	         ADV_ID,
	                                                   FCL_ID,
	                                                     MAX_NUM_SEATS,
	                                                       LA_CENTROID,
	                                                         BA_CENTROID,
	                                                           INDEX_PER_WT_UNIT,
                                                               U_NAME,
	                                                               U_IP,
	                                                                 U_HOST_NAME,
	                                                                   DATE_WRITE)
     select SEC_WB_REF_WS_AIR_CABIN_FD.nextval,
              a.ID_AC,
	              a.ID_WS,
	                a.ID_BORT,
                    a.IDN,
                      a.id,
                        f.P_FCL_ID,

                          case when f.P_MAX_NUM_SEATS_INT_PART='NULL'
                               then NULL
                               else to_number(f.P_MAX_NUM_SEATS_INT_PART||'.'||f.P_MAX_NUM_SEATS_DEC_PART)
                          end,

                            case when f.P_LA_CENTROID_INT_PART='NULL'
                                 then NULL
                                 else to_number(f.P_LA_CENTROID_INT_PART||'.'||f.P_LA_CENTROID_DEC_PART)
                            end,

                             case when f.P_BA_CENTROID_INT_PART='NULL'
                                  then NULL
                                  else to_number(f.P_BA_CENTROID_INT_PART||'.'||f.P_BA_CENTROID_DEC_PART)
                             end,

                              case when f.P_INDEX_PER_WT_UNIT_INT_PART='NULL'
                                   then NULL
                                   else to_number(f.P_INDEX_PER_WT_UNIT_INT_PART||'.'||f.P_INDEX_PER_WT_UNIT_DEC_PART)
                              end,

                                P_U_NAME,
	                                P_U_IP,
	                                  P_U_HOST_NAME,
                                      SYSDATE()
     from WB_REF_WS_AIR_CABIN_ADV a join (select extractValue(value(t),'fd_data/P_FCL_ID[1]') as P_FCL_ID,

                                                   extractValue(value(t),'fd_data/P_MAX_NUM_SEATS_INT_PART[1]') as P_MAX_NUM_SEATS_INT_PART,
                                                     extractValue(value(t),'fd_data/P_MAX_NUM_SEATS_DEC_PART[1]') as P_MAX_NUM_SEATS_DEC_PART,

                                                       extractValue(value(t),'fd_data/P_LA_CENTROID_INT_PART[1]') as P_LA_CENTROID_INT_PART,
                                                         extractValue(value(t),'fd_data/P_LA_CENTROID_DEC_PART[1]') as P_LA_CENTROID_DEC_PART,

                                                           extractValue(value(t),'fd_data/P_BA_CENTROID_INT_PART[1]') as P_BA_CENTROID_INT_PART,
                                                             extractValue(value(t),'fd_data/P_BA_CENTROID_DEC_PART[1]') as P_BA_CENTROID_DEC_PART,

                                                               extractValue(value(t),'fd_data/P_INDEX_PER_WT_UNIT_INT_PART[1]') as P_INDEX_PER_WT_UNIT_INT_PART,
                                                                 extractValue(value(t),'fd_data/P_INDEX_PER_WT_UNIT_DEC_PART[1]') as P_INDEX_PER_WT_UNIT_DEC_PART
                                          from table(xmlsequence(xmltype(cXML_in).extract('//fd_data'))) t) f
     on a.idn=P_ID;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    insert into WB_REF_WS_AIR_CABIN_CCL_HST(ID_,
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
                                                                      DECK_ID,
                                                                        LOCATION,
	                                                                        MAX_NUM_SEATS,
	                                                                          LA_CENTROID,
	                                                                            BA_CENTROID,
	                                                                              INDEX_PER_WT_UNIT,
		                                                                              U_NAME,
		                                                                                U_IP,
		                                                                                  U_HOST_NAME,
		                                                                                    DATE_WRITE)
    select SEC_WB_REF_WS_AIR_CBN_CCL_HST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       'delete',
                         id,
                           ID_AC,
	                           ID_WS,
		                           ID_BORT,
		                             IDN,
	                                 ADV_ID,
                                     DECK_ID,
                                       LOCATION,
	                                       MAX_NUM_SEATS,
	                                         LA_CENTROID,
	                                           BA_CENTROID,
	                                             INDEX_PER_WT_UNIT,
		                         	                   U_NAME,
		                               	               U_IP,
		                                	               U_HOST_NAME,
		                                 	                 DATE_WRITE
     from WB_REF_WS_AIR_CABIN_CCL
     where IDN=P_ID;

     delete from WB_REF_WS_AIR_CABIN_CCL
     where IDN=P_ID;

     insert into WB_REF_WS_AIR_CABIN_CCL (ID,
	                                         ID_AC,
	                                           ID_WS,
	                                             ID_BORT,
                                         	       IDN,
                                         	         ADV_ID,
	                                                   DECK_ID,
                                                       LOCATION,
	                                                       MAX_NUM_SEATS,
	                                                         LA_CENTROID,
	                                                           BA_CENTROID,
	                                                             INDEX_PER_WT_UNIT,
                                                                 U_NAME,
	                                                                 U_IP,
	                                                                   U_HOST_NAME,
	                                                                     DATE_WRITE)
     select SEC_WB_REF_WS_AIR_CABIN_CCL.nextval,
              a.ID_AC,
	              a.ID_WS,
	                a.ID_BORT,
                    a.IDN,
                      a.id,
                        f.P_DECK_ID,
                          f.P_LOCATION,

                            case when f.P_MAX_NUM_SEATS_INT_PART='NULL'
                                 then NULL
                                 else to_number(f.P_MAX_NUM_SEATS_INT_PART||'.'||f.P_MAX_NUM_SEATS_DEC_PART)
                            end,

                              case when f.P_LA_CENTROID_INT_PART='NULL'
                                   then NULL
                                   else to_number(f.P_LA_CENTROID_INT_PART||'.'||f.P_LA_CENTROID_DEC_PART)
                              end,

                               case when f.P_BA_CENTROID_INT_PART='NULL'
                                    then NULL
                                    else to_number(f.P_BA_CENTROID_INT_PART||'.'||f.P_BA_CENTROID_DEC_PART)
                               end,

                                case when f.P_INDEX_PER_WT_UNIT_INT_PART='NULL'
                                     then NULL
                                     else to_number(f.P_INDEX_PER_WT_UNIT_INT_PART||'.'||f.P_INDEX_PER_WT_UNIT_DEC_PART)
                                end,

                                  P_U_NAME,
	                                  P_U_IP,
	                                    P_U_HOST_NAME,
                                        SYSDATE()
     from WB_REF_WS_AIR_CABIN_ADV a join (select extractValue(value(t),'ccl_data/P_DECK_ID[1]') as P_DECK_ID,
                                                   extractValue(value(t),'ccl_data/P_LOCATION[1]') as P_LOCATION,

                                                     extractValue(value(t),'ccl_data/P_MAX_NUM_SEATS_INT_PART[1]') as P_MAX_NUM_SEATS_INT_PART,
                                                       extractValue(value(t),'ccl_data/P_MAX_NUM_SEATS_DEC_PART[1]') as P_MAX_NUM_SEATS_DEC_PART,

                                                         extractValue(value(t),'ccl_data/P_LA_CENTROID_INT_PART[1]') as P_LA_CENTROID_INT_PART,
                                                           extractValue(value(t),'ccl_data/P_LA_CENTROID_DEC_PART[1]') as P_LA_CENTROID_DEC_PART,

                                                             extractValue(value(t),'ccl_data/P_BA_CENTROID_INT_PART[1]') as P_BA_CENTROID_INT_PART,
                                                               extractValue(value(t),'ccl_data/P_BA_CENTROID_DEC_PART[1]') as P_BA_CENTROID_DEC_PART,

                                                                 extractValue(value(t),'ccl_data/P_INDEX_PER_WT_UNIT_INT_PART[1]') as P_INDEX_PER_WT_UNIT_INT_PART,
                                                                   extractValue(value(t),'ccl_data/P_INDEX_PER_WT_UNIT_DEC_PART[1]') as P_INDEX_PER_WT_UNIT_DEC_PART
                                          from table(xmlsequence(xmltype(cXML_in).extract('//ccl_data'))) t) f
     on a.idn=P_ID;
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------

        str_msg:='EMPTY_STRING';
      end;
      end if;

    commit;

    cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(str_msg)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_CABIN_ADV_UPD;
/
