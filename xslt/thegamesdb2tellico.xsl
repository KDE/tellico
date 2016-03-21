<?xml version="1.0" encoding="utf-8" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing thegamesdb.net search data.

   Copyright (C) 2012 Robby Stephenson - <robby@periapsis.org>

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
    <field flags="0" title="TGDB ID" category="General" format="4" type="1" name="thegamesdb-id"/>
   </fields>
   <xsl:apply-templates select="/Data/Game"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="Game">
 <entry>
  <thegamesdb-id>
   <xsl:value-of select="id"/>
  </thegamesdb-id>

  <title>
   <xsl:value-of select="GameTitle"/>
  </title>

  <year>
   <xsl:value-of select="substring(ReleaseDate,string-length(ReleaseDate)-3,4)"/>
  </year>

  <platform i18n="true">
   <xsl:choose>
    <xsl:when test="contains(Platform, '360')">
     <xsl:text>Xbox 360</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Platform, 'Playstation 3')">
     <xsl:text>PlayStation3</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Platform, 'Playstation 2')">
     <xsl:text>PlayStation2</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Platform, 'Playstation')">
     <xsl:text>PlayStation</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Platform, 'GameCube')">
     <xsl:text>GameCube</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Platform, 'Genesis')">
     <xsl:text>Genesis</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Platform, 'Dreamcast')">
     <xsl:text>Dreamcast</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Platform, 'SNES')">
     <xsl:text>Super Nintendo</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Platform, 'NES')">
     <xsl:text>Nintendo</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Platform, 'Game Boy Advance')">
     <xsl:text>Game Boy Advance</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Platform, 'Game Boy Color')">
     <xsl:text>Game Boy Color</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Platform, 'Game Boy')">
     <xsl:text>Game Boy</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Platform, 'Microsoft Xbox')">
     <xsl:text>Xbox</xsl:text>
    </xsl:when>
    <xsl:when test="contains(Platform, 'PSP')">
     <xsl:text>PSP</xsl:text>
    </xsl:when>
    <xsl:when test="Platform = 'PC'">
     <xsl:text>Windows</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="Platform"/>
    </xsl:otherwise>
   </xsl:choose>
  </platform>

  <xsl:variable name="esrb">
   <xsl:value-of select="substring-after(ESRB, '- ')"/>
  </xsl:variable>
  <certification i18n="true">
   <xsl:choose>
    <xsl:when test="contains($esrb, 'Everyone')">
     <xsl:text>Everyone</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="$esrb"/>
    </xsl:otherwise>
   </xsl:choose>
  </certification>

  <description>
   <xsl:value-of select="Overview"/>
  </description>

  <genres i18n="true">
   <xsl:for-each select="Genres/genre">
    <genre i18n="true">
     <xsl:value-of select="."/>
    </genre>
   </xsl:for-each>
  </genres>

  <publishers>
   <publisher>
    <xsl:value-of select="Publisher"/>
   </publisher>
  </publishers>

  <developers>
   <developer>
    <xsl:value-of select="Developer"/>
   </developer>
  </developers>

  <!-- if there are more than one Game, then Tellico is just doing a name search -->
  <!-- for efficiency, don't load the image in that case -->
  <xsl:if test="count(../Game) = 1">
   <cover>
    <xsl:value-of select="concat(../baseImgUrl, Images/boxart[@side='front'])"/>
   </cover>
  </xsl:if>

 </entry>
</xsl:template>

</xsl:stylesheet>
