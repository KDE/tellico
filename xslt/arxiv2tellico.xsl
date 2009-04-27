<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:atom="http://www.w3.org/2005/Atom"
                xmlns:arxiv="http://arxiv.org/schemas/atom"
                xmlns:str="http://exslt.org/strings"
                xmlns:exsl="http://exslt.org/common"
                xmlns:c="uri:category"
                exclude-result-prefixes="atom arxiv"
                extension-element-prefixes="str exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from crossref.org

   See http://www.crossref.org/schema/queryResultSchema/crossref_query_output2.0.7.xsd

   Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   ===================================================================
-->

<!-- lookup table for categories -->
<c:categories>
  <c:category id="astro-ph">Astrophysics</c:category>
  <c:category id="cond-mat">Condensed Matter</c:category>
  <c:category id="cond-mat.dis-nn">Disordered Systems and Neural Networks</c:category>
  <c:category id="cond-mat.mtrl-sci">Materials Science</c:category>
  <c:category id="cond-mat.mes-hall">Mesoscopic Systems and Quantum Hall Effect</c:category>
  <c:category id="cond-mat.other">Other</c:category>
  <c:category id="cond-mat.soft">Soft Condensed Matter</c:category>
  <c:category id="cond-mat.stat-mech">Statistical Mechanics</c:category>
  <c:category id="cond-mat.str-el">Strongly Correlated Electrons</c:category>
  <c:category id="cond-mat.supr-con">Superconductivity</c:category>
  <c:category id="gr-qc">General Relativity and Quantum Cosmology</c:category>
  <c:category id="hep-ex">High Energy Physics - Experiment</c:category>
  <c:category id="hep-lat">High Energy Physics - Lattice</c:category>
  <c:category id="hep-ph">High Energy Physics - Phenomenology</c:category>
  <c:category id="hep-th">High Energy Physics - Theory</c:category>
  <c:category id="math-ph">Mathematical Physics</c:category>
  <c:category id="nucl-ex">Nuclear Experiment</c:category>
  <c:category id="nucl-th">Nuclear Theory</c:category>
  <c:category id="physics">Physics</c:category>
  <c:category id="physics.acc-ph">Accelerator Physics</c:category>
  <c:category id="physics.ao-ph">Atmospheric and Oceanic Physics</c:category>
  <c:category id="physics.atom-ph">Atomic Physics</c:category>
  <c:category id="physics.atm-clus">Atomic and Molecular Clusters</c:category>
  <c:category id="physics.bio-ph">Biological Physics</c:category>
  <c:category id="physics.chem-ph">Chemical Physics</c:category>
  <c:category id="physics.class-ph">Classical Physics</c:category>
  <c:category id="physics.comp-ph">Computational Physics</c:category>
  <c:category id="physics.data-an">Data Analysis, Statistics and Probability</c:category>
  <c:category id="physics.flu-dyn">Fluid Dynamics</c:category>
  <c:category id="physics.gen-ph">General Physics</c:category>
  <c:category id="physics.geo-ph">Geophysics</c:category>
  <c:category id="physics.hist-ph">History of Physics</c:category>
  <c:category id="physics.ins-det">Instrumentation and Detectors</c:category>
  <c:category id="physics.med-ph">Medical Physics</c:category>
  <c:category id="physics.optics">Optics</c:category>
  <c:category id="physics.ed-ph">Physics Education</c:category>
  <c:category id="physics.soc-ph">Physics and Society</c:category>
  <c:category id="physics.plasm-ph">Plasma Physics</c:category>
  <c:category id="physics.pop-ph">Popular Physics</c:category>
  <c:category id="physics.space-ph">Space Physics</c:category>
  <c:category id="quant-ph">Quantum Physics</c:category>
  <c:category id="math">Mathematics</c:category>
  <c:category id="math.AG">Algebraic Geometry</c:category>
  <c:category id="math.AT">Algebraic Topology</c:category>
  <c:category id="math.AP">Analysis of PDEs</c:category>
  <c:category id="math.CT">Category Theory</c:category>
  <c:category id="math.CA">Classical Analysis and ODEs</c:category>
  <c:category id="math.CO">Combinatorics</c:category>
  <c:category id="math.AC">Commutative Algebra</c:category>
  <c:category id="math.CV">Complex Variables</c:category>
  <c:category id="math.DG">Differential Geometry</c:category>
  <c:category id="math.DS">Dynamical Systems</c:category>
  <c:category id="math.FA">Functional Analysis</c:category>
  <c:category id="math.GM">General Mathematics</c:category>
  <c:category id="math.GN">General Topology</c:category>
  <c:category id="math.GT">Geometric Topology</c:category>
  <c:category id="math.GR">Group Theory</c:category>
  <c:category id="math.HO">History and Overview</c:category>
  <c:category id="math.IT">Information Theory</c:category>
  <c:category id="math.KT">K-Theory and Homology</c:category>
  <c:category id="math.LO">Logic</c:category>
  <c:category id="math.MP">Mathematical Physics</c:category>
  <c:category id="math.MG">Metric Geometry</c:category>
  <c:category id="math.NT">Number Theory</c:category>
  <c:category id="math.NA">Numerical Analysis</c:category>
  <c:category id="math.OA">Operator Algebras</c:category>
  <c:category id="math.OC">Optimization and Control</c:category>
  <c:category id="math.PR">Probability</c:category>
  <c:category id="math.QA">Quantum Algebra</c:category>
  <c:category id="math.RT">Representation Theory</c:category>
  <c:category id="math.RA">Rings and Algebras</c:category>
  <c:category id="math.SP">Spectral Theory</c:category>
  <c:category id="math.ST">Statistics</c:category>
  <c:category id="math.SG">Symplectic Geometry</c:category>
  <c:category id="nlin">Nonlinear Sciences</c:category>
  <c:category id="nlin.AO">Adaptation and Self-Organizing Systems</c:category>
  <c:category id="nlin.CG">Cellular Automata and Lattice Gases</c:category>
  <c:category id="nlin.CD">Chaotic Dynamics</c:category>
  <c:category id="nlin.SI">Exactly Solvable and Integrable Systems</c:category>
  <c:category id="nlin.PS">Pattern Formation and Solitons</c:category>
  <c:category id="cs.AR">Architecture</c:category>
  <c:category id="cs.AI">Artificial Intelligence</c:category>
  <c:category id="cs.CL">Computation and Language</c:category>
  <c:category id="cs.CC">Computational Complexity</c:category>
  <c:category id="cs.CE">Computational Engineering, Finance, and Science</c:category>
  <c:category id="cs.CG">Computational Geometry</c:category>
  <c:category id="cs.GT">Computer Science and Game Theory</c:category>
  <c:category id="cs.CV">Computer Vision and Pattern Recognition</c:category>
  <c:category id="cs.CY">Computers and Society</c:category>
  <c:category id="cs.CR">Cryptography and Security</c:category>
  <c:category id="cs.DS">Data Structures and Algorithms</c:category>
  <c:category id="cs.DB">Databases</c:category>
  <c:category id="cs.DL">Digital Libraries</c:category>
  <c:category id="cs.DM">Discrete Mathematics</c:category>
  <c:category id="cs.DC">Distributed, Parallel, and Cluster Computing</c:category>
  <c:category id="cs.GL">General Literature</c:category>
  <c:category id="cs.GR">Graphics</c:category>
  <c:category id="cs.HC">Human-Computer Interaction</c:category>
  <c:category id="cs.IR">Information Retrieval</c:category>
  <c:category id="cs.IT">Information Theory</c:category>
  <c:category id="cs.LG">Learning</c:category>
  <c:category id="cs.LO">Logic in Computer Science</c:category>
  <c:category id="cs.MS">Mathematical Software</c:category>
  <c:category id="cs.MA">Multiagent Systems</c:category>
  <c:category id="cs.MM">Multimedia</c:category>
  <c:category id="cs.NI">Networking and Internet Architecture</c:category>
  <c:category id="cs.NE">Neural and Evolutionary Computing</c:category>
  <c:category id="cs.NA">Numerical Analysis</c:category>
  <c:category id="cs.OS">Operating Systems</c:category>
  <c:category id="cs.OH">Other</c:category>
  <c:category id="cs.PF">Performance</c:category>
  <c:category id="cs.PL">Programming Languages</c:category>
  <c:category id="cs.RO">Robotics</c:category>
  <c:category id="cs.SE">Software Engineering</c:category>
  <c:category id="cs.SD">Sound</c:category>
  <c:category id="cs.SC">Symbolic Computation</c:category>
  <c:category id="q-bio">Quantitative Biology</c:category>
  <c:category id="q-bio.BM">Biomolecules</c:category>
  <c:category id="q-bio.CB">Cell Behavior</c:category>
  <c:category id="q-bio.GN">Genomics</c:category>
  <c:category id="q-bio.MN">Molecular Networks</c:category>
  <c:category id="q-bio.NC">Neurons and Cognition</c:category>
  <c:category id="q-bio.OT">Other</c:category>
  <c:category id="q-bio.PE">Populations and Evolution</c:category>
  <c:category id="q-bio.QM">Quantitative Methods</c:category>
  <c:category id="q-bio.SC">Subcellular Processes</c:category>
  <c:category id="q-bio.TO">Tissues and Organs</c:category>
  <c:category id="stat">Statistics</c:category>
  <c:category id="stat.AP">Applications</c:category>
  <c:category id="stat.CO">Computation</c:category>
  <c:category id="stat.ML">Machine Learning</c:category>
  <c:category id="stat.ME">Methodology</c:category>
  <c:category id="stat.TH">Theory</c:category>
