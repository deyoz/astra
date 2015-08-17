create or replace PACKAGE hist
AS

PROCEDURE synchronize_history(vtable_name   history_tables.code%TYPE,
                              vrow_ident    history_events.row_ident%TYPE,
                              vuser         history_events.open_user%TYPE,
                              vdesk         history_events.open_desk%TYPE);

END hist;
/
