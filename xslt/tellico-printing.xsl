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
   Tellico XSLT file - used for printing

   Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   The exslt extensions from http://www.exslt.org are required.
   Specifically, the string and dynamic modules are used. For
   libxslt, that means the minimum version is 1.0.19.

   This is a horribly messy stylesheet. I would REALLY welcome any
   recommendations in improving its efficiency.

   There may be problems if this stylesheet is used to transform the
   actual Tellico data file, since the application re-arranges the
   DOM for printing. THe primary change is the addition of group
   elements to speed up grouped printing.

   Any version of this file in the user's data directory, such as
     ~/.local/share/tellico/, will override the system file.
   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="tellico-common.xsl"/>

<xsl:output method="html"
            indent="yes"
            doctype-public="-//W3C//DTD HTML 4.01//EN"
            doctype-system="http://www.w3.org/TR/html4/strict.dtd"
            encoding="utf-8"/>

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
<xsl:param name="image-height" select="50"/>
<xsl:param name="image-width" select="50"/>

<!-- This is the title just beside the collection name. It will
     automatically list which fields are used for sorting. -->
<xsl:param name="sort-title" select="''"/>

<!--
   ===================================================================
   The only thing below here that you might want to change is the CSS
   governing the appearance of the output HTML.
   ===================================================================
-->

<!-- The page-title is used for the HTML title -->
<xsl:param name="page-title" select="'Tellico'"/>
<xsl:param name="imgdir"/> <!-- dir where field images are located -->

<xsl:key name="fieldsByName" match="tc:field" use="@name"/>
<xsl:key name="entriesById" match="tc:entry" use="@id"/>
<xsl:key name="imagesById" match="tc:image" use="@id"/>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <!-- This stylesheet is designed for Tellico document syntax version 12 -->
 <xsl:call-template name="syntax-version">
  <xsl:with-param name="this-version" select="'12'"/>
  <xsl:with-param name="data-version" select="@syntaxVersion"/>
 </xsl:call-template>

 <html>
  <head>
   <style type="text/css">
   body {
        font-family: sans-serif;
        <xsl:if test="count($columns) &gt; 3">
        font-size: 80%;
        </xsl:if>
        background-color: #fff;
   }
   #headerblock {
        padding-top: 10px;
        padding-bottom: 10px;
        width: 100%;
        text-align: center;
   }
   div.colltitle {
        padding: 4px;
        font-size: 2em;
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
   th {
        color: #000;
        background-color: #ccc;
        border: 1px solid #999;
        font-size: 1.1em;
        font-weight: bold;
        padding-left: 4px;
        padding-right: 4px;
   }
   tr.entry1 {
   }
   tr.entry2 {
        background-color: #eee;
   }
   tr.groupEntry1 {
   }
   tr.groupEntry2 {
        background-color: #eee;
   }
   td.field {
        margin-left: 0px;
        margin-right: 0px;
        padding-left: 5px;
        padding-right: 5px;
        border: 1px solid #eee;
        text-align: left;
   }
   </style>
   <title>
    <xsl:value-of select="$page-title"/>
   </title>
  </head>
  <body>
   <xsl:apply-templates select="tc:collection"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="tc:collection">
 <div id="headerblock">
  <div class="colltitle">
   <xsl:value-of select="@title"/>
  </div>
  <div class="sortTitle">
   <xsl:value-of select="$sort-title"/>
  </div>
 </div>

 <table>

  <xsl:if test="$show-headers">
   <xsl:variable name="fields" select="tc:fields"/>
   <thead>
    <tr>
     <xsl:for-each select="$columns">
      <xsl:variable name="column" select="."/>
      <th>
       <xsl:call-template name="field-title">
        <xsl:with-param name="fields" select="$fields"/>
        <xsl:with-param name="name" select="$column"/>
       </xsl:call-template>
      </th>
     </xsl:for-each>
    </tr>
   </thead>
  </xsl:if>

  <tbody>

   <!-- If the entries are not being grouped, it's easy -->
   <xsl:if test="not(tc:group)">
    <xsl:for-each select="tc:entry">
     <tr>
      <xsl:choose>
       <xsl:when test="position() mod 2 = 0">
        <xsl:attribute name="class">
         <xsl:text>entry1</xsl:text>
        </xsl:attribute>
       </xsl:when>
       <xsl:otherwise>
        <xsl:attribute name="class">
         <xsl:text>entry2</xsl:text>
        </xsl:attribute>
       </xsl:otherwise>
      </xsl:choose>
      <xsl:apply-templates select="."/>
     </tr>
    </xsl:for-each>
   </xsl:if> <!-- end ungrouped output -->

   <!-- If the entries are being grouped, it's a bit more involved -->
   <xsl:if test="tc:group">
    <xsl:variable name="coll" select="."/>
    <xsl:for-each select="tc:group">
     <tr>
      <td class="groupName">
       <xsl:attribute name="colspan">
        <xsl:value-of select="count($columns)"/>
       </xsl:attribute>
       <xsl:value-of select="@title"/>
      </td>
     </tr>
     <xsl:for-each select="tc:entryRef">
      <tr>
       <xsl:choose>
        <xsl:when test="position() mod 2 = 0">
         <xsl:attribute name="class">
          <xsl:text>groupEntry1</xsl:text>
         </xsl:attribute>
        </xsl:when>
        <xsl:otherwise>
         <xsl:attribute name="class">
          <xsl:text>groupEntry2</xsl:text>
         </xsl:attribute>
        </xsl:otherwise>
       </xsl:choose>
       <xsl:apply-templates select="key('entriesById', @id)"/>
      </tr>
     </xsl:for-each>
    </xsl:for-each>
   </xsl:if>

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
 <xsl:variable name="entry" select="."/>
 <!-- stick all the descendants into a variable -->
 <xsl:variable name="current" select="descendant::*"/>
 <xsl:for-each select="$columns">
  <xsl:variable name="column" select="."/>
  <!-- find all descendants whose name matches the column name -->
  <xsl:variable name="numvalues" select="count($current[local-name() = $column])"/>
  <!-- if the field node exists, output its value, otherwise put in a space -->
  <td class="field">
   <!-- first column should not be centered -->
   <!-- nor should rating images be centered -->
   <xsl:if test="position()=1 or $entry/../tc:fields/tc:field[@name=$column]/@type=14">
    <xsl:attribute name="style">
     <xsl:text>text-align: left; padding-left: 10px</xsl:text>
    </xsl:attribute>
   </xsl:if>
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

       <!-- check for multi-column table -->
       <xsl:when test="$field/@type=8 and $field/tc:prop[@name = 'columns'] &gt; 1">
        <!-- italicize all columns after the first -->
        <xsl:value-of select="tc:column[1]"/>
        <xsl:for-each select="tc:column[position &gt; 1]">
         <xsl:text> - </xsl:text>
         <em>
          <xsl:value-of select="."/>
         </em>
        </xsl:for-each>
        <br/>
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
        <xsl:value-of select="tc:year"/>
        <xsl:text>-</xsl:text>
        <xsl:value-of select="format-number(tc:month,'00')"/>
        <xsl:text>-</xsl:text>
        <xsl:value-of select="format-number(tc:day,'00')"/>
       </xsl:when>

       <!-- paragraphs need to have output escaping disabled so HTML works -->
       <xsl:when test="$field/@type=2">
        <xsl:value-of select="." disable-output-escaping="yes"/>
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
