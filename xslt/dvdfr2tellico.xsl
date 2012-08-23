<?xml version="1.0" encoding="utf-8" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing dvdfr.com search data.

   Copyright (C) 2010 Robby Stephenson - <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Search Results" type="3"> <!-- 11 is video -->
   <fields>
    <field name="_default"/>
    <field flags="0" title="Original Title" category="General" format="1" type="1" name="origtitle" i18n="yes"/>
    <field flags="0" title="DVDFr ID" category="General" format="4" type="1" name="dvdfr-id"/>
    <field flags="0" title="DVDFr Link" category="General" format="4" type="7" name="dvdfr"/>
    <field title="Alternative Titles" flags="1" category="Alternative Titles" format="1" type="8" name="alttitle" i18n="true">
     <prop name="columns">1</prop>
    </field>
   </fields>
   <!-- initial search has dvds/dvd elements, final detailed is only dvd -->
   <xsl:apply-templates select="/dvds/dvd | /dvd"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="dvd">
 <entry>
  <dvdfr>
   <xsl:value-of select="url"/>
  </dvdfr>

  <dvdfr-id>
   <xsl:value-of select="id"/>
  </dvdfr-id>

  <title>
   <xsl:value-of select="titres/fr"/>
   <xsl:if test="not(titres/fr)">
    <xsl:value-of select="titres/vo"/>
   </xsl:if>
  </title>

  <origtitle>
   <xsl:value-of select="titres/vo"/>
  </origtitle>

  <alttitles>
   <alttitle>
    <column>
     <xsl:value-of select="titres/alternatif"/>
    </column>
   </alttitle>
   <alttitle>
    <column>
     <xsl:value-of select="titres/alternatif_vo"/>
    </column>
   </alttitle>
  </alttitles>

  <year>
   <xsl:value-of select="annee"/>
  </year>

  <format>
   <xsl:value-of select="image/standard"/>
  </format>

  <aspect-ratio>
   <xsl:value-of select="image/aspect_ratio"/>
  </aspect-ratio>

  <xsl:if test="zones">
   <region i18n="true">
    <!-- just take first region listed -->
    <xsl:value-of select="concat('Region ', zones/zone[1])"/>
   </region>
  </xsl:if>

  <cover>
   <xsl:value-of select="cover"/>
  </cover>

  <plot>
   <xsl:value-of select="synopsis"/>
  </plot>

  <comments>
   <xsl:value-of select="listeBonusHtml/bonushtml[@type='video']"/>
  </comments>

  <running-time>
   <xsl:value-of select="duree"/>
  </running-time>

  <medium i18n="true">
   <xsl:choose>
    <xsl:when test="media = 'DVD'">
     <xsl:text>DVD</xsl:text>
    </xsl:when>
    <xsl:when test="media = 'BRD'">
     <xsl:text>Blu-ray</xsl:text>
    </xsl:when>
    <xsl:when test="media = 'HDDVD' or media = 'HD-DVD'">
     <xsl:text>HD DVD</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="media"/>
    </xsl:otherwise>
   </xsl:choose>
  </medium>

  <genres i18n="true">
   <xsl:for-each select="categories/categorie">
    <genre>
     <xsl:value-of select="."/>
    </genre>
   </xsl:for-each>
  </genres>

  <casts>
   <xsl:for-each select="stars/star[@type='Acteur']">
    <cast>
     <column>
      <xsl:value-of select="."/>
     </column>
    </cast>
   </xsl:for-each>
  </casts>

  <directors>
   <xsl:for-each select="stars/star[@type='Réalisateur']">
    <director>
     <xsl:value-of select="."/>
    </director>
   </xsl:for-each>
  </directors>

  <composers>
   <xsl:for-each select="stars/star[@type='Compositeur']">
    <composer>
     <xsl:value-of select="."/>
    </composer>
   </xsl:for-each>
  </composers>

  <writers>
   <xsl:for-each select="stars/star[@type='Scénariste']">
    <writer>
     <xsl:value-of select="."/>
    </writer>
   </xsl:for-each>
  </writers>

  <studios>
   <xsl:for-each select="editeur">
    <studio>
     <xsl:value-of select="."/>
    </studio>
   </xsl:for-each>
  </studios>

  <nationalitys>
   <xsl:for-each select="listePays/pays">
    <nationality>
     <xsl:value-of select="."/>
    </nationality>
   </xsl:for-each>
  </nationalitys>

  <subtitles>
   <xsl:for-each select="soustitrage/soustitre">
    <subtitle>
     <xsl:value-of select="."/>
    </subtitle>
   </xsl:for-each>
  </subtitles>

  <audio-tracks>
   <xsl:for-each select="audiotracks/track">
    <audio-track>
     <xsl:value-of select="concat(standard, ' ', encodage)"/>
    </audio-track>
   </xsl:for-each>
  </audio-tracks>

  <languages>
   <xsl:for-each select="audiotracks/track">
    <language>
     <xsl:value-of select="langue"/>
    </language>
   </xsl:for-each>
  </languages>

  <keywords>
   <xsl:for-each select="edition">
    <keyword>
     <xsl:value-of select="."/>
    </keyword>
   </xsl:for-each>
  </keywords>

  <xsl:if test="image/mode = 'Couleurs'">
   <color i18n="true">
    <xsl:text>Color</xsl:text>
   </color>
  </xsl:if>

  <xsl:if test="contains(image/format, '16/9')">
   <widescreen>
    <xsl:text>true</xsl:text>
   </widescreen>
  </xsl:if>

 </entry>
</xsl:template>

</xsl:stylesheet>
