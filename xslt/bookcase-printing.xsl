<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:bc="http://periapsis.org/bookcase/"
                xmlns:str="http://exslt.org/strings"
                xmlns:dyn="http://exslt.org/dynamic"
                extension-element-prefixes="str dyn"
                version="1.0">

<!--
   ================================================================
   Bookcase XSLT file - used for printing

   $Id: bookcase-printing.xsl,v 1.3 2003/05/03 05:50:26 robby Exp $

   Copyright (c) 2003 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Bookcase'
   application, which can be found at http://periapsis.org/bookcase/

   The exslt extensions from http://www.exslt.org are required.
   Specifically, the string and dynamic modules are used. For
   libxslt, that means the minimum version is 1.0.19.

   This is a horribly messy stylesheet. I would REALLY welcome any
   recommendations in improving its efficiency.

   There may be problems if this stylesheet is used to transform the
   actual Bookcase data file, since the application re-arranges the
   DOM for printing.

   Customize this file in order to print different columns of
   attributes for each book. Any version of this file in the user's home
   directory, such as $HOME/.kde/share/apps/bookcase/, will override
   the system file.
   ================================================================
-->

<xsl:output method="html" version="xhtml"/>

<xsl:strip-space elements="*"/>

<xsl:variable name="current-syntax" select="'3'"/>

