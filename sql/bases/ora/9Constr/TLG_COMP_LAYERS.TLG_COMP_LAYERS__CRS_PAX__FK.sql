ALTER TABLE TLG_COMP_LAYERS ADD CONSTRAINT TLG_COMP_LAYERS__CRS_PAX__FK FOREIGN KEY (CRS_PAX_ID) REFERENCES CRS_PAX (PAX_ID) ENABLE NOVALIDATE;