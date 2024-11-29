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
<xsl:param name="basedir"/> <!-- relative dir for template -->

<!-- Sort using user's preferred language -->
<xsl:param name="lang"/>

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
   .header {
        display: flex;
   }
   .box {
        flex: 1;
        display: flex;
        justify-content: center;
        align-items: top;
   }
   .header-left > span {
        margin-right: auto;
        font-size: 80%;
        font-style: italic;
   }
   .header-right > span {
        margin-left: auto;
        font-size: 80%;
        font-style: italic;
   }
   .header-center > h1 {
        margin: 0px;
        padding-bottom: 5px;
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
 <div class="header">
  <div class="box header-left"><span><xsl:value-of select="$filename"/></span></div>
  <div class="box header-center"><h1><xsl:value-of select="@title"/></h1></div>
  <div class="box header-right"><span><xsl:value-of select="$cdate"/></span></div>
 </div>

 <table>
  <tbody>

   <!-- first, build sorted list -->
   <xsl:variable name="sorted-entries">
    <xsl:for-each select="tc:entry">
     <xsl:sort lang="$lang" select=".//tc:title[1]"/>
     <xsl:copy-of select="."/>
    </xsl:for-each>
   </xsl:variable>

   <xsl:variable name="nrows"
                 select="ceiling(count(tc:entry) div $num-columns)"/>

   <!--
   <xsl:for-each select="exsl:node-set($sorted-entries)/tc:entry[position() mod $num-columns = 1]">
    <tr class="r{position() mod 2}">
     <xsl:apply-templates select=".|following-sibling::tc:entry[position() &lt; $num-columns]"/>
    </tr>
   </xsl:for-each>
-->
   <xsl:for-each select="exsl:node-set($sorted-entries)/tc:entry[position() &lt;= $nrows]">
    <xsl:variable name="idx" select="position()"/>
    <tr class="r{$idx mod 2}">
     <xsl:apply-templates select="."/>
     <xsl:for-each
   select="exsl:node-set($sorted-entries)/tc:entry[position() &gt;
   $nrows and position() mod $nrows = ($idx mod $nrows)]">
      <xsl:apply-templates select="."/>
     </xsl:for-each>
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
