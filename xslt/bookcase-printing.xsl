<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:str="http://exslt.org/strings"
                xmlns:dyn="http://exslt.org/dynamic"
                extension-element-prefixes="str dyn" version="1.0">

<!--
   ================================================================
   Bookcase XSLT file - used for printing

   $Id: bookcase-printing.xsl,v 1.6 2002/11/25 00:13:40 robby Exp $

   Copyright (c) 2002 Robby Stephenson

   This XSLT stylesheet is designed to be used with XML data files
   from the 'bookcase' application, which can be found at:
   http://periapsis.org/bookcase/

   The exslt extensions from http://www.exslt.org are required.
   Specifically, the string and dynamic modules are used. For
   libxslt, that means the minimum version is 1.0.19.

   Customize this file in order to print different columns of
   attributes for each book.
   ================================================================
-->

<xsl:output method="html" version="xhtml"/>

<!-- To choose which properties for the books are printed, change the string inside
     the tokenize statement to a white-space separated list of attribute names.
     As of Bookcase 0.4, the available attributes are:
     - title (Title)
     - subtitle (Subtitle)
     - author (Author)
     - binding (Binding)
     - pur_date (Purchase Date)
     - pur_pricec (Purchase Pricee)
     - publisher (Publisher)
     - edition (Edition)
     - cr_year (Copyright Year)
     - pub_year (Publication Year)
     - isbn (ISBN#)
     - lccn (LCCN#)
     - pages (Pages)
     - language (Language)
     - genre (Genre)
     - keywords (Keywords)
     - seres (Series)
     - series_num (Series Number)
     - condition (Condition)
     - signed (Signed)
     - read (Read)
     - gift (Gift)
     - rating (Rating)
     - comments (Comments)
-->
<xsl:variable name="columns" select="str:tokenize('title binding publisher isbn')"/>

<!-- If you want the header row printed, showing which attributes are printed, change this to true() -->
<xsl:variable name="show-headers" select="false()"/>

<!-- The sort-name parameter is a string defining the name() of the book's node used to sort the books -->
<!-- It can't be used to generate a key() since key() can't use a parameter -->
<!-- Sorting by author is the default -->
<xsl:param name="sort-name" select="'author'"/>

<!-- The doc-url parameter is a string containing the url of the file being printed -->
<xsl:param name="doc-url" select="'Bookcase'"/>

<xsl:template match="/">
 <xsl:apply-templates select="bookcase"/>
</xsl:template>
 
<xsl:template match="bookcase">
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
   <xsl:apply-templates select="collection"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="collection">
 <div id="headerblock">
  <div class="colltitle">
   <xsl:value-of select="@title"/>
   <span class="subtitle">(sorted by
    <xsl:value-of select="$sort-name"/>)
   </span>
  </div>
 </div>

 <table>
  <xsl:if test="$show-headers">
   <tr>
    <xsl:for-each select="$columns">
     <xsl:variable name="column" select="."/>
     <!-- the header is the title attribute of the attribute node whose name equals the column name -->
     <th>
      <xsl:value-of select="//attribute[@name = $column]/@title"/>
     </th>
    </xsl:for-each>
   </tr>
  </xsl:if>
  <xsl:for-each select="//book">
   <!-- sort by dynamically evaluating the sort-name variable -->
   <!-- this approach follows the one on page 141 in the book "XSLT" by Doug Tidwell -->
   <xsl:sort select="dyn:evaluate($sort-name)"/>
   <!-- keep track of the last key value -->
   <xsl:variable name="lastKey" select="dyn:evaluate($sort-name)"/>
   <!-- build the test string for convenience -->
   <xsl:variable name="test-str" select="concat('not(preceding-sibling::book[',$sort-name,'=$lastKey])')"/>
   <xsl:if test="dyn:evaluate($test-str)">
    <tr>
     <td class="groupName">
      <xsl:attribute name="colspan">
       <xsl:value-of select="count($columns)"/>
      </xsl:attribute>
      <xsl:value-of select="dyn:evaluate($sort-name)"/>
     </td>
    </tr>
    <xsl:for-each select="dyn:evaluate(concat('//book[',$sort-name,'=$lastKey]'))">
     <xsl:sort select="title"/>
     <!-- keep track of all the current context's children -->
     <xsl:variable name="current" select="child::*"/>
     <tr class="book">
      <xsl:for-each select="$columns">
       <xsl:variable name="column" select="."/>
       <td class="attribute">
        <!-- if the attribute node exists, output its value, otherwise put in a space -->
        <xsl:choose>
         <xsl:when test="$current[name() = $column]">
          <!-- the attribute's value is its text() unless it doesn't have any, then just output an 'x' -->
          <xsl:choose>
           <xsl:when test="$current[name() = $column]">
            <xsl:value-of select="$current[name() = $column]"/>
           </xsl:when>
           <xsl:otherwise>
            <xsl:text>x</xsl:text>
           </xsl:otherwise>
          </xsl:choose>
         </xsl:when>
         <xsl:otherwise>
          <xsl:text> </xsl:text>
         </xsl:otherwise>
        </xsl:choose>
       </td>
      </xsl:for-each>
     </tr>
    </xsl:for-each>
   </xsl:if>
  </xsl:for-each>
 </table>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
