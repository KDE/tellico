<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing Amazon Web Services data.

   Copyright (C) 2004-2005 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V8.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v8/tellico.dtd"/>

<xsl:variable name="mode" select="/ProductInfo/Request/Args/Arg[@name='mode']/@value"/>

<xsl:template match="/">
 <tellico syntaxVersion="8">
  <collection title="Amazon Import">
   <xsl:attribute name="type">
    <xsl:choose>
     <xsl:when test="starts-with($mode,'books')">
      <xsl:text>2</xsl:text>
     </xsl:when>
     <xsl:when test="starts-with($mode,'dvd') or starts-with($mode,'vhs')">
      <xsl:text>3</xsl:text>
     </xsl:when>
     <!-- also can be pop-music-de -->
     <xsl:when test="starts-with($mode,'music') or starts-with($mode,'classical') or $mode='pop-music-de'">
      <xsl:text>4</xsl:text>
     </xsl:when>
     <xsl:when test="starts-with($mode,'video')">
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
   <xsl:for-each select="/ProductInfo/Details">
    <xsl:apply-templates select="."/>
   </xsl:for-each>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="Details">
 <entry>
  <title>
   <xsl:value-of select="ProductName"/>  
  </title>
  
  <amazon>
   <xsl:value-of select="@url"/>
  </amazon>

  <small-image>
   <xsl:value-of select="ImageUrlSmall"/>
  </small-image>

  <medium-image>
   <xsl:value-of select="ImageUrlMedium"/>
  </medium-image>

  <large-image>
   <xsl:value-of select="ImageUrlLarge"/>
  </large-image>

  <xsl:choose>
   <!-- book collection stuff -->
   <xsl:when test="starts-with($mode,'books')">
    <authors>
     <xsl:for-each select="Authors/Author">
      <author>
       <xsl:value-of select="."/>
      </author>   
     </xsl:for-each>
    </authors>
    
    <isbn>
     <xsl:value-of select="Asin"/>   
    </isbn>
    
    <publisher>
     <xsl:value-of select="Manufacturer"/>
    </publisher>
    
    <binding i18n="true">
     <xsl:choose>
      <xsl:when test="Media='Hardcover'">
       <xsl:text>Hardback</xsl:text>
      </xsl:when>
      <xsl:when test="Media='Mass Market Paperback'">
       <xsl:text>Paperback</xsl:text>
      </xsl:when>
     </xsl:choose>
    </binding>
    
    <pub_year>
     <xsl:call-template name="year">
      <xsl:with-param name="value" select="ReleaseDate"/>
     </xsl:call-template>
    </pub_year>
    
    <keywords i18n="true">
     <xsl:for-each select="BrowseList/BrowseNode">
      <keyword>
       <xsl:value-of select="BrowseName"/>
      </keyword>   
     </xsl:for-each>  
    </keywords>
    
    <comments>
     <xsl:value-of select="ProductDescription"/>
    </comments>
   </xsl:when>
   
   <!-- music collection stuff -->
   <xsl:when test="starts-with($mode,'music') or starts-with($mode,'classical') or $mode='pop-music-de'">
    <artists>
     <xsl:for-each select="Artists/Artist">
      <artist>
       <xsl:value-of select="."/>
      </artist>   
     </xsl:for-each>
    </artists>
    
    <year>
     <xsl:call-template name="year">
      <xsl:with-param name="value" select="ReleaseDate"/>
     </xsl:call-template>
    </year>
    
    <labels>
     <label>
      <xsl:value-of select="Manufacturer"/>
     </label>
    </labels>
    
    <tracks>
     <xsl:for-each select="Tracks/Track">
      <track>
       <xsl:value-of select="."/>
      </track>
     </xsl:for-each>
    </tracks>
    
    <medium i18n="true">
     <xsl:variable name="medium" select="Media"/>
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
    
    <genres i18n="true">
     <xsl:for-each select="BrowseList/BrowseNode">
      <genre>
       <xsl:value-of select="BrowseName"/>
      </genre>   
     </xsl:for-each>  
    </genres>
    
    <comments>
     <xsl:value-of select="ProductDescription"/>
    </comments>
   </xsl:when>
   
   <!-- video collection stuff -->
   <xsl:when test="starts-with($mode,'vhs') or starts-with($mode,'dvd')">
    <directors>
     <xsl:for-each select="Directors/Director">
      <director>
       <xsl:value-of select="."/>
      </director>   
     </xsl:for-each>
    </directors>
    
    <casts>
     <xsl:for-each select="Starring/Actor">
      <!-- cast is a 2-column table -->
      <cast>
       <column>
        <xsl:value-of select="."/>
       </column>
      </cast>   
     </xsl:for-each>
    </casts>
    
    <year>
     <xsl:call-template name="year">
      <xsl:with-param name="value" select="TheatricalReleaseDate"/>
     </xsl:call-template>
    </year>
    
    <medium i18n="true">
     <xsl:variable name="medium" select="Media"/>
     <xsl:choose>
      <xsl:when test="$medium='VHS Tape'">
       <xsl:text>VHS</xsl:text>
      </xsl:when>
      <xsl:otherwise>
       <xsl:value-of select="$medium"/>
      </xsl:otherwise>
     </xsl:choose>
    </medium>
    
    <color i18n="true">
     <xsl:choose>
      <xsl:when test="Features/Feature[.='Black &amp; White']">
       <xsl:text>Black &amp; White</xsl:text>
      </xsl:when>
      <xsl:otherwise>
       <xsl:text>Color</xsl:text>
      </xsl:otherwise>
     </xsl:choose>   
    </color>
    
    <widescreen>
     <xsl:if test="Features/Feature[.='Widescreen']">
      <xsl:text>true</xsl:text>
     </xsl:if>
    </widescreen>
    
    <audio-tracks>
     <xsl:if test="Features/Feature[.='Dolby']">
      <audio-track>
       <xsl:text>Dolby</xsl:text>
      </audio-track>
     </xsl:if>
     <xsl:if test="Features/Feature[starts-with(.,'DTS')]">
      <audio-track>
       <xsl:value-of select="Features/Feature[starts-with(.,'DTS')]"/>
      </audio-track>
     </xsl:if>
    </audio-tracks>
    
    <certification i18n="true">
     <xsl:variable name="mpaa" select="MpaaRating"/>
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
      <xsl:value-of select="Manufacturer"/>
     </studio>
    </studios>
    
    <genres i18n="true">
     <xsl:for-each select="BrowseList/BrowseNode">
      <genre>
       <xsl:value-of select="BrowseName"/>
      </genre>   
     </xsl:for-each>  
    </genres>

    <plot>
     <xsl:value-of select="ProductDescription"/>
    </plot>
    
   </xsl:when>
   
   <!-- video game collection stuff -->
   <xsl:when test="starts-with($mode,'video')">
    <publisher>
     <xsl:value-of select="Manufacturer"/>
    </publisher>

    <!-- assume year is last four characters of ReleaseDate -->
    <xsl:if test="ReleaseDate">
     <year>
      <xsl:call-template name="year">
       <xsl:with-param name="value" select="ReleaseDate"/>
      </xsl:call-template>
     </year>
    </xsl:if>

    <description>
     <xsl:value-of select="ProductDescription"/>
    </description>

    <!-- assume that the only time there are multiple platforms is when it's for multiple versions of windows -->
    <platform i18n="true">
     <xsl:call-template name="platform">
      <xsl:with-param name="value" select="Platforms/Platform[1]"/>
     </xsl:call-template>
    </platform>

    <certification i18n="true">
     <xsl:choose>
      <xsl:when test="EsrbRating = 'Rating Pending'">
       <xsl:text>Pending</xsl:text>
      </xsl:when>
      <xsl:otherwise>
       <xsl:value-of select="EsrbRating"/>
      </xsl:otherwise>    
     </xsl:choose>
    </certification>
   </xsl:when>
  </xsl:choose>
  
 </entry>
