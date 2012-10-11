CREATE SCHEMA hours;

SET search_path = hours, pg_catalog;

SET default_tablespace = '';

SET default_with_oids = false;

CREATE TABLE spans (
    id integer NOT NULL,
    start timestamp with time zone DEFAULT now(),
    stop timestamp with time zone DEFAULT now()
);


ALTER TABLE ONLY spans
    ADD CONSTRAINT spans_pkey PRIMARY KEY (id);
