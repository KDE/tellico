<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:db="http://www.douban.com/xmlns/"
                xmlns:atom="http://www.w3.org/2005/Atom"
                xmlns:str="http://exslt.org/strings"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="str exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from douban.com

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
  <collection title="Douban.com Import">
   <xsl:attribute name="type">
    <xsl:choose>
     <xsl:when test=".//atom:entry[1]/atom:category/@term='http://www.douban.com/2007#book'">
      <xsl:text>2</xsl:text>
     </xsl:when>
     <xsl:when test=".//atom:entry[1]/atom:category/@term='http://www.douban.com/2007#movie'">
      <xsl:text>3</xsl:text>
     </xsl:when>
     <xsl:when test=".//atom:entry[1]/atom:category/@term='http://www.douban.com/2007#music'">
      <xsl:text>4</xsl:text>
     </xsl:when>
    </xsl:choose>
   </xsl:attribute>
   <fields>
    <field name="_default"/>
    <field flags="0" title="Douban ID" category="General" format="4" type="1" name="douban-id"/>
    <field flags="0" title="Douban Link" category="General" format="4" type="7" name="douban" i18n="true"/>
    <field flags="0" title="IMDb Link" category="General" format="4" type="7" name="imdb" i18n="true"/>
    <field flags="1" title="Alternative Titles" category="Alternative Titles" format="1" type="8" name="alttitle" i18n="true"/>
   </fields>
   <xsl:apply-templates select=".//atom:entry"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="atom:entry">
 <entry>

  <title>
   <xsl:value-of select="atom:title"/>
  </title>

  <isbn>
   <xsl:choose>
    <xsl:when test="db:attribute[@name='isbn13']">
     <xsl:value-of select="db:attribute[@name='isbn13']"/>
    </xsl:when>
    <xsl:when test="db:attribute[@name='isbn10']">
     <xsl:value-of select="db:attribute[@name='isbn10']"/>
    </xsl:when>
   </xsl:choose>
  </isbn>

  <publisher>
   <xsl:value-of select="db:attribute[@name='publisher']"/>
  </publisher>

  <label>
   <xsl:value-of select="db:attribute[@name='publisher']"/>
  </label>

  <authors>
   <xsl:for-each select="db:attribute[@name='author']">
    <author>
     <xsl:value-of select="."/>
    </author>
   </xsl:for-each>
  </authors>

  <translators>
   <xsl:for-each select="db:attribute[@name='translator']">
    <translator>
     <xsl:value-of select="."/>
    </translator>
   </xsl:for-each>
  </translators>

  <directors>
   <xsl:for-each select="db:attribute[@name='director']">
    <director>
     <xsl:value-of select="."/>
    </director>
   </xsl:for-each>
  </directors>

  <writers>
   <xsl:for-each select="db:attribute[@name='writer']">
    <writer>
     <xsl:value-of select="."/>
    </writer>
   </xsl:for-each>
  </writers>

  <artists>
   <xsl:for-each select="db:attribute[@name='singer']">
    <artist>
     <xsl:value-of select="."/>
    </artist>
   </xsl:for-each>
  </artists>

  <genres>
   <xsl:for-each select="db:attribute[@name='movie_type']">
    <genre>
     <xsl:value-of select="."/>
    </genre>
   </xsl:for-each>
  </genres>

  <alttitles>
   <xsl:for-each select="db:attribute[@name='aka']">
    <alttitle>
     <xsl:value-of select="."/>
    </alttitle>
   </xsl:for-each>
  </alttitles>

  <pub_year>
   <xsl:value-of select="substring(db:attribute[@name='pubdate'],1,4)"/>
  </pub_year>

  <year>
   <xsl:choose>
    <xsl:when test="db:attribute[@name='year']">
     <xsl:value-of select="db:attribute[@name='year']"/>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="substring(db:attribute[@name='pubdate'],1,4)"/>
    </xsl:otherwise>
   </xsl:choose>
  </year>

  <pages>
   <xsl:value-of select="db:attribute[@name='pages']"/>
  </pages>

  <binding i18n="true">
   <xsl:value-of select="db:attribute[@name='binding']"/>
  </binding>

  <running-time>
   <xsl:value-of select="translate(db:attribute[@name='movie_duration'],
                         translate(db:attribute[@name='movie_duration'],'0123456789', ''),'')"/>
  </running-time>

  <medium i18n="true">
   <xsl:if test="contains(db:attribute[@name='media'], 'CD')">
    <xsl:text>Compact Disc</xsl:text>
   </xsl:if>
  </medium>

  <cover>
   <xsl:value-of select="atom:link[@rel='image']/@href"/>
  </cover>

  <keywords i18n="true">
   <xsl:for-each select="db:attribute[@name='version']">
    <keyword>
     <xsl:value-of select="."/>
    </keyword>
   </xsl:for-each>
   <xsl:for-each select="db:tag">
    <keyword>
     <xsl:value-of select="@name"/>
    </keyword>
   </xsl:for-each>
  </keywords>

  <xsl:choose>
   <xsl:when test="atom:category/@term='http://www.douban.com/2007#book'">
    <comments>
     <xsl:value-of select="atom:summary"/>
    </comments>
   </xsl:when>
   <xsl:when test="atom:category/@term='http://www.douban.com/2007#movie'">
    <plot>
     <xsl:value-of select="atom:summary"/>
    </plot>
   </xsl:when>
   <xsl:when test="atom:category/@term='http://www.douban.com/2007#music'">
    <comments>
     <xsl:value-of select="atom:summary"/>
    </comments>
   </xsl:when>
  </xsl:choose>

  <casts>
   <xsl:for-each select="db:attribute[@name='cast']">
    <cast>
     <xsl:value-of select="."/>
    </cast>
   </xsl:for-each>
  </casts>

  <languages>
   <xsl:for-each select="db:attribute[@name='language']">
    <language i18n="true">
     <xsl:value-of select="."/>
    </language>
   </xsl:for-each>
  </languages>

  <nationalitys>
   <xsl:for-each select="db:attribute[@name='country']">
    <nationality i18n="true">
     <xsl:value-of select="."/>
    </nationality>
   </xsl:for-each>
  </nationalitys>

  <tracks>
   <xsl:for-each select="db:attribute[@name='tracks']">
    <xsl:sort select="@index" data-type="number"/>
    <track>
     <column>
      <xsl:value-of select="substring-before(.,' -')"/>
     </column>
     <column>
      <xsl:value-of select="substring-after(.,'- ')"/>
     </column>
    </track>
   </xsl:for-each>
  </tracks>

  <douban-id>
   <xsl:value-of select="atom:id"/>
  </douban-id>

  <douban>
   <xsl:value-of select="atom:link[@rel='alternate']/@href"/>
  </douban>

  <imdb>
   <xsl:value-of select="db:attribute[@name='imdb']"/>
  </imdb>

 </entry>
</xsl:template>

</xsl:stylesheet>
