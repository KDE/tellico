<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                extension-element-prefixes="str"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing themoviedb.org search data.

   Copyright (C) 2009 Robby Stephenson -robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Search Results" type="3"> <!-- 3 is videos -->
   <fields>
    <field name="_default"/>
    <field flags="0" title="TMDb ID" category="General" format="4" type="1" name="tmdb-id"/>
    <field flags="0" title="TMDb Link" category="General" format="4" type="7" name="tmdb" i18n="true"/>
    <field title="Alternative Titles" flags="1" category="Alternative Titles" format="1" type="8" name="alttitle" i18n="true">
     <prop name="columns">1</prop>
    </field>
   </fields>
   <xsl:apply-templates select="//movies/movie|//filmography/movie"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="movie">
 <entry>
  <title>
   <xsl:value-of select="name|@name"/>
  </title>

  <alttitles>
   <alttitle>
    <column>
     <xsl:value-of select="alternative_name"/>
    </column>
   </alttitle>
  </alttitles>

  <year>
   <xsl:value-of select="substring(released, 1, 4)"/>
  </year>

  <genres>
   <xsl:for-each select="categories/category">
    <genre>
     <xsl:value-of select="@name"/>
    </genre>
   </xsl:for-each>
  </genres>

  <directors>
   <xsl:for-each select="cast/person[@job='Director']">
    <director>
     <xsl:value-of select="@name"/>
    </director>
   </xsl:for-each>
  </directors>

  <producers>
   <xsl:for-each select="cast/person[@job='Producer']">
    <producer>
     <xsl:value-of select="@name"/>
    </producer>
   </xsl:for-each>
  </producers>

  <casts>
   <xsl:for-each select="cast/person[@job='Actor']">
    <cast>
     <xsl:value-of select="concat(@name,'::',@character)"/>
    </cast>
   </xsl:for-each>
  </casts>

  <studios>
   <xsl:for-each select="studios/studio">
    <studio>
     <xsl:value-of select="@name"/>
    </studio>
   </xsl:for-each>
  </studios>

  <nationalitys>
   <xsl:for-each select="countries/country">
    <nationality>
     <xsl:value-of select="@name"/>
    </nationality>
   </xsl:for-each>
  </nationalitys>

  <plot>
   <xsl:value-of select="overview"/>
  </plot>

  <!-- only want the cover if this is a Movie.getInfo search -->
  <xsl:if test="not(score)">
   <cover>
    <xsl:value-of select="images/image[@size='cover'][1]/@url"/>
   </cover>
  </xsl:if>

  <tmdb-id>
   <xsl:value-of select="id|@id"/>
  </tmdb-id>

  <tmdb>
   <xsl:value-of select="url"/>
  </tmdb>

  <xsl:if test="imdb_id">
   <imdb>
    <xsl:value-of select="concat('http://www.imdb.com/title/',imdb_id)"/>
   </imdb>
  </xsl:if>

 </entry>
</xsl:template>

</xsl:stylesheet>
