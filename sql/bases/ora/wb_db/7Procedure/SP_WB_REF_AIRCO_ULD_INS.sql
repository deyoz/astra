create or replace procedure SP_WB_REF_AIRCO_ULD_INS
(cXML_in in clob,
   cXML_out out clob)
as

P_LANG varchar2(50):='';
P_ID_AC number:=-1;
V_ID number:=-1;

P_ULD_TYPE_ID number:=-1;
P_ULD_ATA_ID number:=-1;
P_ULD_IATA_ID number:=-1;
P_BEG_SER_NUM varchar2(50):='';

P_END_SER_NUM varchar2(50):='';
P_IS_END_SER_NUM_EMPTY number:=1;
P_OWNER_CODE varchar2(50):='';
P_BY_DEFAULT number:=NULL;

V_BEG_SER_NUM number:=-1;
V_END_SER_NUM number:=-1;

P_TARE_WEIGHT number:=NULL;
P_TARE_WEIGHT_INT_PART varchar2(50):='NULL';
P_TARE_WEIGHT_DEC_PART varchar2(50):='NULL';

P_MAX_WEIGHT number:=NULL;
P_MAX_WEIGHT_INT_PART varchar2(50):='NULL';
P_MAX_WEIGHT_DEC_PART varchar2(50):='NULL';

P_MAX_VOLUME number:=NULL;
P_MAX_VOLUME_INT_PART varchar2(50):='NULL';
P_MAX_VOLUME_DEC_PART varchar2(50):='NULL';

P_WIDTH number:=NULL;
P_WIDTH_INT_PART varchar2(50):='NULL';
P_WIDTH_DEC_PART varchar2(50):='NULL';

P_LENGTH number:=NULL;
P_LENGTH_INT_PART varchar2(50):='NULL';
P_LENGTH_DEC_PART varchar2(50):='NULL';

