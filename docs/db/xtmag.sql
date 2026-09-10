--
-- PostgreSQL database dump
--

-- Dumped from database version 12.0
-- Dumped by pg_dump version 12.0

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- Name: xttype; Type: TYPE; Schema: public; Owner: postgres
--

CREATE TYPE public.xttype AS ENUM (
    '实测磁力数据',
    '模型数据',
    '网格数据'
);


ALTER TYPE public.xttype OWNER TO postgres;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- Name: metadata; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.metadata (
    type public.xttype NOT NULL,
    name character varying(255) NOT NULL,
    path text,
    x_min_value numeric,
    x_max_value numeric,
    y_min_value numeric,
    y_max_value numeric
);


ALTER TABLE public.metadata OWNER TO postgres;

--
-- Data for Name: metadata; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.metadata (type, name, path, x_min_value, x_max_value, y_min_value, y_max_value) FROM stdin;
实测磁力数据	line99	/data/1113/lines/M99.txt	17.931	24.32	112.064	112.066
实测磁力数据	line98	/data/1113/lines/M98.txt	15.458	19.992	112.06	112.061
实测磁力数据	line97	/data/1113/lines/M97.txt	12.572	20.129	112.052	112.058
网格数据	out1	/data/testdata/navigation/out.txt	0	53.4987	0	27.4925
网格数据	out3	/data/testdata/out3.dat	0	53.5164	0	27.4897
网格数据	big	/home/greatwall/下载/magnetic_anomaly_data1.txt	0	120	0	120
模型数据	juxie	/home/greatwall/xtmag/referencemap/datafile.TXT	0	2310946	0	590555
\.


--
-- Name: metadata metadata_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.metadata
    ADD CONSTRAINT metadata_pkey PRIMARY KEY (name);


--
-- PostgreSQL database dump complete
--

