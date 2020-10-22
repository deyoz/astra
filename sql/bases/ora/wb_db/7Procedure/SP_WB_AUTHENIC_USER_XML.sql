create or replace procedure SP_WB_AUTHENIC_USER_XML
(cXML_in in clob,
   cXML_out out clob)
as
r_count int:=0;
V_STR_MSG varchar2(500):='';

P_USER_NAME varchar2(100):='';
P_USER_PASSWORD varchar2(100):='';
P_U_IP varchar2(100):='';
P_U_COMP_NAME varchar2(100):='';
P_U_HOST_NAME varchar2(100):='';
P_U_ACTION varchar2(100):='';
P_A_VERSION varchar2(100):='';
P_DATE_WRITE date:=sysdate();
P_cXML_out clob;
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_USER_NAME[1]') into P_USER_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_USER_PASSWORD[1]') into P_USER_PASSWORD from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_ACTION[1]') into P_U_ACTION from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_A_VERSION[1]') into P_A_VERSION from dual;

    cXML_out:='<?xml version="1.0" ?><root>';

    select count(id) into r_count
    from wb_users
    where u_name=P_USER_NAME and
          u_password=P_USER_PASSWORD;

    if r_count>0 then
      begin
        SP_WB_WB_USERS_IN_OUT_INS_XML(cXML_in, P_cXML_out);

        V_STR_MSG:='EMPTY_STRING';
      end;
    else
      begin
        V_STR_MSG:='Invalid UserName or Password!';


      end;
    end if;

    cXML_out:=cXML_out||'<list str_msg="'||V_STR_MSG||'"/>'||'</root>';
    commit;
  end SP_WB_AUTHENIC_USER_XML;
/
