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
    pcs,weight,unchecked,excess_wt,excess_pc)
  SELECT
    vpoint_id,b.trfer_route,b.client_type,
    f,c,y,adult,child,baby,child_wop,baby_wop,
    NVL(pcs,0),NVL(weight,0),NVL(unchecked,0),NVL(excess_wt,0),NVL(excess_pc,0)
  FROM
   (SELECT
      statist.get_trfer_route(pax_grp.grp_id) trfer_route,
      client_type,
      SUM(DECODE(class,'è',1,0)) AS f,
      SUM(DECODE(class,'Å',1,0)) AS c,
      SUM(DECODE(class,'ù',1,0)) AS y,
      SUM(DECODE(pers_type,'Çá',1,0)) AS adult,
      SUM(DECODE(pers_type,'êÅ',1,0)) AS child,
      SUM(DECODE(pers_type,'êå',1,0)) AS baby,
      SUM(DECODE(seats,0,DECODE(pers_type,'êÅ',1,0),0)) AS child_wop,
      SUM(DECODE(seats,0,DECODE(pers_type,'êå',1,0),0)) AS baby_wop
    FROM pax_grp,pax,transfer
    WHERE pax_grp.grp_id=pax.grp_id(+) AND point_dep=vpoint_id AND pax_grp.status NOT IN ('E') AND
          pax.refuse is null and
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
          ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 and
          pax_grp.grp_id=transfer.grp_id AND transfer_num=1
    GROUP BY statist.get_trfer_route(pax_grp.grp_id),client_type) c,
   (SELECT
      statist.get_trfer_route(pax_grp.grp_id) trfer_route,
      client_type,
      SUM(DECODE(bag_refuse,0,excess_wt,0)) AS excess_wt,
      SUM(DECODE(bag_refuse,0,excess_pc,0)) AS excess_pc
    FROM pax_grp,transfer
    WHERE point_dep=vpoint_id AND pax_grp.status NOT IN ('E') AND
          pax_grp.grp_id=transfer.grp_id AND transfer_num=1
    GROUP BY statist.get_trfer_route(pax_grp.grp_id),client_type) d
  WHERE b.trfer_route=c.trfer_route(+) AND
        b.client_type=c.client_type(+) AND
        b.trfer_route=d.trfer_route(+) AND
        b.client_type=d.client_type(+);
END get_trfer_stat;

END statist;
/
