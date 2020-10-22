create or replace PROCEDURE SP_WB_REF_GET_CONTROL_SEATING(cXML_in IN clob,
                                           cXML_out OUT CLOB)
AS
cXML_data clob:='';
BEGIN
cXML_data:='<?xml version="1.0" encoding="utf-8"?>
<root name="get_load_control_seating" result="ok">
    <list Seat="0A" Estimated="5" Actual="1" Maximum="12" />
    <list Seat="0B" Estimated="15" Actual="3" Maximum="42" />
    <list Seat="0C" Estimated="20" Actual="7" Maximum="42" />
    <list Seat="0D" Estimated="15" Actual="12" Maximum="36" />
</root>';

cXML_out:=cXML_data;
END SP_WB_REF_GET_CONTROL_SEATING;
/
