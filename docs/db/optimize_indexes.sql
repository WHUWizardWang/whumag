-- Index maintenance for the EMAG2 / MAMEA reference-grid tables.
--
-- emag2_v3_20170530 and mamea are not part of this repo's schema (they were
-- populated directly in Postgres, outside of xtmag.sql), so their current
-- index status is unknown from the code alone. Every point query in
-- src/MagAno/maganoquery.h filters by index_ij; emag2_v3_20170530 in
-- particular can hold up to ~58M rows (a 2-arcminute global grid), so a
-- query against it without an index on index_ij is a full table scan per
-- batch instead of an index lookup.
--
-- Safe to run against an existing database: CREATE INDEX IF NOT EXISTS is a
-- no-op if the index (or an equivalent one) already exists.

CREATE INDEX IF NOT EXISTS idx_emag2_v3_20170530_index_ij
    ON emag2_v3_20170530 (index_ij);

CREATE INDEX IF NOT EXISTS idx_mamea_index_ij
    ON mamea (index_ij);

-- Refresh planner statistics so it actually picks up the new indexes.
ANALYZE emag2_v3_20170530;
ANALYZE mamea;
