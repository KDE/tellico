<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:aws="http://webservices.amazon.com/AWSECommerceService/2007-10-29"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing Amazon Web Services data.

   Copyright (C) 2004-2007 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V10.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v10/tellico.dtd"/>


<!-- need to figure out what type of collection -->
<xsl:variable name="args" select="//aws:OperationRequest/aws:Arguments"/>
<xsl:variable name="tmp">
 <xsl:choose>
  <!-- SearchIndex is mode, unless doing ISBN or UPC search -->
  <xsl:when test="$args/aws:Argument[@Name='SearchIndex']">
   <xsl:value-of select="$args/aws:Argument[@Name='SearchIndex']/@Value"/>
  </xsl:when>
  <!-- only happens for books -->
  <xsl:when test="$args/aws:Argument[@Name='Operation']/@Value='ItemLookup'">
   <xsl:text>Books</xsl:text>
  </xsl:when>
 </xsl:choose>
</xsl:variable>
<!-- France might have DVD instead of Video -->
<xsl:variable name="mode">
 <xsl:choose>
  <xsl:when test="$tmp='DVD'">
   <xsl:text>Video</xsl:text>
  </xsl:when>
  <xsl:otherwise>
   <xsl:value-of select="$tmp"/>
  </xsl:otherwise>
 </xsl:choose>
</xsl:variable>

<!-- for lower-casing -->
<xsl:variable name="lcletters">abcdefghijklmnopqrstuvwxyz</xsl:variable>
<xsl:variable name="ucletters">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>

<xsl:template match="/">
 <tellico syntaxVersion="10">
  <collection title="Amazon Import">
   <xsl:attribute name="type">
    <xsl:choose>
     <xsl:when test="$mode='Books'">
      <xsl:text>2</xsl:text>
     </xsl:when>
     <xsl:when test="$mode='Video'">
      <xsl:text>3</xsl:text>
     </xsl:when>
     <!-- also can be pop-music-de -->
     <xsl:when test="$mode='Music'">
      <xsl:text>4</xsl:text>
     </xsl:when>
     <xsl:when test="$mode='VideoGames'">
      <xsl:text>11</xsl:text>
     </xsl:when>
    </xsl:choose>
   </xsl:attribute>
   <fields>
    <field name="_default"/>
    <field flags="0" title="Amazon Link" category="General" format="4" type="7" name="amazon" i18n="true"/>
    <!-- the amazon importer will actually download the images and ignore these fields -->
    <field flags="0" title="Small Image" category="Images"  format="4" type="7" name="small-image"/>
    <field flags="0" title="Medium Image" category="Images"  format="4" type="7" name="medium-image"/>
    <field flags="0" title="Large Image" category="Images" format="4" type="7" name="large-image"/>
   </fields>
   <xsl:for-each select="//aws:Items/aws:Item">
    <xsl:apply-templates select="."/>
   </xsl:for-each>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="aws:Item">
 <entry>
  <amazon>
   <xsl:value-of select="aws:DetailPageURL"/>
  </amazon>

  <small-image>
   <xsl:value-of select="aws:SmallImage/aws:URL"/>
  </small-image>

  <medium-image>
   <xsl:value-of select="aws:MediumImage/aws:URL"/>
  </medium-image>

  <large-image>
   <xsl:value-of select="aws:LargeImage/aws:URL"/>
  </large-image>

  <xsl:choose>
   <xsl:when test="$mode='Books'">
    <comments>
     <xsl:value-of select="aws:EditorialReviews/aws:EditorialReview/aws:Content"/>
    </comments>

    <keywords i18n="true">
     <xsl:for-each select="aws:BrowseNodes/aws:BrowseNode">
      <keyword>
       <xsl:call-template name="nodes">
        <xsl:with-param name="node" select="."/>
       </xsl:call-template>
      </keyword>
     </xsl:for-each>
    </keywords>
   </xsl:when>

   <xsl:when test="$mode='Music'">
    <comments>
     <xsl:value-of select="aws:EditorialReviews/aws:EditorialReview[1]/aws:Content"/>
    </comments>

    <tracks>
     <!-- too hard to know which artist goes with which track, just grab first -->
     <xsl:variable name="artist" select="aws:ItemAttributes/aws:Artist[1]"/>
     <xsl:for-each select="aws:Tracks//aws:Track">
      <track>
       <xsl:value-of select="."/>
       <xsl:text>::</xsl:text>
       <!-- too hard to know which artist goes with which track, just grab first -->
       <xsl:value-of select="$artist"/>
      </track>
     </xsl:for-each>
    </tracks>

    <!-- these get cleaned up within Tellico itself -->
    <genres i18n="true">
     <xsl:for-each select="aws:BrowseNodes/aws:BrowseNode">
      <genre>
       <xsl:call-template name="nodes">
        <xsl:with-param name="node" select="."/>
       </xsl:call-template>
      </genre>
     </xsl:for-each>
    </genres>
   </xsl:when>

   <xsl:when test="$mode='Video'">
    <plot>
     <xsl:value-of select="aws:EditorialReviews/aws:EditorialReview[1]/aws:Content"/>
    </plot>

    <!-- these get cleaned up within Tellico itself -->
    <genres i18n="true">
     <xsl:for-each select="aws:BrowseNodes/aws:BrowseNode">
      <genre>
       <xsl:call-template name="nodes">
        <xsl:with-param name="node" select="."/>
       </xsl:call-template>
      </genre>
     </xsl:for-each>
    </genres>
   </xsl:when>

   <xsl:when test="$mode='VideoGames'">
    <description>
     <xsl:value-of select="aws:EditorialReviews/aws:EditorialReview[1]/aws:Content"/>
    </description>
   </xsl:when>
  </xsl:choose>

  <xsl:apply-templates select="aws:ItemAttributes"/>
 </entry>
