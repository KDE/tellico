<?xml version="1.0"?>
<!-- WARNING: Tellico uses tc as the internal namespace declaration, and it must be identical here!! -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                extension-element-prefixes="str"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - Group View Report

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

<!-- To choose which fields of each entry are printed, change the
     string to a space separated list of field names. To know what
     fields are available, check the Tellico data file for <field>
     elements. -->
<xsl:param name="column-names" select="'title'"/>
<xsl:variable name="columns" select="str:tokenize($column-names)"/>

<!-- If you want the header row printed, showing which fields
     are printed, change this to true(), otherwise false() -->
<xsl:param name="show-headers" select="true()"/>

<!-- set the maximum image size -->
<xsl:param name="image-height" select="'100'"/>
<xsl:param name="image-width" select="'100'"/>

<xsl:param name="datadir"/> <!-- dir where Tellico data files are located -->
<xsl:param name="imgdir"/> <!-- dir where field images are located -->
<xsl:param name="basedir"/> <!-- relative dir for template -->

<xsl:key name="fieldsByName" match="tc:field" use="@name"/>
<xsl:key name="imagesById" match="tc:image" use="@id"/>
<xsl:key name="entriesById" match="tc:entry" use="@id"/>

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
        <xsl:if test="count($columns) &gt; 3">
        font-size: 80%;
        </xsl:if>
        background-color: #fff;
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
   td.groupName {
        margin-top: 10px;
        margin-bottom: 2px;
        padding-left: 4px;
        background: #ccc;
        font-size: 1.1em;
        font-weight: bolder;
   }
   td.fieldName {
        margin-top: 10px;
        margin-bottom: 2px;
        color: #666;
        background-color: #ccc;
        font-size: 1em;
        text-align: center;
        font-style: italic;
        padding-left: 4px;
        padding-right: 4px;
   }
   tr.r0 {
   }
   tr.r1 {
        background-color: #eee;
   }
   td.field {
        margin-left: 0px;
        margin-right: 0px;
        padding-left: 10px;
        padding-right: 5px;
        border: 1px solid #eee;
        text-align: left;
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
   <xsl:variable name="fields" select="tc:fields"/>
   <xsl:for-each select="tc:group">
    <xsl:sort lang="$lang" select="@title"/>
    <tr>
     <td class="groupName">
      <xsl:value-of select="@title"/>
     </td>
     <xsl:for-each select="$columns[position() &gt; 1]">
      <td class="fieldName">
       <xsl:call-template name="field-title">
        <xsl:with-param name="fields" select="$fields"/>
        <xsl:with-param name="name" select="."/>
       </xsl:call-template>
      </td>
     </xsl:for-each>
    </tr>

    <xsl:for-each select="key('entriesById', tc:entryRef/@id)">
     <tr class="r{position() mod 2}">
      <xsl:apply-templates select="."/>
     </tr>
    </xsl:for-each>
   </xsl:for-each>

  </tbody>
 </table>
</xsl:template>

<xsl:template name="field-title">
 <xsl:param name="fields"/>
 <xsl:param name="name"/>
 <xsl:variable name="name-tokens" select="str:tokenize($name, ':')"/>
 <!-- the header is the title field of the field node whose name equals the column name -->
 <xsl:choose>
  <xsl:when test="$fields">
   <xsl:value-of select="$fields/tc:field[@name = $name-tokens[last()]]/@title"/>
  </xsl:when>
  <xsl:otherwise>
   <xsl:value-of select="$name-tokens[last()]"/>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template match="tc:entry">
 <!-- stick all the descendants into a variable -->
 <xsl:variable name="current" select="descendant::*"/>
 <xsl:variable name="entry" select="."/>

 <xsl:for-each select="$columns">
  <xsl:variable name="column" select="."/>

  <!-- find all descendants whose name matches the column name -->
  <xsl:variable name="numvalues" select="count($current[local-name() = $column])"/>
  <!-- if the field node exists, output its value, otherwise put in a space -->
  <td class="field">
   <xsl:choose>
    <!-- when there is at least one value... -->
    <xsl:when test="$numvalues &gt; 1">
     <xsl:call-template name="simple-field-value">
      <xsl:with-param name="entry" select="$entry"/>
      <xsl:with-param name="field" select="$column"/>
     </xsl:call-template>
    </xsl:when>

    <xsl:when test="$numvalues = 1">
     <xsl:for-each select="$current[local-name() = $column]">

      <xsl:variable name="field" select="key('fieldsByName', $column)"/>
      <xsl:choose>

       <!-- boolean values end up as 'true', output 'X' -->
       <xsl:when test="$field/@type=4 or $field/@type=6">
        <xsl:attribute name="style">
         <xsl:text>text-align: center; padding-left: 5px</xsl:text>
        </xsl:attribute>
        <xsl:call-template name="simple-field-value">
         <xsl:with-param name="entry" select="$entry"/>
         <xsl:with-param name="field" select="$column"/>
        </xsl:call-template>
       </xsl:when>

       <!-- next, check for images -->
       <xsl:when test="$field/@type=10">
        <xsl:attribute name="style">
         <xsl:text>text-align: center; padding-left: 5px</xsl:text>
        </xsl:attribute>
        <img>
         <xsl:attribute name="src">
          <xsl:call-template name="image-link">
           <xsl:with-param name="image" select="key('imagesById', .)"/>
           <xsl:with-param name="dir" select="$imgdir"/>
          </xsl:call-template>
         </xsl:attribute>
         <xsl:call-template name="image-size">
          <xsl:with-param name="limit-width" select="$image-width"/>
          <xsl:with-param name="limit-height" select="$image-height"/>
          <xsl:with-param name="image" select="key('imagesById', .)"/>
         </xsl:call-template>
        </img>
       </xsl:when>

       <!-- if it's a date, format with hyphens -->
       <xsl:when test="$field/@type=12">
        <xsl:attribute name="style">
         <xsl:text>text-align: center; padding-left: 5px</xsl:text>
        </xsl:attribute>
        <xsl:call-template name="simple-field-value">
         <xsl:with-param name="entry" select="$entry"/>
         <xsl:with-param name="field" select="$column"/>
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
         <xsl:with-param name="field" select="$column"/>
        </xsl:call-template>
       </xsl:otherwise>

      </xsl:choose>
     </xsl:for-each>
    </xsl:when>
    <xsl:otherwise>
     <xsl:text> </xsl:text>
    </xsl:otherwise>
   </xsl:choose>
  </td>
 </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
