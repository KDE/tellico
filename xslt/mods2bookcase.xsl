<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/bookcase/"
                xmlns:mods="http://www.loc.gov/mods/v3"
                exclude-result-prefixes="mods"
                version="1.0">

<!--
   ===================================================================
   Bookcase XSLT file - used for importing MODS files.

   $Id: mods2bookcase.xsl 885 2004-09-20 05:56:18Z robby $

   Copyright (C) 2004 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Bookcase'
   application, which can be found at http://www.periapsis.org/tellico/

   Currently, only book collections are supported for MOD import.

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Bookcase V6.0//EN"
            doctype-system="http://periapsis.org/bookcase/dtd/v6/bookcase.dtd"/>

<xsl:template match="/">
 <bookcase syntaxVersion="6">
  <collection unitTitle="Books" title="MODS Import" type="2">
   <fields>
    <field name="_default"/>
    <!-- the default book collection does not have multiple publishers -->
    <xsl:if test=".//mods:mods/mods:originInfo[count(mods:publisher) &gt; 1]">
     <field flags="7" title="Publisher" category="Publishing" format="0" type="1" name="publisher" i18n="true"/>
    </xsl:if>
    <xsl:if test=".//mods:mods/mods:abstract">
     <field flags="0" title="Abstract" format="4" type="2" name="abstract" description="Abstract" i18n="true"/>
    </xsl:if>
   </fields>
<!-- for now, go the route of bibliox, and assume only text records
  with an originInfo/publisher element are actually books -->
   <xsl:for-each select=".//mods:mods[mods:typeOfResource='text' and mods:originInfo/mods:publisher]">
    <xsl:apply-templates select="."/>
   </xsl:for-each>
  </collection>
 </bookcase>
</xsl:template>

<xsl:template match="mods:mods">
 <entry>
  <title>
   <xsl:value-of select="mods:titleInfo/mods:nonSort"/>  
   <xsl:value-of select="mods:titleInfo/mods:title"/>  
  </title>

  <subtitle>
   <xsl:value-of select="mods:titleInfo/mods:subTitle"/>  
  </subtitle>

  <authors>
   <xsl:for-each select="mods:name[@type='personal']">
    <!-- don't be picky right now, but could test for -->
    <!--    <xsl:if test="mods:role[mods:roleTerm/@authority='marcrelator' and mods:roleTerm='creator']">-->
     <author>
      <xsl:for-each select="mods:namePart">
       <xsl:value-of select="."/>
       <xsl:if test="position() &lt; last()">
        <xsl:text> </xsl:text>
       </xsl:if>
      </xsl:for-each>
     </author>
   </xsl:for-each>
  </authors>

  <genres i18n="true">
   <xsl:for-each select="mods:genre">
    <genre>
     <xsl:value-of select="."/>
    </genre>
   </xsl:for-each>
  </genres>

  <xsl:choose>
   <xsl:when test="mods:originInfo[count(mods:publisher) &gt; 1]">
    <publishers>
     <xsl:for-each select="mods:originInfo/mods:publisher">
      <publisher>
       <xsl:value-of select="."/>  
      </publisher>
     </xsl:for-each>
    </publishers>
   </xsl:when>
   <xsl:otherwise>
    <publisher>
     <xsl:value-of select="mods:originInfo/mods:publisher"/>  
    </publisher>
   </xsl:otherwise>
  </xsl:choose>

  <!-- prefer the marc encoding for year -->
  <pub_year>
   <xsl:choose>
    <xsl:when test="mods:originInfo/mods:dateIssued[@encoding='marc']">
     <xsl:value-of select="mods:originInfo/mods:dateIssued[@encoding='marc']"/>
    </xsl:when>
    <xsl:when test="mods:originInfo/mods:dateIssued">
     <xsl:value-of select="mods:originInfo/mods:dateIssued[1]"/>
    </xsl:when>
   </xsl:choose>
  </pub_year>
  
  <cr_year>
   <xsl:value-of select="mods:originInfo/mods:copyrightDate[@encoding='marc']"/>  
  </cr_year>

  <edition i18n="true">
   <xsl:value-of select="mods:originInfo/edition"/>  
  </edition>

  <languages i18n="true">
   <xsl:for-each select="mods:language/mods:languageTerm">
    <language>
     <xsl:value-of select="."/>
    </language>
   </xsl:for-each>
  </languages>

  <isbn>
   <xsl:call-template name="numbers">
    <xsl:with-param name="value" select="mods:identifier[@type='isbn']"/>
   </xsl:call-template>
  </isbn>

  <lccn>
   <xsl:value-of select="mods:identifier[@type='lccn']"/>
  </lccn>

  <comments>
   <xsl:for-each select="mods:note">
    <xsl:value-of select="."/>
   </xsl:for-each>
  </comments>

  <abstract>
   <xsl:value-of select="mods:abstract"/>  
  </abstract>

  <keywords i18n="true">
   <xsl:for-each select="mods:subject/mods:topic">
    <keyword>
     <xsl:value-of select="."/>
    </keyword>
   </xsl:for-each>
  </keywords>

 </entry>
</xsl:template>

<xsl:template name="numbers">
 <xsl:param name="value"/>
 <xsl:value-of select="translate($value, translate($value, '0123456789', ''), '')"/>
</xsl:template>

</xsl:stylesheet>
