<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:bc="http://periapsis.org/bookcase/"
                exclude-result-prefixes="bc"
                version="1.0">

<!--
   ===================================================================
   Bookcase XSLT file - Entry template for videos

   $Id: Video.xsl 421 2004-02-03 05:11:17Z robby $

   Copyright (C) 2003, 2004 Robby Stephenson - robby@periapsis.org

   Known Issues:
   o If there is more an one image, there's a lot of white space under
     the top tables.

   This XSLT stylesheet is designed to be used with the 'Bookcase'
   application, which can be found at http://www.periapsis.org/bookcase/
   ===================================================================
-->

<xsl:output method="html"/>

<xsl:param name="datadir"/> <!-- dir where Bookcase data are located -->
<xsl:param name="tmpdir"/> <!-- dir where field images are located -->
<xsl:param name="font"/> <!-- default KDE font family -->
<xsl:param name="fgcolor"/> <!-- default KDE foreground color -->
<xsl:param name="bgcolor"/> <!-- default KDE background color -->
<xsl:param name="color1"/> <!-- default KDE highlighted text color -->
<xsl:param name="color2"/> <!-- default KDE highlighted background color -->

<xsl:strip-space elements="*"/>

<xsl:key name="fieldsByName" match="bc:field" use="@name"/>
<xsl:key name="fieldsByCat" match="bc:field" use="@category"/>
<xsl:key name="imagesById" match="bc:image" use="@id"/>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<!-- all the categories -->
<xsl:variable name="categories" select="/bc:bookcase/bc:collection/bc:fields/bc:field[generate-id(.)=generate-id(key('fieldsByCat',@category)[1])]/@category"/>

<xsl:template match="/">
 <xsl:apply-templates select="bc:bookcase"/>
</xsl:template>

<!-- The default layout is pretty boring, but catches every field value in
     the entry. The title is in the top H1 element. -->
<xsl:template match="bc:bookcase">
 <!-- This stylesheet is designed for Bookcase document syntax version 5 -->
 <xsl:if test="@syntaxVersion != '5'">
  <xsl:message>
   <xsl:text>This stylesheet was designed for Bookcase DTD version 5, &#xa;
             but the input data file is version </xsl:text>
   <xsl:value-of select="@syntaxVersion"/>
   <xsl:text>. There might be some &#xa;problems with the output.</xsl:text>
  </xsl:message>
 </xsl:if>

 <html>
  <head>
  <style type="text/css">
   body {
        margin: 4px;
        padding: 0px;
<!--     
        font-family: sans-serif;
        color: <xsl:value-of select="$fgcolor"/>;
        background-color: <xsl:value-of select="$bgcolor"/>;
-->
        font-family: <xsl:value-of select="$font"/>;
        color: #000;
        background-color: #fff;
        font-size: 1em;
   }
   #banner {
        padding-bottom: 5px;
        margin-bottom: 8px;
        border-bottom: 2px ridge <xsl:value-of select="$color2"/>;
   }
   img#logo {
        padding-top: 2px; /* match h1 */
   }
   h1 {
        color: <xsl:value-of select="$color2"/>;
        background-color: <xsl:value-of select="$color1"/>;
        font-size: 1.8em;
        text-align: left;
        padding: 0px;
        padding-bottom: 2px;
        margin: 0px;
        font-weight: bold;
   }
   span.year {
        font-size: 0.8em;
   }
   span.country {
        font-size: 0.8em;
        font-style: italics;
   }
   h2 {
        font-size: 1.2em;
        margin: 0px;
        padding: 0px;
   }
   img {
        padding-top: 1px; /* match cellspacing of table */
        padding-right: 10px;
        padding-bottom: 9px;
   }
   table.category {
       margin-bottom: 10px;
   }
   tr.category {
        font-weight: bold;
        font-size: 1.2em;
        color: #fff;
        background-color: #666;
        text-align: center;
   }
   th {
        font-weight: bold;
        text-align: left;
        background-color: #ccc;
        padding-left: 3px;
        padding-right: 3px;
   }
  </style>
  </head>
  <body>
   <xsl:apply-templates select="bc:collection"/>
  </body>
 </html>
</xsl:template>

<!-- type 3 is video collections -->
<!-- warn user that this template is meant for videos only. -->
<xsl:template match="bc:collection[@type!=3]">
 <h1>The <em>Video</em> template is meant for video collections only.</h1>
</xsl:template>

<xsl:template match="bc:collection[@type=3]">
 <xsl:apply-templates select="bc:entry[1]"/>
</xsl:template>

