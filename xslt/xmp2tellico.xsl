<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
                xmlns:dc="http://purl.org/dc/elements/1.1/"
                xmlns:jabref="http://jabref.sourceforge.net/bibteXMP/"
                xmlns:str="http://exslt.org/strings"
                xmlns:m="uri:months"
                extension-element-prefixes="str"
                exclude-result-prefixes="rdf dc jabref"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for converting XMP data

   Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<!-- lookup table for months -->
<m:months>
 <m:month id="Jan">1</m:month>
 <m:month id="Feb">2</m:month>
 <m:month id="Mar">3</m:month>
 <m:month id="Apr">4</m:month>
 <m:month id="May">5</m:month>
 <m:month id="Jun">6</m:month>
 <m:month id="Jul">7</m:month>
 <m:month id="Aug">8</m:month>
 <m:month id="Sep">9</m:month>
 <m:month id="Oct">10</m:month>
 <m:month id="Nov">11</m:month>
 <m:month id="Dec">12</m:month>
 <!-- months in other languages could be added easily -->
</m:months>
<xsl:key name="months" match="m:month" use="@id"/>
<xsl:variable name="months-top" select="document('')/*/m:months"/>

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<!-- for lower-casing -->
<xsl:variable name="lcletters">abcdefghijklmnopqrstuvwxyz</xsl:variable>
<xsl:variable name="ucletters">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>

<!-- disable default behavior -->
<xsl:template match="text()|@*"></xsl:template>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="XMP Conversion" type="5">
   <fields>
    <field name="_default"/>
   </fields>
   <xsl:apply-templates select=".//rdf:RDF"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="rdf:RDF">
 <entry>
  <xsl:apply-templates/>
 </entry>
</xsl:template>

<xsl:template match="dc:title">
 <title><xsl:value-of select="normalize-space(.)"/></title>
</xsl:template>

<xsl:template match="dc:type">
 <entry-type><xsl:value-of select="normalize-space(translate(., $ucletters, $lcletters))"/></entry-type>
</xsl:template>

<xsl:template match="dc:creator">
 <authors>
  <xsl:call-template name="multiple-values">
   <xsl:with-param name="value" select="."/>
   <xsl:with-param name="field" select="'author'"/>
  </xsl:call-template>
 </authors>
</xsl:template>

<xsl:template match="dc:subject">
 <keywords>
  <xsl:call-template name="multiple-values">
   <xsl:with-param name="value" select="."/>
   <xsl:with-param name="field" select="'keyword'"/>
  </xsl:call-template>
 </keywords>
</xsl:template>

<xsl:template match="dc:date">
 <xsl:variable name="tokens" select="str:tokenize(., '-')"/>
 <year><xsl:value-of select="normalize-space($tokens[1])"/></year>
 <month><xsl:value-of select="normalize-space($tokens[2])"/></month>
</xsl:template>

<xsl:template match="dc:identifier">
 <!-- assume DOI requires a period and a slash -->
 <xsl:if test="not(//jabref:doi) and
               (contains(.,'.') and contains(.,'/'))">
  <doi><xsl:value-of select="normalize-space(.)"/></doi>
 </xsl:if>
</xsl:template>

<xsl:template match="jabref:year">
 <xsl:if test="not(//dc:date)">
  <year><xsl:value-of select="normalize-space(.)"/></year>
 </xsl:if>
</xsl:template>

<xsl:template match="jabref:month">
 <xsl:if test="not(//dc:date)">
  <month>
   <xsl:apply-templates select="$months-top">
    <xsl:with-param name="month-id" select="normalize-space(.)"/>
   </xsl:apply-templates>
  </month>
 </xsl:if>
</xsl:template>

<xsl:template match="jabref:jabrefkey">
 <bibtex-key><xsl:value-of select="normalize-space(.)"/></bibtex-key>
</xsl:template>

<xsl:template match="jabref:journal">
 <journal><xsl:value-of select="normalize-space(.)"/></journal>
</xsl:template>

<xsl:template match="jabref:url">
 <url><xsl:value-of select="normalize-space(.)"/></url>
</xsl:template>

<xsl:template match="jabref:doi">
 <doi><xsl:value-of select="normalize-space(.)"/></doi>
</xsl:template>

<xsl:template name="multiple-values">
 <xsl:param name="value"/>
 <xsl:param name="field"/>
 <xsl:choose>
  <xsl:when test="$value/rdf:Seq">
   <xsl:for-each select="$value/rdf:Seq/rdf:li">
    <xsl:element name="{$field}">
     <xsl:value-of select="normalize-space(.)"/>
    </xsl:element>
   </xsl:for-each>
  </xsl:when>
  <xsl:otherwise>
   <xsl:for-each select="$value/*">
    <xsl:element name="{$field}">
     <xsl:value-of select="normalize-space(.)"/>
    </xsl:element>
   </xsl:for-each>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template match="m:months">
 <xsl:param name="month-id"/>
 <xsl:variable name="m" select="key('months', $month-id)"/>
 <xsl:if test="$m">
  <xsl:value-of select="$m"/>
 </xsl:if>
 <xsl:if test="not($m)">
  <xsl:value-of select="$month-id"/>
 </xsl:if>
</xsl:template>

</xsl:stylesheet>
