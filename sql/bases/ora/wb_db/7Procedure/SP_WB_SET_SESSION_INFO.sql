create or replace PROCEDURE SP_WB_SET_SESSION_INFO(cXML_in IN VARCHAR2,
                                                cXML_out OUT CLOB)
AS
cXML_data CLOB := '';
BEGIN
    cXML_out:=to_clob('<?xml version="1.0" ?><root>');

    insert into WB_TEMP_SESSION_INFO(
          U_NAME
        , U_IP
        , U_COMP_NAME
        , U_HOST_NAME
    )
    select
          extractValue( xmltype(cXML_in), '/root[1]/P_U_NAME[1]' )
        , extractValue( xmltype(cXML_in), '/root[1]/P_U_IP[1]' )
        , extractValue( xmltype(cXML_in), '/root[1]/P_U_COMP_NAME[1]' )
        , extractValue( xmltype(cXML_in), '/root[1]/P_U_HOST_NAME[1]' )
    from dual;

    cXML_out:=cXML_out||'</root>';

    commit;
END SP_WB_SET_SESSION_INFO;
/