</xsl:template>

<xsl:template match="aws:ItemAttributes">
  <title>
   <xsl:value-of select="aws:Title"/>
  </title>

  <languages>
   <xsl:for-each select="aws:Languages/aws:Language[not(aws:Name=preceding-sibling::aws:Language/aws:Name)]">
    <language>
     <xsl:value-of select="aws:Name"/>
    </language>
   </xsl:for-each>
  </languages>

  <xsl:choose>
   <!-- book collection stuff -->
   <xsl:when test="$mode='Books'">
    <authors>
     <xsl:for-each select="aws:Author">
      <author>
       <xsl:value-of select="."/>
      </author>   
     </xsl:for-each>
    </authors>

    <isbn>
     <!-- the EAN is the isbn-13 value for books -->
     <xsl:choose>
      <xsl:when test="aws:ISBN">
       <xsl:value-of select="aws:ISBN"/>
      </xsl:when>
      <xsl:when test="aws:EAN">
       <xsl:value-of select="aws:EAN"/>
      </xsl:when>
     </xsl:choose>
    </isbn>

    <publisher>
     <xsl:value-of select="aws:Publisher"/>
    </publisher>

    <editors>
     <xsl:for-each select="aws:Creator[@Role='Editor']">
      <editor>
       <xsl:value-of select="."/>
      </editor>
     </xsl:for-each>
    </editors>

    <binding i18n="true">
     <xsl:choose>
      <xsl:when test="aws:Binding='Hardcover'">
       <xsl:text>Hardback</xsl:text>
      </xsl:when>
      <xsl:when test="contains(aws:Binding, 'Paperback')">
       <xsl:text>Paperback</xsl:text>
      </xsl:when>
      <!-- specifically for France -->
      <xsl:when test="aws:Binding='Broché' or aws:Binding='Poche'">
       <xsl:text>Souple</xsl:text>
      </xsl:when>
      <xsl:otherwise>
       <xsl:value-of select="aws:Binding"/>
      </xsl:otherwise>
     </xsl:choose>
    </binding>

    <pub_year>
     <xsl:call-template name="year">
      <xsl:with-param name="value" select="aws:PublicationDate"/>
     </xsl:call-template>
    </pub_year>

    <edition>
     <xsl:value-of select="aws:Edition"/>
    </edition>

    <pages>
     <xsl:value-of select="aws:NumberOfPages"/>
    </pages>
   </xsl:when>

   <!-- music collection stuff -->
   <xsl:when test="$mode='Music'">
    <artists>
     <xsl:for-each select="aws:Artist">
      <artist>
       <xsl:value-of select="."/>
      </artist>
     </xsl:for-each>
     <!-- only add composers if no artist -->
     <xsl:if test="not(aws:Artist)">
      <xsl:for-each select="aws:Creator[@Role='Composer']">
       <artist>
        <xsl:value-of select="."/>
       </artist>
      </xsl:for-each>
     </xsl:if>
    </artists>

    <year>
     <xsl:call-template name="year">
      <xsl:with-param name="value" select="aws:ReleaseDate"/>
     </xsl:call-template>
    </year>

    <labels>
     <label>
      <xsl:value-of select="aws:Label"/>
     </label>
    </labels>

    <medium i18n="true">
     <xsl:variable name="medium" select="aws:Binding"/>
     <xsl:choose>
      <xsl:when test="$medium='Audio CD'">
       <xsl:text>Compact Disc</xsl:text>
      </xsl:when>
      <xsl:otherwise>
       <xsl:comment>
        <xsl:value-of select="$medium"/>
       </xsl:comment>
      </xsl:otherwise>
     </xsl:choose>
    </medium>
   </xsl:when>

   <!-- video collection stuff -->
   <xsl:when test="$mode='Video'">
    <directors>
     <xsl:for-each select="aws:Director">
      <director>
       <xsl:value-of select="."/>
      </director>   
     </xsl:for-each>
    </directors>

    <casts>
     <!-- special case for french, there might be more actors than aws:Actor elements -->
     <xsl:for-each select="aws:Creator[@Role='Acteur']">
      <!-- cast is a 2-column table -->
      <cast>
       <column>
        <xsl:value-of select="."/>
       </column>
      </cast>
     </xsl:for-each>
     <!-- assume only add actor if no creators -->
     <xsl:if test="not(aws:Creator[@Role='Acteur'])">
      <xsl:for-each select="aws:Actor">
       <!-- cast is a 2-column table -->
       <cast>
        <column>
         <xsl:value-of select="."/>
        </column>
       </cast>
      </xsl:for-each>
     </xsl:if>
    </casts>

    <year>
     <xsl:call-template name="year">
      <xsl:with-param name="value" select="aws:TheatricalReleaseDate"/>
     </xsl:call-template>
    </year>

    <medium i18n="true">
     <xsl:variable name="medium" select="aws:ProductGroup"/>
     <xsl:choose>
      <xsl:when test="$medium='Video'">
       <xsl:text>VHS</xsl:text>
      </xsl:when>
      <xsl:when test="contains(translate($medium,$lcletters,$ucletters),'blu-ray')">
       <xsl:text>Blu-ray</xsl:text>
      </xsl:when>
      <xsl:otherwise>
       <xsl:value-of select="$medium"/>
      </xsl:otherwise>
     </xsl:choose>
    </medium>

    <color i18n="true">
     <xsl:choose>
      <xsl:when test="aws:Format[.='Black &amp; White']">
       <xsl:text>Black &amp; White</xsl:text>
      </xsl:when>
      <xsl:otherwise>
       <xsl:text>Color</xsl:text>
      </xsl:otherwise>
     </xsl:choose>   
    </color>

    <aspect-ratios>
     <xsl:for-each select="aws:AspectRatio">
      <aspect-ratio>
       <xsl:value-of select="."/>
      </aspect-ratio>
     </xsl:for-each>
    </aspect-ratios>

    <xsl:if test="aws:Format[.='Widescreen']">
     <widescreen>
      <xsl:text>true</xsl:text>
     </widescreen>
    </xsl:if>

    <languages>
     <xsl:for-each select="aws:Languages/aws:Language[not(aws:Name=preceding-sibling::aws:Language/aws:Name)]">
      <language>
       <xsl:value-of select="aws:Name"/>
      </language>
     </xsl:for-each>
    </languages>

    <audio-tracks>
     <xsl:choose>
      <xsl:when test="aws:Languages/aws:Language/aws:AudioFormat[starts-with(.,'Dolby')]">
       <audio-track>
        <xsl:value-of select="aws:Languages/aws:Language/aws:AudioFormat[starts-with(.,'Dolby')][1]"/>
       </audio-track>
      </xsl:when>
      <xsl:when test="aws:Format[starts-with(.,'Dolby')]">
       <audio-track>
        <xsl:value-of select="aws:Format[starts-with(.,'Dolby')][1]"/>
       </audio-track>
      </xsl:when>
     </xsl:choose>
     <xsl:if test="aws:Languages/aws:Language/aws:AudioFormat[starts-with(.,'DTS')]">
      <audio-track>
       <xsl:value-of select="aws:Languages/aws:Language/aws:AudioFormat[starts-with(.,'DTS')][1]"/>
      </audio-track>
     </xsl:if>
    </audio-tracks>

    <certification i18n="true">
     <xsl:variable name="mpaa" select="aws:AudienceRating"/>
     <xsl:choose>
      <xsl:when test="starts-with($mpaa, 'PG-13')">
       <xsl:text>PG-13 (USA)</xsl:text>
      </xsl:when>
      <xsl:when test="starts-with($mpaa, 'PG')">
       <xsl:text>PG (USA)</xsl:text>
      </xsl:when>
      <xsl:when test="starts-with($mpaa, 'R')">
       <xsl:text>R (USA)</xsl:text>
      </xsl:when>
      <xsl:when test="starts-with($mpaa, 'G')">
       <xsl:text>G (USA)</xsl:text>
      </xsl:when>
      <xsl:otherwise>
       <xsl:comment>
        <xsl:value-of select="$mpaa"/>
       </xsl:comment>
      </xsl:otherwise>
     </xsl:choose>
    </certification>

    <studios>
     <studio>
      <xsl:value-of select="aws:Studio"/>
     </studio>
    </studios>

    <xsl:if test="aws:RegionCode">
     <region i18n="true">
      <xsl:value-of select="concat('Region ', aws:RegionCode)"/>
     </region>
    </xsl:if>

    <xsl:if test="aws:RunningTime[@Units='minutes']">
     <running-time>
      <xsl:value-of select="aws:RunningTime[@Units='minutes']"/>
     </running-time>
    </xsl:if>

   </xsl:when>

   <!-- video game collection stuff -->
   <xsl:when test="$mode='VideoGames'">
    <publisher>
     <xsl:value-of select="aws:Publisher"/>
    </publisher>

    <!-- assume year is last four characters of ReleaseDate -->
    <year>
     <xsl:call-template name="year">
      <xsl:with-param name="value" select="aws:ReleaseDate"/>
     </xsl:call-template>
    </year>

    <!-- assume that the only time there are multiple platforms is when it's for multiple versions of windows -->
    <platform i18n="true">
     <xsl:call-template name="platform">
      <xsl:with-param name="value" select="aws:Platform"/>
     </xsl:call-template>
    </platform>

    <genres i18n="true">
     <xsl:for-each select="aws:Feature[starts-with(., 'Genre:')]">
      <genre>
       <xsl:value-of select="substring(., 7)"/> <!-- ends with a space -->
      </genre>
     </xsl:for-each>
    </genres>

    <certification i18n="true">
     <xsl:choose>
      <xsl:when test="aws:ESRBAgeRating = 'Rating Pending'">
       <xsl:text>Pending</xsl:text>
      </xsl:when>
      <xsl:otherwise>
       <xsl:value-of select="aws:ESRBAgeRating"/>
      </xsl:otherwise>    
     </xsl:choose>
    </certification>
   </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template name="nodes">
 <xsl:param name="node"/>

 <xsl:variable name="firstNode" select="$node/aws:Name"/>

 <xsl:variable name="tailNodes">
  <xsl:choose>
   <xsl:when test="$node/aws:Ancestors/aws:BrowseNode[1]/aws:Name">
    <xsl:call-template name="nodes">
     <xsl:with-param name="node" select="$node/aws:Ancestors/aws:BrowseNode[1]"/>
    </xsl:call-template>
   </xsl:when>
  </xsl:choose>
 </xsl:variable>

 <xsl:choose>
  <xsl:when test="string-length($firstNode)">
   <xsl:choose>
    <xsl:when test="string-length($tailNodes)">
     <xsl:value-of select="concat($tailNodes, '/', $firstNode)"/>     
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="$firstNode"/>
    </xsl:otherwise>
   </xsl:choose>
  </xsl:when>
  <xsl:when test="string-length($tailNodes)">
   <xsl:value-of select="$tailNodes"/>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template name="year">
 <xsl:param name="value"/>
 <xsl:variable name="numbers">
  <xsl:value-of select="translate($value, translate($value, '0123456789', ''), '')"/>
 </xsl:variable>
 <!-- assume that Amazon always encodes the date with the 4-digit year first -->
 <xsl:value-of select="substring($numbers, 0, 5)"/>
