ALTER TABLE DESK_BP_SET ADD CONSTRAINT DESK_BP_SET__BP_TYPES__FK FOREIGN KEY (BP_TYPE,OP_TYPE) REFERENCES BP_TYPES (CODE,OP_TYPE) ENABLE NOVALIDATE;