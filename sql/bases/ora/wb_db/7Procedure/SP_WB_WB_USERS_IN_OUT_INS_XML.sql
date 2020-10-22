create or replace procedure SP_WB_WB_USERS_IN_OUT_INS_XML
(cXML_in in clob,
   cXML_out out clob)
as
id int:=SEC_WB_USERS_IN_OUT.nextval();

P_USER_NAME varchar2(100):='';
P_U_IP varchar2(100):='';
P_U_COMP_NAME varchar2(100):='';
P_U_HOST_NAME varchar2(100):='';
P_U_ACTION varchar2(100):='';
P_A_VERSION varchar2(100):='';
  begin

    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_USER_NAME[1]') into P_USER_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_IP[1]') into P_U_IP from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_COMP_NAME[1]') into P_U_COMP_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_HOST_NAME[1]') into P_U_HOST_NAME from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_U_ACTION[1]') into P_U_ACTION from dual;
    select extractValue(xmltype(cXML_in),'/root[1]/list[1]/P_A_VERSION[1]') into P_A_VERSION from dual;

    insert into WB_USERS_IN_OUT(id,
                                  U_NAME,
                                    u_ip,
                                      U_COMP_NAME,
                                        U_HOST_NAME,
                                          U_ACTION,
                                            A_VERSION,
                                              DATE_WRITE)
    select id,
             P_USER_NAME,
               P_U_IP,
                 P_U_COMP_NAME,
                   P_U_HOST_NAME,
                     P_U_ACTION,
                       P_A_VERSION,
                         SYSDATE--DATE_WRITE
    from dual;

    commit;
  end;
/
