<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:bc="http://periapsis.org/bookcase/"
                xmlns:str="http://exslt.org/strings"
                xmlns:dyn="http://exslt.org/dynamic"
                extension-element-prefixes="str dyn"
                exclude-result-prefixes="bc"
                version="1.0">

<!--
   ===================================================================
   Bookcase XSLT file - used for exporting to HTML

   $Id: bookcase2html.xsl 613 2004-04-17 18:07:28Z robby $

   Copyright (C) 2003, 2004 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Bookcase'
   application, which can be found at http://www.periapsis.org/bookcase/

   The exslt extensions from http://www.exslt.org are required.
   Specifically, the string and dynamic modules are used. For
   libxslt, that means the minimum version is 1.0.19.

   This is a horribly messy stylesheet. I would REALLY welcome any
   recommendations in improving its efficiency.

   There may be problems if this stylesheet is used to transform the
   actual Bookcase data file, since the application re-arranges the
   DOM for printing.

   Customize this file in order to print different columns of
   fields for each entry. Any version of this file in the user's
   KDE home directory, such as $KDEHOME/share/apps/bookcase/, will
   override the system file.
   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="bookcase-common.xsl"/>

<xsl:output method="html" version="xhtml" encoding="utf-8"/>

<xsl:strip-space elements="*"/>

<!-- To choose which fields of each entry are printed, change the
     string to a space separated list of field names. To know what
     fields are available, check the Bookcase data file for <field>
     elements. -->
<xsl:param name="column-names" select="'title'"/>
<xsl:variable name="columns" select="str:tokenize($column-names)"/>

<!-- If you want the header row printed, showing which fields
     are printed, change this to true(), otherwise false() -->
<xsl:param name="show-headers" select="true()"/>

<!-- The entries may be grouped by a certain field. Keys are needed
     for both the entries and the grouped field values -->
<xsl:param name="group-entries" select="false()"/>

<!-- set the maximum image size -->
<xsl:param name="image-height" select="'50'"/>
<xsl:param name="image-width" select="'50'"/>

<!-- DO NOT CHANGE THE NAME OF THESE KEYS -->
<xsl:key name="entries" match="bc:entry" use=".//bc:author"/>
<xsl:key name="groups" match="bc:author" use="."/>
<xsl:variable name="all-groups" select="//bc:author"/>

<!-- Up to three fields may be used for sorting. -->
<xsl:param name="sort-name1" select="'title'"/>
<xsl:param name="sort-name2" select="''"/>
<xsl:param name="sort-name3" select="''"/>
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
<xsl:param name="page-title" select="'Bookcase'"/>

<xsl:param name="imgdir"/> <!-- dir where field images are located -->
<xsl:param name="link-entries" select="false()"/> <!-- link entries -->

<xsl:variable name="sort1">
 <xsl:if test="string-length($sort-name1) &gt; 0">
  <xsl:value-of select="concat('.//bc:', $sort-name1, '[1]')"/>
 </xsl:if>
</xsl:variable>
<xsl:variable name="sort2">
 <xsl:if test="string-length($sort-name2) &gt; 0">
  <xsl:value-of select="concat('.//bc:', $sort-name2, '[1]')"/>
 </xsl:if>
</xsl:variable>
<xsl:variable name="sort3">
 <xsl:if test="string-length($sort-name3) &gt; 0">
  <xsl:value-of select="concat('.//bc:', $sort-name3, '[1]')"/>
 </xsl:if>
</xsl:variable>

<xsl:key name="fieldsByName" match="bc:field" use="@name"/>
<xsl:key name="imagesById" match="bc:image" use="@id"/>

<!-- filename conversion is weird, need a variable for easy replacement -->
<xsl:variable name="weird">&apos;&quot;</xsl:variable>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<xsl:template match="/">
 <xsl:apply-templates select="bc:bookcase"/>
</xsl:template>

