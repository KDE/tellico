<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:bibtex="http://bibtexml.sf.net/"
                exclude-result-prefixes="bibtex"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing bibtexml, bibtexml.sf.net

   Copyright (C) 2018 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<!-- disable default behavior to print all text -->
<xsl:template match="text()|@*"></xsl:template>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="BibTeXML Import" type="5">
   <fields>
    <field name="_default"/>
    <!-- match pubmed2tellico -->
    <!-- institution is not in the default list -->
    <field flags="6" title="Institution" category="Institution" format="0" type="1" name="institution" i18n="true">
     <prop name="bibtex">institution</prop>
    </field>
    <xsl:if test=".//bibtex:subtitle">
     <field flags="0" title="Subtitle" category="General" format="4" type="1" name="subtitle" i18n="true">
      <prop name="bibtex">subtitle</prop>
     </field>
    </xsl:if>
   </fields>
   <xsl:apply-templates select=".//bibtex:entry"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="bibtex:entry">
 <entry>
  <bibtex-key><xsl:value-of select="@id"/></bibtex-key>
  <xsl:apply-templates/>
 </entry>
</xsl:template>

<xsl:template match="bibtex:article|
                     bibtex:book|
                     bibtex:booklet|
                     bibtex:manual|
                     bibtex:techreport|
                     bibtex:mastersthesis|
                     bibtex:phdthesis|
                     bibtex:inbook|
                     bibtex:incollection|
                     bibtex:inproceedings|
                     bibtex:proceedings|
                     bibtex:conference|
                     bibtex:misc">
 <entry-type>
  <xsl:value-of select="local-name()"/>
 </entry-type>
 <xsl:apply-templates/>
</xsl:template>

<xsl:template match="bibtex:titlelist">
 <xsl:apply-templates select="bibtex:title[1]"/>
</xsl:template>

<xsl:template match="bibtex:title">
 <title>
  <xsl:choose>
   <xsl:when test="bibtex:title">
    <xsl:value-of select="normalize-space(bibtex:title)"/>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="normalize-space(.)"/>
   </xsl:otherwise>
  </xsl:choose>
 </title>
 <xsl:apply-templates select="bibtex:subtitle"/>
</xsl:template>

<xsl:template match="bibtex:authorlist">
 <authors>
  <xsl:apply-templates select="bibtex:author"/>
 </authors>
</xsl:template>

<xsl:template match="bibtex:author">
 <xsl:choose>
  <xsl:when test="bibtex:person">
   <authors>
    <xsl:for-each select="bibtex:person">
     <author>
      <xsl:call-template name="person-name">
       <xsl:with-param name="p" select="."/>
      </xsl:call-template>
     </author>
    </xsl:for-each>
   </authors>
  </xsl:when>
  <xsl:when test="not(child::*)">
   <author>
    <xsl:value-of select="text()"/>
   </author>
  </xsl:when>
  <xsl:when test="not(bibtex:person)">
   <author>
    <xsl:apply-templates/>
   </author>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="bibtex:editorlist">
 <editors>
  <xsl:apply-templates select="bibtex:editor"/>
 </editors>
</xsl:template>

<xsl:template match="bibtex:keywords">
 <keywords>
  <xsl:apply-templates select="bibtex:keyword"/>
 </keywords>
</xsl:template>

<xsl:template match="bibtex:editor">
 <xsl:choose>
  <xsl:when test="bibtex:person">
   <editors>
    <xsl:for-each select="bibtex:person">
     <editor>
      <xsl:call-template name="person-name">
       <xsl:with-param name="p" select="."/>
      </xsl:call-template>
     </editor>
    </xsl:for-each>
   </editors>
  </xsl:when>
  <xsl:when test="not(child::*)">
   <editor>
    <xsl:value-of select="text()"/>
   </editor>
  </xsl:when>
  <xsl:when test="not(bibtex:person)">
   <editor>
    <xsl:apply-templates/>
   </editor>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="bibtex:chapter">
 <chapter>
  <xsl:choose>
   <xsl:when test="bibtex:title">
    <xsl:value-of select="normalize-space(bibtex:title)"/>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="normalize-space(.)"/>
   </xsl:otherwise>
  </xsl:choose>
 </chapter>
</xsl:template>

<!-- for every other element, the tellico name is identical -->
<xsl:template match="*">
 <xsl:element name="{local-name()}">
  <xsl:value-of select="normalize-space(.)"/>
 </xsl:element>
</xsl:template>

<xsl:template name="person-name">
 <!-- the person element -->
 <xsl:param name="p"/>
 <xsl:choose>
  <!-- when there are no children elements, just print the text -->
  <xsl:when test="not(child::*)">
   <xsl:value-of select="text()"/>
  </xsl:when>
  <xsl:otherwise>
   <xsl:variable name="s">
    <xsl:value-of select="concat($p/bibtex:initials, ' ',
                                 $p/bibtex:first, ' ',
                                 $p/bibtex:middle, ' ',
                                 $p/bibtex:prelast, ' ',
                                 $p/bibtex:last, ' ',
                                 $p/bibtex:lineage)"/>
   </xsl:variable>
   <xsl:value-of select="normalize-space($s)"/>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

</xsl:stylesheet>
