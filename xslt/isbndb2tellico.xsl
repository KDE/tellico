<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                xmlns:exsl="http://exslt.org/common"
                xmlns:l="uri:langs"
                extension-element-prefixes="str exsl"
                exclude-result-prefixes="l"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from isbndb.com

   Copyright (C) 2006 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   ===================================================================
-->

<l:langs>
 <l:lang id="ara">Arabic</l:lang>
 <l:lang id="cat">Catalan</l:lang>
 <l:lang id="cze">Czech</l:lang>
 <l:lang id="dut">Dutch</l:lang>
 <l:lang id="eng">English</l:lang>
 <l:lang id="fre">French</l:lang>
 <l:lang id="ger">German</l:lang>
 <l:lang id="dut">Dutch</l:lang>
 <l:lang id="heb">Hebrew</l:lang>
 <l:lang id="gre">Greek</l:lang>
 <l:lang id="hin">Hindi</l:lang>
 <l:lang id="hun">Hungarian</l:lang>
 <l:lang id="ita">Italian</l:lang>
 <l:lang id="jpn">Japanese</l:lang>
 <l:lang id="kor">Korean</l:lang>
 <l:lang id="lat">Latin</l:lang>
 <l:lang id="lit">Lithuanian</l:lang>
 <l:lang id="nob">Norwegian Bokm&#229;l</l:lang>
 <l:lang id="nor">Norwegian</l:lang>
 <l:lang id="nno">Norwegian Nynorsk</l:lang>
 <l:lang id="pol">Polish</l:lang>
 <l:lang id="por">Portuguese</l:lang>
 <l:lang id="rus">Russian</l:lang>
 <l:lang id="slo">Slovak</l:lang>
 <l:lang id="spa">Spanish</l:lang>
 <l:lang id="swe">Swedish</l:lang>
 <l:lang id="chi">Chinese</l:lang>
</l:langs>
<xsl:key name="langs" match="l:lang" use="@id"/>
<xsl:variable name="langs-top" select="document('')/*/l:langs"/>

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V9.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v9/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="9">
  <collection title="ISBNdb.com Import" type="2">
   <fields>
    <field name="_default"/>
    <!-- Here so that we can retrive the real publisher name later, gets removed inside Tellico -->
    <field name="pub_id" flags="0" title="Publisher Key" category="Publishing" format="0" type="1"/>
   </fields>
   <xsl:apply-templates select="ISBNdb/BookList/BookData"/>
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

  <pub_id>
   <xsl:value-of select="PublisherText/@publisher_id"/>
  </pub_id>

  <publisher>
   <xsl:value-of select="PublisherText"/>
  </publisher>

  <authors>
   <xsl:for-each select="Authors/Person">
    <author>
     <xsl:value-of select="."/>
    </author>
   </xsl:for-each>
  </authors>

  <keywords>
   <xsl:for-each select="Subjects/Subject">
    <xsl:variable name="subject-tokens1">
     <xsl:call-template name="split-string">
      <xsl:with-param name="string" select="."/>
      <xsl:with-param name="pattern" select="' -- '"/>
     </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="subject-tokens2">
     <xsl:call-template name="token-union">
      <xsl:with-param name="tokens" select="exsl:node-set($subject-tokens1)/*"/>
     </xsl:call-template>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($subject-tokens2)/*">
     <keyword>
      <xsl:value-of select="normalize-space()"/>
     </keyword>
    </xsl:for-each>
   </xsl:for-each>
  </keywords>

  <comments>
   <xsl:value-of select="Details/@physical_description_text"/>
   <xsl:text>&lt;br/&gt;&lt;br/&gt;</xsl:text>
   <xsl:value-of select="Summary"/>
   <xsl:if test="string-length(Summary) &gt; 0 and string-length(Notes) &gt; 0">
    <xsl:text>&lt;br/&gt;&lt;br/&gt;</xsl:text>
   </xsl:if>
   <xsl:value-of select="Notes"/>
  </comments>

  <xsl:variable name="binding">
   <xsl:value-of select="substring-before(Details/@edition_info, ';')"/>
  </xsl:variable>
  <binding i18n="true">
   <xsl:choose>
    <xsl:when test="$binding='Hardcover'">
     <xsl:text>Hardback</xsl:text>
    </xsl:when>
    <xsl:when test="contains($binding, 'Paperback')">
     <xsl:text>Paperback</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="$binding"/>
    </xsl:otherwise>
   </xsl:choose>
  </binding>

  <languages i18n="true">
   <language>
    <xsl:apply-templates select="$langs-top">
     <xsl:with-param name="lang-id" select="Details/@language"/>
    </xsl:apply-templates>
   </language>
  </languages>

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

<xsl:template name="token-union">
 <xsl:param name="tokens"/>
 <xsl:choose>
  <xsl:when test="not($tokens)"/>
  <xsl:otherwise>
   <xsl:copy-of select="exsl:node-set(str:tokenize($tokens[1], ','))"/>
   <xsl:call-template name="token-union">
    <xsl:with-param name="tokens" select="$tokens[position() &gt; 1]"/>
   </xsl:call-template>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template match="l:langs">
 <xsl:param name="lang-id"/>
 <xsl:variable name="l" select="key('langs', $lang-id)"/>
 <xsl:if test="$l">
  <xsl:value-of select="$l"/>
 </xsl:if>
 <xsl:if test="not($l)">
  <xsl:value-of select="$lang-id"/>
 </xsl:if>
</xsl:template>

</xsl:stylesheet>
