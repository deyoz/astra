create or replace procedure SP_WB_REF_AC_C_DATA_1_DELETE
(cXML_in in clob,
   cXML_out out clob)
as
P_AIRCO_ID number:=-1;
P_LANG varchar2(50):='';
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

    insert into WB_REF_AIRCO_C_DATA_1_HISTORY (ID_,
                                               U_NAME_,
	                                               U_IP_,
	                                                 U_HOST_NAME_,
	                                                   DATE_WRITE_,
	                                                     ACTION,
	                                                       ID,
	                                                         ID_AC_OLD,
	                                                           DATE_FROM_OLD,
                                                               H_ADRESS_OLD,
	                                                               C_PERSONE_OLD,
	                                                                 E_MAIL_ADRESS_OLD,
	                                                                   TELETYPE_ADRESS_OLD,
	                                                                     PHONE_NUMBER_OLD,
	                                                                     	 FAX_NUMBER_OLD,
		                                                                       REMARK_OLD,
		                                                                         U_NAME_OLD,
		                                                                           U_IP_OLD,
		                                                                             U_HOST_NAME_OLD,
		                                                                               DATE_WRITE_OLD,
		                                                                                 ID_AC_NEW,
	                                                                               	     DATE_FROM_NEW,
	                                                                 	                     H_ADRESS_NEW,
	                                                                     	                   C_PERSONE_NEW,
	                                                                     	                     E_MAIL_ADRESS_NEW,
	                                                                     	                       TELETYPE_ADRESS_NEW,
	                                                                     	                         PHONE_NUMBER_NEW,
	                                                                     	                           FAX_NUMBER_NEW,
	                                                                     	                             REMARK_NEW,
	                                                                     	                               U_NAME_NEW,
	                                                                     	                                 U_IP_NEW,
	                                                                     	                                   U_HOST_NAME_NEW,
	                                                                     	                                     DATE_WRITE_NEW)
    select SEC_WB_REF_AIRCO_C_DATA_1_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       d.id,
                         d.id_ac,
	                         d.DATE_FROM,
                             d.H_ADRESS,
	                             d.C_PERSONE,
	                               d.E_MAIL_ADRESS,
	                                 d.TELETYPE_ADRESS,
	                                   d.PHONE_NUMBER,
	                                     d.FAX_NUMBER,
	                                       d.REMARK,	
	                                         d.U_NAME,
	                                           d.U_IP,
	                                             d.U_HOST_NAME,
	                                               d.DATE_WRITE,
                                                   d.id_ac,
                                                     d.DATE_FROM,
                                                       d.H_ADRESS,
                         	                               d.C_PERSONE,
	                                                         d.E_MAIL_ADRESS,
	                                                           d.TELETYPE_ADRESS,
	                                                             d.PHONE_NUMBER,
	                                                               d.FAX_NUMBER,
	                                                                 d.REMARK,	
	                                                                   d.U_NAME,
	                                                                     d.U_IP,
	                                                                       d.U_HOST_NAME,
	                                                                         d.DATE_WRITE
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_C_DATA_1 d
    on d.id=t.id;

    delete from WB_REF_AIRCO_C_DATA_1
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_C_DATA_1.id);

    str_msg:='EMPTY_STRING';


    cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';

    commit;
  end SP_WB_REF_AC_C_DATA_1_DELETE;
/
