<?xml version="1.0"?>
<!-- WARNING: Tellico uses tc as the internal namespace declaration, and it must be identical here!! -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                xmlns:dyn="http://exslt.org/dynamic"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="str dyn exsl"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for exporting to HTML

   $Id: tellico2html.xsl 949 2004-11-13 01:28:50Z robby $

   Copyright (C) 2004 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   The exslt extensions from http://www.exslt.org are required.
   Specifically, the string and dynamic modules are used. For
   libxslt, that means the minimum version is 1.0.19.

   This is a horribly messy stylesheet. I would REALLY welcome any
   recommendations in improving its efficiency.

   The app itself adds group elements that aren't in the data file
   itself, in order to speed up exporting. So if this stylesheet is
   being used outside the app, it should still work, but may give
   slightly different results than when exporting to HTML from
   within Tellico.

   Customize this file in order to print different columns of
   fields for each entry. Any version of this file in the user's
   KDE home directory, such as $KDEHOME/share/apps/tellico/, will
   override the system file.
   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="tellico-common.xsl"/>

<xsl:output method="html" version="xhtml" encoding="utf-8"/>

<xsl:strip-space elements="*"/>

<!-- To choose which fields of each entry are printed, change the
     string to a space separated list of field names. To know what
     fields are available, check the Tellico data file for <field>
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
<xsl:param name="image-height" select="'100'"/>
<xsl:param name="image-width" select="'100'"/>

<!-- Set the string representing the element name of the field to group by -->
<!-- The string will be used to dynamically traverse and select the tree -->
<!-- It should be an XPath relative to the tc:entry element -->
<xsl:param name="group-fields" select="'tc:authors/tc:author'"/>
<xsl:param name="empty-group" select="'(Empty)'"/>

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
<xsl:param name="page-title" select="'Tellico'"/>
<xsl:param name="imgdir"/> <!-- dir where field images are located -->

<xsl:param name="entrydir"/> <!-- dir where entry links are located -->
<xsl:param name="link-entries" select="false()"/> <!-- link entries -->

<!-- In case the field has multiple values, only sort by first one -->
<xsl:variable name="sort1">
 <xsl:if test="string-length($sort-name1) &gt; 0">
  <xsl:value-of select="concat('.//tc:', $sort-name1, '[1]')"/>
 </xsl:if>
</xsl:variable>
<xsl:variable name="sort2">
 <xsl:if test="string-length($sort-name2) &gt; 0">
  <xsl:value-of select="concat('.//tc:', $sort-name2, '[1]')"/>
 </xsl:if>
</xsl:variable>
<xsl:variable name="sort3">
 <xsl:if test="string-length($sort-name3) &gt; 0">
  <xsl:value-of select="concat('.//tc:', $sort-name3, '[1]')"/>
 </xsl:if>
</xsl:variable>

<!-- keys ends up useless since we're using exsl:node-set
<xsl:key name="fieldsByName" match="tc:field" use="@name"/>
<xsl:key name="imagesById" match="tc:image" use="@id"/>
-->
<xsl:key name="entriesById" match="tc:entry" use="@id"/>