<xsl:template match="bc:bookcase">
 <!-- This stylesheet is designed for Bookcase document syntax version 5 -->
 <xsl:call-template name="syntax-version">
  <xsl:with-param name="this-version" select="'5'"/>
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
   }     
   #headerblock {
        padding-top: 10px;
        padding-bottom: 10px;
        margin-bottom: 5px;
   }
   div.colltitle {
        padding: 4px;
        font-size: 2em;
        border-bottom: 1px solid black;
   }
   span.subtitle {
        margin-left: 20px;
        font-size: 0.8em;
   }
   td.groupName {
        margin-top: 10px;
        margin-bottom: 2px;
        background: #eee;
        font-size: 1.2em;
        font-weight: bolder;
   }
   tr.header {
        background-color: #ccc;
        font-weight: bolder;
        font-size: 1.1em;
   }
   th {
        padding-left: 1em;
        padding-right: 1em;
   }
   tr.entry1 {
   }
   tr.entry2 {
        background-color: #eee;
   }
   tr.groupEntry {
        padding-left: 20px;
   }
   td.field {
        margin-left: 0px;
        margin-right: 0px;
        padding-left: 5px;
        padding-right: 5px;
   }
   </style>
   <title>
    <xsl:value-of select="$page-title"/>
   </title>
  </head>
  <body>
   <xsl:apply-templates select="bc:collection"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="bc:collection">
 <div id="headerblock">
  <div class="colltitle">
   <xsl:value-of select="@title"/>
    <span class="subtitle">
     <xsl:choose>
      <xsl:when test="string-length($sort-title) &gt; 0">
       <xsl:value-of select="$sort-title"/>
      </xsl:when>
      <xsl:otherwise>
       <xsl:text>(sorted by </xsl:text>
       <xsl:if test="string-length($sort-name1) &gt; 0">
        <xsl:call-template name="field-title">
         <xsl:with-param name="fields" select="bc:fields"/>
         <xsl:with-param name="name" select="$sort-name1"/>
        </xsl:call-template>
       </xsl:if>
       <xsl:if test="string-length($sort-name2) &gt; 0">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="field-title">
         <xsl:with-param name="fields" select="bc:fields"/>
         <xsl:with-param name="name" select="$sort-name2"/>
        </xsl:call-template>
       </xsl:if>
       <xsl:if test="string-length($sort-name3) &gt; 0">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="field-title">
         <xsl:with-param name="fields" select="bc:fields"/>
         <xsl:with-param name="name" select="$sort-name3"/>
        </xsl:call-template>
       </xsl:if>
       <xsl:text>)</xsl:text>
      </xsl:otherwise>
     </xsl:choose>
    </span>
  </div>
 </div>

 <table class="entries">

  <xsl:if test="$show-headers">
   <xsl:variable name="fields" select="bc:fields"/>
   <thead>
    <tr class="header">
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
   <xsl:if test="not($group-entries)">
    <xsl:for-each select="bc:entry">
     <!-- In case the field has multiple values, only sort by first one -->
     <xsl:sort select="dyn:evaluate($sort1)"/>
     <xsl:sort select="dyn:evaluate($sort2)"/>
     <xsl:sort select="dyn:evaluate($sort3)"/>
     <tr>
      <xsl:choose>
       <xsl:when test="position() mod 2 = 1">
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
   <xsl:if test="$group-entries">
    <!-- first loop through unique groups -->
    <xsl:for-each select="$all-groups[generate-id(.)=generate-id(key('groups', .)[1])]">
     <xsl:sort select="."/>
     <tr>
      <td class="groupName">
       <xsl:attribute name="colspan">
        <xsl:value-of select="count($columns)"/>
       </xsl:attribute>
       <xsl:choose>
        <xsl:when test="count(bc:column) &gt; 1">
         <!-- just output first column -->
         <xsl:value-of select="bc:column[1]"/>
        </xsl:when>
        <xsl:otherwise>
         <xsl:value-of select="."/>
        </xsl:otherwise>
       </xsl:choose>
      </td>
     </tr>
     <!-- now loop through every entry with this group value -->
     <xsl:for-each select="key('entries', .)">
      <!-- In case the field has multiple values, only sort by first one -->
      <xsl:sort select="dyn:evaluate($sort1)"/>
      <xsl:sort select="dyn:evaluate($sort2)"/>
      <xsl:sort select="dyn:evaluate($sort3)"/>
      <tr class="groupEntry">
       <xsl:apply-templates select="."/>
      </tr>
     </xsl:for-each>
    </xsl:for-each>
   </xsl:if>

  </tbody>
 </table>

 <hr/>
 <h4>Generated by <a href="http://www.periapsis.org/bookcase/">Bookcase</a>.</h4>
