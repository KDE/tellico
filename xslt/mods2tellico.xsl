<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:mods="http://www.loc.gov/mods/v3"
                xmlns:l="uri:langs"
                exclude-result-prefixes="mods"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing MODS files.

   $Id: mods2tellico.xsl 964 2004-11-19 06:54:49Z robby $

   Copyright (C) 2004 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   Currently, only book collections are supported for MOD import.

   ===================================================================
-->

<!-- lookup table for languages -->
<!-- according to http://www.oasis-open.org/cover/iso639-2-en971019.html -->
<!-- I just added common ones -->
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
 <l:lang id="ita">Italian</l:lang>
 <l:lang id="jpn">Japanese</l:lang>
 <l:lang id="kor">Korean</l:lang>
 <l:lang id="lat">Latin</l:lang>
 <l:lang id="nor">Norwegian</l:lang>
 <l:lang id="por">Portuguese</l:lang>
 <l:lang id="rus">Russian</l:lang>
 <l:lang id="spa">Spanish</l:lang>
 <l:lang id="swe">Swedish</l:lang>
 <l:lang id="chi">Chinese</l:lang>
</l:langs>
<xsl:key name="langs" match="l:lang" use="@id"/>
<xsl:variable name="langs-top" select="document('')/*/l:langs"/>

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V7.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v7/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="7">
  <!-- consider it a book collection, type = "2" -->
  <collection title="MODS Import" type="2">
   <fields>
    <field name="_default"/>
    <!-- the default book collection does not have multiple publishers -->
    <xsl:if test=".//mods:mods/mods:originInfo[count(mods:publisher) &gt; 1]">
     <field flags="7" title="Publisher" category="Publishing" format="0" type="1" name="publisher" i18n="true">
      <prop name="bibtex">publisher</prop>
     </field>
    </xsl:if>
    <!-- default field list does not include an abstract -->
    <xsl:if test=".//mods:mods/mods:abstract">
     <field flags="0" title="Abstract" format="4" type="2" name="abstract" i18n="true">
    <prop name="bibtex">abstract</prop>
   </field>
    </xsl:if>
    <!-- default book collection field list does not include an address -->
    <xsl:if test=".//mods:mods/mods:originInfo/mods:place/mods:placeTerm[@type='text']">
     <field flags="6" title="Address" category="Publishing" format="4" type="1" name="address" i18n="true">
      <prop name="bibtex">address</prop>
     </field>
    </xsl:if>
   </fields>
<!-- for now, go the route of bibliox, and assume only text records
  with an originInfo/publisher element are actually books -->
   <xsl:for-each select=".//mods:mods[mods:typeOfResource='text' and mods:originInfo/mods:publisher]">
    <xsl:apply-templates select="."/>
   </xsl:for-each>
  </collection>
 </tellico>
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
      <xsl:for-each select="mods:namePart[@type ='' or not(@type = 'date')]">
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
     <xsl:apply-templates select="$langs-top">
      <xsl:with-param name="lang-id" select="text()"/>
     </xsl:apply-templates>
    </language>
   </xsl:for-each>
  </languages>

  <address>
   <xsl:value-of select="mods:originInfo/mods:place/mods:placeTerm[@type='text'][1]"/>  
  </address>

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
    <!-- add separating line, Tellico understands HTML now -->
    <xsl:text>&lt;br/&gt;&lt;br/&gt;</xsl:text>
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
