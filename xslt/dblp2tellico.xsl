<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from dblp.org

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
  <collection title="DBLP Import" type="5">
   <fields>
    <field name="_default"/>
   </fields>
   <xsl:apply-templates select="result/hits/hit/info"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="info">
 <entry>

  <title>
   <xsl:value-of select="title"/>
  </title>

  <booktitle>
   <xsl:value-of select="booktitle"/>
  </booktitle>

  <entry-type>
   <xsl:value-of select="type"/>
  </entry-type>

  <year>
   <xsl:value-of select="year"/>
  </year>

  <pages>
   <xsl:value-of select="venue/@pages"/>
  </pages>

  <journal>
   <xsl:value-of select="venue/@journal"/>
  </journal>

  <volume>
   <xsl:value-of select="venue/@volume"/>
  </volume>

  <number>
   <xsl:value-of select="venue/@number"/>
  </number>

  <booktitle>
   <xsl:value-of select="venue/@conference"/>
  </booktitle>

  <publishers>
   <publisher>
    <xsl:value-of select="venue/@publisher"/>
   </publisher>
  </publishers>

  <url>
   <xsl:value-of select="title/@ee"/>
  </url>

  <authors>
   <xsl:for-each select="authors/author">
    <author>
     <xsl:value-of select="."/>
    </author>
   </xsl:for-each>
  </authors>

  <bibtex-key>
   <xsl:value-of select="substring-after(venue/@url, '#')"/>
  </bibtex-key>

  <!-- just assume DOI starts with 10. -->
  <xsl:variable name="doi" select="substring-after(title/@ee, '/10.')"/>
  <doi>
   <xsl:if test="$doi">
    <xsl:text>10.</xsl:text>
    <xsl:value-of select="$doi"/>
   </xsl:if>
  </doi>

 </entry>

</xsl:template>

</xsl:stylesheet>
