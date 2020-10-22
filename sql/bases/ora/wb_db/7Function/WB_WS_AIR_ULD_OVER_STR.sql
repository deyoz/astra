create or replace FUNCTION WB_WS_AIR_ULD_OVER_STR(P_ID_AC number,
                                                    P_ID_WS number,
                                                      P_ID_BORT number,
                                                        P_POSITION varchar2)
RETURN varchar2
is
V_RETVAL varchar2(5000):='';

TYPE EXIST_REF_REC IS REF CURSOR;
CUR_EXIST_REF_REC EXIST_REF_REC;
over_name varchar2(200);
  begin

    open CUR_EXIST_REF_REC
    for 'select OVERLAY '||
        'from WB_REF_WS_AIR_ULD_OVER  '||
        'where ID_AC='||to_char(P_ID_AC)||' and '||
              'ID_WS='||to_char(P_ID_WS)||' and '||
              'ID_BORT='||to_char(P_ID_BORT)||' and  '||
              'POSITION='''||P_POSITION||''' '||
        'order by OVERLAY';

     LOOP
      FETCH CUR_EXIST_REF_REC INTO over_name;
      EXIT WHEN CUR_EXIST_REF_REC%NOTFOUND;

      V_RETVAL:=V_RETVAL||over_name||'/';

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