<xsl:template match="bc:entry">
 <xsl:variable name="entry" select="."/>
 <xsl:variable name="titleCat" select="key('fieldsByName','title')/@category"/>
 <xsl:variable name="castCat" select="key('fieldsByName','cast')/@category"/>

 <div id="banner">
  <!-- do format logo -->
  <xsl:choose>
   <xsl:when test="$datadir and bc:medium = 'DVD'">
    <img width="74" height="30" align="right" id="logo">
     <xsl:attribute name="src">
      <xsl:value-of select="concat($datadir,'dvd-logo.png')"/>
     </xsl:attribute>
    </img>
   </xsl:when>
   <xsl:when test="$datadir and bc:medium = 'VHS'">
    <img width="108" height="58" align="right" id="logo">
     <xsl:attribute name="src">
      <xsl:value-of select="concat($datadir,'vhs-logo.png')"/>
     </xsl:attribute>
    </img>
   </xsl:when>
  </xsl:choose>
  
  <!-- title block -->
  <h1>
   <xsl:value-of select="bc:title"/>
   <xsl:text> - </xsl:text>
   <!-- Bookcase 0.8 had multiple years in the default video collection -->
   <xsl:if test=".//bc:year">
    <span class="year">
     <xsl:value-of select="concat(.//bc:year, ' ')"/>
    </span>
   </xsl:if>
   <xsl:if test="bc:nationality">
    <span class="country">
     <xsl:text>(</xsl:text>
     <xsl:value-of select="bc:nationality"/>
     <xsl:text>)</xsl:text>
    </span>
   </xsl:if>
  </h1>
  
  <h2>
   <xsl:if test="bc:widescreen">
    <xsl:value-of select="concat(key('fieldsByName', 'widescreen')/@title, ' ')"/>
   </xsl:if>
   <xsl:if test="bc:directors-cut">
    <xsl:value-of select="key('fieldsByName', 'directors-cut')/@title"/>
   </xsl:if>
  </h2>
 </div>
 
 <!-- now, show all the images in the entry, field type 10 -->
 <xsl:variable name="images" select="../bc:fields/bc:field[@type=10]"/>
 <xsl:if test="count($images) &gt; 0">
  <table align="left" cellpadding="0" cellspacing="0">
   <!-- now, show all the images in the entry, type 10 -->
   <xsl:for-each select="$images">
    <tr>
     <td>
      
      <!-- images will never be multiple, so no need to check for that -->
      <!-- find the value of the image field in the entry -->
      <xsl:variable name="image" select="$entry/*[local-name(.) = current()/@name]"/>
      <!-- check if the value is not empty -->
      <xsl:if test="$image">
       <img>
        <xsl:attribute name="src">
         <xsl:value-of select="concat($tmpdir, $image)"/>
        </xsl:attribute>
        <!-- limit to maximum widht of 200 of height of 300 -->
        <xsl:call-template name="image-size">
         <xsl:with-param name="limit-width" select="200"/>
         <xsl:with-param name="limit-height" select="300"/>
         <xsl:with-param name="image" select="key('imagesById', $image)"/>
        </xsl:call-template>
       </img>
      </xsl:if>
     </td>
    </tr>
   </xsl:for-each>
  </table>
 </xsl:if>

 <!-- the general group and the cast are each in a table cell -->
 <table cellspacing="1" cellpadding="0" width="100%" class="category">
  <tr>
   <td valign="top">
    <!-- show the general group, or more accurately, the title's group -->
    <table cellspacing="1" cellpadding="0" width="100%">
     <tr class="category">
      <td colspan="2">
       <xsl:value-of select="$titleCat"/>
      </td>
     </tr>
     <xsl:for-each select="key('fieldsByCat', $titleCat)">
      <xsl:if test="@name != 'title' and
                    @name != 'year' and
                    @name != 'nationality'">
       <tr>
        <th>
         <xsl:value-of select="@title"/>
        </th>
        <td width="75%">
         <xsl:call-template name="output-field">
          <xsl:with-param name="entry" select="$entry"/>
          <xsl:with-param name="field" select="@name"/>
         </xsl:call-template>
        </td>
       </tr>
      </xsl:if>
     </xsl:for-each>
    </table>
    
   </td>
   <td valign="top">
    <!-- now for the cast -->
    <xsl:if test="bc:casts">
     <table cellspacing="1" cellpadding="0" width="100%">
      <tr class="category">
       <td colspan="2">
        <xsl:value-of select="key('fieldsByName', 'cast')/@title"/>
       </td>
      </tr>
      <tr>
       <td>
        <table width="100%" cellspacing="0" cellpadding="0">
         <xsl:for-each select="$entry/bc:casts/bc:cast">
          <tr>
           <td width="50%">
            <xsl:value-of select="bc:column[1]"/>
           </td>
           <td width="50%">
            <em>
             <xsl:value-of select="bc:column[2]"/>
            </em>
           </td>
          </tr>
         </xsl:for-each>
        </table>
       </td>
      </tr>
     </table>
    </xsl:if>
   </td>
  </tr>
 </table>

 <br clear="all"/>

 <!-- now for every thing else -->
 <!-- write categories other than general and images -->
 <xsl:for-each select="$categories[. != $titleCat and
                                   . != $castCat and
                                   key('fieldsByCat',.)[1]/@type != 10]">
  <table cellspacing="1" cellpadding="0" width="50%" align="left" class="category">
   <tr class="category">
    <td colspan="2">
     <xsl:value-of select="."/>
    </td>
   </tr>
   <xsl:for-each select="key('fieldsByCat', .)[@name != 'directors-cut' and
                                               @name != 'widescreen']">
    <xsl:if test="$entry//*[local-name(.)=current()/@name]">
     <tr>
      <xsl:choose>
       <!-- paragraphs -->
       <xsl:when test="@type=2">
        <td>
         <p>
          <xsl:value-of select="$entry/*[local-name(.)=current()/@name]"/>
         </p>
        </td>
       </xsl:when>
       <!-- single-column table -->
       <xsl:when test="@type=8">
        <td>
         <ul>
          <xsl:for-each select="$entry//*[local-name(.)=current()/@name]">
           <li>
            <xsl:value-of select="."/>
           </li>
          </xsl:for-each>
         </ul>
        </td>
       </xsl:when>
       <!-- double-column table -->
       <xsl:when test="@type=9">
        <td>
         <table width="100%" cellspacing="0" cellpadding="0">
          <xsl:for-each select="$entry//*[local-name(.)=current()/@name]">
           <tr>
            <td width="50%">
             <xsl:value-of select="bc:column[1]"/>
            </td>
            <td width="50%">
             <em>
              <xsl:value-of select="bc:column[2]"/>
             </em>
            </td>
           </tr>
          </xsl:for-each>
         </table>
        </td>
       </xsl:when>
       <!-- everything else -->
       <xsl:otherwise>
        <th>
         <xsl:value-of select="@title"/>
        </th>
        <td with="75%">
         <xsl:call-template name="output-field">
          <xsl:with-param name="entry" select="$entry"/>
          <xsl:with-param name="field" select="@name"/>
         </xsl:call-template>
        </td>
       </xsl:otherwise>
      </xsl:choose>
     </tr>
    </xsl:if>
   </xsl:for-each>
  </table>
  <xsl:if test="position() mod 2 = 0">
   <br clear="left"/>
  </xsl:if>
 </xsl:for-each>
