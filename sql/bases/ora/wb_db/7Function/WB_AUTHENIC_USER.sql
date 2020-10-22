create or replace FUNCTION WB_AUTHENIC_USER
(USER_NAME varchar2,
   USER_PASSWORD varchar2,
      U_IP varchar2,
        U_COMP_NAME varchar2,
          U_HOST_NAME varchar2,
            U_ACTION varchar2,
              A_VERSION varchar2,
                DATE_WRITE date)
RETURN  t_tf_basic_message_tab
as
m_tab t_tf_basic_message_tab:=t_tf_basic_message_tab();
r_count int:=0;
str_msg varchar2(100):='';
  begin
    m_tab.extend;
    ----------------------------------------------------------------------------
    --�஢�ઠ ���������� ���짮��⥫�-----------------------------------------
    select count(id) into r_count
    from wb_users
    where u_name=USER_NAME and
          u_password=USER_PASSWORD;

    if r_count>0 then
      begin
         SP_WB_WB_USERS_IN_OUT_INSERT(USER_NAME,
                                       U_IP,
                                         U_COMP_NAME,
                                           U_HOST_NAME,
                                             U_ACTION,
                                               A_VERSION,
                                                 DATE_WRITE);
      end;
    else
      begin
        str_msg:='Invalid UserName or Password!';


      end;
    end if;
    --�஢�ઠ ���������� ���짮��⥫�-----------------------------------------
    ----------------------------------------------------------------------------

    m_tab(m_tab.last):=t_tf_basic_message_row(1, str_msg);
    RETURN m_tab;
  end;
/
