<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:atom="http://www.w3.org/2005/Atom"
                xmlns:dcterms="http://purl.org/dc/terms/"
                xmlns:schema="http://schema.org"
                xmlns:str="http://exslt.org/strings"
                xmlns:exsl="http://exslt.org/common"
                exclude-result-prefixes="atom dcterms schema"
                extension-element-prefixes="str exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from an atom feed

   Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Atom Search" type="2">
   <fields>
    <field name="_default"/>
    <field flags="0" title="URL" category="General" format="4" type="7" name="url" i18n="true"/>
   </fields>
   <!-- Project Gutenberg returns an entry without author when there are no search results -->
   <xsl:apply-templates select="atom:feed/atom:entry[atom:author]"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="atom:entry">
 <entry>

  <title>
   <xsl:value-of select="normalize-space(atom:title)"/>
  </title>

  <authors>
   <xsl:for-each select="atom:author">
    <author>
     <xsl:value-of select="normalize-space(atom:name)"/>
    </author>
   </xsl:for-each>
  </authors>

  <publishers>
   <xsl:for-each select="dcterms:publisher">
    <publisher>
     <xsl:value-of select="normalize-space(.)"/>
    </publisher>
   </xsl:for-each>
  </publishers>

  <url>
   <xsl:value-of select="atom:id[starts-with(.,'http')]"/>
  </url>

  <isbn>
   <xsl:value-of select="substring-after(dcterms:identifier[starts-with(.,'urn:ISBN')],'ISBN:')"/>
  </isbn>

  <pub_year>
   <xsl:value-of select="substring(dcterms:issued,1,4)"/>
  </pub_year>

  <pages>
   <xsl:value-of select="schema:numberOfPages"/>
  </pages>

  <plot>
   <!-- prefer summary over content -->
   <xsl:if test="atom:summary">
    <xsl:value-of select="normalize-space(atom:summary)"/>
   </xsl:if>
   <xsl:if test="not(atom:summary)">
    <xsl:value-of select="normalize-space(atom:content)"/>
   </xsl:if>
  </plot>

  <cover>
   <!-- need to handle relative image links -->
   <xsl:value-of select="(atom:link[@rel='http://opds-spec.org/image']/@href |
                          atom:link[@rel='http://opds-spec.org/image/thumbnail']/@href)[1]"/>
  </cover>

  <genres>
   <xsl:for-each select="atom:category">
    <genre>
     <xsl:value-of select="@label"/>
    </genre>
   </xsl:for-each>
  </genres>
 </entry>
</xsl:template>

</xsl:stylesheet>
