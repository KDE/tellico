<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - some common templates.

   Copyright (C) 2004-2020 Robby Stephenson <robby@periapsis.org>
                           John Zaitseff <J.Zaitseff@zap.org.au>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org
   ===================================================================
-->

<xsl:output method="html"
            indent="yes"
            encoding="utf-8"/>

<!-- Template for checking syntax version -->
<xsl:template name="syntax-version">
 <xsl:param name="this-version"/>
 <xsl:param name="data-version"/>
 <xsl:if test="$data-version &gt; $this-version">
  <xsl:message>
   <xsl:text>This stylesheet was designed for Tellico DTD version </xsl:text>
   <xsl:value-of select="$this-version"/>
   <xsl:text> or earlier, &#xa;but the input data file is version </xsl:text>
   <xsl:value-of select="$data-version"/>
   <xsl:text>. There might be some &#xa;problems with the output.</xsl:text>
  </xsl:message>
 </xsl:if>
</xsl:template>

<!-- template for creating attributes to scale an image to a max boundary size -->
<xsl:template name="image-size">
 <xsl:param name="limit-height" select="'0'"/>
 <xsl:param name="limit-width" select="'0'"/>
 <xsl:param name="image"/>

 <xsl:variable name="actual-width" select="$image/@width"/>
 <xsl:variable name="actual-height" select="$image/@height"/>

 <xsl:choose>
  <xsl:when test="$limit-width &gt; 0 and $limit-height &gt; 0 and
                  ($actual-width &gt; $limit-width or $actual-height &gt; $limit-height)">

   <xsl:choose>

    <xsl:when test="$actual-width * $limit-height &lt; $actual-height * $limit-width">
     <xsl:attribute name="height">
      <xsl:value-of select="round($limit-height)"/>
     </xsl:attribute>
     <xsl:attribute name="width">
      <xsl:value-of select="round($actual-width * $limit-height div $actual-height)"/>
     </xsl:attribute>
    </xsl:when>

    <xsl:otherwise>
     <xsl:attribute name="width">
      <xsl:value-of select="round($limit-width)"/>
     </xsl:attribute>
     <xsl:attribute name="height">
      <xsl:value-of select="round($actual-height * $limit-width div $actual-width)"/>
     </xsl:attribute>
    </xsl:otherwise>

   </xsl:choose>

  </xsl:when>

  <!-- if both are smaller, no change -->
  <xsl:when test="$actual-width &gt; 0 and $actual-height &gt; 0">
   <xsl:attribute name="width">
    <xsl:value-of select="$actual-width"/>
   </xsl:attribute>
   <xsl:attribute name="height">
    <xsl:value-of select="$actual-height"/>
   </xsl:attribute>
  </xsl:when>

 </xsl:choose>
</xsl:template>