<!-- To choose which properties for the books are printed, change the
     string to a space separated list of attribute names.
     As of Bookcase 0.5, the available attribute names are:
     - title (Title)
     - subtitle (Subtitle)
     - author (Author)
     - binding (Binding)
     - pur_date (Purchase Date)
     - pur_price (Purchase Pricee)
     - publisher (Publisher)
     - edition (Edition)
     - cr_year (Copyright Year)
     - pub_year (Publication Year)
     - isbn (ISBN#)
     - lccn (LCCN#)
     - pages (Pages)
     - language (Language)
     - genre (Genre)
     - keyword (Keywords)
     - series (Series)
     - series_num (Series Number)
     - condition (Condition)
     - signed (Signed)
     - read (Read)
     - gift (Gift)
     - rating (Rating)
     - comments (Comments)
-->
<xsl:param name="column-names" select="'title binding publisher read'"/>

<!-- If you want the header row printed, showing which attributes
     are printed, change this to true() -->
<xsl:param name="show-headers" select="true()"/>

<!-- The sort-name parameter is a string defining the name() of the 
     node used to sort the books. It can't be used to generate a key()
     since key() can't use a parameter. Sorting by author is the default
     Important: some of the fields, which can contain multiple entries,
     need to be listed in the proper XPath position, as children of their
     parent nodes. Author is one of those. -->
<!-- Remember to add the bc namespace. -->
<xsl:param name="sort-name" select="'bc:authors/bc:author'"/>

<!-- The doc-url parameter is a string containing the url of the
     file being printed -->
<xsl:param name="doc-url" select="'Bookcase'"/>

<!-- The sort-title parameter is a string sontaining a description of the sort -->
<xsl:param name="sort-title" select="'(sorted by author)'"/>

<xsl:variable name="columns" select="str:tokenize($column-names)"/>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<xsl:template match="/">
 <xsl:apply-templates select="bc:bookcase"/>
</xsl:template>
 
<xsl:template match="bc:bookcase">
 <xsl:if test="not(@syntaxVersion = $current-syntax)">
  <xsl:message>
   <xsl:text>This stylesheet was designed for Bookcase DTD version </xsl:text>
   <xsl:value-of select="$current-syntax"/>
   <xsl:text>, </xsl:text>
   <xsl:value-of select="$endl"/>
   <xsl:text>but the data file is version </xsl:text>
   <xsl:value-of select="@syntaxVersion"/>
   <xsl:text>.</xsl:text>
  </xsl:message>
 </xsl:if>
 <html>
  <head>
   <style type="text/css">
   html {
        margin: 0px;
        padding: 0px;
   }
   body {
        margin: 0px;
        padding: 0px;
        fon-family: sans-serif;
   }     
   #headerblock {
        padding-top: 10px;
        padding-bottom: 10px;
        margin-bottom: 5px;
   }
   div.colltitle {
        padding: 4px;
        line-height: 18px;
        font-size: 2em;
        border-bottom: 1px solid black;
        margin: 0px;
   }
   span.subtitle {
        margin-left: 20px;
        font-size: 0.5em;
   }
   td.groupName {
        margin-right: 3px;
        margin-top: 10px;
        margin-bottom: 2px;
        background: #eee;
        font-size: 1.2em;
        font-weight: bold;
   }
   tr.book {
        margin-left: 15px;
        margin-bottom: 5px;
        margin-right: 15px;
        font-size: 1em;
   }
   th {
   }
   td.attribute {
        padding-left: 20px;
   }
   </style>
   <title>
    <xsl:value-of select="$doc-url"/>
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
       <xsl:if test="string-length($sort-name) &gt; 0">
        <xsl:text>(sorted by </xsl:text>
        <xsl:call-template name="attribute-title">
         <xsl:with-param name="attributes" select="bc:attributes"/>
         <xsl:with-param name="name" select="$sort-name"/>
        </xsl:call-template>
        <xsl:text>)</xsl:text>
       </xsl:if>
      </xsl:otherwise>
     </xsl:choose>
    </span>
  </div>
 </div>

 <table>
  <xsl:if test="$show-headers">
   <xsl:variable name="attributes" select="bc:attributes"/>
   <tr>
    <xsl:for-each select="$columns">
     <xsl:variable name="column" select="."/>
     <th>
      <xsl:call-template name="attribute-title">
       <xsl:with-param name="attributes" select="$attributes"/>
       <xsl:with-param name="name" select="$column"/>
      </xsl:call-template>
     </th>
    </xsl:for-each>
   </tr>
  </xsl:if>
  <xsl:choose> <!-- if sort-name is not empty, do the funky sort -->
   <xsl:when test="string-length($sort-name) &gt; 0">
    <xsl:for-each select="bc:book">
     <!-- Sort by dynamically evaluating the sort-name variable
          this approach follows the one on page 141 in the book "XSLT"
          by Doug Tidwell -->
     <xsl:sort select="dyn:evaluate($sort-name)"/>
     <!-- keep track of the last key value -->
     <xsl:variable name="lastKey" select="dyn:evaluate($sort-name)"/>
     <!-- build the test string for convenience -->
     <xsl:variable name="test-str" select="concat('not(preceding-sibling::bc:book[',$sort-name,'=$lastKey])')"/>
     <xsl:if test="dyn:evaluate($test-str)">
      <tr>
       <td class="groupName">
        <xsl:attribute name="colspan">
         <xsl:value-of select="count($columns)"/>
        </xsl:attribute>
        <xsl:value-of select="dyn:evaluate($sort-name)"/>
       </td>
      </tr>
      <xsl:for-each select="dyn:evaluate(concat('../bc:book[',$sort-name,'=$lastKey]'))">
       <xsl:sort select="dyn:evaluate(concat('//bc:', $columns[1]))"/>
       <tr class="book">
        <xsl:apply-templates select="."/>
       </tr>
      </xsl:for-each>
     </xsl:if>
    </xsl:for-each>
   </xsl:when>
   <xsl:otherwise>
    <xsl:for-each select="bc:book">
     <xsl:sort select="dyn:evaluate(concat('//bc:', $columns[1]))"/>
     <tr class="book">
      <xsl:apply-templates select="."/>
     </tr>
    </xsl:for-each>
   </xsl:otherwise>
  </xsl:choose>
 </table>
</xsl:template>

<xsl:template name="attribute-title">
 <xsl:param name="attributes"/>
 <xsl:param name="name"/>
 <xsl:variable name="name-tokens" select="str:tokenize($name, ':')"/>
 <!-- the header is the title attribute of the attribute node whose name equals the column name -->
 <xsl:choose>
  <xsl:when test="$attributes">
   <xsl:value-of select="$attributes/bc:attribute[@name = $name-tokens[last()]]/@title"/>
  </xsl:when>
  <xsl:otherwise>
   <xsl:value-of select="$name-tokens[last()]"/>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template match="bc:book">
 <xsl:variable name="current" select="descendant::*"/>
 <xsl:for-each select="$columns">
  <xsl:variable name="column" select="."/>
  <!-- if the attribute node exists, output its value, otherwise put in a space -->
  <xsl:choose>
   <xsl:when test="count($current[local-name() = $column]) &gt; 0">
    <!-- the attribute's value is its text() unless it doesn't have any, then just output an 'X' -->
    <xsl:choose>
     <xsl:when test="string-length($current[local-name() = $column]) &gt; 0">
      <td class="attribute">
       <xsl:value-of select="$current[local-name() = $column]"/>
      </td>
     </xsl:when>
     <xsl:otherwise>
      <th class="attribute">
       <xsl:text>X</xsl:text>
      </th>
     </xsl:otherwise>
    </xsl:choose>
   </xsl:when>
   <xsl:otherwise>
    <td class="attribute">
     <xsl:text> </xsl:text>
    </td>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
