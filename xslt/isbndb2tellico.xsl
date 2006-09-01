<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="str exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from isbndb.com

   Copyright (C) 2006 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V9.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v9/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="9">
  <collection title="ISBNdb.com Import" type="1">
   <fields>
    <field name="_default"/>
   </fields>
   <xsl:for-each select="ISBNdb/BookList/BookData">
    <xsl:apply-templates select="."/>
   </xsl:for-each>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="BookData">
 <entry>

  <title>
   <xsl:value-of select="Title"/>
  </title>

  <isbn>
   <xsl:value-of select="@isbn"/>
  </isbn>

  <publisher>
   <xsl:value-of select="PublisherText"/>
  </publisher>

<!--<xsl:variable name="author-tokens" select="str:tokenize(AuthorsText, ' ')"/>-->
  <xsl:variable name="author-tokens1">
   <xsl:call-template name="split-string">
    <xsl:with-param name="string" select="AuthorsText"/>
    <xsl:with-param name="pattern" select="' and '"/>
   </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="author-tokens2">
   <xsl:call-template name="author-union">
    <xsl:with-param name="tokens" select="exsl:node-set($author-tokens1)/*"/>
   </xsl:call-template>
  </xsl:variable>

  <authors>
   <xsl:for-each select="exsl:node-set($author-tokens2)/*">
    <author>
     <xsl:value-of select="normalize-space()"/>
    </author>
   </xsl:for-each>
  </authors>

  <comments>
   <xsl:value-of select="Summary"/>
   <xsl:if test="Summary and Notes">
    <xsl:text>&lt;br/&gt;&lt;br/&gt;</xsl:text>
   </xsl:if>
   <xsl:value-of select="Notes"/>
  </comments>

 </entry>
</xsl:template>

<xsl:template name="split-string">
 <xsl:param name="string"/>
 <xsl:param name="pattern"/>
  <xsl:choose>
   <xsl:when test="contains($string, $pattern)">
    <xsl:if test="not(starts-with($string, $pattern))">
     <token>
      <xsl:value-of select="substring-before($string, $pattern)"/>
     </token>
    </xsl:if>
    <xsl:call-template name="split-string">
     <xsl:with-param name="string" select="substring-after($string, $pattern)"/>
     <xsl:with-param name="pattern" select="$pattern"/>
    </xsl:call-template>
   </xsl:when>
   <xsl:otherwise>
    <token>
     <xsl:value-of select="$string"/>
    </token>
   </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="author-union">
 <xsl:param name="tokens"/>
 <xsl:choose>
  <xsl:when test="not($tokens)"/>
  <xsl:otherwise>
   <xsl:copy-of select="exsl:node-set(str:tokenize($tokens[1], ','))"/>
   <xsl:call-template name="author-union">
    <xsl:with-param name="tokens" select="$tokens[position() &gt; 1]"/>
   </xsl:call-template>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

</xsl:stylesheet>
