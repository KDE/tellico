<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:dc="http://purl.org/dc/elements/1.1/"
                xmlns:pam="http://prismstandard.org/namespaces/pam/2.0/"
                xmlns:prism="http://prismstandard.org/namespaces/basic/2.0/"
                xmlns:xhtml="http://www.w3.org/1999/xhtml"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing PAM data from Springer Link

   Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Springer Search" type="5">
   <fields>
    <field name="_default"/>
   </fields>
   <xsl:apply-templates select="response/records/pam:message"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="pam:message">
 <entry>

  <xsl:apply-templates select="xhtml:head/pam:article"/>

  <abstract>
   <xsl:value-of select="normalize-space(xhtml:body/p[1])"/>
  </abstract>

 </entry>

</xsl:template>

<xsl:template match="pam:article">
 <title>
  <xsl:value-of select="normalize-space(dc:title)"/>
 </title>

 <authors>
  <xsl:for-each select="dc:creator">
   <author>
    <xsl:value-of select="normalize-space(.)"/>
   </author>
  </xsl:for-each>
 </authors>

 <publisher>
  <xsl:value-of select="normalize-space(dc:publisher)"/>
 </publisher>

 <year>
  <xsl:value-of select="substring(prism:publicationDate, 1, 4)"/>
 </year>

 <journal>
  <xsl:value-of select="prism:publicationName"/>
 </journal>

 <!-- assume journalId presence implies article -->
 <xsl:if test="journalId">
  <entry-type>
   <xsl:text>article</xsl:text>
  </entry-type>
 </xsl:if>

 <!-- assume isbn presence implies book -->
 <xsl:if test="(printIsbn|prism:isbn|electronicIsbn)">
  <entry-type>
   <xsl:text>book</xsl:text>
  </entry-type>
 </xsl:if>

 <isbn>
  <!-- prefer print ISBN first -->
  <xsl:value-of select="(printIsbn|prism:isbn|electronicIsbn)[1]"/>
 </isbn>

 <doi>
  <xsl:value-of select="prism:doi"/>
 </doi>

 <volume>
  <xsl:value-of select="prism:volume"/>
 </volume>

 <number>
  <xsl:value-of select="prism:number"/>
 </number>

</xsl:template>

</xsl:stylesheet>
