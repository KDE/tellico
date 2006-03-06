<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="exsl"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - Tri-Column Report
                       Modified from VideoDB

   Copyright (C) 2005-2006 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="../tellico-common.xsl"/>

<xsl:output method="html" version="xhtml" encoding="utf-8"/>

<!-- Sort using user's preferred language -->
<xsl:param name="lang" select="'en'"/>

<xsl:variable name="image-field" select="tc:tellico/tc:collection[1]/tc:fields/tc:field[@type=10][1]/@name"/>

<!-- set the maximum image size -->
<xsl:param name="image-height" select="'150'"/>
<xsl:param name="image-width"  select="'150'"/>

<xsl:param name="datadir"/> <!-- dir where Tellico data files are located -->
<xsl:param name="imgdir"/> <!-- dir where field images are located -->

<xsl:key name="imagesById" match="tc:image" use="@id"/>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <html>
  <head>
   <style type="text/css">
   body {
        font-family: sans-serif;
        background-color: #fff;
        color: #000;
   }
   h1.colltitle {
        margin: 0px;
        padding-bottom: 5px;
        font-size: 2em;
        text-align: center;
  }
   tr.r0 {
        border: 1px inset #666;
        background-color: #eee;
   }
   tr.r1 {
        border: 1px inset #666;
        background-color: #ddd;
   }
   img.float {
        float: left;
        margin-right: 10px;
        margin-bottom: 10px;
        border: 2px outset #ccc;
   }
   td {
        vertical-align: top;
   }
   span.title {
       font-size: 1.2em;
       font-weight: bold;
   }
   span.info {
       font-size: 1.1em;
       font-style italic;
       color: #006;
   }
   span.plot {
       font-size: 1em;
   }
   </style>
   <title>Tellico</title>
  </head>
  <body>
   <xsl:apply-templates select="tc:collection"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="tc:collection[@type!=3]">
 <h1 class="colltitle">
  <i18n>This template is meant for video collections only.</i18n>
 </h1>
</xsl:template>

<xsl:template match="tc:collection[@type=3]">
 <h1 class="colltitle">
  <xsl:value-of select="@title"/>
 </h1>
 
 <!-- first, build sorted list -->
 <xsl:variable name="sorted-entries">
  <xsl:for-each select="tc:entry">
   <xsl:sort lang="$lang" select=".//tc:title[1]"/>
   <xsl:copy-of select="."/>
  </xsl:for-each>
 </xsl:variable>
 
 <!-- needed for key context -->
 <xsl:variable name="coll" select="."/>
 
 <table class="tablelist" cellpadding="0" cellspacing="0" width="100%">
  <tbody>
   <!-- three columns -->
   <!-- have to pass in image width and height because
        context changes inside the exsl:node-set -->
   <xsl:variable name="entries" select="exsl:node-set($sorted-entries)/tc:entry"/>
   
   <xsl:for-each select="$entries[position() mod 3 = 1]">
    <xsl:variable name="e1" select="."/>
    <xsl:variable name="e2" select="$e1/following-sibling::tc:entry[position() = 1]"/>
    <xsl:variable name="e3" select="$e2/following-sibling::tc:entry[position() = 1]"/>
    
    <tr class="r{position() mod 2}">
     
     <!-- switch context back to document -->
     <xsl:for-each select="$coll">
      
      <td align="left" width="33%">
       <xsl:apply-templates select="$e1">
        <xsl:with-param name="img" select="key('imagesById', $e1/*[local-name() = $image-field])"/>
       </xsl:apply-templates>
      </td>
      <td align="left" width="34%">
       <xsl:apply-templates select="$e2">
        <xsl:with-param name="img" select="key('imagesById', $e2/*[local-name() = $image-field])"/>
       </xsl:apply-templates>
      </td>
      <td align="left" width="33%">
       <xsl:apply-templates select="$e3">
        <xsl:with-param name="img" select="key('imagesById', $e3/*[local-name() = $image-field])"/>
       </xsl:apply-templates>
      </td>
     </xsl:for-each>
    </tr>
   </xsl:for-each>
   
  </tbody>
 </table>
 
</xsl:template>

<xsl:template match="tc:entry">
 <xsl:param name="img"/>
 <table>
  <tbody>
   <tr>
    <td>
     <xsl:variable name="id" select="./*[local-name() = $image-field]"/>
     <xsl:if test="$id">
      <img class="float">
       <xsl:attribute name="src">
        <xsl:value-of select="concat($imgdir, $id)"/>
       </xsl:attribute>
       <xsl:call-template name="image-size">
        <xsl:with-param name="limit-width" select="$image-width"/>
        <xsl:with-param name="limit-height" select="$image-height"/>
        <xsl:with-param name="image" select="$img"/>
       </xsl:call-template>
      </img>
     </xsl:if>
    </td>
    <td width="100%">
     <span class="title">
      <xsl:value-of select=".//tc:title[1]"/>
     </span>
     <br/>
     <span class="info">
      <xsl:text>[</xsl:text>
      <xsl:value-of select=".//tc:year[1]"/>
      <xsl:text>; </xsl:text>
      <xsl:value-of select=".//tc:director[1]"/>
      <xsl:text>]</xsl:text>
     </span>
     <br/>
     <span class="plot">
      <xsl:value-of select="normalize-space(substring(./tc:plot, 1, 150))" disable-output-escaping="yes"/>
      <xsl:text>&#xa0;&#x2026;</xsl:text>
     </span>
    </td>
   </tr>
  </tbody>
 </table>
</xsl:template>

</xsl:stylesheet>

<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
