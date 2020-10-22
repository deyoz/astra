create or replace PROCEDURE SP_WBA_GET_FLIGHT_STAGES
(cXML_in in clob,
   cXML_out out clob)
AS
cXML_data XMLType; P_ID number:=-1; vID_AC number; vDt varchar(30);

BEGIN
  -- Получит входной параметр
  select to_number(extractValue(xmltype(cXML_in),'/root[1]/@elem_id'))
  into P_ID
  from dual;

  select to_char(sysdate, 'DD.MM.YYYY HH24:MI') into vDt from dual;

  -- Получить список параметров для условий выборки из временной таблицы расписания
  select t1.ID_AC
  into vID_AC
  from WB_SHED t1
  where t1.ID = P_ID;

  cXML_out := '<?xml version="1.0" ?><root>';

  select XMLAGG(
                  XMLELEMENT("stage",
                              XMLATTRIBUTES(tt1.id "id", tt1.stage_ti "stage_ti", tt1.is_private "is_private", tt1.en_name "en_name", tt1.ru_name "ru_name",
                                            tt1.done "done", tt1.done_dt "done_dt", tt1.en_reason "en_reason", tt1.ru_reason "ru_reason", tt1.user_know "user_know",
                                            tt1.user_know_dt "user_know_dt", tt1.user_done "user_done", tt1.user_done_dt "user_done_dt")
                            )
                )
  INTO cXML_data
  from
  (
    select P_ID id, '-mmmm' stage_ti, 'N' is_private, 'Send EZFW' en_name, 'Послать EZFW' ru_name, 'N' done, vDt done_dt, 'Mail do not work.' en_reason,
          'Почтовая система не работает.' ru_reason, 'Y' user_know, vDt user_know_dt, 'Y' user_done, vDt user_done_dt
    from dual
  ) tt1;

  if cXML_data is not NULL then
  begin
    cXML_out := '<?xml version="1.0" ?><root name="get_flight_stages" result="ok">' || cXML_data.getClobVal() || '</root>';
  end;
  else
  begin
    cXML_out := '<?xml version="1.0" encoding="utf-8"?><root name="get_flight_stages" result="ok1"></root>';
  end;
  end if;

  commit;
END SP_WBA_GET_FLIGHT_STAGES;
/