</xsl:template>

<xsl:template name="year">
 <xsl:param name="value"/>
 <xsl:variable name="numbers">
  <xsl:value-of select="translate($value, translate($value, '0123456789', ''), '')"/>
 </xsl:variable>
 <!-- assume that Amazon always encodes the date with the 4-digit year last -->
 <xsl:value-of select="substring($numbers, string-length($numbers)-3, 4)"/>
</xsl:template>

<xsl:template name="platform">
 <xsl:param name="value"/>
 <xsl:choose>
  <xsl:when test="starts-with($value, 'X')">
   <xsl:text>Xbox</xsl:text> <!-- as defined in the default field -->
  </xsl:when>
  <xsl:when test="$value = 'Sony PSP'">
   <xsl:text>PSP</xsl:text> <!-- as defined in the default field -->
  </xsl:when>
  <xsl:when test="starts-with($value, 'Windows')">
   <xsl:text>Windows</xsl:text> <!-- as defined in the default field -->
  </xsl:when>
  <xsl:when test="starts-with($value, 'Mac')">
   <xsl:text>Mac OS</xsl:text> <!-- as defined in the default field -->
  </xsl:when>
  <xsl:when test="$value = 'Sega Dreamcast'">
   <xsl:text>Dreamcast</xsl:text> <!-- as defined in the default field -->
  </xsl:when>
  <xsl:otherwise>
   <xsl:value-of select="$value"/>
  </xsl:otherwise>
 </xsl:choose> 
</xsl:template>

</xsl:stylesheet>
