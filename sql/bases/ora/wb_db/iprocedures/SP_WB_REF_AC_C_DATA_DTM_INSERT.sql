create or replace procedure SP_WB_REF_AC_C_DATA_DTM_INSERT
(cXML_in in clob,
   cXML_out out clob)
as
P_AIRCO_ID number:=-1;
P_LANG varchar2(50):='';
P_REMARK clob:='';
P_IS_REMARK_EMPTY number:=0;
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
r_count number:=-1;

TYPE DEL_DTM_REC IS REF CURSOR;
cur_DEL_DTM_REC DEL_DTM_REC;
DEL_DTM_NAME varchar2(200);
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_LANG[1]') into P_LANG from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_AIRCO_ID[1]') into P_AIRCO_ID from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;
    select to_number(extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_IS_REMARK_EMPTY[1]')) into P_IS_REMARK_EMPTY from dual;

    select value(t).extract('/P_REMARK/text()').getclobval() into P_REMARK
    from table(xmlsequence(xmltype(cXML_in).extract('//user/*'))) t
    where value(t).extract('/P_REMARK/text()').getclobval() is not null;

    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID (ID,
                                  num,
                                    date_from,
                                      string_val)
    select distinct f.id,
                      f.num,
                        f.date_from,
                          f.string_val
    from (select to_number(extractValue(value(t),'list/P_AIRCO_ID[1]')) as id,
                 to_number(extractValue(value(t),'list/P_DTM_ID[1]')) as num,
                 to_date(extractValue(value(t),'list/P_DATE_FROM_D[1]')||
                           '.'||
                             extractValue(value(t),'list/P_DATE_FROM_M[1]')||
                               '.'||
                                 extractValue(value(t),'list/P_DATE_FROM_Y[1]'), 'dd.mm.yyyy') as date_from,
                 extractValue(value(t),'list/P_DTM_NAME[1]') as string_val
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    select count(id) into r_count
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
        if p_IS_REMARK_EMPTY=1 then
          begin

            P_REMARK:=null;
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
    if str_msg is null then
      begin
        select count(num) into r_count
        from WB_TEMP_XML_ID
        where not exists(select 1
                         from WB_REF_DATA_TRANSFER_MODES d
                         where d.id=WB_TEMP_XML_ID.num);

        if r_count>0 then
          begin
            if P_LANG='RUS' then
              begin
                str_msg:='Следущие наименования удалены из справочника:'||chr(10)||chr(10);
              end;
            else
              begin
                str_msg:='Next name removed from the directory:'||chr(10)||chr(10);
              end;
          end if;

            open cur_DEL_DTM_REC
            for 'select distinct string_val
                 from WB_TEMP_XML_ID
                 where not exists(select 1
                                  from WB_REF_DATA_TRANSFER_MODES d
                                  where d.id=WB_TEMP_XML_ID.num)
                 order by string_val';

            LOOP
              FETCH cur_DEL_DTM_REC INTO DEL_DTM_NAME;
              EXIT WHEN cur_DEL_DTM_REC%NOTFOUND;

              str_msg:=str_msg||' - '||
                         DEL_DTM_NAME||
                           chr(10);

            END LOOP;
            CLOSE cur_DEL_DTM_REC;

            str_msg:=str_msg||chr(10);

            if P_LANG='RUS' then
              begin
                str_msg:=str_msg||'Операция запрещена!';
              end;
            else
              begin
                str_msg:=str_msg||'Operation is prohibited!';
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
        insert into WB_REF_AIRCO_C_DATA_DTM_HIST (ID_,
                                                    U_NAME_,
	                                                    U_IP_,
	                                                      U_HOST_NAME_,
	                                                        DATE_WRITE_,
	                                                          ACTION_,
	                                                            ID,
	                                                              ID_AC_OLD,
	                                                                DATE_FROM_OLD,
                                                                    REMARK_OLD,
	                                                                    U_NAME_OLD,
	                                                                      U_IP_OLD,
                                                        	                 U_HOST_NAME_OLD,
                                                        	                   DATE_WRITE_OLD,
	                                                                             ID_AC_NEW,
	                                                                               DATE_FROM_NEW,
	                                                                                 REMARK_NEW,
	                                                                                   U_NAME_NEW,
	                                                                                     U_IP_NEW,
	                                                                                       U_HOST_NAME_NEW,
	                                                                                         DATE_WRITE_NEW,
	                                                                                           DTM_ID_OLD,
	                                                                                             DTM_ID_NEW)
        select SEC_WB_REF_AC_C_DATA_DTM_HIST.nextval,
                 P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE(),
                         'delete',
                           q.id,
                             q.id_ac_OLD,
	                             q.DATE_FROM_OLD,
                                 to_clob(q.REMARK_OLD),
	                                 q.U_NAME_NEW,
	                                   q.U_IP_OLD,
                                       q.U_HOST_NAME_OLD,
                                         q.DATE_WRITE_OLD,
	                                         q.ID_AC_NEW,
	                                           q.DATE_FROM_NEW,
	                                             to_clob(q.REMARK_NEW),
	                                               q.U_NAME_NEW,
	                                                 q.U_IP_NEW,
	                                                   q.U_HOST_NAME_NEW,
	                                                     q.DATE_WRITE_NEW,
	                                                       q.DTM_ID_OLD,
	                                                         q.DTM_ID_NEW
        from (select i.id,
                       i.id_ac ID_AC_OLD,
	                       i.DATE_FROM DATE_FROM_OLD,
                           i.REMARK REMARK_OLD,
	                           i.U_NAME U_NAME_OLD,
	                             i.U_IP U_IP_OLD,
                                 i.U_HOST_NAME U_HOST_NAME_OLD,
                                   i.DATE_WRITE DATE_WRITE_OLD,
	                                   i.ID_AC ID_AC_NEW,
	                                     i.DATE_FROM DATE_FROM_NEW,
	                                       i.REMARK REMARK_NEW,
	                                         i.U_NAME U_NAME_NEW,
	                                           i.U_IP U_IP_NEW,
	                                             i.U_HOST_NAME U_HOST_NAME_NEW,
	                                               i.DATE_WRITE DATE_WRITE_NEW,
	                                                 i.DTM_ID dtm_id_old,
	                                                   i.DTM_ID dtm_id_new
           from (select distinct id,
                                 date_from
                 from WB_TEMP_XML_ID) t join WB_REF_AIRCO_C_DATA_DTM i
           on i.id_ac=t.id and
              i.date_from=t.date_from) q;

        delete from WB_REF_AIRCO_C_DATA_DTM
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_AIRCO_C_DATA_DTM.id_ac and
                           t.date_from=WB_REF_AIRCO_C_DATA_DTM.date_from);

        insert into WB_REF_AIRCO_C_DATA_DTM (ID,
	                                             ID_AC,
	                                               DATE_FROM,
	                                                 DTM_ID,
	                                                   REMARK,
	                                                     U_NAME,
	                                                       U_IP,
	                                                         U_HOST_NAME,
	                                                           DATE_WRITE)
       select SEC_WB_REF_AIRCO_C_DATA_DTM.nextval,
                id,
                  date_from,
                    num,
                      P_REMARK,
                        P_U_NAME,
	                        P_U_IP,
	                          P_U_HOST_NAME,
                              SYSDATE()
       from WB_TEMP_XML_ID;

        str_msg:='EMPTY_STRING';
      end;
    end if;

      cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';
    commit;
  end SP_WB_REF_AC_C_DATA_DTM_INSERT;
/
