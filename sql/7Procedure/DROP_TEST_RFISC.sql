create or replace PROCEDURE drop_test_rfisc(vstart_grp_id pax_grp.grp_id%TYPE DEFAULT NULL) IS
curr_grp_id pax_grp.grp_id%TYPE;
max_grp_id pax_grp.grp_id%TYPE;
processed BINARY_INTEGER;
baggage_airline bag_types_lists.airline%TYPE;

CURSOR cur(low_grp_id pax_grp.grp_id%TYPE,
           high_grp_id pax_grp.grp_id%TYPE) IS
  SELECT pax_grp.grp_id, pax_grp.bag_types_id, bag_types_lists.airline
  FROM pax_grp, bag_types_lists
  WHERE pax_grp.grp_id>=low_grp_id AND pax_grp.grp_id<high_grp_id AND
        pax_grp.bag_types_id=bag_types_lists.id;

CURSOR cur2(vgrp_id pax_grp.grp_id%TYPE) IS
  SELECT rfisc_list_items.airline, rfisc_list_items.rfisc
  FROM rfisc_list_items, pax_service_lists, pax
  WHERE rfisc_list_items.list_id=pax_service_lists.list_id AND
        pax_service_lists.pax_id=pax.pax_id AND
        pax.grp_id=vgrp_id AND
        pax_service_lists.transfer_num=0 AND
        pax_service_lists.category IN (1,2) AND
        rfisc_list_items.category IN (1,2) AND
        rfisc_list_items.service_type='C'
  UNION ALL
  SELECT rfisc_list_items.airline, rfisc_list_items.rfisc
  FROM rfisc_list_items, grp_service_lists
  WHERE rfisc_list_items.list_id=grp_service_lists.list_id AND
        grp_service_lists.grp_id=vgrp_id AND
        grp_service_lists.transfer_num=0 AND
        grp_service_lists.category IN (1,2) AND
        rfisc_list_items.category IN (1,2) AND
        rfisc_list_items.service_type='C'
  ORDER BY rfisc, airline;
cur2Row cur2%ROWTYPE;
BEGIN
  IF vstart_grp_id IS NOT NULL THEN
    DELETE FROM drop_test_rfisc_progress;
    INSERT INTO drop_test_rfisc_progress(start_grp_id) VALUES(vstart_grp_id);
  END IF;

  SELECT start_grp_id INTO curr_grp_id FROM drop_test_rfisc_progress;

  processed:=0;

  SELECT MAX(grp_id) INTO max_grp_id FROM pax_grp;

  DELETE FROM drop_test_rfisc_diff;
  COMMIT;

  WHILE curr_grp_id<=max_grp_id LOOP
    FOR curRow IN cur(curr_grp_id, curr_grp_id+1000) LOOP
      baggage_airline:=NULL;
      OPEN cur2(curRow.grp_id);
      FETCH cur2 INTO cur2Row;
      IF cur2%FOUND THEN
        baggage_airline:=cur2Row.airline;
      END IF;
      CLOSE cur2;

INSERT INTO drop_test_rfisc_diff(grp_id, bag_types_id, rfisc, name, name_lat, airline, priority, pr_cabin)
(
SELECT curRow.grp_id, NULL,
       rfisc AS code, LOWER(name) AS name, LOWER(name_lat) AS name_lat,
       r.airline, NVL(rfisc_bag_props.priority, 100000) AS priority, pr_cabin
FROM rfisc_bag_props,
(SELECT pax.grp_id AS bag_types_id,
        rfisc_list_items.rfisc,
        rfisc_list_items.name,
        rfisc_list_items.name_lat,
        baggage_airline AS airline,
        DECODE(rfisc_list_items.category, 1, 0, 1) AS pr_cabin
 FROM rfisc_list_items, pax_service_lists, pax
 WHERE rfisc_list_items.list_id=pax_service_lists.list_id AND
       pax_service_lists.pax_id=pax.pax_id AND
       pax.grp_id=curRow.grp_id AND
       pax_service_lists.transfer_num=0 AND
       pax_service_lists.category IN (1,2) AND
       rfisc_list_items.category IN (1,2) AND
       rfisc_list_items.visible<>0 AND
       rfisc_list_items.service_type='C'
 UNION
 SELECT grp_service_lists.grp_id,
        rfisc_list_items.rfisc,
        rfisc_list_items.name,
        rfisc_list_items.name_lat,
        baggage_airline AS airline,
        DECODE(rfisc_list_items.category, 1, 0, 1) AS pr_cabin
 FROM rfisc_list_items, grp_service_lists
 WHERE rfisc_list_items.list_id=grp_service_lists.list_id AND
       grp_service_lists.grp_id=curRow.grp_id AND
       grp_service_lists.transfer_num=0 AND
       grp_service_lists.category IN (1,2) AND
       rfisc_list_items.category IN (1,2) AND
       rfisc_list_items.visible<>0 AND
       rfisc_list_items.service_type='C'
) r
WHERE r.airline=rfisc_bag_props.airline(+) AND
      r.rfisc=rfisc_bag_props.code(+)
MINUS
SELECT curRow.grp_id, NULL,
       rfisc AS code, LOWER(name) AS name, LOWER(name_lat) AS name_lat,
       r.airline, NVL(rfisc_bag_props.priority, 100000) AS priority, pr_cabin
FROM rfisc_bag_props,
(SELECT list_id AS bag_types_id,
        rfisc, name, name_lat,
        bag_types_lists.airline, pr_cabin
 FROM grp_rfisc_lists, bag_types_lists
 WHERE grp_rfisc_lists.list_id=bag_types_lists.id AND
       list_id=curRow.bag_types_id) r
WHERE r.airline=rfisc_bag_props.airline(+) AND
      r.rfisc=rfisc_bag_props.code(+)
);

