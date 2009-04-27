<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:cr="http://www.crossref.org/qrschema/2.0"
                xmlns:str="http://exslt.org/strings"
                xmlns:exsl="http://exslt.org/common"
                exclude-result-prefixes="cr"
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

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V10.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v10/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="10">
  <collection title="CrossRef Import" type="5">
   <fields>
    <field name="_default"/>
    
    <xsl:if test=".//cr:issn">
     <field flags="0" title="ISSN" category="Publishing" format="4" type="1" name="issn" i18n="true"/>
    </xsl:if>
    
   </fields>
   <xsl:apply-templates select="cr:crossref_result/cr:query_result/cr:body/cr:query"/>
  </collection>
 </tellico>
</xsl:template>

<!-- ignore unresolved queries -->
<xsl:template match="cr:query[@status='unresolved']">
</xsl:template>

<xsl:template match="cr:query">
 <entry>
  
  <title>
   <xsl:value-of select="cr:article_title"/>
  </title>
  
  <booktitle>
   <xsl:value-of select="cr:volume_title"/>
  </booktitle>
  
  <entry-type>
   <xsl:choose>
    <xsl:when test="cr:doi/@type = 'conference_paper'">
     <xsl:text>inproceedings</xsl:text>
    </xsl:when>
    <xsl:when test="cr:doi/@type = 'book_title'">
     <xsl:text>book</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:text>article</xsl:text>
    </xsl:otherwise>
   </xsl:choose>
  </entry-type>
  
  <year>
   <xsl:value-of select="cr:year"/>
  </year>
  
  <doi>
   <xsl:value-of select="cr:doi"/>
  </doi>
  
  <pages>
   <xsl:value-of select="cr:first_page"/>
  </pages>
  
  <journal>
   <xsl:value-of select="cr:journal_title"/>
  </journal>
  
  <volume>
   <xsl:value-of select="cr:volume"/>
  </volume>
  
  <isbn>
   <xsl:value-of select="cr:isbn"/>
  </isbn>
  
  <issn>
   <xsl:choose>
    <xsl:when test="cr:issn[@type='print']">
     <xsl:value-of select="cr:issn[@type='print']"/>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="cr:issn[1]"/>
    </xsl:otherwise>
   </xsl:choose>
  </issn>
  
  <authors>
   <xsl:for-each select="cr:author">
    <author>
     <xsl:value-of select="."/>
    </author>
   </xsl:for-each>
  </authors>
  
  <series>
   <xsl:value-of select="cr:series_title"/>
  </series>
  
 </entry>
 
</xsl:template>

</xsl:stylesheet>