<!-- filename conversion is weird, need a variable for easy replacement -->
<xsl:variable name="weird">&apos;&quot;</xsl:variable>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <!-- This stylesheet is designed for Tellico document syntax version 7 -->
 <xsl:call-template name="syntax-version">
  <xsl:with-param name="this-version" select="'7'"/>
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
   table, h4 {
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
        text-align: center;
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
    <span class="subtitle">
     <xsl:choose>
      <xsl:when test="string-length($sort-title) &gt; 0">
       <xsl:value-of select="$sort-title"/>
      </xsl:when>
      <xsl:otherwise>
       <xsl:text>(sorted by </xsl:text>
       <xsl:if test="string-length($sort-name1) &gt; 0">
        <xsl:call-template name="field-title">
         <xsl:with-param name="fields" select="tc:fields"/>
         <xsl:with-param name="name" select="$sort-name1"/>
        </xsl:call-template>
       </xsl:if>
       <xsl:if test="string-length($sort-name2) &gt; 0">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="field-title">
         <xsl:with-param name="fields" select="tc:fields"/>
         <xsl:with-param name="name" select="$sort-name2"/>
        </xsl:call-template>
       </xsl:if>
       <xsl:if test="string-length($sort-name3) &gt; 0">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="field-title">
         <xsl:with-param name="fields" select="tc:fields"/>
         <xsl:with-param name="name" select="$sort-name3"/>
        </xsl:call-template>
       </xsl:if>
       <xsl:text>)</xsl:text>
      </xsl:otherwise>
     </xsl:choose>
    </span>
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
   <xsl:choose>
    
    <!-- If the entries are not being grouped, it's easy -->
    <xsl:when test="not($group-entries)">
     <xsl:for-each select="tc:entry">
      <xsl:sort select="dyn:evaluate($sort1)"/>
      <xsl:sort select="dyn:evaluate($sort2)"/>
      <xsl:sort select="dyn:evaluate($sort3)"/>
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
    </xsl:when> <!-- end ungrouped output -->

    <!-- If the entries are being grouped, it's a bit more involved -->
    <!-- Tellico helps out by creating groups, but I also want this -->
    <!-- stylesheet to stand alone, so add additional test for groups -->
    <xsl:when test="$group-entries and tc:group">
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
    </xsl:when>

    <!-- Now is the hard way, use XSL itself to do all thr groups -->
    <xsl:otherwise>
     
     <xsl:variable name="coll" select="."/>
     
     <!-- first, copy each entry and add a group attribute -->
     <xsl:variable name="listing">
      <xsl:for-each select="tc:entry">
       <xsl:sort select="dyn:evaluate($sort1)"/>
       <xsl:sort select="dyn:evaluate($sort2)"/>
       <xsl:sort select="dyn:evaluate($sort3)"/>
       <xsl:variable name="entry" select="."/>
       <xsl:for-each select="dyn:evaluate($group-fields)">
        <tc:entry group="{.}" id="{$entry/@id}"/>
       </xsl:for-each>
      </xsl:for-each>
     </xsl:variable>
     
     <!-- now, loop again, while sorting by group and title -->
     <xsl:variable name="sorted">
      <xsl:for-each select="exsl:node-set($listing)/tc:entry">
       <xsl:sort select="@group"/>
       <!-- don't repeat an entry in the same group -->
       <xsl:if test="not(preceding-sibling::*[@group=current()/@group and @id=current()/@id])">
        <xsl:copy-of select="."/>
       </xsl:if>
      </xsl:for-each>
     </xsl:variable>
     
     <!-- now finally, loop through and print out the entry -->
     <xsl:for-each select="exsl:node-set($sorted)/tc:entry">
      <xsl:variable name="g" select="@group"/>
      <xsl:if test="not(preceding-sibling::tc:entry[@group=$g])">
       <tr>
        <td class="groupName">
         <xsl:attribute name="colspan">
          <xsl:value-of select="count($columns)"/>
         </xsl:attribute>
         <xsl:value-of select="$g"/>
        </td>
       </tr>
      </xsl:if>
      <tr>
       <xsl:choose>
        <xsl:when test="count(preceding-sibling::tc:entry[@group=$g]) mod 2 = 0">
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
       <!-- I need the fields and images as variables since exsl:node-set
            can't use keys in the current document -->
       <xsl:apply-templates select="$coll/tc:entry[@id=current()/@id]">
        <xsl:with-param name="fields" select="$coll/tc:fields"/>
        <xsl:with-param name="images" select="$coll/tc:images"/>
       </xsl:apply-templates>
      </tr>
     </xsl:for-each>
     
     <!-- don't forget entries in no group -->
     <xsl:for-each select="dyn:evaluate(concat('tc:entry[not(',$group-fields,')]'))">
      <xsl:sort select="dyn:evaluate($sort1)"/>
      <xsl:sort select="dyn:evaluate($sort2)"/>
      <xsl:sort select="dyn:evaluate($sort3)"/>
      <xsl:if test="position()=1">
       <tr>
        <td class="groupName">
         <xsl:attribute name="colspan">
          <xsl:value-of select="count($columns)"/>
         </xsl:attribute>
         <xsl:value-of select="$empty-group"/>
        </td>
       </tr>
      </xsl:if>
      <tr>
       <xsl:choose>
        <xsl:when test="position() mod 2 = 1">
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
       <xsl:apply-templates select="."/>
      </tr>
     </xsl:for-each>
    </xsl:otherwise>
   </xsl:choose>
   
  </tbody>
 </table>

 <hr/>
 <h4>Generated by <a href="http://www.periapsis.org/tellico/">Tellico</a>.</h4>
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

<xsl:template name="filename">
 <xsl:param name="entry"/>
 <xsl:variable name="bad-chars">
  <xsl:value-of select="translate($entry//tc:title[1],
                        'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-',
                        '')"/>
 </xsl:variable>
 <xsl:variable name="name">
  <!-- there should be at least as many underscores as bad characters -->
  <xsl:value-of select="translate($entry//tc:title[1],
                                  concat($bad-chars, $weird),
                                  '_________________________________________________________________________________')"/>
 </xsl:variable>
 <xsl:value-of select="concat($entrydir, $name, '-', $entry/@id, '.html')"/>
