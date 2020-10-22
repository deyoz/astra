create or replace procedure SP_WB_REF_AC_PAX_WEIGHT_DELETE
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
                                  num)
    select distinct f.id,
                      -1
    from (select to_number(extractValue(value(t),'list/P_ID[1]')) as id
          from table(xmlsequence(xmltype(cXML_in).extract('//list'))) t) f;

    insert into WB_REF_AIRCO_PAX_WEIGHTS_HIST (ID_,
	                                               U_NAME_,
	                                                 U_IP_,
	                                                   U_HOST_NAME_,
	                                                     DATE_WRITE_,
	                                                       ACTION_,
	                                                         ID,
	                                                           ID_AC_OLD,
                                                               DESCRIPTION_OLD,
	                                                               DATE_FROM_OLD,
	                                                                 ADULT_OLD,
	                                                                   MALE_OLD,
	                                                                     FEMALE_OLD,
	                                                                       CHILD_OLD,
	                                                                         INFANT_OLD,
	                                                                           HAND_BAG_OLD,
	                                                                             HAND_BAG_INCLUDE_OLD,
	                                                                               U_NAME_OLD,
	                                                                                 U_IP_OLD,
	                                                                                   U_HOST_NAME_OLD,
	                                                                                     DATE_WRITE_OLD,
	                                                                                       ID_CLASS_OLD,
	                                                                                         BY_DEFAULT_OLD,
	                                                                                           ID_AC_NEW,
	                                                                                             DESCRIPTION_NEW,
	                                                                                               DATE_FROM_NEW,
	                                                                                                 ADULT_NEW,
	                                                                                                   MALE_NEW,
	                                                                                                     FEMALE_NEW,
	                                                                                                       CHILD_NEW,
	                                                                                                         INFANT_NEW,
	                                                                                                           HAND_BAG_NEW,
	                                                                                                             HAND_BAG_INCLUDE_NEW,
	                                                                                                               U_NAME_NEW,
	                                                                                                                 U_IP_NEW,
	                                                                                                                   U_HOST_NAME_NEW,
	                                                                                                                     DATE_WRITE_NEW,
	                                                                                                                       ID_CLASS_NEW,
	                                                                                                                         BY_DEFAULT_NEW)
    select SEC_WB_REF_AC_PAX_WEIGHTS_HIST.nextval,
             P_U_NAME,
	             P_U_IP,
	               P_U_HOST_NAME,
                   SYSDATE(),
                     'delete',
                       i.id,
                         i.ID_AC,
                           i.DESCRIPTION,
	                           i.DATE_FROM,
	                             i.ADULT,
	                               i.MALE,
	                                 i.FEMALE,
	                                   i.CHILD,
	                                     i.INFANT,
	                                       i.HAND_BAG,
	                                         i.HAND_BAG_INCLUDE,
	                                           i.U_NAME,
	                                             i.U_IP,
	                                               i.U_HOST_NAME,
	                                                 i.DATE_WRITE,
	                                                   i.ID_CLASS,
	                                                     i.BY_DEFAULT,
                                                         i.ID_AC,
                                                           i.DESCRIPTION,
	                                                           i.DATE_FROM,
	                                                             i.ADULT,
	                                                               i.MALE,
	                                                                 i.FEMALE,
	                                                                   i.CHILD,
	                                                                     i.INFANT,
	                                                                       i.HAND_BAG,
	                                                                         i.HAND_BAG_INCLUDE,
	                                                                           i.U_NAME,
	                                                                             i.U_IP,
	                                                                               i.U_HOST_NAME,
	                                                                                 i.DATE_WRITE,
	                                                                                   i.ID_CLASS,
	                                                                                     i.BY_DEFAULT
    from WB_TEMP_XML_ID t join WB_REF_AIRCO_PAX_WEIGHTS i
    on i.id=t.id;

    delete from WB_REF_AIRCO_PAX_WEIGHTS
    where exists(select 1
                 from WB_TEMP_XML_ID t
                 where t.id=WB_REF_AIRCO_PAX_WEIGHTS.id);


      str_msg:='EMPTY_STRING';

      cXML_out:=cXML_out||'<list str_msg="'||str_msg||'"/>'||'</root>';
    commit;
  end SP_WB_REF_AC_PAX_WEIGHT_DELETE;
/