</xsl:template>

<xsl:template name="field-title">
 <xsl:param name="fields"/>
 <xsl:param name="name"/>
 <xsl:variable name="name-tokens" select="str:tokenize($name, ':')"/>
 <!-- the header is the title field of the field node whose name equals the column name -->
 <xsl:choose>
  <xsl:when test="$fields">
   <xsl:value-of select="$fields/bc:field[@name = $name-tokens[last()]]/@title"/>
  </xsl:when>
  <xsl:otherwise>
   <xsl:value-of select="$name-tokens[last()]"/>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template name="filename">
 <xsl:param name="entry"/>
 <xsl:variable name="bad-chars">
  <xsl:value-of select="translate($entry/bc:title,
                        'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-',
                        '')"/>
 </xsl:variable>
 <xsl:variable name="name">
  <!-- there should be at least as many underscores as bad characters -->
  <xsl:value-of select="translate($entry/bc:title,
                                  concat($bad-chars, $weird),
                                  '_________________________________________________________________________________')"/>
 </xsl:variable>
 <xsl:value-of select="concat($name, '-', $entry/@id, '.html')"/>
</xsl:template>

<xsl:template match="bc:entry">
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
    <xsl:when test="$numvalues &gt; 0">
     <xsl:for-each select="$current[local-name() = $column]">
      <xsl:variable name="field" select="key('fieldsByName', local-name())"/>
      
      <xsl:choose>

       <!-- boolean values end up as 'true', output 'X' --> 
       <xsl:when test="$field/@type=4 and . = 'true'">
        <xsl:text>X</xsl:text>
       </xsl:when>

       <!-- next, check for 2-column table -->
       <xsl:when test="$field/@type=9">
        <!-- italicize second column -->
        <xsl:value-of select="bc:column[1]"/>
        <xsl:text> - </xsl:text>
        <em>
         <xsl:value-of select="bc:column[2]"/>
        </em>
        <br/>
       </xsl:when>

       <!-- next, check for images -->
       <xsl:when test="$field/@type=10">
        <!-- text align center of the td -->
        <xsl:attribute name="style">text-align: center;</xsl:attribute>
        <img>
         <xsl:attribute name="src">
          <xsl:value-of select="concat($imgdir, .)"/>
         </xsl:attribute>
         <xsl:call-template name="image-size">
          <xsl:with-param name="limit-width" select="$image-width"/>
          <xsl:with-param name="limit-height" select="$image-height"/>
          <xsl:with-param name="image" select="key('imagesById', .)"/>
         </xsl:call-template>
        </img>
       </xsl:when>
       
       <!-- next, check for URLs -->
       <xsl:when test="$field/@type = 7">
        <a href="{.}">
         <xsl:value-of select="."/>
        </a>
       </xsl:when>

       <!-- finally, it's just a regular value -->
       <xsl:otherwise>
        <xsl:choose>
         <xsl:when test="$field/@name = 'title' and $link-entries">
          <a>
           <xsl:attribute name="href">
            <xsl:call-template name="filename">
             <xsl:with-param name="entry" select="$entry"/>
            </xsl:call-template>
           </xsl:attribute>
           <xsl:value-of select="."/>
          </a>
          <xsl:comment>
           <xsl:value-of select="translate('jer&amp;pos;','&amp;pos;','')"/>
          </xsl:comment>
         </xsl:when>
         <xsl:otherwise>
          <xsl:value-of select="."/>
          <!-- if there is more than one value, add the semi-colon -->
          <xsl:if test="position() &lt; $numvalues">
           <xsl:text>; </xsl:text>
          </xsl:if>
         </xsl:otherwise>
        </xsl:choose>

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