INSERT INTO drop_test_rfisc_diff(grp_id, bag_types_id, rfisc, name, name_lat, airline, priority, pr_cabin)
(
SELECT curRow.grp_id, curRow.bag_types_id,
       rfisc AS code, LOWER(name) AS name, LOWER(name_lat) AS name_lat,
       r.airline, NVL(rfisc_bag_props.priority, 100000) AS priority, pr_cabin
FROM rfisc_bag_props,
(SELECT list_id AS bag_types_id,
        rfisc, name, name_lat,
        bag_types_lists.airline, pr_cabin
 FROM grp_rfisc_lists, bag_types_lists
 WHERE grp_rfisc_lists.list_id=bag_types_lists.id AND
       list_id=curRow.bag_types_id) r
WHERE r.airline=rfisc_bag_props.airline(+) AND
      r.rfisc=rfisc_bag_props.code(+)
MINUS
SELECT curRow.grp_id, curRow.bag_types_id,
       rfisc AS code, LOWER(name) AS name, LOWER(name_lat) AS name_lat,
       r.airline, NVL(rfisc_bag_props.priority, 100000) AS priority, pr_cabin
FROM rfisc_bag_props,
(SELECT pax.grp_id AS bag_types_id,
        rfisc_list_items.rfisc,
        rfisc_list_items.name,
        rfisc_list_items.name_lat,
        baggage_airline AS airline,
        DECODE(rfisc_list_items.category, 1, 0, 1) AS pr_cabin
 FROM rfisc_list_items, pax_service_lists, pax
 WHERE rfisc_list_items.list_id=pax_service_lists.list_id AND
       pax_service_lists.pax_id=pax.pax_id AND
       pax.grp_id=curRow.grp_id AND
       pax_service_lists.transfer_num=0 AND
       pax_service_lists.category IN (1,2) AND
       rfisc_list_items.category IN (1,2) AND
       rfisc_list_items.visible<>0 AND
       rfisc_list_items.service_type='C'
 UNION
 SELECT grp_service_lists.grp_id,
        rfisc_list_items.rfisc,
        rfisc_list_items.name,
        rfisc_list_items.name_lat,
        baggage_airline AS airline,
        DECODE(rfisc_list_items.category, 1, 0, 1) AS pr_cabin
 FROM rfisc_list_items, grp_service_lists
 WHERE rfisc_list_items.list_id=grp_service_lists.list_id AND
       grp_service_lists.grp_id=curRow.grp_id AND
       grp_service_lists.transfer_num=0 AND
       grp_service_lists.category IN (1,2) AND
       rfisc_list_items.category IN (1,2) AND
       rfisc_list_items.visible<>0 AND
       rfisc_list_items.service_type='C'
) r
WHERE r.airline=rfisc_bag_props.airline(+) AND
      r.rfisc=rfisc_bag_props.code(+)
);


      processed:=processed+1;
    END LOOP;
    UPDATE drop_test_rfisc_progress SET start_grp_id=curr_grp_id+1000;
    COMMIT;
    curr_grp_id:=curr_grp_id+1000;
  END LOOP;
  DBMS_OUTPUT.PUT_LINE('processed='||processed);
END;
/
