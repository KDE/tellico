<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="exsl"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - Title List Report

   Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="../tellico-common.xsl"/>

<xsl:output method="html"
            indent="yes"
            doctype-system="about:legacy-compat"
            encoding="utf-8"/>

<xsl:param name="filename"/>
<xsl:param name="cdate"/>

<!-- Sort using user's preferred language -->
<xsl:param name="lang"/>
<xsl:param name="basedir"/> <!-- relative dir for template -->

<xsl:param name="num-columns" select="3"/>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <html>
  <head>
   <base href="{$basedir}"/>
   <meta name="viewport" content="width=device-width, initial-scale=1"/>
   <style type="text/css">
   body {
        font-family: sans-serif;
        background-color: #fff;
        color: #000;
   }
   #header-left {
        margin-top: 0;
        float: left;
        font-size: 80%;
        font-style: italic;
   }
   #header-right {
        margin-top: 0;
        float: right;
        font-size: 80%;
        font-style: italic;
   }
   h1.colltitle {
        margin: 0px;
        padding-bottom: 5px;
        font-size: 2em;
        text-align: center;
   }
   table {
        margin-left: auto;
        margin-right: auto;
   }
   td {
        margin-left: 0px;
        margin-right: 0px;
        padding-left: 10px;
        padding-right: 5px;
        border: 1px solid #eee;
        text-align: left;
   }
   tr.r0 {
        background-color: #fff;
   }
   tr.r1 {
        background-color: #eee;
   }
   </style>
   <title>
    <xsl:value-of select="tc:collection/@title"/>
   </title>
  </head>
  <body>
   <xsl:apply-templates select="tc:collection"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="tc:collection">
 <p id="header-left"><xsl:value-of select="$filename"/></p>
 <p id="header-right"><xsl:value-of select="$cdate"/></p>
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

 <table>
  <tbody>

<!--
   <xsl:variable name="nrows"
                 select="ceiling(count(tc:entry) div $num-columns)"/>
-->
   <xsl:for-each select="exsl:node-set($sorted-entries)/tc:entry[position() mod $num-columns = 1]">
    <tr class="r{position() mod 2}">
     <xsl:apply-templates select=".|following-sibling::tc:entry[position() &lt; $num-columns]"/>
    </tr>
   </xsl:for-each>

  </tbody>
 </table>

</xsl:template>

<xsl:template match="tc:entry">
 <td>
  <xsl:for-each select=".//tc:title">
   <xsl:value-of select="."/>
   <xsl:if test="position() &lt; last()">
    <xsl:text>; </xsl:text>
    <br/>
   </xsl:if>
  </xsl:for-each>
</td>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
