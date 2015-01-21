create or replace PACKAGE BODY statist
AS

function get_trfer_route(agrp_id pax_grp.grp_id%type) return varchar2
is
res trfer_stat.trfer_route%type;
cursor ctrfer is
select
   pax_grp.airp_arv,
   0 transfer_num
from
   pax_grp
where
   pax_grp.grp_id = agrp_id
union
select
   transfer.airp_arv,
   transfer.transfer_num
from
   transfer
where
   transfer.grp_id = agrp_id
order by
   transfer_num;
transfer_num transfer.transfer_num%type;
begin
    for c in ctrfer loop
        transfer_num := c.transfer_num;
        if length(res) > 0 then
            res := res||'-';
        end if;
        res := res||c.airp_arv;
    end loop;
    if transfer_num = 0 then
        res := null;
    end if;
    return res;
end;

PROCEDURE get_trfer_stat(vpoint_id IN points.point_id%TYPE)
is
BEGIN
  DELETE FROM trfer_stat WHERE point_id=vpoint_id;
  INSERT INTO trfer_stat
   (point_id,trfer_route,client_type,
    f,c,y,adult,child,baby,child_wop,baby_wop,
    pcs,weight,unchecked,excess)
  SELECT
    vpoint_id,b.trfer_route,b.client_type,
    f,c,y,adult,child,baby,child_wop,baby_wop,
    NVL(pcs,0),NVL(weight,0),NVL(unchecked,0),NVL(excess,0)
  FROM
   (SELECT
      statist.get_trfer_route(pax_grp.grp_id) trfer_route,
      client_type,
      SUM(DECODE(class,'',1,0)) AS f,
      SUM(DECODE(class,'',1,0)) AS c,
      SUM(DECODE(class,'',1,0)) AS y,
      SUM(DECODE(pers_type,'‚‡',1,0)) AS adult,
      SUM(DECODE(pers_type,'',1,0)) AS child,
      SUM(DECODE(pers_type,'',1,0)) AS baby,
      SUM(DECODE(seats,0,DECODE(pers_type,'',1,0),0)) AS child_wop,
      SUM(DECODE(seats,0,DECODE(pers_type,'',1,0),0)) AS baby_wop
    FROM pax_grp,pax,transfer
    WHERE pax_grp.grp_id=pax.grp_id(+) AND point_dep=vpoint_id AND pax_grp.status NOT IN ('E') AND
          pax_grp.grp_id=transfer.grp_id AND transfer_num=1 AND
          (pax.grp_id IS NULL AND pax_grp.class IS NULL OR pax.grp_id IS NOT NULL)
    GROUP BY statist.get_trfer_route(pax_grp.grp_id),client_type) b,
   (SELECT
      statist.get_trfer_route(pax_grp.grp_id) trfer_route,
      client_type,
      SUM(DECODE(pr_cabin,0,amount,0)) AS pcs,
      SUM(DECODE(pr_cabin,0,weight,0)) AS weight,
      SUM(DECODE(pr_cabin,1,weight,0)) AS unchecked
    FROM bag2,pax_grp,transfer
    WHERE pax_grp.grp_id=bag2.grp_id AND point_dep=vpoint_id AND pax_grp.status NOT IN ('E') AND
          pax_grp.grp_id=transfer.grp_id AND transfer_num=1
    GROUP BY statist.get_trfer_route(pax_grp.grp_id),client_type) c,
   (SELECT
      statist.get_trfer_route(pax_grp.grp_id) trfer_route,
      client_type,
      SUM(DECODE(bag_refuse,0,excess,0)) AS excess
    FROM pax_grp,transfer
    WHERE point_dep=vpoint_id AND pax_grp.status NOT IN ('E') AND
          pax_grp.grp_id=transfer.grp_id AND transfer_num=1
    GROUP BY statist.get_trfer_route(pax_grp.grp_id),client_type) d
  WHERE b.trfer_route=c.trfer_route(+) AND
        b.client_type=c.client_type(+) AND
        b.trfer_route=d.trfer_route(+) AND
        b.client_type=d.client_type(+);
END get_trfer_stat;

PROCEDURE get_kiosk_stat(vpoint_id    IN points.point_id%TYPE)
is
begin
  delete  from kiosk_stat where point_id = vpoint_id;
  insert into kiosk_stat (
    point_id,
    desk,
    desk_airp,
    descr,
    adult,
    child,
    baby,
    tckin
  )
  select
      point_dep,
      web_clients.desk,
      desk_grp.airp,
      web_clients.descr,
      SUM(DECODE(pers_type,'‚‡',1,0)) AS adult,
      SUM(DECODE(pers_type,'',1,0)) AS child,
      SUM(DECODE(pers_type,'',1,0)) AS baby,
      sum(nvl2(tckin_pax_grp.grp_id, 1, 0)) tckin
  from
      pax_grp,
      pax,
      tckin_pax_grp,
      web_clients,
      desks,
      desk_grp
  where
      pax_grp.point_dep = vpoint_id and
      pax_grp.status NOT IN ('E') and
      pax_grp.grp_id = pax.grp_id and
      pax_grp.user_id = web_clients.user_id and
      web_clients.client_type = 'KIOSK' and
      web_clients.desk = desks.code and
      desks.grp_id = desk_grp.grp_id and
      pax_grp.grp_id = tckin_pax_grp.grp_id(+) and
      tckin_pax_grp.seg_no(+) = 1
  group by
      point_dep,
      web_clients.desk,
      desk_grp.airp,
      web_clients.descr;
end get_kiosk_stat;

