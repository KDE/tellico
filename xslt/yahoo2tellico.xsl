<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:yh="urn:yahoo:srchmm"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing Yahoo! album search data.

   Copyright (C) 2004-2006 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V9.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v9/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="9">
  <collection title="Yahoo! Import" type="4"> <!-- 4 is music -->
   <fields>
    <field name="_default"/>
    <!-- the importer will actually download the image and ignore this field -->
    <field flags="0" title="Yahoo ALbum" category="General" format="4" type="7" name="yahoo"/>
    <field flags="0" title="Image" category="Images" format="4" type="7" name="image"/>
   </fields>
   <xsl:for-each select="yh:ResultSet/yh:Result">
    <xsl:apply-templates select="."/>
   </xsl:for-each>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="yh:Result">
 <entry>
  <yahoo>
   <xsl:value-of select="@id"/>
  </yahoo>

  <image>
   <xsl:value-of select="yh:Thumbnail/yh:Url"/>
  </image>

  <title>
   <xsl:value-of select="yh:Title"/>
  </title>

  <artists>
   <xsl:for-each select="yh:Artist">
    <artist>
     <xsl:value-of select="."/>
    </artist>
   </xsl:for-each>
  </artists>

  <year>
   <xsl:call-template name="year">
    <xsl:with-param name="value" select="yh:ReleaseDate"/>
   </xsl:call-template>
  </year>
  
  <labels>
   <xsl:for-each select="yh:Publisher">
    <label>
     <xsl:value-of select="."/>
    </label>
   </xsl:for-each>
  </labels>
 </entry>
 
</xsl:template>

<xsl:template name="year">
 <xsl:param name="value"/>
 <!-- assume that Yahoo always puts the year first -->
 <xsl:value-of select="substring($value, 0, 5)"/>
</xsl:template>

</xsl:stylesheet>
