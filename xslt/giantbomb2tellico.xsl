<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing Giant Bomb search data.

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
  <collection title="Search Results" type="11"> <!-- 11 is game -->
   <fields>
    <field name="_default"/>
    <field flags="0" title="Giant Bomb ID" category="General" format="4" type="1" name="giantbomb-id"/>
    <field flags="0" title="Giant Bomb Link" category="General" format="4" type="7" name="giantbomb"/>
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
   <xsl:value-of select="platforms/platform[1]/name"/>
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

 </entry>
</xsl:template>

</xsl:stylesheet>
