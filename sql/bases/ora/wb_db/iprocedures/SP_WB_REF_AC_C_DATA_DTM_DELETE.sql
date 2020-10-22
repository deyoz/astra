create or replace procedure SP_WB_REF_AC_C_DATA_DTM_DELETE
(cXML_in in clob,
   cXML_out out clob)
as
P_U_NAME varchar2(50):='';
P_U_IP varchar2(50):='';
P_U_COMP_NAME varchar2(50):='';
P_U_HOST_NAME varchar2(50):='';
str_msg clob:=null;
  begin
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_NAME[1]') into P_U_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/user[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    insert into WB_TEMP_XML_ID (ID,
                                  num,
                                    date_from)
    select distinct f.id,
                      -1,
                        f.date_from
    from (select to_number(extractValue(value(t),'list/P_AIRCO_ID[1]')) as id,
                 to_date(extractValue(value(t),'list/P_DATE_FROM_D[1]')||
                           '.'||
                             extractValue(value(t),'list/P_DATE_FROM_M[1]')||
                               '.'||
                                 extractValue(value(t),'list/P_DATE_FROM_Y[1]'), 'dd.mm.yyyy') as date_from
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;


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


      str_msg:='EMPTY_STRING';

      cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';
    commit;
  end SP_WB_REF_AC_C_DATA_DTM_DELETE;
/
