ALTER TABLE STAGE_STATUSES ADD CONSTRAINT STAGE_STATUSES__GRAPH_STAGES__ FOREIGN KEY (STAGE_ID) REFERENCES GRAPH_STAGES (STAGE_ID) ENABLE NOVALIDATE;