<!-- template for outputting most value types -->
<xsl:template name="simple-field-value">
 <xsl:param name="entry"/>
 <xsl:param name="field"/>

 <!-- if the field has multiple values, then there is
      no child of the entry with the field name -->
 <xsl:variable name="child" select="$entry/*[local-name(.)=$field]"/>
 <xsl:choose>
  <xsl:when test="$child">
   <xsl:variable name="f" select="$entry/../tc:fields/tc:field[@name = $field]"/>

   <!-- if the field is a bool type, just output an X, or use data image -->
   <xsl:choose>
    <!-- paragraphs need to have output escaping disabled so HTML works -->
    <xsl:when test="$f/@type=2">
     <xsl:value-of select="$child" disable-output-escaping="yes"/>
    </xsl:when>

    <xsl:when test="$f/@type=4">
     <img height="14" alt="&#x2713;">
      <xsl:attribute name="src">
       <xsl:value-of select="concat($datadir,'pics/checkmark.png')"/>
      </xsl:attribute>
     </img>
    </xsl:when>

    <!-- if it's a url, then add a hyperlink -->
    <xsl:when test="$f/@type=7">
     <a href="{$child}">
      <xsl:choose>
       <!-- The Amazon Web Services license requires the link -->
       <xsl:when test="$field = 'amazon'">
        <xsl:text>Buy from Amazon.com</xsl:text>
       </xsl:when>

       <!-- Requested by Giant Bomb API documentation -->
       <xsl:when test="$field = 'giantbomb'">
        <xsl:text>Find more information on Giant Bomb</xsl:text>
       </xsl:when>

       <xsl:otherwise>
        <xsl:call-template name="break-url-slash">
         <xsl:with-param name="url" select="$child"/>
        </xsl:call-template>
       </xsl:otherwise>
      </xsl:choose>
     </a>
    </xsl:when>

    <!-- if it's a date, format with hyphens -->
    <xsl:when test="$f/@type=12">
     <xsl:value-of select="$child/tc:year"/>
     <xsl:if test="$child/tc:month">
      <xsl:text>-</xsl:text>
      <xsl:value-of select="format-number($child/tc:month,'00')"/>
      <xsl:if test="$child/tc:day">
       <xsl:text>-</xsl:text>
       <xsl:value-of select="format-number($child/tc:day,'00')"/>
      </xsl:if>
     </xsl:if>
    </xsl:when>

    <!-- special case for rating -->
    <xsl:when test="$f/@type=14">
     <!-- get the number, could be 10, so can't just grab first digit -->
     <xsl:variable name="n">
      <xsl:choose>
       <xsl:when test="number(substring($child,1,1))">
        <xsl:choose>
         <xsl:when test="number(substring($child,1,2)) &lt; 11">
          <xsl:value-of select="number(substring($child,1,2))"/>
         </xsl:when>
         <xsl:otherwise>
          <xsl:value-of select="number(substring($child,1,1))"/>
         </xsl:otherwise>
        </xsl:choose>
       </xsl:when>
       <xsl:otherwise>
        <xsl:value-of select="false()"/>
       </xsl:otherwise>
      </xsl:choose>
     </xsl:variable>
     <xsl:if test="$n &gt; 0">
      <!-- the image is really 18 pixels high, but make it smaller to match default font -->
      <img height="14">
       <xsl:attribute name="src">
        <xsl:value-of select="concat($datadir,'pics/stars',$n,'.png')"/>
       </xsl:attribute>
       <xsl:attribute name="alt">
        <xsl:value-of select="concat($n,' stars')"/>
       </xsl:attribute>
      </img>
     </xsl:if>
     <xsl:if test="not($n)">
      <xsl:value-of select="$child"/>
     </xsl:if>
    </xsl:when>

    <xsl:otherwise>
     <xsl:value-of select="$child"/>
    </xsl:otherwise>
   </xsl:choose>
  </xsl:when>

  <!-- now handle fields with multiple values -->
  <xsl:otherwise>
   <xsl:for-each select="$entry/*[local-name()=concat($field,'s')]/*">
    <xsl:value-of select="."/>
    <xsl:if test="position() != last()">
     <xsl:text>; </xsl:text>
    </xsl:if>
   </xsl:for-each>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template name="image-link">
 <xsl:param name="image"/>
 <xsl:param name="dir"/>
 <xsl:variable name="id" select="$image/@id"/>
 <xsl:choose>
  <xsl:when test="$image/@link = 'true'">
   <!-- the id _is_ the link -->
   <xsl:value-of select="$id"/>
  </xsl:when>
  <xsl:when test="string-length($dir) &gt; 0">
   <xsl:value-of select="concat($dir, $id)"/>
  </xsl:when>
  <!-- otherwise try $imgdir and $datadir -->
  <xsl:when test="string-length($datadir) &gt; 0">
   <xsl:value-of select="concat($datadir, $id)"/>
  </xsl:when>
  <xsl:otherwise>
   <xsl:value-of select="concat($imgdir, $id)"/>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<!-- sums all nodes, assuming they are in MM:SS format -->
<xsl:template name="sumTime">
 <xsl:param name="nodes" select="/.."/>
 <xsl:param name="totalMin" select="0"/>
 <xsl:param name="totalSec" select="0"/>

 <xsl:choose>

  <xsl:when test="not($nodes)">
   <xsl:value-of select="$totalMin + floor($totalSec div 60)"/>
   <xsl:text>:</xsl:text>
   <xsl:value-of select="format-number($totalSec mod 60, '00')"/>
  </xsl:when>

  <xsl:when test="string-length($nodes[1]) &gt; 0">
   <xsl:variable name="min">
    <xsl:value-of select="substring-before($nodes[1], ':')"/>
   </xsl:variable>
   <xsl:variable name="sec">
    <xsl:value-of select="substring-after($nodes[1], ':')"/>
   </xsl:variable>
   <xsl:call-template name="sumTime">
    <xsl:with-param name="nodes" select="$nodes[position() != 1]"/>
    <xsl:with-param name="totalMin" select="$totalMin + $min"/>
    <xsl:with-param name="totalSec" select="$totalSec + $sec"/>
   </xsl:call-template>
  </xsl:when>

  <xsl:otherwise>
   <xsl:call-template name="sumTime">
    <xsl:with-param name="nodes" select="$nodes[position() != 1]"/>
    <xsl:with-param name="totalMin" select="$totalMin"/>
    <xsl:with-param name="totalSec" select="$totalSec"/>
   </xsl:call-template>
  </xsl:otherwise>

 </xsl:choose>
