ALTER TABLE GRAPH_RULES ADD CONSTRAINT GRAPH_RULES__TARGET_STAGE__FK FOREIGN KEY (TARGET_STAGE) REFERENCES GRAPH_STAGES (STAGE_ID) ENABLE NOVALIDATE;