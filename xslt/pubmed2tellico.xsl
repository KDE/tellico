<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:m="uri:months"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing PubMed data.

   Copyright (C) 2005 Michaël Zugaro <michael.zugaro@college-de-france.fr>
                 2005 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

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
            doctype-public="-//Robby Stephenson/DTD Bookcase V8.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v8/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="8">
  <collection title="PubMed Import" type="5">
   <fields>
    <field name="_default"/>
    <field flags="0" title="Institution" category="Institution" format="4" type="2" name="institution" i18n="true">
     <prop name="bibtex" >institution</prop>
    </field>
    <field flags="0" title="Abstract" category="Abstract" format="4" type="2" name="abstract" i18n="true">
     <prop name="bibtex" >abstract</prop>
    </field>
   </fields>
   <xsl:apply-templates select="/PubmedArticleSet/PubmedArticle/MedlineCitation"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="MedlineCitation">
 <entry>
  <entry-type>
   <xsl:text>article</xsl:text>
  </entry-type>

  <volume>
   <xsl:value-of select="Article/Journal/JournalIssue/Volume"/>  
  </volume>
  
  <number>
   <xsl:value-of select="Article/Journal/JournalIssue/Issue"/>  
  </number>
  
  <xsl:variable name="year">
   <xsl:variable name="year1" select="Article/Journal/JournalIssue/PubDate/Year"/>
   <xsl:variable name="year2" select="Article/Journal/JournalIssue/PubDate/MedlineDate"/>
   <xsl:choose>
    <xsl:when test="string-length($year1) != 0">
     <xsl:value-of select="$year1"/>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="substring($year2,1,4)"/>
    </xsl:otherwise>
   </xsl:choose>
  </xsl:variable>
  
  <year>
   <xsl:value-of select="$year"/>  
  </year>
  
  <month>
   <xsl:apply-templates select="$months-top">
    <xsl:with-param name="month-id" select="Article/Journal/JournalIssue/PubDate/Month"/>
   </xsl:apply-templates>
  </month>
  
  <title>
   <xsl:value-of select="Article/ArticleTitle"/>  
  </title>
  
  <pages>
   <xsl:value-of select="Article/Pagination/MedlinePgn"/>  
  </pages>
  
  <abstract>
   <xsl:value-of select="Article/Abstract/AbstractText"/>  
  </abstract>
  
  <institution>
   <xsl:value-of select="Article/Affiliation"/>  
  </institution>
  
  <authors>
   <xsl:for-each select="Article/AuthorList/Author">
    <author>
     <xsl:call-template name="TransformInitials">
      <xsl:with-param name="initials" select="Initials"/>
     </xsl:call-template>
     <xsl:value-of select="LastName"/>
    </author>   
   </xsl:for-each>
  </authors>
  
  <journal>
   <xsl:value-of select="MedlineJournalInfo/MedlineTA"/>  
  </journal>
  
 </entry>
</xsl:template>

<xsl:template name="TransformInitials">
 <xsl:param name="initials"/>
 <xsl:variable name="n" select="string-length($initials)"/>
 <xsl:if test="$n &gt;= 1">
  <xsl:value-of select="substring($initials,1,1)"/>
  <xsl:text>. </xsl:text>
  <xsl:call-template name="TransformInitials">
   <xsl:with-param name="initials" select="substring($initials,2,$n - 1)"/>
  </xsl:call-template>
 </xsl:if>
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
