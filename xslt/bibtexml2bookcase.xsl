<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/bookcase/"
                xmlns:str="http://exslt.org/strings"
                xmlns:bibtexml="http://bibtexml.sf.net/"
                extension-element-prefixes="str"
                exclude-result-prefixes="bibtexml str"
                version="1.0">

<!--
   ================================================================
   Bookcase XSLT file - used for importing from bibtexml format

   $Id: bibtexml2bookcase.xsl,v 1.7 2003/05/10 19:40:25 robby Exp $

   Copyright (c) 2003 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with XML data files
   from the 'Bookcase' application, which can be found at:
   http://www.periapsis.org/bookcase/
   ================================================================
-->

<xsl:output method="xml" indent="yes"
            encoding="UTF-8"
            doctype-system="bookcase.dtd"
            doctype-public="bookcase"/>

<xsl:strip-space elements="*"/>

<xsl:variable name="current-syntax" select="'3'"/>

<xsl:template match="/">
 <xsl:apply-templates select="bibtexml:file"/>
</xsl:template>

<xsl:template match="bibtexml:file">
 <bookcase version="{$current-syntax}">
 <!-- if title is empty, then Bookcase uses default -->
  <collection title="" unit="book">
  <!-- want to store key (id) values -->
  <!-- type = "1" is BCAttribute::Line -->
   <attribute name="bibtex-id" title="Bibtex ID" type="1" flags="0"/>
   <xsl:apply-templates select="bibtexml:entry/bibtexml:book"/>
  </collection>
 </bookcase>
</xsl:template>

<xsl:template match="bibtexml:book">
 <book>
  <xsl:if test="../@id">
   <bibtex-id>
    <xsl:value-of select="../@id"/>
   </bibtex-id>
  </xsl:if>
  <xsl:apply-templates select="bibtexml:author|bibtexml:title|bibtexml:publisher|bibtexml:year|bibtexml:isbn|bibtexml:lccn|bibtexml:edition|bibtexml:series|bibtexml:number|bibtexml:keywords|bibtexml:languages|bibtexml:pricec|bibtexml:note"/>
 </book>
</xsl:template>

<xsl:template match="bibtexml:title|bibtexml:publisher|bibtexml:isbn|bibtexml:lccn|bibtexml:edition|bibtexml:series|bibtexml:price">
 <xsl:element name="{local-name()}">
<!--  <xsl:value-of select="."/> -->
  <xsl:call-template name="clean-up-entry"/>
 </xsl:element>
</xsl:template>

<xsl:template match="bibtexml:author">
 <authors>
  <xsl:for-each select="str:tokenize(., ';,')">
   <author>
    <xsl:call-template name="clean-up-entry">
     <xsl:with-param name="norm-space" select="true()"/>
    </xsl:call-template>
   </author>
  </xsl:for-each>
 </authors>
</xsl:template>

<xsl:template match="bibtexml:year">
 <cr_years>
  <cr_year>
   <xsl:call-template name="clean-up-entry"/>
  </cr_year>
 </cr_years>
</xsl:template>

<xsl:template match="bibtexml:number">
 <series_num>
  <xsl:call-template name="clean-up-entry"/>
 </series_num>
</xsl:template>

<xsl:template match="bibtexml:keywords">
 <keywords>
  <xsl:for-each select="str:tokenize(., ',')">
   <keyword>
    <xsl:value-of select="normalize-space()"/>
   </keyword>
  </xsl:for-each>
 </keywords>
</xsl:template>

<xsl:template match="bibtexml:languages">
 <languages>
  <xsl:for-each select="str:tokenize(., ',')">
   <language>
    <xsl:call-template name="clean-up-entry">
     <xsl:with-param name="norm-space" select="true()"/>
    </xsl:call-template>
   </language>
  </xsl:for-each>
 </languages>
</xsl:template>

<xsl:template match="bibtexml:note">
 <comments>
  <xsl:value-of select="."/>
 </comments>
</xsl:template>

<!-- remove braces, backslashes, and quotes -->
<xsl:template name="clean-up-entry">
 <!-- should normalize-space be called? -->
 <xsl:param name="norm-space" select="false()"/>
 
 <xsl:variable name="temp">
  <xsl:choose>
   <xsl:when test="$norm-space">
    <xsl:value-of select="normalize-space(.)"/>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="."/>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:variable>
 
 <xsl:value-of select="translate($temp, '{}\&quot;', '')"/>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
