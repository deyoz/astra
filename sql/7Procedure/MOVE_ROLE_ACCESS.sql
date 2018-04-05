create or replace PROCEDURE move_role_access(role_id1 IN roles.role_id%TYPE,
                           role_id2 IN roles.role_id%TYPE)
AS
role_name1 VARCHAR2(100);
role_name2 VARCHAR2(100);
BEGIN
--  SELECT adm.get_role_name(role_id) INTO role_name1 FROM roles WHERE role_id=role_id1;
--  SELECT adm.get_role_name(role_id) INTO role_name2 FROM roles WHERE role_id=role_id2;
  INSERT INTO role_rights(role_id,right_id)
  SELECT role_id2,right_id FROM role_rights WHERE role_id=role_id1;
  INSERT INTO role_assign_rights(role_id,right_id)
  SELECT role_id2,right_id FROM role_assign_rights WHERE role_id=role_id1;
END;

/
