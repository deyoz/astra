ALTER TABLE STAGE_NAMES ADD CONSTRAINT STAGE_NAMES__GRAPH_STAGES__FK FOREIGN KEY (STAGE_ID) REFERENCES GRAPH_STAGES (STAGE_ID) ;
