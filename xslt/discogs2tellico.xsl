<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                extension-element-prefixes="str"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing Discogs release search data.

   Copyright (C) 2008-2009 Robby Stephenson -robby@periapsis.org
                      Roman L. Senn - roman.l.senn@gmail.com

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Discogs API" type="4"> <!-- 4 is music -->
   <fields>
    <field name="_default"/>
    <!-- the importer will actually download the image and ignore this field -->
    <field flags="0" title="Discogs ID" category="General" format="4" type="1" name="discogs-id"/>
    <field flags="0" title="Discogs Link" category="General" format="4" type="7" name="discogs"/>
   <field flags="7" title="Nationality" category="General" format="0" type="1" name="nationality" i18n="true"/>
    <xsl:if test="contains(resp/release/extraartists/artist/role, 'Producer')">
     <field flags="7" title="Producer" category="General" format="2" type="1" name="producer" i18n="true"/>
    </xsl:if>

   </fields>
   <xsl:apply-templates select="/resp/searchresults/result[@type='release'] | resp/artist | resp/release"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="resp/searchresults/result">
 <entry>
  <title>
   <xsl:value-of select="substring-after(title,' - ')"/>
  </title>

  <artists>
   <artist>
    <xsl:value-of select="substring-before(title,' - ')"/>
   </artist>
  </artists>

  <labels>
   <label>
    <xsl:value-of select="substring-before(substring-after(normalize-space(summary),'Label: '),' Catalog#:')"/>
   </label>
  </labels>

  <comments>
   <xsl:value-of select="summary"/>
  </comments>

  <discogs>
   <xsl:value-of select="uri"/>
  </discogs>

  <discogs-id>
   <xsl:variable name="tokens" select="str:tokenize(uri, '/')"/>
   <xsl:value-of select="$tokens[count($tokens)]"/>
  </discogs-id>
 </entry>
</xsl:template>

<xsl:template match="resp/artist">
 <xsl:for-each select="releases/release">
  <entry>
   <title>
    <xsl:value-of select="title"/>
   </title>

   <artists>
    <artist>
     <xsl:value-of select="../../name"/>
    </artist>
   </artists>

   <labels>
    <label>
     <xsl:value-of select="label"/>
    </label>
   </labels>

   <discogs-id>
    <xsl:value-of select="@id"/>
   </discogs-id>

  </entry>
 </xsl:for-each>

</xsl:template>

<xsl:template match="resp/release">
 <entry>
  <title>
   <xsl:value-of select="title"/>
  </title>

  <artists>
   <xsl:for-each select="artists/artist">
    <artist>
     <xsl:value-of select="."/>
    </artist>
   </xsl:for-each>
  </artists>

  <labels>
   <xsl:for-each select="labels/label">
    <label>
     <xsl:value-of select="@name"/>
    </label>
   </xsl:for-each>
  </labels>

  <genres i18n="true">
   <xsl:for-each select="genres/genre">
    <genre>
     <xsl:value-of select="."/>
    </genre>
   </xsl:for-each>
  </genres>

  <keywords i18n="true">
   <xsl:for-each select="styles/style">
    <keyword>
     <xsl:value-of select="."/>
    </keyword>
   </xsl:for-each>
  </keywords>

  <discogs-id>
   <xsl:value-of select="@id"/>
  </discogs-id>

  <comments>
   <xsl:value-of select="notes"/>
  </comments>

  <medium i18n="true">
   <xsl:variable name="medium" select="formats/format[1]/@name"/>
   <xsl:choose>
    <xsl:when test="$medium='CD'">
     <xsl:text>Compact Disc</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="$medium"/>
    </xsl:otherwise>
   </xsl:choose>
  </medium>

  <year>
   <xsl:value-of select="substring(released,0,5)"/>
  </year>

  <tracks>
   <xsl:for-each select="tracklist/track">
    <xsl:sort select="position" data-type="number"/>
    <track>
     <column><xsl:value-of select="title"/></column>
     <column><xsl:value-of select="../../artists/artist[1]"/></column>
     <column><xsl:value-of select="duration"/></column>
    </track>
   </xsl:for-each>
  </tracks>

  <producers>
   <xsl:for-each select="extraartists/artist[contains(role, 'Producer')]">
    <producer>
     <xsl:value-of select="name"/>
    </producer>
   </xsl:for-each>
  </producers>

  <nationalitys>
   <nationality>
    <xsl:value-of select="country"/>
   </nationality>
  </nationalitys>

  <cover>
   <xsl:for-each select="images/image">
    <xsl:sort select="type"/> <!-- 'primary' before 'secondary' -->
    <xsl:sort select="height*width" data-type="number" order="descending"/>
    <xsl:if test="position() = 1">
     <xsl:value-of select="@uri150"/><!-- I wish I could put a 'break' statement here -->
     <!-- use small image -->
    </xsl:if>
   </xsl:for-each>
  </cover>

 </entry>
</xsl:template>

</xsl:stylesheet>
