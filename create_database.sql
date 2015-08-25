DROP TABLE IF EXISTS introns;
DROP TABLE IF EXISTS coding_exons;
DROP TABLE IF EXISTS isoforms;
DROP TABLE IF EXISTS genes;
DROP TABLE IF EXISTS sequences;
DROP TABLE IF EXISTS organisms;
DROP TABLE IF EXISTS chromosomes;
DROP TABLE IF EXISTS intron_types;
DROP TABLE IF EXISTS tax_groups2;
DROP TABLE IF EXISTS tax_groups1;
DROP TABLE IF EXISTS tax_kingdoms;
DROP TABLE IF EXISTS orthologous_groups;


/* STATIC TABLE intron_types */
CREATE TABLE intron_types(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    representation VARCHAR(5) NOT NULL UNIQUE
);

/* GROUP 0 */
INSERT INTO intron_types(representation) VALUES ('00#00');
INSERT INTO intron_types(representation) VALUES ('00#01');
INSERT INTO intron_types(representation) VALUES ('00#02');

INSERT INTO intron_types(representation) VALUES ('01#10');
INSERT INTO intron_types(representation) VALUES ('01#11');
INSERT INTO intron_types(representation) VALUES ('01#12');

INSERT INTO intron_types(representation) VALUES ('02#20');
INSERT INTO intron_types(representation) VALUES ('02#21');
INSERT INTO intron_types(representation) VALUES ('02#22');

/* GROUP 1 */
INSERT INTO intron_types(representation) VALUES ('10#00');
INSERT INTO intron_types(representation) VALUES ('10#01');
INSERT INTO intron_types(representation) VALUES ('10#02');

INSERT INTO intron_types(representation) VALUES ('11#10');
INSERT INTO intron_types(representation) VALUES ('11#11');
INSERT INTO intron_types(representation) VALUES ('11#12');

INSERT INTO intron_types(representation) VALUES ('12#20');
INSERT INTO intron_types(representation) VALUES ('12#21');
INSERT INTO intron_types(representation) VALUES ('12#22');

/* GROUP 2 */
INSERT INTO intron_types(representation) VALUES ('20#00');
INSERT INTO intron_types(representation) VALUES ('20#01');
INSERT INTO intron_types(representation) VALUES ('20#02');

INSERT INTO intron_types(representation) VALUES ('21#10');
INSERT INTO intron_types(representation) VALUES ('21#11');
INSERT INTO intron_types(representation) VALUES ('21#12');

INSERT INTO intron_types(representation) VALUES ('22#20');
INSERT INTO intron_types(representation) VALUES ('22#21');
INSERT INTO intron_types(representation) VALUES ('22#22');


/* SUPPLEMENTARY TABLES */

CREATE TABLE tax_kingdoms(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(30) UNIQUE NOT NULL
);

CREATE TABLE tax_groups1(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    id_tax_kingdoms INT NOT NULL,
    name VARCHAR(30) UNIQUE NOT NULL,
    typee VARCHAR(500)
);

CREATE TABLE tax_groups2(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    id_tax_groups1 INT NOT NULL,
    name VARCHAR(30) UNIQUE NOT NULL,
    typee VARCHAR(500)
);

/* OTHER TABLES */

CREATE TABLE orthologous_groups(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(30),
    fullName VARCHAR(100)
);


/* SPECIES TABLES */

CREATE TABLE organisms(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(200) NOT NULL,
    ref_seq_assembly_id VARCHAR(20),
    annotation_release VARCHAR(200),
    annotation_date DATE,
    taxonomy_xref VARCHAR(50),
    taxonomy_list VARCHAR(500),
    id_tax_groups2 INT,
    real_chromosome_count INT DEFAULT 0,
    db_chromosome_count INT DEFAULT 0,
    real_mitochondria BOOLEAN DEFAULT 0,
    db_mitochondria BOOLEAN DEFAULT 0,
    unknown_sequences_count INT DEFAULT 0,
    total_sequences_length BIGINT DEFAULT 0,
    b_genes_count INT DEFAULT 0,
    r_genes_count INT DEFAULT 0,
    cds_count INT DEFAULT 0,
    rna_count INT DEFAULT 0,
    unknown_prot_genes_count INT DEFAULT 0,
    unknown_prot_cds_count INT DEFAULT 0,
    exons_count INT DEFAULT 0,
    introns_count INT DEFAULT 0
);