</c:categories>
<xsl:key name="categories" match="c:category" use="@id"/>
<xsl:variable name="categories-top" select="document('')/*/c:categories"/>

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V10.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v10/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="10">
  <collection title="Arxiv Search" type="5">
   <fields>
    <field name="_default"/>
     <field flags="0" title="arXiv ID" category="Publishing" format="4" type="1" name="arxiv" description="arXiv ID"/>
   </fields>
   <xsl:apply-templates select="atom:feed/atom:entry"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="atom:entry">
 <entry>

  <title>
   <xsl:value-of select="normalize-space(atom:title)"/>
  </title>

  <entry-type>
   <xsl:text>article</xsl:text>
  </entry-type>

  <authors>
   <xsl:for-each select="atom:author">
    <author>
     <xsl:value-of select="normalize-space(atom:name)"/>
    </author>
   </xsl:for-each>
  </authors>

  <abstract>
   <xsl:value-of select="normalize-space(atom:summary)"/>
  </abstract>

  <note>
   <xsl:value-of select="normalize-space(arxiv:comment)"/>
  </note>

  <keywords>
   <xsl:for-each select="atom:category">
    <keyword>
     <xsl:apply-templates select="$categories-top">
      <xsl:with-param name="cat" select="@term"/>
     </xsl:apply-templates>
    </keyword>
   </xsl:for-each>
  </keywords>

  <url>
   <xsl:choose>
    <xsl:when test="atom:link[@title='pdf']">
     <xsl:value-of select="atom:link[@title='pdf']/@href"/>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="atom:link[1]/@href"/>
    </xsl:otherwise>
   </xsl:choose>
  </url>

  <doi>
   <xsl:value-of select="arxiv:doi"/>
  </doi>

  <xsl:call-template name="journal_ref">
   <xsl:with-param name="value" select="arxiv:journal_ref"/>
  </xsl:call-template>

  <arxiv>
   <!-- remove http://arxiv.org/abs/ from beginning -->
   <xsl:value-of select="substring(atom:id, string-length('http://arxiv.org/abs/')+1)"/>
  </arxiv>

 </entry>

</xsl:template>

<xsl:template name="journal_ref">
 <xsl:param name="value"/>

 <journal>
  <xsl:value-of select="substring-before($value, '(')"/>
 </journal>

 <xsl:variable name="after1" select="substring-after($value, '(')"/>
 <xsl:variable name="year" select="substring-before($after1, ')')"/>
 <year>
  <xsl:value-of select="normalize-space($year)"/>
 </year>

 <xsl:variable name="after2" select="substring-after($after1, ')')"/>
 <pages>
  <xsl:value-of select="normalize-space($after2)"/>
 </pages>
</xsl:template>

<xsl:template match="c:categories">
 <xsl:param name="cat"/>
 <xsl:variable name="c" select="key('categories', $cat)"/>
 <xsl:if test="$c">
  <xsl:value-of select="$c"/>
 </xsl:if>
 <xsl:if test="not($c)">
  <xsl:value-of select="$cat"/>
 </xsl:if>
</xsl:template>

</xsl:stylesheet>