</xsl:template>

<xsl:template name="output-field">
 <xsl:param name="entry"/>
 <xsl:param name="field"/>

 <!-- if the field has multiple values, then there is no child of the entry with the field name -->
 <xsl:variable name="child" select="$entry/*[local-name(.)=$field]"/>
 <xsl:choose>
  <xsl:when test="$child">

   <!-- if the field is a bool type, just ouput an X -->
   <xsl:choose>
    <xsl:when test="key('fieldsByName',$field)/@type=4">
     <xsl:text>X</xsl:text>
    </xsl:when>

    <!-- if it's a url, then add a hyperlink -->
    <xsl:when test="key('fieldsByName',$field)/@type=7">
     <a href="{$child}">
      <xsl:value-of select="$child"/>
     </a>
    </xsl:when>

    <xsl:otherwise>
     <xsl:value-of select="$child"/>
    </xsl:otherwise>
   </xsl:choose>
  </xsl:when>

  <!-- now handle fields with multiple values -->
  <xsl:otherwise>
   <xsl:for-each select="$entry/*[local-name(.)=concat($field,'s')]/*">
    <xsl:value-of select="."/>
    <xsl:if test="position()!=last()">
     <xsl:text>; </xsl:text>
    </xsl:if>
   </xsl:for-each>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template name="image-size">
 <xsl:param name="limit-height"/>
 <xsl:param name="limit-width"/>
 <xsl:param name="image"/>

 <xsl:variable name="actual-width" select="$image/@width"/>
 <xsl:variable name="actual-height" select="$image/@height"/>

 <xsl:choose>
  <xsl:when test="$actual-width &gt; $limit-width or $actual-height &gt; $limit-height">

   <!-- -->
   <xsl:choose>
    
    <xsl:when test="$actual-width * $limit-height &lt; $actual-height * $limit-width">
     <xsl:attribute name="height">
      <xsl:value-of select="$limit-height"/>
     </xsl:attribute>
     <xsl:attribute name="width">
      <xsl:value-of select="$actual-width * $limit-height div $actual-height"/>
     </xsl:attribute>
    </xsl:when>
    
    <xsl:otherwise>
     <xsl:attribute name="width">
      <xsl:value-of select="$limit-width"/>
     </xsl:attribute>
     <xsl:attribute name="height">
      <xsl:value-of select="$actual-height * $limit-width div $actual-width"/>
     </xsl:attribute>
    </xsl:otherwise>

   </xsl:choose>

  </xsl:when>

  <!-- if both are smaller, no change -->
  <xsl:otherwise>
   <xsl:attribute name="height">
    <xsl:value-of select="$actual-height"/>
   </xsl:attribute>
   <xsl:attribute name="width">
    <xsl:value-of select="$actual-width"/>
   </xsl:attribute>
  </xsl:otherwise>
  
 </xsl:choose>
</xsl:template>

</xsl:stylesheet>
