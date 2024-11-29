<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                xmlns:dyn="http://exslt.org/dynamic"
                extension-element-prefixes="str dyn"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - Column View Report

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

<!-- To choose which fields of each entry are printed, change the
     string to a space separated list of field names. To know what
     fields are available, check the Tellico data file for <field>
     elements. -->
<xsl:param name="column-names" select="'title'"/>
<xsl:variable name="columns" select="str:tokenize($column-names)"/>

<!-- set the maximum image size -->
<xsl:param name="image-height" select="'100'"/>
<xsl:param name="image-width" select="'100'"/>

<!-- Up to three fields may be used for sorting. -->
<xsl:param name="sort-name1" select="'title'"/>
<xsl:param name="sort-name2" select="''"/>
<xsl:param name="sort-name3" select="''"/>
<!-- This is the title just beside the collection name. It will
     automatically list which fields are used for sorting. -->
<xsl:param name="sort-title" select="''"/>
<!-- Sort using user's preferred language -->
<xsl:param name="lang"/>

<xsl:param name="datadir"/> <!-- dir where Tellico data files are located -->
<xsl:param name="imgdir"/> <!-- dir where field images are located -->
<xsl:param name="basedir"/> <!-- relative dir for template -->

<xsl:key name="fieldsByName" match="tc:field" use="@name"/>
<xsl:key name="imagesById" match="tc:image" use="@id"/>

<!-- In case the field has multiple values, only sort by first one -->
<xsl:variable name="sort1">
 <xsl:if test="string-length($sort-name1) &gt; 0">
  <xsl:value-of select="concat('.//tc:', $sort-name1, '[1]')"/>
 </xsl:if>
</xsl:variable>

<xsl:variable name="sort1-type">
 <xsl:choose>
  <xsl:when test=".//tc:field[@name=$sort-name1]/@type = 6">number</xsl:when>
  <xsl:otherwise>text</xsl:otherwise>
 </xsl:choose>
</xsl:variable>

<xsl:variable name="sort2">
 <xsl:if test="string-length($sort-name2) &gt; 0">
  <xsl:value-of select="concat('.//tc:', $sort-name2, '[1]')"/>
 </xsl:if>
</xsl:variable>

<xsl:variable name="sort2-type">
 <xsl:choose>
  <xsl:when test=".//tc:field[@name=$sort-name2]/@type = 6">number</xsl:when>
  <xsl:otherwise>text</xsl:otherwise>
 </xsl:choose>
</xsl:variable>

<xsl:variable name="sort3">
 <xsl:if test="string-length($sort-name3) &gt; 0">
  <xsl:value-of select="concat('.//tc:', $sort-name3, '[1]')"/>
 </xsl:if>
</xsl:variable>

<xsl:variable name="sort3-type">
 <xsl:choose>
  <xsl:when test=".//tc:field[@name=$sort-name3]/@type = 6">number</xsl:when>
  <xsl:otherwise>text</xsl:otherwise>
 </xsl:choose>
</xsl:variable>

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
   th {
        color: #000;
        background-color: #ccc;
        border: 1px solid #999;
        font-size: 1.1em;
        font-weight: bold;
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
   <script type="text/javascript">
    <xsl:call-template name="sort-array">
     <xsl:with-param name="fields" select="tc:collection[1]/tc:fields"/>
     <xsl:with-param name="columns" select="$columns"/>
    </xsl:call-template>
   </script>
   <script type="text/javascript" src="../tellico2html.js"/>
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

 <table class="sortable">
  <!-- always print headers -->
  <thead>
   <tr>
    <xsl:variable name="fields" select="tc:fields"/>
    <xsl:for-each select="$columns">
     <th>
      <xsl:call-template name="field-title">
       <xsl:with-param name="fields" select="$fields"/>
       <xsl:with-param name="name" select="."/>
      </xsl:call-template>
     </th>
    </xsl:for-each>
   </tr>
  </thead>

  <tbody>
   <xsl:for-each select="tc:entry">
    <xsl:sort lang="$lang" select="dyn:evaluate($sort1)" data-type="{$sort1-type}"/>
    <xsl:sort lang="$lang" select="dyn:evaluate($sort2)" data-type="{$sort2-type}"/>
    <xsl:sort lang="$lang" select="dyn:evaluate($sort3)" data-type="{$sort3-type}"/>
    <tr class="r{position() mod 2}">
     <xsl:apply-templates select="."/>
    </tr>
   </xsl:for-each>
  </tbody>
 </table>
</xsl:template>

<xsl:template name="field-title">
 <xsl:param name="fields"/>
 <xsl:param name="name"/>
 <!-- remove namespace portion of qname -->
 <xsl:variable name="name-tokens" select="str:tokenize($name, ':')"/>
 <!-- the header is the title attribute of the field node whose
      name equals the column name -->
 <xsl:value-of select="$fields/tc:field[@name = $name-tokens[last()]]/@title"/>
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

       <!-- boolean and number values -->
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
