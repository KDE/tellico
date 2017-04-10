<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:exsl="http://exslt.org/common"
                xmlns:dbc="http://oss.dbc.dk/ns/opensearch"
                xmlns:dc="http://purl.org/dc/elements/1.1/"
                xmlns:dcterms="http://purl.org/dc/terms/"
                xmlns:dkabm="http://biblstandard.dk/abm/namespace/dkabm/"
                xmlns:oss="http://oss.dbc.dk/ns/osstypes"
                xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
                extension-element-prefixes="exsl"
                exclude-result-prefixes="dbc dc dcterms dkabm oss xsi"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from dbc.dk

   Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:variable name="letters">ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz</xsl:variable>

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="DBC Import" type="5">
   <fields>
    <field name="_default"/>
   </fields>
   <xsl:apply-templates select="dbc:searchResponse/dbc:result/dbc:searchResult/dbc:collection/dbc:object/dkabm:record"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="dkabm:record">
 <entry>

  <title>
   <xsl:value-of select="dc:title"/>
  </title>

  <year>
   <xsl:value-of select="substring(dc:date,1,4)"/>
  </year>

  <xsl:if test="dcterms:extent[contains(.,'sider')]">
   <pages>
    <xsl:value-of select="translate(dcterms:extent[contains(.,'sider')],$letters,'')"/>
   </pages>
  </xsl:if>

  <publishers>
   <xsl:for-each select="dc:publisher">
    <publisher>
     <xsl:value-of select="."/>
    </publisher>
   </xsl:for-each>
  </publishers>

  <isbn>
   <xsl:value-of select="dc:identifier[@xsi:type='dkdcplus:ISBN']"/>
  </isbn>

  <authors>
   <xsl:for-each select="dc:creator[@xsi:type='dkdcplus:aut']">
    <author>
     <xsl:value-of select="."/>
    </author>
   </xsl:for-each>
  </authors>

 </entry>
</xsl:template>

</xsl:stylesheet>
