<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from isfdb.org

   Copyright (C) 2025 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Import" type="2">
   <fields>
    <field name="_default"/>
    <field flags="0" title="ISFDB Link" category="General" format="4" type="7" name="isfdb" i18n="true"/>
   </fields>
   <xsl:apply-templates select="ISFDB/Publications/Publication"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="Publication">
 <entry>
  <title>
   <xsl:value-of select="Title"/>
  </title>

  <isbn>
   <xsl:value-of select="Isbn"/>
  </isbn>

  <publisher>
   <xsl:value-of select="Publisher"/>
  </publisher>

  <pub_year>
   <xsl:value-of select="substring(Year, 1, 4)"/>
  </pub_year>

  <authors>
   <xsl:for-each select="Authors/Author">
    <author>
     <xsl:value-of select="."/>
    </author>
   </xsl:for-each>
  </authors>

  <binding i18n="true">
   <xsl:choose>
    <xsl:when test="Binding='hc'">
     <xsl:text>Hardback</xsl:text>
    </xsl:when>
    <xsl:when test="Binding='pb'">
     <xsl:text>Paperback</xsl:text>
    </xsl:when>
    <xsl:when test="Binding='tp'">
     <xsl:text>Trade Paperback</xsl:text>
    </xsl:when>
    <xsl:when test="Binding='ebook'">
     <xsl:text>E-Book</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="Binding"/>
    </xsl:otherwise>
   </xsl:choose>
  </binding>

  <cover>
   <xsl:value-of select="Image"/>
  </cover>

  <comments>
   <xsl:value-of select="Note"/>
  </comments>

  <pages>
   <xsl:value-of select="Pages"/>
  </pages>

  <series>
   <xsl:value-of select="PubSeries"/>
  </series>
  
  <series_num>
   <xsl:value-of select="PubSeriesNum"/>
  </series_num>

  <lccn>
   <xsl:value-of select="External_IDs/External_ID[IDtype='10']/IDvalue"/>
  </lccn>

  <isfdb>
   <xsl:value-of select="concat('https://www.isfdb.org/cgi-bin/pl.cgi?', Record)"/>
  </isfdb>

 </entry>
</xsl:template>

</xsl:stylesheet>
