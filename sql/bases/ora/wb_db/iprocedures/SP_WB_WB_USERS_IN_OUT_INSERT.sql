create or replace procedure SP_WB_WB_USERS_IN_OUT_INSERT
(U_NAME varchar2,
   U_IP varchar2,
     U_COMP_NAME varchar2,
       U_HOST_NAME varchar2,
         U_ACTION varchar2,
           A_VERSION varchar2,
             DATE_WRITE date)
as
id int:=SEC_WB_USERS_IN_OUT.nextval();
  begin

    insert into WB_USERS_IN_OUT(id,
                                  U_NAME,
                                    u_ip,
                                      U_COMP_NAME,
                                        U_HOST_NAME,
                                          U_ACTION,
                                            A_VERSION,
                                              DATE_WRITE)
    select id,
             U_NAME,
               U_IP,
                 U_COMP_NAME,
                   U_HOST_NAME,
                     U_ACTION,
                       A_VERSION,
                         SYSDATE--DATE_WRITE
    from dual;

    commit;
  end;
/
