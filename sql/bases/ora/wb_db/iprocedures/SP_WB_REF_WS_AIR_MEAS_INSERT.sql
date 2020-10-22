create or replace procedure SP_WB_REF_WS_AIR_MEAS_INSERT
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

P_MEAS_WEIGHT number:=0;
P_MEAS_LENGTH number:=0;
P_MEAS_LIQUID_VOLUME number:=0;
P_MEAS_VOLUME number:=-1;
P_MEAS_FUEL_DENSITY number:=-1;
P_MEAS_MOMENTS number:=0;

P_REMARK clob:='';
P_IS_REMARK_EMPTY number:=0;

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

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MEAS_WEIGHT[1]')) into P_MEAS_WEIGHT from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MEAS_LENGTH[1]')) into P_MEAS_LENGTH from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MEAS_LIQUID_VOLUME[1]')) into P_MEAS_LIQUID_VOLUME from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MEAS_VOLUME[1]')) into P_MEAS_VOLUME from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MEAS_FUEL_DENSITY[1]')) into P_MEAS_FUEL_DENSITY from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_MEAS_MOMENTS[1]')) into P_MEAS_MOMENTS from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_IS_REMARK_EMPTY[1]')) into P_IS_REMARK_EMPTY from dual;

    select value(t).extract('/P_REMARK/text()').getclobval() into P_REMARK
    from table(xmlsequence(xmltype(cXML_in).extract('//list/*'))) t
    where value(t).extract('/P_REMARK/text()').getclobval() is not null;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');
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
         from WB_REF_MEASUREMENT_WEIGHT
         where ID=P_MEAS_WEIGHT;

         if r_count=0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='The selected unit weight is removed!';
               end;
             else
               begin
                 str_msg:='Выбранная еденица веса удалена!';
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
         from WB_REF_MEASUREMENT_LENGTH
         where ID=P_MEAS_LENGTH;

         if r_count=0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='The selected unit length is removed!';
               end;
             else
               begin
                 str_msg:='Выбранная еденица длины удалена!';
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
         from WB_REF_MEASUREMENT_VOLUME
         where ID=P_MEAS_LIQUID_VOLUME;

         if r_count=0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='The selected unit liquid volume is removed!';
               end;
             else
               begin
                 str_msg:='Выбранная еденица объема жидкости удалена!';
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
         from WB_REF_MEASUREMENT_VOLUME
         where ID=P_MEAS_VOLUME;

         if r_count=0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='The selected unit volume is removed!';
               end;
             else
               begin
                 str_msg:='Выбранная еденица объема удалена!';
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
         from WB_REF_MEASUREMENT_DENSITY
         where ID=P_MEAS_FUEL_DENSITY;

         if r_count=0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='The selected unit fuel density is removed!';
               end;
             else
               begin
                 str_msg:='Выбранная еденица полтности топлива удалена!';
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
         from WB_REF_MEASUREMENT_MOMENTS
         where ID=P_MEAS_MOMENTS;

         if r_count=0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='The selected unit moments is removed!';
               end;
             else
               begin
                 str_msg:='Выбранная еденица измерения момента удалена!';
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
         from WB_REF_WS_AIR_MEASUREMENT
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
                    str_msg:='Value field &quot;Remarks&quot; is a phrase reserved!';
                  end;
                else
                  begin
                    str_msg:='Знаачение поля &quot;Примечание&quot; является зарезервированной фразой!';
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
    if (str_msg is null) then
      begin
        P_ID:=SEC_WB_REF_WS_AIR_MEAS.nextval();

        insert into WB_REF_WS_AIR_MEASUREMENT (ID,
                                                 ID_AC,
	                                                 ID_WS,
	                                                   ID_BORT,
	                                                     DATE_FROM,
	                                                       WEIGHT_ID,
	                                                         VOLUME_ID_LIQUID,
	                                                           VOLUME_ID,
	                                                             LENGTH_ID,
	                                                               DENSITY_ID_FUEL,
	                                                                 MOMENTS_ID,
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
	                       P_MEAS_WEIGHT,
                           P_MEAS_LIQUID_VOLUME,
                             P_MEAS_VOLUME,
                               P_MEAS_LENGTH,
                                 P_MEAS_FUEL_DENSITY,
                                   P_MEAS_MOMENTS,
	                                   P_REMARK,
	                                     P_U_NAME,
	                                       P_U_IP,
	                                         P_U_HOST_NAME,
                                             sysdate()

        from dual;

        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list id="'||to_char(P_ID)||'" str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_WS_AIR_MEAS_INSERT;
/
