<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from biblioshare.org

   Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="BiblioShare Import" type="2">
   <fields>
    <field name="_default"/>
   </fields>
   <xsl:apply-templates select="BiblioSimple"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="BiblioSimple[Title]">
 <entry>

  <title>
   <xsl:value-of select="Title[1]"/>
  </title>

  <subtitle>
   <xsl:value-of select="Subtitle[1]"/>
  </subtitle>

  <isbn>
   <xsl:choose>
    <xsl:when test="ISBN10">
     <xsl:value-of select="ISBN10"/>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="ISBN13"/>
    </xsl:otherwise>
   </xsl:choose>
  </isbn>

  <authors>
   <xsl:for-each select="Contributor">
    <author>
     <xsl:value-of select="."/>
    </author>
   </xsl:for-each>
  </authors>

  <publisher>
   <xsl:value-of select="Publisher[1]"/>
  </publisher>

  <binding i18n="true">
   <xsl:choose>
    <xsl:when test="contains(Format, 'Hardcover') or contains(Format, 'Hardback')">
     <xsl:text>Hardback</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Format, 'Paperback')">
     <xsl:text>Paperback</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="Format"/>
    </xsl:otherwise>
   </xsl:choose>
  </binding>

  <pub_year>
   <xsl:value-of select="substring(PublicationDate,0,5)"/>
  </pub_year>

 </entry>
</xsl:template>

<xsl:template match="BiblioSimple"/>

</xsl:stylesheet>