CREATE TABLE chromosomes(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    id_organisms INT NOT NULL,
    name VARCHAR(20),
    lengthh int
);

/* GENES TABLES */

CREATE TABLE sequences(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    source_file_name VARCHAR(50),
    ref_seq_id VARCHAR(20),
    description TEXT,
    lengthh INT NOT NULL DEFAULT 0,
    id_organisms INT NOT NULL,
    id_chromosomes INT,
    origin_file_name VARCHAR(50)
);


CREATE TABLE genes(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    id_sequences INT NOT NULL,
    id_orthologous_groups INT,
    name VARCHAR(20),
    backward_chain BOOLEAN DEFAULT 0,
    protein_but_not_rna BOOLEAN,
    pseudo_gene BOOLEAN,
    startt INT,
    endd INT,
    start_code INT,
    end_code INT,
    max_introns_count INT DEFAULT 0
);


create TABLE isoforms(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    id_genes INT NOT NULL,
    id_sequences INT NOT NULL,
    protein_xref VARCHAR(20),
    protein_name VARCHAR(100),
    product VARCHAR(250),
    cds_start INT,
    cds_end INT,
    mrna_start INT,
    mrna_end INT,
    mrna_length INT,
    exons_cds_count INT,
    exons_mrna_count INT,
    exons_length INT,
    start_codon VARCHAR(3),
    end_codon VARCHAR(3),
    maximum_by_introns BOOLEAN,

    error_in_length BOOLEAN NOT NULL DEFAULT 0,
    error_in_start_codon BOOLEAN NOT NULL DEFAULT 0,
    error_in_end_codon BOOLEAN NOT NULL DEFAULT 0,
    error_in_intron BOOLEAN NOT NULL DEFAULT 0,
    error_in_coding_exon BOOLEAN NOT NULL DEFAULT 0,
    error_main BOOLEAN NOT NULL DEFAULT 0,
    error_comment TEXT
);

create TABLE coding_exons(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    id_isoforms INT NOT NULL,
    id_genes INT NOT NULL,
    id_sequences INT NOT NULL,

    startt INT NOT NULL,
    endd INT NOT NULL,
    lengthh INT,
    typee SMALLINT NOT NULL DEFAULT 4 /* = Unknown */,
    start_phase SMALLINT,
    end_phase SMALLINT,
    length_phase SMALLINT,
    indexx INT,
    rev_index INT,
    start_codon VARCHAR(3),
    end_codon VARCHAR(3),

    prev_intron INT DEFAULT 0,
    next_intron INT DEFAULT 0,

    error_in_pseudo_flag BOOLEAN NOT NULL DEFAULT 0,
    error_n_in_sequence BOOLEAN NOT NULL DEFAULT 0
);

create TABLE introns(
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    id_isoforms INT NOT NULL,
    id_genes INT NOT NULL,
    id_sequences INT NOT NULL,

    prev_exon INT NOT NULL,
    next_exon INT NOT NULL,
    id_intron_types INT,

    start_dinucleotide VARCHAR(2),
    end_dinucleotide VARCHAR(2),

    startt INT NOT NULL,
    endd INT NOT NULL,
    lengthh INT,
    indexx INT,
    rev_index INT,
    length_phase SMALLINT,
    phase SMALLINT,

    error_start_dinucleotide BOOLEAN NOT NULL DEFAULT 0,
    error_end_dinucleotide BOOLEAN NOT NULL DEFAULT 0,
    error_main BOOLEAN NOT NULL DEFAULT 0,

    warning_n_in_sequence BOOLEAN NOT NULL DEFAULT 0
);

ALTER TABLE  introns AUTO_INCREMENT = 1;
ALTER TABLE  coding_exons AUTO_INCREMENT = 1;
ALTER TABLE  isoforms AUTO_INCREMENT = 1;
ALTER TABLE  genes AUTO_INCREMENT = 1;
ALTER TABLE  sequences AUTO_INCREMENT = 1;
ALTER TABLE  organisms AUTO_INCREMENT = 1;
ALTER TABLE  chromosomes AUTO_INCREMENT = 1;
ALTER TABLE  intron_types AUTO_INCREMENT = 1;
ALTER TABLE  tax_groups2 AUTO_INCREMENT = 1;
ALTER TABLE  tax_groups1 AUTO_INCREMENT = 1;
ALTER TABLE  tax_kingdoms AUTO_INCREMENT = 1;
ALTER TABLE  orthologous_groups AUTO_INCREMENT = 1;