PROCEDURE get_stat(vpoint_id    IN points.point_id%TYPE)
IS
  CURSOR cur1 IS
    SELECT
      airp_arv,
      bag2.hall,
      DECODE(status,'T','T','N') AS status,
      client_type,
      SUM(DECODE(pr_cabin,0,amount,0)) AS pcs,
      SUM(DECODE(pr_cabin,0,weight,0)) AS weight,
      SUM(DECODE(pr_cabin,1,weight,0)) AS unchecked
    FROM pax_grp,bag2
    WHERE pax_grp.grp_id=bag2.grp_id AND point_dep=vpoint_id AND pax_grp.status NOT IN ('E')
    GROUP BY airp_arv,bag2.hall,DECODE(status,'T','T','N'),client_type;
  CURSOR cur2 IS
    SELECT
      airp_arv,
      NVL(bag2.hall,pax_grp.hall) AS hall,
      DECODE(status,'T','T','N') AS status,
      client_type,
      SUM(DECODE(bag_refuse,0,excess,0)) AS excess
    FROM pax_grp,
         (SELECT bag2.grp_id,bag2.hall
          FROM bag2,
               (SELECT bag2.grp_id,MAX(bag2.num) AS num
                FROM pax_grp,bag2
                WHERE pax_grp.grp_id=bag2.grp_id AND point_dep=vpoint_id
                GROUP BY bag2.grp_id) last_bag
          WHERE bag2.grp_id=last_bag.grp_id AND bag2.num=last_bag.num) bag2
    WHERE pax_grp.grp_id=bag2.grp_id(+) AND point_dep=vpoint_id AND pax_grp.status NOT IN ('E')
    GROUP BY airp_arv,NVL(bag2.hall,pax_grp.hall),DECODE(status,'T','T','N'),client_type;

BEGIN
  DELETE FROM stat WHERE point_id=vpoint_id;
  INSERT INTO stat
   (point_id,airp_arv,hall,status,client_type,
    f,c,y,adult,child,baby,child_wop,baby_wop,
    pcs,weight,unchecked,excess)
  SELECT
    vpoint_id,
    airp_arv,
    hall,
    DECODE(status,'T','T','N') AS status,
    client_type,
    SUM(DECODE(class,'',1,0)) AS f,
    SUM(DECODE(class,'',1,0)) AS c,
    SUM(DECODE(class,'',1,0)) AS y,
    SUM(DECODE(pers_type,'‚‡',1,0)) AS adult,
    SUM(DECODE(pers_type,'',1,0)) AS child,
    SUM(DECODE(pers_type,'',1,0)) AS baby,
    SUM(DECODE(seats,0,DECODE(pers_type,'',1,0),0)) AS child_wop,
    SUM(DECODE(seats,0,DECODE(pers_type,'',1,0),0)) AS baby_wop,
    0,0,0,0
  FROM pax_grp,pax
  WHERE pax_grp.grp_id=pax.grp_id AND point_dep=vpoint_id AND pax_grp.status NOT IN ('E')
  GROUP BY airp_arv,hall,DECODE(status,'T','T','N'),client_type;

  FOR cur1Row IN cur1 LOOP
    UPDATE stat
    SET pcs=pcs+cur1Row.pcs,
        weight=weight+cur1Row.weight,
        unchecked=unchecked+cur1Row.unchecked
    WHERE point_id=vpoint_id AND
          airp_arv=cur1Row.airp_arv AND
          (hall IS NULL AND cur1Row.hall IS NULL OR hall=cur1Row.hall) AND
          status=cur1Row.status AND
          client_type=cur1Row.client_type;
    IF SQL%NOTFOUND THEN
      INSERT INTO stat
       (point_id,airp_arv,hall,status,client_type,
        f,c,y,adult,child,baby,child_wop,baby_wop,
        pcs,weight,unchecked,excess)
      VALUES
       (vpoint_id,cur1Row.airp_arv,cur1Row.hall,cur1Row.status,cur1Row.client_type,
        0,0,0,0,0,0,0,0,
        cur1Row.pcs,cur1Row.weight,cur1Row.unchecked,0);
    END IF;
  END LOOP;
  FOR cur2Row IN cur2 LOOP
    UPDATE stat
    SET excess=excess+cur2Row.excess
    WHERE point_id=vpoint_id AND
          airp_arv=cur2Row.airp_arv AND
          (hall IS NULL AND cur2Row.hall IS NULL OR hall=cur2Row.hall) AND
          status=cur2Row.status AND
          client_type=cur2Row.client_type;
    IF SQL%NOTFOUND THEN
      INSERT INTO stat
       (point_id,airp_arv,hall,status,client_type,
        f,c,y,adult,child,baby,child_wop,baby_wop,
        pcs,weight,unchecked,excess)
      VALUES
       (vpoint_id,cur2Row.airp_arv,cur2Row.hall,cur2Row.status,cur2Row.client_type,
        0,0,0,0,0,0,0,0,
        0,0,0,cur2Row.excess);
    END IF;
  END LOOP;
END get_stat;

PROCEDURE get_full_stat(vpoint_id IN points.point_id%TYPE,
                        vpr_stat  IN trip_sets.pr_stat%TYPE)
IS
BEGIN
  get_stat(vpoint_id);
  get_trfer_stat(vpoint_id);
  get_kiosk_stat(vpoint_id);
  IF vpr_stat<>0 THEN
    UPDATE trip_sets SET pr_stat=vpr_stat WHERE point_id=vpoint_id;
  END IF;
  system.MsgToLog('‘΅®ΰ αβ β¨αβ¨ª¨ ―® ΰ¥©αγ',system.evtFlt,vpoint_id);
EXCEPTION
  WHEN OTHERS THEN
    raise_application_error(-20002,'get_full_stat: '||SQLERRM);
END get_full_stat;

END statist;
/
