create or replace procedure SP_WB_REF_AC_MEAS_UPDATE
(cXML_in in clob,
   cXML_out out clob)
as
P_ID number:=-1;
P_AIRCO_ID number:=-1;
P_LANG varchar2(50):='';

P_DATE_FROM date;
P_DATE_FROM_D varchar2(50);
P_DATE_FROM_M varchar2(50);
P_DATE_FROM_Y varchar2(50);

P_ID_DENSITY number:=-1;
P_ID_VOLUME number:=-1;
P_ID_LENGTH number:=-1;
P_ID_WEIGHT number:=-1;

P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
STR_MSG clob:=null;
R_COUNT int:=0;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_LANG[1]') into P_LANG from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_AIRCO_ID[1]')) into P_AIRCO_ID from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID[1]')) into P_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_D[1]') into P_DATE_FROM_D from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_M[1]') into P_DATE_FROM_M from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_DATE_FROM_Y[1]') into P_DATE_FROM_Y from dual;

    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_DENSITY[1]')) into P_ID_DENSITY from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_VOLUME[1]')) into P_ID_VOLUME from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_LENGTH[1]')) into P_ID_LENGTH from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_ID_WEIGHT[1]')) into P_ID_WEIGHT from dual;

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    P_DATE_FROM:=to_date(P_DATE_FROM_D||'.'||P_DATE_FROM_M||'.'||P_DATE_FROM_Y, 'dd.mm.yyyy');
    cXML_out:='<?xml version="1.0" ?><root>';
    ----------------------------------------------------------------------------
    select count(id) into R_COUNT
    from WB_REF_AIRCOMPANY_KEY
    where id=P_AIRCO_ID;

    if r_count=0 then
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
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    if str_msg is null then
      begin
         select count(id) into R_COUNT
         from WB_REF_AIRCO_MEAS
         where ID=P_ID;

         if r_count=0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='The selected record is removed!';
               end;
             else
               begin
                 str_msg:='Выбранная запись удалена!';
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
         where ID=P_ID_DENSITY;

         if r_count=0 then
           begin
             if P_LANG='ENG' then
               begin
                 str_msg:='The selected unit density is removed!';
               end;
             else
               begin
                 str_msg:='Выбранная еденица полтности удалена!';
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
         where ID=P_ID_VOLUME;

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
         from WB_REF_MEASUREMENT_LENGTH
         where ID=P_ID_LENGTH;

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
         from WB_REF_MEASUREMENT_WEIGHT
         where ID=P_ID_WEIGHT;

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
         select count(id) into r_count
         from  WB_REF_AIRCO_MEAS
         where ID<>P_ID and
               ID_AC=P_AIRCO_ID and
               DATE_FROM=P_DATE_FROM;

         if r_count>0 then
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
        update WB_REF_AIRCO_MEAS
        set DATE_FROM=P_DATE_FROM,
	          WEIGHT_ID=P_ID_WEIGHT,
	          VOLUME_ID=P_ID_VOLUME,
	          LENGTH_ID=P_ID_LENGTH,
	          DENSITY_ID=P_ID_DENSITY,
	          U_NAME=P_U_NAME,
	          U_IP=P_U_IP,
	          U_HOST_NAME=P_U_HOST_NAME,
	          DATE_WRITE=sysdate()
        where id=P_ID;

        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_AC_MEAS_UPDATE;
/
