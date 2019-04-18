<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing ComicVine.com search data.

   Copyright (C) 2019 Robby Stephenson - <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Search Results" type="6"> <!-- 6 is comic -->
   <fields>
    <field name="_default"/>
    <field flags="0" title="ComicVine Link" category="General" format="4" type="7" name="comicvine" i18n="true"/>
    <field flags="0" title="ComicVine API Link" category="General" format="4" type="7" name="comicvine-api"/>
    <field flags="0" title="ComicVine Volume API Link" category="General" format="4" type="7" name="comicvine-volume-api"/>
   </fields>
   <!-- initial search has results/issue elements, final detailed is only results, only ones with name child -->
   <xsl:apply-templates select="/response/results/issue | /response/results[name]"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="issue|results">
 <entry>
  <comicvine>
   <xsl:value-of select="site_detail_url"/>
  </comicvine>

  <comicvine-api>
   <xsl:value-of select="api_detail_url"/>
  </comicvine-api>

  <comicvine-volume-api>
   <xsl:value-of select="volume/api_detail_url"/>
  </comicvine-volume-api>

  <title>
   <!-- TODO: Tellico's schema for comics is not the best. Issue title here will have to do. -->
   <xsl:value-of select="name"/>
  </title>

  <pub_year>
   <xsl:value-of select="start_year"/>
   <xsl:if test="string-length(start_year)=0">
    <xsl:value-of select="substring(cover_date, 1, 4)"/>
   </xsl:if>
  </pub_year>

  <plot>
   <xsl:value-of select="description"/>
  </plot>

  <issue>
   <xsl:value-of select="issue_number"/>
  </issue>

  <series>
   <xsl:value-of select="volume/name"/>
  </series>

  <cover>
   <xsl:value-of select="image/thumb_url"/>
  </cover>

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

  <writers>
   <xsl:for-each select="person_credits/person[role='writer']">
    <writer>
     <xsl:value-of select="name"/>
    </writer>
   </xsl:for-each>
  </writers>

  <artists>
   <xsl:for-each select="person_credits/person[contains(role,'penciler') or
                                               contains(role,'letterer') or
                                               contains(role,'colorist') or
                                               contains(role,'inker')]">
    <artist>
     <xsl:value-of select="name"/>
    </artist>
   </xsl:for-each>
  </artists>

 </entry>
</xsl:template>

</xsl:stylesheet>
