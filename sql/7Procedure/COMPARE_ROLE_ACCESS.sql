create or replace PROCEDURE compare_role_access(role_id1 IN roles.role_id%TYPE,
                              role_id2 IN roles.role_id%TYPE)
AS
  CURSOR cur IS
    SELECT name FROM rights_list WHERE ida IN
      (SELECT right_id FROM role_rights WHERE role_id=role_id1
       UNION
       SELECT right_id FROM role_assign_rights WHERE role_id=role_id1
       MINUS
       SELECT right_id FROM role_assign_rights WHERE role_id=role_id2)
    ORDER BY ida;
role_name1 roles.name%TYPE;
role_name2 roles.name%TYPE;
BEGIN
  SELECT name||RTRIM(' '||airline)||RTRIM(' '||airp) INTO role_name1 FROM roles WHERE role_id=role_id1;
  SELECT name||RTRIM(' '||airline)||RTRIM(' '||airp) INTO role_name2 FROM roles WHERE role_id=role_id2;
  DBMS_OUTPUT.PUT_LINE('Права роли '''||role_name1||''',');
  DBMS_OUTPUT.PUT_LINE('которые не делегируются ролью '''||role_name2||''':');
  FOR curRow IN cur LOOP
    DBMS_OUTPUT.PUT_LINE(' - '||curRow.name);
  END LOOP;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    DBMS_OUTPUT.PUT_LINE('Не найдена одна из ролей');
END;
/
