ALTER TABLE GRAPH_RULES ADD CONSTRAINT GRAPH_RULES__COND_STAGE__FK FOREIGN KEY (COND_STAGE) REFERENCES GRAPH_STAGES (STAGE_ID) ENABLE NOVALIDATE;
