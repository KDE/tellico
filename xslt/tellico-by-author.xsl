<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ================================================================
   Tellico XSLT file - sort by author

   Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with XML data files
   from the 'tellico' application, which can be found at http://tellico-project.org
   ================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="tellico-common.xsl"/>

<xsl:output method="html"
            indent="yes"
            doctype-public="-//W3C//DTD HTML 4.01//EN"
            doctype-system="http://www.w3.org/TR/html4/strict.dtd"
            encoding="utf-8"/>

<!-- Sort using user's preferred language -->
<xsl:param name="lang"/>

<xsl:param name="version"/>

<xsl:key name="books" match="tc:entry" use=".//tc:author"/>
<xsl:key name="authors" match="tc:author" use="."/>

<!-- more efficient to specify complete XPath like this than to use //tc:author -->
<xsl:variable name="unique-authors" select="/tc:tellico/tc:collection/tc:entry/tc:authors/tc:author[generate-id(.)=generate-id(key('authors', .)[1])]"/>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <xsl:call-template name="syntax-version">
  <xsl:with-param name="this-version" select="'12'"/>
  <xsl:with-param name="data-version" select="@syntaxVersion"/>
 </xsl:call-template>

 <html>
  <head>
   <title>My Book Collections - sorted by author</title>
   <style type="text/css">
   body {
         background: #999999;
         margin: 0px;
         font-family: Verdana, Arial, sans-serif;
         color: black;
   }
   #headerblock {
         padding-top: 10px;
         padding-bottom: 10px;
         margin-bottom: 5px;
   }
   .title {
         padding: 4px;
         line-height: 18px;
         font-size: 24px;
         border-top: 1px solid black;
         border-bottom: 1px solid black;
         margin: 0px;
   }
   .subtitle {
         margin-left: 10px;
         font-size: 12px;
   }
   .author {
         margin-right: 3px;
         margin-bottom: 2px;
         background: #eee;
         font-size: 14px;
         font-weight: bold;
   }
   .books {
         background: rgb(204,204,204);
         padding-left: 4px;
         margin-left: 15px;
         margin-bottom: 5px;
         margin-right: 15px;
         font-size: 12px;
   }
   ul {
         margin: 0px;
         padding: 0px;
   }
   </style>
  </head>
  <body>
   <xsl:apply-templates select="tc:collection"/>
  </body>
 </html>
</xsl:template>

<xsl:template match="tc:collection">
 <div id="headerblock">
  <div class="title">
   <xsl:value-of select="@title"/>
   <xsl:text> </xsl:text>
   <span class="subtitle">sorted by author</span>
  </div>
 </div>

 <!-- first output any with no author -->
 <xsl:variable name="no-author" select="/tc:tellico/tc:collection/tc:entry[count(tc:authors/tc:author) = 0]"/>
 <xsl:if test="count($no-author) &gt; 0">
  <div class="author">
   <xsl:text>(Empty)</xsl:text>
  </div>
  <div class="books">
   <ul>
    <xsl:for-each select="$no-author">
     <xsl:sort lang="$lang" select="tc:title"/>
     <xsl:apply-templates select="."/>
    </xsl:for-each>
   </ul>
  </div>
 </xsl:if>

 <xsl:for-each select="$unique-authors">
  <xsl:sort lang="$lang" select="."/>
  <div class="author">
   <xsl:value-of select="."/>
  </div>
  <div class="books">
   <ul>
    <xsl:for-each select="key('books', .)">
     <xsl:sort lang="$lang" select="tc:title"/>
<!-- or sort by series and number -->
<!-- <xsl:sort select="tc:series"/>
     <xsl:sort select="tc:series_num"/> -->
     <xsl:apply-templates select="."/>
    </xsl:for-each>
   </ul>
  </div>
 </xsl:for-each>
</xsl:template>

<xsl:template match="tc:entry">
 <li>
  <xsl:value-of select="tc:title"/>
 </li>
</xsl:template>

</xsl:stylesheet>
