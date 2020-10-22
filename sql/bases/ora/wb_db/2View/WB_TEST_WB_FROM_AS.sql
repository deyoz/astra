create or replace view WB_TEST_WB_FROM_AS (
ID
,AP
,IATA
,NEW_COL
) as
select
    w.id as ID,
    A.code as AP,
    w.iata as IATA,
    w.new_col as NEW_COL
from WB_TEST_AS A , WB_TEST_WB W
where
  A.code=W.AP(+)

/