</xsl:template>

<xsl:template match="tc:entry">
 <xsl:param name="fields" select="../tc:fields"/>
 <xsl:param name="images" select="../tc:images"/>
 <!-- stick all the descendants into a variable -->
 <xsl:variable name="current" select="descendant::*"/>
 <xsl:variable name="entry" select="."/>
 <xsl:for-each select="$columns">
  <xsl:variable name="column" select="."/>
  <!-- find all descendants whose name matches the column name -->
  <xsl:variable name="numvalues" select="count($current[local-name() = $column])"/>
  <!-- if the field node exists, output its value, otherwise put in a space -->
  <td class="field">
   <!-- first column should not be centered -->
   <xsl:if test="position()=1">
    <xsl:attribute name="style">
     <xsl:text>text-align: left; padding-left: 10px</xsl:text>
    </xsl:attribute>
   </xsl:if>
   <xsl:choose>
    <!-- when there is at least one value... -->
    <xsl:when test="$numvalues &gt; 0">
     <xsl:for-each select="$current[local-name() = $column]">
      <!-- key() doesn't work when using exsl:node-set since the context node
           is no longer in the current document
      <xsl:variable name="field" select="key('fieldsByName', $column)"/>
      -->
      <xsl:variable name="field" select="$fields/tc:field[@name = $column]"/>

      <xsl:choose>

       <!-- boolean values end up as 'true', output 'X' -->
       <xsl:when test="$field/@type=4 and . = 'true'">
        <xsl:text>X</xsl:text>
       </xsl:when>

       <!-- next, check for 2-column table -->
       <xsl:when test="$field/@type=9">
        <!-- italicize second column -->
        <xsl:value-of select="tc:column[1]"/>
        <xsl:text> - </xsl:text>
        <em>
         <xsl:value-of select="tc:column[2]"/>
        </em>
        <br/>
       </xsl:when>

       <!-- next, check for images -->
       <xsl:when test="$field/@type=10">
        <img>
         <xsl:attribute name="src">
          <xsl:value-of select="concat($imgdir, .)"/>
         </xsl:attribute>
         <xsl:call-template name="image-size">
          <xsl:with-param name="limit-width" select="$image-width"/>
          <xsl:with-param name="limit-height" select="$image-height"/>
          <xsl:with-param name="image" select="$images/tc:image[@id=current()]"/>
         </xsl:call-template>
        </img>
       </xsl:when>

       <!-- next, check for URLs -->
       <xsl:when test="$field/@type = 7">
        <a href="{.}">
         <xsl:value-of select="."/>
        </a>
       </xsl:when>

       <!-- if it's a date, format with hyphens -->
       <xsl:when test="$field/@type=12">
        <xsl:value-of select="tc:year"/>
        <xsl:text>-</xsl:text>
        <xsl:value-of select="tc:month"/>
        <xsl:text>-</xsl:text>
        <xsl:value-of select="tc:day"/>
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
          <xsl:if test="position() &lt; $numvalues">
           <br/>
          </xsl:if>
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