P_HEIGHT number:=NULL;
P_HEIGHT_INT_PART varchar2(50):='NULL';
P_HEIGHT_DEC_PART varchar2(50):='NULL';

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;
V_R_COUNT number:=-1;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_AC[1]')) into P_ID_AC from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ULD_TYPE_ID[1]')) into P_ULD_TYPE_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ULD_ATA_ID[1]')) into P_ULD_ATA_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ULD_IATA_ID[1]')) into P_ULD_IATA_ID from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BEG_SER_NUM[1]') into P_BEG_SER_NUM from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_END_SER_NUM[1]') into P_END_SER_NUM from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_END_SER_NUM_EMPTY[1]')) into P_IS_END_SER_NUM_EMPTY from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_OWNER_CODE[1]') into P_OWNER_CODE from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_BY_DEFAULT[1]')) into P_BY_DEFAULT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_TARE_WEIGHT_INT_PART[1]') into P_TARE_WEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_TARE_WEIGHT_DEC_PART[1]') into P_TARE_WEIGHT_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MAX_WEIGHT_INT_PART[1]') into P_MAX_WEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MAX_WEIGHT_DEC_PART[1]') into P_MAX_WEIGHT_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MAX_VOLUME_INT_PART[1]') into P_MAX_VOLUME_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MAX_VOLUME_DEC_PART[1]') into P_MAX_VOLUME_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_WIDTH_INT_PART[1]') into P_WIDTH_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_WIDTH_DEC_PART[1]') into P_WIDTH_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LENGTH_INT_PART[1]') into P_LENGTH_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LENGTH_DEC_PART[1]') into P_LENGTH_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_HEIGHT_INT_PART[1]') into P_HEIGHT_INT_PART from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_HEIGHT_DEC_PART[1]') into P_HEIGHT_DEC_PART from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    if P_TARE_WEIGHT_INT_PART<>'NULL' then P_TARE_WEIGHT:=to_number(P_TARE_WEIGHT_INT_PART||'.'||P_TARE_WEIGHT_DEC_PART); end if;
    if P_MAX_WEIGHT_INT_PART<>'NULL' then P_MAX_WEIGHT:=to_number(P_MAX_WEIGHT_INT_PART||'.'||P_MAX_WEIGHT_DEC_PART); end if;
    if P_MAX_VOLUME_INT_PART<>'NULL' then P_MAX_VOLUME:=to_number(P_MAX_VOLUME_INT_PART||'.'||P_MAX_VOLUME_DEC_PART); end if;

    if P_WIDTH_INT_PART<>'NULL' then P_WIDTH:=to_number(P_WIDTH_INT_PART||'.'||P_WIDTH_DEC_PART); end if;
    if P_LENGTH_INT_PART<>'NULL' then P_LENGTH:=to_number(P_LENGTH_INT_PART||'.'||P_LENGTH_DEC_PART); end if;
    if P_HEIGHT_INT_PART<>'NULL' then P_HEIGHT:=to_number(P_HEIGHT_INT_PART||'.'||P_HEIGHT_DEC_PART); end if;

    ----------------------------------------------------------------------------
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
            V_STR_MSG:='Этa авиакомпания удалена!';
          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    if (V_STR_MSG is null) then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_ULD_TYPES
        where id=P_ULD_TYPE_ID;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='The selected name "Container/Pallet" is removed from the directory!';
              end;
            else
              begin
                V_STR_MSG:='Выбранное наименование "Container/Pallet" удалено из справочника!';
              end;
            end if;

          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if (V_STR_MSG is null) then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_ULD_ATA
        where id=P_ULD_ATA_ID;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='The selected name "ULD Type Cross Ref" is removed from the directory!';
              end;
            else
              begin
                V_STR_MSG:='Выбранное наименование "ULD Type Cross Ref" удалено из справочника!';
              end;
            end if;

          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if (V_STR_MSG is null) then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_ULD_IATA
        where id=P_ULD_IATA_ID;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='The selected name "IATA ULD Type" is removed from the directory!';
              end;
            else
              begin
                V_STR_MSG:='Выбранное наименование "IATA ULD Type" удалено из справочника!';
              end;
            end if;

          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if (V_STR_MSG is null) then
      begin
        select count(id) into V_R_COUNT
        from WB_REF_ULD_IATA_ATA
        where IATA_ID=P_ULD_IATA_ID and
              ATA_ID=P_ULD_ATA_ID;

        if V_R_COUNT=0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='Selected bond "ULD Type Cross Ref" - "IATA ULD Type" is not active!';
              end;
            else
              begin
                V_STR_MSG:='Выбранная связь "ULD Type Cross Ref" - "IATA ULD Type" более не существует!';
              end;
            end if;

          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        if P_IS_END_SER_NUM_EMPTY=1 then
          begin

            P_END_SER_NUM:='EMPTY_STRING';
          end;
        end if;

        if P_IS_END_SER_NUM_EMPTY=0 then
          begin
            if P_END_SER_NUM='EMPTY_STRING' then
              begin
                if P_LANG='ENG' then
                  begin
                    V_STR_MSG:='Value field "End Serial №" is a phrase reserved!';
                  end;
                else
                  begin
                    V_STR_MSG:='Значение поля "End Serial №" является зарезервированной фразой!';
                  end;
                end if;
              end;
            end if;

          end;
        end if;

      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        V_BEG_SER_NUM:=to_number(P_BEG_SER_NUM);

        if P_IS_END_SER_NUM_EMPTY=1
          then V_END_SER_NUM:=V_BEG_SER_NUM;
          else V_END_SER_NUM:=to_number(P_END_SER_NUM);
        end if;

        if V_END_SER_NUM<V_BEG_SER_NUM then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='The field "End Serial №" less than "Begin Serial №"!';
              end;
            else
              begin
                V_STR_MSG:='Значение поля "End Serial №" меньше "Begin Serial №"!';
              end;
            end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(q.id) into V_R_COUNT
        from (select id,
                     to_number(BEG_SER_NUM) BEG_SER_NUM,
                     case when END_SER_NUM='EMPTY_STRING'
                          then to_number(BEG_SER_NUM)
                          else to_number(END_SER_NUM)
                     end END_SER_NUM
             from WB_REF_AIRCO_ULD
             where ID_AC=P_ID_AC and
                   ULD_TYPE_ID=P_ULD_TYPE_ID and
                   ULD_ATA_ID=P_ULD_ATA_ID and
                   ULD_IATA_ID=P_ULD_IATA_ID) q
        where q.BEG_SER_NUM between V_BEG_SER_NUM and V_END_SER_NUM or
              q.END_SER_NUM between V_BEG_SER_NUM and V_END_SER_NUM or
              V_BEG_SER_NUM between q.BEG_SER_NUM and q.END_SER_NUM or
              V_END_SER_NUM between q.BEG_SER_NUM and q.END_SER_NUM;

        if V_R_COUNT>0 then
          begin
            if P_LANG='ENG' then
              begin
                V_STR_MSG:='Recording with such values ??of the fields "Container / Pallet", "ULD Type Cross Ref", "IATA ULD Type" '||
                           'and crossover band "Begin Serial №" and "End Serial №" already exists!';
              end;
            else
              begin
                V_STR_MSG:='Запись с такими значениями полей "Container/Pallet", "ULD Type Cross Ref", "IATA ULD Type" '||
                           'и пересекающимся диапазоном "Begin Serial №" и "End Serial №" уже существует!';
              end;
            end if;
          end;
        end if;
      end;
    end if;
    ----------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        V_ID:=SEC_WB_REF_AIRCO_ULD.nextval();

        insert into WB_REF_AIRCO_ULD(ID,
	                                     ID_AC,
                                         ULD_TYPE_ID,
	                                         ULD_ATA_ID,
	                                           ULD_IATA_ID,
	                                             BEG_SER_NUM,
	                                               END_SER_NUM,
	                                                 OWNER_CODE,
	                                                   TARE_WEIGHT,
	                                                     MAX_WEIGHT,
	                                                       MAX_VOLUME,
	                                                         WIDTH,
	                                                           LENGTH,
	                                                             HEIGHT,
	                                                               BY_DEFAULT,
	                                                                 U_NAME,
		                                                                 U_IP,
		                                                                   U_HOST_NAME,
		                                                                     DATE_WRITE)
        select V_ID,
                 P_ID_AC,
                   P_ULD_TYPE_ID,
	                   P_ULD_ATA_ID,
	                     P_ULD_IATA_ID,
	                       P_BEG_SER_NUM,
	                         P_END_SER_NUM,
	                           P_OWNER_CODE,
	                             P_TARE_WEIGHT,
	                               P_MAX_WEIGHT,
	                                 P_MAX_VOLUME,
	                                   P_WIDTH,
	                                     P_LENGTH,
	                                       P_HEIGHT,
	                                         P_BY_DEFAULT,
                                             P_U_NAME,
	                                             P_U_IP,
	                                               P_U_HOST_NAME,
                                                   sysdate()
        from dual;

        V_STR_MSG:='EMPTY_STRING';
      end;
    end if;

    cXML_out:=cXML_out||'<list id="'||to_char(V_ID)||'" str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';

    commit;
  end SP_WB_REF_AIRCO_ULD_INS;
/