</xsl:template>

<xsl:template name="columnTitle">
 <xsl:param name="index" select="1"/>
 <xsl:param name="max" select="10"/>
 <xsl:param name="elem" select="'th'"/>
 <xsl:param name="style"/>
 <xsl:param name="field" select="/.."/>

 <xsl:if test="not($index &gt; $max)">
  <xsl:element name="{$elem}">
   <xsl:if test="string-length($style)">
    <xsl:attribute name="style">
     <xsl:value-of select="$style"/>
    </xsl:attribute>
   </xsl:if>
   <xsl:value-of select="$field/tc:prop[@name = concat('column', $index)]"/>
  </xsl:element>

  <xsl:call-template name="columnTitle">
   <xsl:with-param name="index" select="$index + 1"/>
   <xsl:with-param name="max" select="$max"/>
   <xsl:with-param name="elem" select="$elem"/>
   <xsl:with-param name="style" select="$style"/>
   <xsl:with-param name="field" select="$field"/>
  </xsl:call-template>
 </xsl:if>
</xsl:template>

<xsl:template name="sort-array">
 <xsl:param name="fields"/>
 <xsl:param name="columns"/>
 var COL_SORT_ARRAY = new Array()
 <xsl:for-each select="$columns">
  <xsl:variable name="column" select="."/>
  <xsl:variable name="field" select="$fields/tc:field[@name = $column]"/>
  <!-- number sorting is 1, date is 2, everything else is 0 -->
  <xsl:variable name="sort-type">
   <xsl:choose>
    <xsl:when test="$field/@type = 12">
     <xsl:text>2</xsl:text>
    </xsl:when>
    <xsl:when test="$field/@type = 6">
     <xsl:text>1</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:text>0</xsl:text>
    </xsl:otherwise>
   </xsl:choose>
  </xsl:variable>
  COL_SORT_ARRAY[<xsl:value-of select="position()-1"/>] = <xsl:value-of select="$sort-type"/>
 </xsl:for-each>
</xsl:template>

<!-- Output a full URL text that can line-break correctly -->
<xsl:template name="break-url-slash">
 <xsl:param name="url" />

 <xsl:variable name="first" select="substring-before($url, '/')" />
 <xsl:variable name="next" select="substring-after($url, '/')" />

 <xsl:choose>
  <xsl:when test="$first or $next">
   <xsl:call-template name="break-url-hyphen">
    <xsl:with-param name="url" select="$first" />
   </xsl:call-template>
   <xsl:text>/</xsl:text>
   <xsl:text disable-output-escaping="yes">&lt;wbr&gt;</xsl:text>
   <xsl:call-template name="break-url-slash">
    <xsl:with-param name="url" select="$next" />
   </xsl:call-template>
  </xsl:when>
  <xsl:otherwise>
   <xsl:call-template name="break-url-hyphen">
    <xsl:with-param name="url" select="$url" />
   </xsl:call-template>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template name="break-url-hyphen">
 <xsl:param name="url" />

 <xsl:variable name="first" select="substring-before($url, '-')" />
 <xsl:variable name="next" select="substring-after($url, '-')" />

 <xsl:choose>
  <xsl:when test="$first or $next">
   <xsl:value-of select="$first" />
   <xsl:text>-</xsl:text>
   <xsl:text disable-output-escaping="yes">&lt;wbr&gt;</xsl:text>
   <xsl:call-template name="break-url-hyphen">
    <xsl:with-param name="url" select="$next" />
   </xsl:call-template>
  </xsl:when>
  <xsl:otherwise>
   <xsl:value-of select="$url" />
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

</xsl:stylesheet>
