<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="str"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - Image List Report

   Copyright (C) 2005 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="../tellico-common.xsl"/>

<xsl:output method="html" version="xhtml" encoding="utf-8"/>

<!-- To choose which fields of each entry are printed, change the
     string to a space separated list of field names. To know what
     fields are available, check the Tellico data file for <field>
     elements. -->
<xsl:param name="column-names" select="'title'"/>
<xsl:variable name="columns" select="str:tokenize($column-names)"/>

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
   div.r0 {
        border: 1px inset #666;
        clear: left;
        padding: 10px 10px 0px 10px;
        background-color: #eee;
   }
   div.r1 {
        border: 1px inset #666;
        clear: left;
        padding: 10px 10px 0px 10px;
        background-color: #ddd;
   }
   img.float {
        float: left;
        margin-right: 10px;
        margin-bottom: 10px;
        border: 2px outset #ccc;
   }
   td.title {
       font-size: 1.4em;
       font-weight: bold;
   }
   td.fieldName {
        color: #666;
        vertical-align: top;
   }
   td.fieldValue {
        padding-left: 4px;
        font-weight: bold;
        vertical-align: top;
   }
   </style>
   <title>Tellico</title>
  </head>
  <body>
   <xsl:apply-templates select="tc:collection"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="tc:collection">
 <h1 class="colltitle">
  <xsl:value-of select="@title"/>
 </h1>

 <!-- find first image field -->
 <xsl:variable name="image-field" select="tc:fields/tc:field[@type=10][1]/@name"/>
 
 <xsl:for-each select="tc:entry">
  <xsl:sort select=".//tc:title[1]"/>
  <xsl:variable name="entry" select="."/>

  <div class="r{position() mod 2}">
   <xsl:variable name="id" select="./*[local-name() = $image-field]"/>
   <xsl:if test="$id">
    <img class="float">
     <xsl:attribute name="src">
      <xsl:value-of select="concat($imgdir, $id)"/>
     </xsl:attribute>
     <xsl:call-template name="image-size">
      <xsl:with-param name="limit-width" select="$image-width"/>
      <xsl:with-param name="limit-height" select="$image-height"/>
      <xsl:with-param name="image" select="key('imagesById', $id)"/>
     </xsl:call-template>
    </img>
   </xsl:if>
  
   <table>
    <thead>
     <tr>
      <td colspan="2" class="title">
       <xsl:value-of select=".//tc:title[1]"/>
      </td>
     </tr>
    </thead>

    <tbody>
     <!-- don't repeat title -->
     <xsl:for-each select="$columns[. != 'title']">
      <!-- no other images allowed -->
      <xsl:if test="$entry/../tc:fields/tc:field[@name = current()]/@type != 10">
       <xsl:call-template name="field-output">
        <xsl:with-param name="entry" select="$entry"/>
        <!-- can't use a key, the context is not the document -->
        <xsl:with-param name="field" select="$entry/../tc:fields/tc:field[@name = current()]"/>
       </xsl:call-template>
      </xsl:if>
     </xsl:for-each>
     
     <!-- add all paragraph fields, too -->
     <xsl:for-each select="../tc:fields/tc:field[@type = 2]">
      <xsl:call-template name="field-output">
       <xsl:with-param name="entry" select="$entry"/>
       <xsl:with-param name="field" select="."/>
      </xsl:call-template>
      
     </xsl:for-each>
    </tbody>
   </table>
   <div style="clear: left"/>
  </div>
 </xsl:for-each>
</xsl:template>

<xsl:template name="field-output">
 <xsl:param name="entry"/>
 <xsl:param name="field"/>
 <tr>
  <td class="fieldName">
   <!-- can't use key here, context is not in document -->
   <xsl:value-of select="$field/@title"/>
   <xsl:text>: </xsl:text>
  </td>

  <td class="fieldValue">
   <xsl:variable name="numvalues" select="count($entry//*[local-name() = $field/@name])"/>
   
   <xsl:choose>
    <xsl:when test="$numvalues &gt; 1">
     <xsl:call-template name="simple-field-value">
      <xsl:with-param name="entry" select="$entry"/>
      <xsl:with-param name="field" select="$field/@name"/>
     </xsl:call-template>
    </xsl:when>
    
    <xsl:when test="$numvalues = 1">
     <xsl:choose>
      
      <!-- boolean values end up as 'true', output 'X' -->
      <xsl:when test="$field/@type=4 and . = 'true'">
       <xsl:call-template name="simple-field-value">
        <xsl:with-param name="entry" select="$entry"/>
        <xsl:with-param name="field" select="$field/@name"/>
       </xsl:call-template>
      </xsl:when>
      
      <!-- handle URL here, so no link created -->
      <xsl:when test="$field/@type=7">
       <xsl:value-of select="."/>
      </xsl:when>

      <!-- finally, it's just a regular value -->
      <xsl:otherwise>
       <xsl:call-template name="simple-field-value">
        <xsl:with-param name="entry" select="$entry"/>
        <xsl:with-param name="field" select="$field/@name"/>
       </xsl:call-template>
      </xsl:otherwise>
     </xsl:choose>
    </xsl:when>

    <xsl:otherwise>
     <xsl:text> </xsl:text>
    </xsl:otherwise>
   </xsl:choose>
  </td>
 </tr>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
