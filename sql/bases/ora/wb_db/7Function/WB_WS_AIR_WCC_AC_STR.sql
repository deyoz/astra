create or replace FUNCTION WB_WS_AIR_WCC_AC_STR
(T_ID number)
RETURN varchar2
is
V_RETVAL varchar2(5000):='';
TYPE EXIST_REF_REC IS REF CURSOR;
CUR_EXIST_REF_REC EXIST_REF_REC;
u_name varchar2(200);
  begin

    open CUR_EXIST_REF_REC
    for 'select u.CODE_NAME_1 '||
        'from WB_REF_WS_AIR_WCC_AC t join WB_REF_WS_AIR_DOW_SWA_CODES u '||
        'on t.WCC_ID='||to_char(T_ID)||' and '||
           'u.id=t.ADJ_CODE_ID '||
        'order by u.CODE_NAME_1';

     LOOP
      FETCH CUR_EXIST_REF_REC INTO u_name;
      EXIT WHEN CUR_EXIST_REF_REC%NOTFOUND;

      V_RETVAL:=V_RETVAL||u_name||'/';

     END LOOP;
     CLOSE CUR_EXIST_REF_REC;

    if V_RETVAL is not null then
      begin
        V_RETVAL:=SUBSTR(V_RETVAL, 1, length(V_RETVAL)-1);

      end;
    end if;

    return nvl(V_RETVAL, 'EMPTY_STRING');
  end;
/
