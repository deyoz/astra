create or replace procedure SP_WB_REF_AC_CLASS_CODE_DELETE
(cXML_in in clob,
   cXML_out out clob)
as
P_LANG varchar2(50);
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
V_STR_MSG clob:=null;

R_COUNT number:=0;
TYPE EXIST_REF_REC IS REF CURSOR;
CUR_EXIST_REF_REC EXIST_REF_REC;
EXIST_REF_NAME varchar2(200);
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_LANG[1]') into P_LANG from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID (ID,
                                  num)
    select distinct f.id,
                      cc.id_ac
    from (select to_number(extractValue(value(t),'list/P_ID[1]')) as id
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f join WB_REF_AIRCO_CLASS_CODES cc
    on cc.id=f.id;

    --savepoint sp_1;

    select count(distinct(tt.id)) into R_COUNT
    from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SL_CAI_TT tt
    on tt.id_ac=t.num and
       not exists(select 1
                  from WB_REF_AIRCO_CLASS_CODES cc
                  where cc.id_ac=tt.id_ac and
                        cc.CLASS_CODE=tt.class_code and
                        cc.id not in (select distinct ttt.id from WB_TEMP_XML_ID ttt));

    if R_COUNT>0 then
      begin
        if P_LANG='ENG' then V_STR_MSG:='Referenced in blocks "Airline Fleet"->"Seating Layout"->"Configuration"->"Cabin Area Information"!'; end if;
        if P_LANG='RUS' then V_STR_MSG:='Имеются ссылки в блоках "Airline Fleet"->"Seating Layout"->"Configuration"->"Cabin Area Information"!'; end if;
      end;
    end if;
    -------------------------------------------------------------------------------------------
    -------------------------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(distinct(tt.id)) into R_COUNT
        from WB_REF_WS_AIR_SL_CAI_TT tt
        where tt.id_ac=-1 and
        not exists(select 1
                  from WB_REF_AIRCO_CLASS_CODES cc
                  where cc.CLASS_CODE=tt.class_code and
                        cc.id not in (select distinct ttt.id from WB_TEMP_XML_ID ttt));

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then V_STR_MSG:='Referenced in blocks "Types of Aircraft"->"Seating Layout"->"Configuration"->"Cabin Area Information"!'; end if;
            if P_LANG='RUS' then V_STR_MSG:='Имеются ссылки в блоках "Types of Aircraft"->"Seating Layout"->"Configuration"->"Cabin Area Information"!'; end if;
          end;
        end if;
      end;
    end if;
    -------------------------------------------------------------------------------------------
    -------------------------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(distinct(tt.id)) into R_COUNT
        from WB_TEMP_XML_ID t join WB_REF_WS_AIR_SL_CI_T tt
        on tt.id_ac=t.num and
           not exists(select 1
                      from WB_REF_AIRCO_CLASS_CODES cc
                       where cc.id_ac=tt.id_ac and
                             cc.CLASS_CODE=tt.class_code and
                             cc.id not in (select distinct ttt.id from WB_TEMP_XML_ID ttt));

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then V_STR_MSG:='Referenced in blocks "Airline Fleet"->"Seating Layout"->"Configuration"->"Class Information"!'; end if;
            if P_LANG='RUS' then V_STR_MSG:='Имеются ссылки в блоках "Airline Fleet"->"Seating Layout"->"Configuration"->"Class Information"!'; end if;
          end;
        end if;
      end;
    end if;
    -------------------------------------------------------------------------------------------
    -------------------------------------------------------------------------------------------
    if V_STR_MSG is null then
      begin
        select count(distinct(tt.id)) into R_COUNT
        from WB_REF_WS_AIR_SL_CI_T tt
        where tt.id_ac=-1 and
        not exists(select 1
                  from WB_REF_AIRCO_CLASS_CODES cc
                  where cc.CLASS_CODE=tt.class_code and
                        cc.id not in (select distinct ttt.id from WB_TEMP_XML_ID ttt));

        if R_COUNT>0 then
          begin
            if P_LANG='ENG' then V_STR_MSG:='Referenced in blocks "Types of Aircraft"->"Seating Layout"->"Configuration"->"Class Information"!'; end if;
            if P_LANG='RUS' then V_STR_MSG:='Имеются ссылки в блоках "Types of Aircraft"->"Seating Layout"->"Configuration"->"Class Information"!'; end if;
          end;
        end if;
      end;
    end if;
    -------------------------------------------------------------------------------------------
    -------------------------------------------------------------------------------------------

    --WB_REF_AIRCO_PAX_WEIGHTS--------------
    if V_STR_MSG is null then
      begin
        select count(p.id) into R_COUNT
        from WB_TEMP_XML_ID t join WB_REF_AIRCO_PAX_WEIGHTS p
        on t.id=p.id_class;

        if R_COUNT>0 then
          begin
            if P_LANG='RUS'
              then V_STR_MSG:='Следущие наименования используются в таблице WB_REF_AIRCO_PAX_WEIGHTS:'||chr(10)||chr(10);
              else V_STR_MSG:='The following names are used in the table WB_REF_AIRCO_PAX_WEIGHTS:'||chr(10)||chr(10);
            end if;

            open CUR_EXIST_REF_REC
            for 'select distinct i.CLASS_CODE
                 from WB_TEMP_XML_ID t join WB_REF_AIRCO_PAX_WEIGHTS p
                 on t.id=p.id_class join WB_REF_AIRCO_CLASS_CODES i
                    on i.id=t.id
                 order by i.CLASS_CODE';

                LOOP
                  FETCH CUR_EXIST_REF_REC INTO EXIST_REF_NAME;
                  EXIT WHEN CUR_EXIST_REF_REC%NOTFOUND;

                  V_STR_MSG:=V_STR_MSG||' - '||
                               EXIST_REF_NAME||
                                 chr(10);

                END LOOP;
                CLOSE CUR_EXIST_REF_REC;

            V_STR_MSG:=V_STR_MSG||chr(10);

            if P_LANG='RUS' then
              begin
                V_STR_MSG:=V_STR_MSG||'Операция запрещена!';
              end;
            else
              begin
                V_STR_MSG:=V_STR_MSG||'Operation is prohibited!';
              end;
            end if;

            delete from WB_TEMP_XML_ID
            where exists(select 1
                         from WB_REF_AIRCO_PAX_WEIGHTS t
                         where t.id_class=WB_TEMP_XML_ID.id);
          end;
        end if;
        --WB_REF_AIRCO_PAX_WEIGHTS--------------
        --------------------------------------------------------------------------------------------
        --------------------------------------------------------------------------------------------
        insert into WB_REF_AIRCO_CLASS_CODES_HIST (ID_,
	                                                   U_NAME_,
	                                                     U_IP_,
	                                                       U_HOST_NAME_,
	                                                         DATE_WRITE_,
	                                                           ACTION_,
	                                                             ID,
	                                                               ID_AC_OLD,
                                                                   CLASS_CODE_OLD,
	                                                                   DATE_FROM_OLD,
                                                                       DATE_TO_OLD,
                                                                         PRIORITY_CODE_OLD,
                                                          	               DESCRIPTION_OLD,
	                                                                           U_NAME_OLD,
	                                                                             U_IP_OLD,
                                                      	                         U_HOST_NAME_OLD,
	                                                                                 DATE_WRITE_OLD,
	                                                                                   ID_AC_NEW,
                                                                                       CLASS_CODE_NEW,
	                                                                                       DATE_FROM_NEW,
                                                                                           DATE_TO_NEW,
                                                                                             PRIORITY_CODE_NEW,
	                                                                                             DESCRIPTION_NEW,
	                                                                                               U_NAME_NEW,
	                                                                                                 U_IP_NEW,
	                                                                                                   U_HOST_NAME_NEW,
	                                                                                                     DATE_WRITE_NEW)
        select SEC_WB_REF_AC_CLASS_CODES_HIST.nextval,
                 P_U_NAME,
	                 P_U_IP,
	                   P_U_HOST_NAME,
                       SYSDATE(),
                         'delete',
                           i.id,
                             i.ID_AC,
                               i.CLASS_CODE,
	                               i.DATE_FROM,
                                   i.DATE_TO,
                                     i.priority_code,
                                       i.DESCRIPTION,
	                                       i.U_NAME,
	                                         i.U_IP,
	                                           i.U_HOST_NAME,
	                                             i.DATE_WRITE,
                                                 i.ID_AC,
                                                   i.CLASS_CODE,
	                                                   i.DATE_FROM,
                                                       i.DATE_TO,
                                                         i.priority_code,
                                                           i.DESCRIPTION,
	                                                           i.U_NAME,
	                                                             i.U_IP,
	                                                               i.U_HOST_NAME,
	                                                                 i.DATE_WRITE
        from WB_TEMP_XML_ID t join WB_REF_AIRCO_CLASS_CODES i
        on i.id=t.id;

        delete from WB_REF_AIRCO_CLASS_CODES
        where exists(select 1
                     from WB_TEMP_XML_ID t
                     where t.id=WB_REF_AIRCO_CLASS_CODES.id);

        if V_STR_MSG is null then
          begin
            V_STR_MSG:='EMPTY_STRING';
          end;
        end if;

        commit;
      end;
    end if;

      cXML_out:=cXML_out||'<list str_msg="'||WB_CLEAR_XML(V_STR_MSG)||'"/>'||'</root>';
    --commit;
  end SP_WB_REF_AC_CLASS_CODE_DELETE;
/