</xsl:template>

<xsl:template name="platform">
 <xsl:param name="value"/>
 <xsl:variable name="lcvalue">
  <xsl:value-of select="translate($value, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ',
                                          'abcdefghijklmnopqrstuvwxyz')"/>
 </xsl:variable>
 <xsl:choose>
  <xsl:when test="contains($lcvalue, '360')">
   <xsl:text>Xbox 360</xsl:text>
  </xsl:when>
  <xsl:when test="starts-with($lcvalue, 'x')">
   <xsl:text>Xbox</xsl:text> <!-- as defined in the default field -->
  </xsl:when>
  <xsl:when test="contains($lcvalue, 'wii')">
   <xsl:text>Nintendo Wii</xsl:text> <!-- as defined in the default field -->
  </xsl:when>
  <xsl:when test="$lcvalue = 'sony psp'">
   <xsl:text>PSP</xsl:text> <!-- as defined in the default field -->
  </xsl:when>
  <xsl:when test="starts-with($lcvalue, 'windows')">
   <xsl:text>Windows</xsl:text> <!-- as defined in the default field -->
  </xsl:when>
  <xsl:when test="starts-with($lcvalue, 'mac')">
   <xsl:text>Mac OS</xsl:text> <!-- as defined in the default field -->
  </xsl:when>
  <xsl:when test="$lcvalue = 'sega dreamcast'">
   <xsl:text>Dreamcast</xsl:text> <!-- as defined in the default field -->
  </xsl:when>
  <xsl:when test="starts-with($lcvalue, 'playstation')">
   <xsl:choose>
    <xsl:when test="contains($lcvalue, '3')">
     <xsl:text>PlayStation3</xsl:text>
    </xsl:when>
    <xsl:when test="contains($lcvalue, '2')">
     <xsl:text>PlayStation2</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="$value"/>
    </xsl:otherwise>
   </xsl:choose>
  </xsl:when>
  <xsl:otherwise>
   <xsl:value-of select="$value"/>
  </xsl:otherwise>
 </xsl:choose> 
</xsl:template>

</xsl:stylesheet>
