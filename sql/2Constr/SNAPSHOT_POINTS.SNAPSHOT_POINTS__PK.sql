ALTER TABLE SNAPSHOT_POINTS ADD CONSTRAINT SNAPSHOT_POINTS__PK PRIMARY KEY (POINT_ID,FILE_TYPE,POINT_ADDR,PAGE_NO) USING INDEX SNAPSHOT_POINTS__PK;

