-- as sysdba
create user queadm identified by queadm;

grant connect, resource, aq_administrator_role to queadm;
/
grant execute on dbms_aq to queadm
/
grant execute on dbms_aqadm to queadm
/
grant execute on dbms_lock to queadm
/
alter user queadm quota unlimited on users
/

-- as queadm
connect queadm/queadm

begin
  dbms_aqadm.create_queue_table(queue_table => 'test_queue1_table', queue_payload_type => 'SYS.aq$_jms_text_message');
  dbms_aqadm.create_queue(queue_name => 'test_queue1', queue_table => 'test_queue1_table');
  dbms_aqadm.start_queue(queue_name => 'test_queue1');
end;
/
begin
  dbms_aqadm.create_queue_table(queue_table => 'test_queue2_table', queue_payload_type => 'SYS.aq$_jms_text_message');
  dbms_aqadm.create_queue(queue_name => 'test_queue2', queue_table => 'test_queue2_table');
  dbms_aqadm.start_queue(queue_name => 'test_queue2');
end;
/
begin
  dbms_aqadm.create_queue_table(queue_table => 'mc_test_queue1_table', multiple_consumers => TRUE, queue_payload_type => 'SYS.aq$_jms_text_message');
  dbms_aqadm.create_queue(queue_name => 'mc_test_queue1', queue_table => 'mc_test_queue1_table');
  dbms_aqadm.start_queue(queue_name => 'mc_test_queue1');
end;
/
CREATE OR REPLACE Type BinaryAQPayload as object( message BLOB );
/
begin
  dbms_aqadm.create_queue_table(queue_table => 'bin_test_queue1_table', queue_payload_type => 'BINARYAQPAYLOAD');
  dbms_aqadm.create_queue(queue_name => 'bin_test_queue1', queue_table => 'bin_test_queue1_table');
  dbms_aqadm.start_queue(queue_name => 'bin_test_queue1');
end;
/
begin
  dbms_aqadm.create_queue_table(queue_table => 'bin_test_queue2_table', queue_payload_type => 'BINARYAQPAYLOAD');
  dbms_aqadm.create_queue(queue_name => 'bin_test_queue2', queue_table => 'bin_test_queue2_table');
  dbms_aqadm.start_queue(queue_name => 'bin_test_queue2');
end;
/
begin
  dbms_aqadm.create_queue_table(queue_table => 'bin_mc_test_queue1_table', multiple_consumers => TRUE, queue_payload_type => 'BINARYAQPAYLOAD');
  dbms_aqadm.create_queue(queue_name => 'bin_mc_test_queue1', queue_table => 'bin_mc_test_queue1_table');
  dbms_aqadm.start_queue(queue_name => 'bin_mc_test_queue1');
end;
/

