<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:bc="http://periapsis.org/bookcase/"
                exclude-result-prefixes="bc"
                version="1.0">

<!--
   ===================================================================
   Bookcase XSLT file - some common templates.

   $Id: bookcase-common.xsl 620 2004-04-22 02:38:47Z robby $

   Copyright (C) 2004 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Bookcase'
   application, which can be found at http://www.periapsis.org/bookcase/
   ===================================================================
-->

<xsl:output method="html"/>

<!-- Template for checking syntax version -->
<xsl:template name="syntax-version">
 <xsl:param name="this-version"/>
 <xsl:param name="data-version"/>
 <xsl:if test="$data-version != $this-version">
  <xsl:message>
   <xsl:text>This stylesheet was designed for Bookcase DTD version </xsl:text>
   <xsl:value-of select="$this-version"/>
   <xsl:text>, &#xa;but the input data file is version </xsl:text>
   <xsl:value-of select="$data-version"/>
   <xsl:text>. There might be some &#xa;problems with the output.</xsl:text>
  </xsl:message>
 </xsl:if>
</xsl:template>

<!-- template for creating attributes to scale an image to a max boundary size -->
<xsl:template name="image-size">
 <xsl:param name="limit-height"/>
 <xsl:param name="limit-width"/>
 <xsl:param name="image"/>

 <xsl:variable name="actual-width" select="$image/@width"/>
 <xsl:variable name="actual-height" select="$image/@height"/>

 <xsl:choose>
  <xsl:when test="$actual-width &gt; $limit-width or $actual-height &gt; $limit-height">

   <!-- -->
   <xsl:choose>

    <xsl:when test="$actual-width * $limit-height &lt; $actual-height * $limit-width">
     <xsl:attribute name="height">
      <xsl:value-of select="$limit-height"/>
     </xsl:attribute>
     <xsl:attribute name="width">
      <xsl:value-of select="round($actual-width * $limit-height div $actual-height)"/>
     </xsl:attribute>
    </xsl:when>

    <xsl:otherwise>
     <xsl:attribute name="width">
      <xsl:value-of select="$limit-width"/>
     </xsl:attribute>
     <xsl:attribute name="height">
      <xsl:value-of select="round($actual-height * $limit-width div $actual-width)"/>
     </xsl:attribute>
    </xsl:otherwise>

   </xsl:choose>

  </xsl:when>

  <!-- if both are smaller, no change -->
  <xsl:otherwise>
   <xsl:attribute name="height">
    <xsl:value-of select="$actual-height"/>
   </xsl:attribute>
   <xsl:attribute name="width">
    <xsl:value-of select="$actual-width"/>
   </xsl:attribute>
  </xsl:otherwise>

 </xsl:choose>
</xsl:template>

<!-- template for outputing most value types -->
<xsl:template name="simple-field-value">
 <xsl:param name="entry"/>
 <xsl:param name="field"/>

 <!-- if the field has multiple values, then there is
      no child of the entry with the field name -->
 <xsl:variable name="child" select="$entry/*[local-name(.)=$field]"/>
 <xsl:choose>
  <xsl:when test="$child">

   <!-- if the field is a bool type, just ouput an X -->
   <xsl:choose>
    <xsl:when test="key('fieldsByName',$field)/@type=4">
     <xsl:text>X</xsl:text>
    </xsl:when>

    <!-- if it's a url, then add a hyperlink -->
    <xsl:when test="key('fieldsByName',$field)/@type=7">
     <a href="{$child}">
      <xsl:value-of select="$child"/>
     </a>
    </xsl:when>

    <xsl:otherwise>
     <xsl:value-of select="$child"/>
     <!-- hack for running-time in videos -->
     <!--
     <xsl:if test="$field='running-time' and key('fieldsByName',$field)/@type=6">
      <xsl:text> minutes</xsl:text>
     </xsl:if>
     -->
    </xsl:otherwise>
   </xsl:choose>
  </xsl:when>

  <!-- now handle fields with multiple values -->
  <xsl:otherwise>
   <xsl:for-each select="$entry/*[local-name(.)=concat($field,'s')]/*">
    <xsl:value-of select="."/>
    <xsl:if test="position() != last()">
     <xsl:text>; </xsl:text>
    </xsl:if>
   </xsl:for-each>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

</xsl:stylesheet>
