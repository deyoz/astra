create or replace procedure SP_WB_REF_AIRCO_ADV_DELETE
(cXML_in in clob,
   cXML_out out clob)
as
P_AIRCO_ID number:=-1;
P_LANG varchar2(50):='';
r_count_1 int:=0;
r_count_2 int:=0;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_LANG[1]') into P_LANG from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_AIRCO_ID[1]') into P_AIRCO_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID (ID, num)
    select f.id,
             f.id
    from (select to_number(extractValue(value(t),'list/id[1]')) as id
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    delete from WB_TEMP_XML_ID
    where not exists(select 1
                     from WB_REF_AIRCOMPANY_ADV_INFO t
                     where t.id=WB_TEMP_XML_ID.id);

    select count(id) into r_count_1
    from WB_TEMP_XML_ID;

    select count(id) into r_count_2
    from WB_REF_AIRCOMPANY_ADV_INFO
    where ID_AC=P_AIRCO_ID;

    if r_count_1>=r_count_2 then
      begin

        if P_LANG='ENG' then
          begin
            str_msg:='This operation will erase all data on airlines!';
          end;
        else
          begin
            str_msg:='Эта операция приведет к удалению ВСЕХ данных об авиакомпании!';
          end;
        end if;

      end;
    end if;

    if str_msg is null then
      begin
        select count(q.id_ac) into r_count_1
        from (select i.id_ac,
                     i.iata_code,
                     i.date_from,
                     nvl((select min(ii.date_from)-1
                          from WB_REF_AIRCOMPANY_ADV_INFO ii
                          where ii.id_ac=i.id_ac and
                                ii.date_from>i.date_from and
                                not exists(select 1
                                           from WB_TEMP_XML_ID t
                                           where t.id=ii.id)), to_date('01.01.9999', 'dd.mm.yyyy')) date_to
              from WB_REF_AIRCOMPANY_ADV_INFO i
              where i.id_ac=P_AIRCO_ID and
                    not exists(select 1
                               from WB_TEMP_XML_ID t
                               where t.id=i.id)) q join (select i.id_ac,
                                                                i.iata_code,
                                                                i.date_from,
                                                                nvl((select min(ii.date_from)-1
                                                                     from WB_REF_AIRCOMPANY_ADV_INFO ii
                                                                    where ii.id_ac=i.id_ac and
                                                                           ii.date_from>i.date_from), to_date('01.01.9999', 'dd.mm.yyyy')) date_to
                                                         from WB_REF_AIRCOMPANY_ADV_INFO i
                                                         where i.id_ac<>P_AIRCO_ID) qq
        on q.iata_code<>'EMPTY_STRING' and
           qq.iata_code<>'EMPTY_STRING' and
           q.iata_code=qq.iata_code and
           (q.date_from between qq.date_from and qq.date_to or
            q.date_to between qq.date_from and qq.date_to or
            qq.date_from between q.date_from and q.date_to or
            qq.date_to between q.date_from and q.date_to);

        if r_count_1>0 then
          begin
            if P_LANG='ENG' then
               begin
                 str_msg:='This operation will lead to the availability of the same in different codes IATA airlines in the same period of time!';
               end;
             else
               begin
                 str_msg:='Выполнение этой операции приведет к наличию одинаковых IATA кодов у разных авиакомпаний в один период времени!';
               end;
            end if;
          end;
        end if;


      end;
    end if;

    if str_msg is null then
      begin
        select count(q.id_ac) into r_count_1
        from (select i.id_ac,
                     i.icao_code,
                     i.date_from,
                     nvl((select min(ii.date_from)-1
                          from WB_REF_AIRCOMPANY_ADV_INFO ii
                          where ii.id_ac=i.id_ac and
                                ii.date_from>i.date_from and
                                not exists(select 1
                                           from WB_TEMP_XML_ID t
                                           where t.id=ii.id)), to_date('01.01.9999', 'dd.mm.yyyy')) date_to
              from WB_REF_AIRCOMPANY_ADV_INFO i
              where i.id_ac=P_AIRCO_ID and
                    not exists(select 1
                               from WB_TEMP_XML_ID t
                               where t.id=i.id)) q join (select i.id_ac,
                                                                i.icao_code,
                                                                i.date_from,
                                                                nvl((select min(ii.date_from)-1
                                                                     from WB_REF_AIRCOMPANY_ADV_INFO ii
                                                                    where ii.id_ac=i.id_ac and
                                                                           ii.date_from>i.date_from), to_date('01.01.9999', 'dd.mm.yyyy')) date_to
                                                         from WB_REF_AIRCOMPANY_ADV_INFO i
                                                         where i.id_ac<>P_AIRCO_ID) qq
        on q.icao_code<>'EMPTY_STRING' and
           qq.icao_code<>'EMPTY_STRING' and
           q.icao_code=qq.icao_code and
           (q.date_from between qq.date_from and qq.date_to or
            q.date_to between qq.date_from and qq.date_to or
            qq.date_from between q.date_from and q.date_to or
            qq.date_to between q.date_from and q.date_to);

        if r_count_1>0 then
          begin
            if P_LANG='ENG' then
               begin
                 str_msg:='This operation will lead to the availability of the same in different codes ICAO airlines in the same period of time!';
               end;
             else
               begin
                 str_msg:='Выполнение этой операции приведет к наличию одинаковых ICAO кодов у разных авиакомпаний в один период времени!';
               end;
            end if;
          end;
        end if;


      end;
    end if;
    -------------------------------------------------------------------------------------------------------------
    -------------------------------------------------------------------------------------------------------------

    if (str_msg is null) then
      begin
        insert into WB_REF_AIRCO_ADV_INFO_HISTORY (ID_,
	                                                   U_NAME_,
	                                                     U_IP_,
	                                                       U_HOST_NAME_,
	                                                         DATE_WRITE_,
	                                                           ACTION,
	                                                             ID,
	                                                               ID_AC_OLD,
	                                                                 DATE_FROM_OLD,
	                                                                   ID_CITY_OLD,
	                                                                     IATA_CODE_OLD,
	                                                                       ICAO_CODE_OLD,
                                                                           OTHER_CODE_OLD,
                                                               	             NAME_RUS_SMALL_OLD,
	                                                                             NAME_RUS_FULL_OLD,
	                                                                               NAME_ENG_SMALL_OLD,
	                                                                                 NAME_ENG_FULL_OLD,
	                                                                                   U_NAME_OLD,
	                                                                                     U_IP_OLD,
	                                                                                       U_HOST_NAME_OLD,
	                                                                                         DATE_WRITE_OLD,
	                                                                                           ID_AC_NEW,
	                                                                                             DATE_FOM_NEW,
	                                                                                               ID_CITY_NEW,
	                                                                                                 IATA_CODE_NEW,
	                                                                                                   ICAO_CODE_NEW,
	                                                                                                     OTHER_CODE_NEW,
	                                                                                                       NAME_RUS_SMALL_NEW,
	                                                                                                         NAME_RUS_FULL_NEW,
	                                                                                                           NAME_ENG_SMALL_NEW,
	                                                                                                             NAME_ENG_FULL_NEW,
	                                                                                                               U_NAME_NEW,
	                                                                                                                 U_IP_NEW,
	                                                                                                                   U_HOST_NAME_NEW,
	                                                                                                                     DATE_WRITE_NEW,
                                                                                                                         remark_old,
                                                                                                                           remark_new)
        select SEC_WB_REF_AIRCO_ADV_INFO_HIST.nextval,
                 P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE,
                         'delete',
                           d.id,
                             d.ID_AC,
	                             d.DATE_FROM,
	                               d.ID_CITY,
	                                 d.IATA_CODE,
	                                   d.ICAO_CODE,
	                                     d.OTHER_CODE,
	                                       d.NAME_RUS_SMALL,
	                                         d.NAME_RUS_FULL,
	                                           d.NAME_ENG_SMALL,
	                                             d.NAME_ENG_FULL,
	                                               d.U_NAME,
	                                                 d.U_IP,
	                                                   d.U_HOST_NAME,
	                                                     d.DATE_WRITE,
                                                         d.ID_AC,
	                                                         d.DATE_FROM,
	                                                           d.ID_CITY,
	                                                             d.IATA_CODE,
	                                                               d.ICAO_CODE,
	                                                                 d.OTHER_CODE,
	                                                                   d.NAME_RUS_SMALL,
	                                                                     d.NAME_RUS_FULL,
	                                                                       d.NAME_ENG_SMALL,
	                                                                         d.NAME_ENG_FULL,
	                                                                           d.U_NAME,
	                                                                             d.U_IP,
	                                                                               d.U_HOST_NAME,
	                                                                                 d.DATE_WRITE,
                                                                                     d.remark,
                                                                                       d.remark

        from WB_TEMP_XML_ID t join WB_REF_AIRCOMPANY_ADV_INFO d
        on d.id=t.id;

        delete from WB_REF_AIRCOMPANY_ADV_INFO
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_AIRCOMPANY_ADV_INFO.id);

        str_msg:='EMPTY_STRING';
      end;
      end if;

    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_AIRCO_ADV_DELETE;
/
