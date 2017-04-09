<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing Giant Bomb search data.

   Copyright (C) 201-2017 Robby Stephenson - <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Search Results" type="11"> <!-- 11 is game -->
   <fields>
    <field name="_default"/>
    <field flags="0" title="Giant Bomb ID" category="General" format="4" type="1" name="giantbomb-id"/>
    <field flags="0" title="Giant Bomb Link" category="General" format="4" type="7" name="giantbomb" i18n="true"/>
   </fields>
   <!-- initial search has results/game elements, final detailed is only results, only ones with name child -->
   <xsl:apply-templates select="/response/results/game | /response/results[name]"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="game|results">
 <entry>
  <giantbomb>
   <xsl:value-of select="site_detail_url"/>
  </giantbomb>

  <giantbomb-id>
   <xsl:value-of select="id"/>
  </giantbomb-id>

  <title>
   <xsl:value-of select="name"/>
  </title>

  <year>
   <xsl:value-of select="expected_release_year"/>
   <xsl:if test="string-length(expected_release_year)=0">
    <xsl:value-of select="substring(original_release_date, 1, 4)"/>
   </xsl:if>
  </year>

  <description>
   <xsl:value-of select="deck"/>
  </description>

  <cover>
   <xsl:value-of select="image/thumb_url"/>
  </cover>

  <platform>
   <xsl:variable name="p" select="platforms/platform[1]/name"/>
   <!-- convert to default Tellico spelling -->
   <xsl:choose>
    <xsl:when test="$p='PlayStation 4'">
     <xsl:value-of select="'PlayStation4'"/>
    </xsl:when>
    <xsl:when test="$p='PlayStation 3'">
     <xsl:value-of select="'PlayStation3'"/>
    </xsl:when>
    <xsl:when test="$p='PlayStation 2'">
     <xsl:value-of select="'PlayStation2'"/>
    </xsl:when>
    <xsl:when test="$p='PlayStation Portable'">
     <xsl:value-of select="'PSP'"/>
    </xsl:when>
    <xsl:when test="contains($p, '360')">
     <xsl:value-of select="'Xbox 360'"/>
    </xsl:when>
    <xsl:when test="contains($p, 'Wii')">
     <xsl:value-of select="'Nintendo Wii'"/>
    </xsl:when>
    <xsl:when test="contains($p, '3DS')">
     <xsl:value-of select="'Nintendo 3DS'"/>
    </xsl:when>
    <xsl:when test="contains($p, 'Super Nintendo')">
     <xsl:value-of select="'Super Nintendo'"/>
    </xsl:when>
    <xsl:when test="$p = 'Nintendo Entertainment System'">
     <xsl:value-of select="'Nintendo'"/>
    </xsl:when>
    <xsl:when test="$p='PC'">
     <xsl:value-of select="'Windows'"/>
    </xsl:when>
    <xsl:when test="$p='Mac'">
     <xsl:value-of select="'Mac OS'"/>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="$p"/>
    </xsl:otherwise>
   </xsl:choose>
  </platform>

  <genres>
   <xsl:for-each select="genres/genre">
    <genre>
     <xsl:value-of select="name"/>
    </genre>
   </xsl:for-each>
  </genres>

  <publishers>
   <xsl:for-each select="publishers/company|publishers/publisher">
    <publisher>
     <xsl:value-of select="name"/>
    </publisher>
   </xsl:for-each>
  </publishers>

  <developers>
   <xsl:for-each select="developers/company|developers/developer">
    <developer>
     <xsl:value-of select="name"/>
    </developer>
   </xsl:for-each>
  </developers>

  <certification i18n="true">
   <xsl:for-each select="original_game_rating/game_rating">
    <xsl:if test="starts-with(name, 'ESRB')">
     <!-- string starts with 'ESRB: ' -->
     <xsl:variable name="esrb" select="substring(name, 7)"/>
     <xsl:choose>
      <xsl:when test="$esrb='U'">
       <xsl:value-of select="'Unrated'"/>
      </xsl:when>
      <xsl:when test="$esrb='AO'">
       <xsl:value-of select="'Adults Only'"/>
      </xsl:when>
      <xsl:when test="$esrb='M'">
       <xsl:value-of select="'Mature'"/>
      </xsl:when>
      <xsl:when test="$esrb='T'">
       <xsl:value-of select="'Teen'"/>
      </xsl:when>
      <xsl:when test="$esrb='E10+'">
       <xsl:value-of select="'Everyone 10+'"/>
      </xsl:when>
      <xsl:when test="$esrb='EC'">
       <xsl:value-of select="'Everyone'"/>
      </xsl:when>
      <xsl:when test="$esrb='C'">
       <xsl:value-of select="'Early Childhood'"/>
      </xsl:when>
     </xsl:choose>
    </xsl:if>
   </xsl:for-each>
  </certification>

 </entry>
</xsl:template>

</xsl:stylesheet>
