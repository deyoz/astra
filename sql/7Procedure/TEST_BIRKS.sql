create or replace PROCEDURE test_birks(vlang	IN lang_types.code%TYPE)
IS
res	VARCHAR2(1000);
cur ckin.birks_cursor_ref;
BEGIN
  OPEN cur FOR
    SELECT tag_type,no_len,
           tag_colors.code AS color,
           DECODE(vlang,'RU',tag_colors.code,NVL(tag_colors.code_lat,tag_colors.code)) AS color_view,
           TRUNC(no/1000) AS first,
           MOD(no,1000) AS last,
           no
    FROM drop_test_birks, tag_colors
    WHERE drop_test_birks.color=tag_colors.code(+)
    ORDER BY tag_type,color,no;
  res:=ckin.build_birks_str(cur);
  CLOSE cur;
  DBMS_OUTPUT.PUT_LINE(res);
END test_birks;
/
