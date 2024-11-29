<?xml version="1.0"?>
<!-- WARNING: Tellico uses tc as the internal namespace declaration, and it must be identical here!! -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:exsl="http://exslt.org/common"
                xmlns:dyn="http://exslt.org/dynamic"
                extension-element-prefixes="exsl dyn"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - Collection Summary Report

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

<xsl:variable name="limit" select="5"/>

<xsl:key name="fieldsByName" match="tc:field" use="@name"/>

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
   div.field {
        margin-left: auto;
        margin-right: auto;
        margin-bottom: 20px;
        width: 750px;
        background-color: #ddd;
        border: solid 1px #999;
        overflow: auto;
        padding: 0px 0px 0px 0px;
   }
   h2 {
        font-size: 125%;
        border-bottom: solid 1px #999;
        text-align: center;
        margin-top: 5px;
        margin-bottom: 0px;
   }
   h3 {
        font-size: 0.9em;
        color: #666;
        text-align: right;
        padding-right: 4px;
        margin-top: -1.1em;
        margin-bottom: 0px;
   }
   h4 {
        text-align: center;
   }
   table {
        margin-left: auto;
        margin-right: auto;
        padding: 10px 0px 10px 0px;
   }
   div.row {
        margin: 0px 0px 0px 0px;
        padding: 0px 0px 0px 0px;
        border: solid 1px transparent;
        clear: left;
        overflow: auto;
        line-height: 120%;
   }
   div.group {
        text-align: left;
        margin: 0px 0px 0px 0px;
        padding: 0px 10px 0px 4px;
        float: left;
        min-height: 1em;
   }
   span.bar {
        width: 590px;
        margin: 1px 2px 1px 150px;
        position: absolute;
        float: left;
        border-left: solid 5px #ddd; /* padding of a sort */
   }
   span.bar span {
        border-top: outset 1px #669;
        border-right: outset 2px #003;
        border-bottom: outset 2px #003;
        border-left: outset 2px #669;
        float: left;
        position: absolute;
        background-color: #336;
        color: white;
        padding-right: 4px;
        padding-bottom: 2px;
        font-size: 0.9em;
        line-height: 0.9em;
        text-align: right;
        font-style: italic;
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
  <div class="box header-center"><h1>
   <xsl:value-of select="@title"/>
   <i18n>: Group Summary</i18n></h1>
  </div>
  <div class="box header-right"><span><xsl:value-of select="$cdate"/></span></div>
 </div>

 <xsl:variable name="coll" select="."/>

 <table>
  <tbody>
   <tr>
    <th><i18n>Total number of fields:</i18n></th>
    <th style="padding-right: 50px; color: #006;">
     <xsl:value-of select="count(tc:fields/tc:field)"/>
    </th>
    <th><i18n>Total number of entries:</i18n></th>
    <th style="color:#006;">
     <xsl:value-of select="count(tc:entry)"/>
    </th>
   </tr>
  </tbody>
 </table>
 <!-- grouping flag is second bit from right -->
 <xsl:for-each select="tc:fields/tc:field[boolean(floor(@flags div 2) mod 2)]">
  <xsl:call-template name="output-group">
   <xsl:with-param name="coll" select="$coll"/>
   <xsl:with-param name="field" select="."/>
  </xsl:call-template>
 </xsl:for-each>

 <!--
 <h4><a href="https://tellico-project.org"><i18n>Generated by Tellico</i18n></a></h4>
-->
</xsl:template>

<xsl:template name="output-group">
 <xsl:param name="coll"/>
 <xsl:param name="field"/>

 <xsl:variable name="fieldname" select="$field/@name"/>
 <xsl:variable name="value-expr">
  <xsl:for-each select="$coll/tc:entry">
   <xsl:choose>
    <!-- tables -->
    <xsl:when test="$field/@type=8">
     <xsl:for-each select="./*/*[local-name() = $fieldname]">
      <value>
       <xsl:value-of select="tc:column[1]"/>
      </value>
     </xsl:for-each>
    </xsl:when>
    <!-- multiple values...could also use "./*/*[local-name() = $fieldname]" -->
    <xsl:when test="boolean(floor(key('fieldsByName', $fieldname)/@flags mod 2))">
     <xsl:for-each select="./*/*[local-name() = $fieldname]">
      <value>
       <xsl:value-of select="."/>
      </value>
     </xsl:for-each>
    </xsl:when>
    <xsl:otherwise>
     <value>
      <xsl:call-template name="simple-field-value">
       <xsl:with-param name="entry" select="."/>
       <xsl:with-param name="field" select="$fieldname"/>
     </xsl:call-template>
     </value>
    </xsl:otherwise>
   </xsl:choose>
  </xsl:for-each>
 </xsl:variable>

 <xsl:variable name="values" select="exsl:node-set($value-expr)/value"/>
 <xsl:variable name="listing">
  <xsl:for-each select="$values[not(. = preceding-sibling::*)]">
   <!-- speed sorting by ignoring groups with a count of 1 -->
   <xsl:variable name="c" select="count($values[. = current()])"/>
   <xsl:if test="$c &gt; 1">
    <group>
     <xsl:attribute name="name">
      <xsl:value-of select="."/>
     </xsl:attribute>
     <xsl:attribute name="count">
      <xsl:value-of select="$c"/>
     </xsl:attribute>
    </group>
   </xsl:if>
  </xsl:for-each>
 </xsl:variable>

 <xsl:variable name="groups" select="exsl:node-set($listing)/group"/>
 <xsl:variable name="total" select="count($groups)"/>

 <xsl:variable name="max">
  <xsl:call-template name="max-count">
   <xsl:with-param name="nodes" select="$groups"/>
  </xsl:call-template>
 </xsl:variable>

 <xsl:if test="$total &gt; 2">

  <div class="field">
   <h2>
    <xsl:value-of select="key('fieldsByName', $fieldname)/@title"/>
   </h2>

   <xsl:for-each select="$groups">
    <xsl:sort lang="$lang" select="@count" data-type="number" order="descending" />
    <xsl:sort lang="$lang" select="@name"/>
    <xsl:if test="position() &lt;= $limit">
     <div class="row">
      <div class="group">
       <xsl:value-of select="@name"/>
      </div>
      <span class="bar">
       <span>
        <xsl:attribute name="style">
         <xsl:text>padding-left:</xsl:text>
         <!-- the 540 is rather arbitrarily dependent on font-size, seem to work -->
         <xsl:value-of select="floor(540 * @count div $max)"/>
         <xsl:text>px;</xsl:text>
        </xsl:attribute>
        (<xsl:value-of select="@count"/>)
       </span>
      </span>
     </div>
    </xsl:if>
   </xsl:for-each>

   <h3>
    <i18n>Distinct values: </i18n><xsl:value-of select="$total"/>
   </h3>
  </div>

 </xsl:if>
</xsl:template>

<xsl:template name="max-count">
 <xsl:param name="nodes" select="/.."/>
 <xsl:param name="max"/>

 <xsl:variable name="count" select="count($nodes)"/>
 <xsl:variable name="aNode" select="$nodes[ceiling($count div 2)]"/>

 <xsl:choose>
  <xsl:when test="$count = 0">
   <xsl:value-of select="number($max)"/>
  </xsl:when>

  <xsl:otherwise>
   <xsl:call-template name="max-count">

    <xsl:with-param name="nodes" select="$nodes[not(@count &lt;= number($aNode/@count))]"/>
    <xsl:with-param name="max">
     <xsl:choose>
      <xsl:when test="not($max) or $aNode/@count &gt; $max">
       <xsl:value-of select="$aNode/@count"/>
      </xsl:when>
      <xsl:otherwise>
       <xsl:value-of select="$max"/>
      </xsl:otherwise>
     </xsl:choose>
    </xsl:with-param>
   </xsl:call-template>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
