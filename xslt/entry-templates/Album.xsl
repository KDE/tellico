<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:bc="http://periapsis.org/bookcase/"
                exclude-result-prefixes="bc"
                version="1.0">

<!--
   ===================================================================
   Bookcase XSLT file - Entry template for videos

   $Id: Album.xsl 614 2004-04-17 18:52:48Z robby $

   Copyright (C) 2003, 2004 Robby Stephenson - robby@periapsis.org

   Known Issues:
   o If there is more an one image, there's a lot of white space under
     the top tables.

   This XSLT stylesheet is designed to be used with the 'Bookcase'
   application, which can be found at http://www.periapsis.org/bookcase/
   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="../bookcase-common.xsl"/>

<xsl:output method="html"/>

<xsl:param name="datadir"/> <!-- dir where Bookcase data are located -->
<xsl:param name="imgdir"/> <!-- dir where field images are located -->
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
 <xsl:call-template name="syntax-version">
  <xsl:with-param name="this-version" select="'5'"/>
  <xsl:with-param name="data-version" select="@syntaxVersion"/>
 </xsl:call-template>

 <html>
  <head>
  <style type="text/css">
   body {
        margin: 4px;
        padding: 0px;
        font-family: "<xsl:value-of select="$font"/>";
        color: #000;
        background-color: #fff;
        font-size: 1em;
   }
   #banner {
        padding-bottom: 2px;
        margin-bottom: 2px;
   }
   img#logo {
        padding-top: 2px; /* match h1 */
   }
   h1 {
        color: <xsl:value-of select="$color1"/>;
        background-color: <xsl:value-of select="$color2"/>;
        font-size: 1.8em;
        text-align: left;
        padding: 4px;
        margin: 0px;
        font-weight: bold;
   }
   span.year {
        padding-left: 8px;
   }
   img {
        padding-right: 6px;
        padding-bottom: 6px;
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
   td.fieldValue {
        background-color: #ddd;
        padding-left: 3px;
        padding-right: 3px;
   }
  </style>
  <title>
   <xsl:value-of select="bc:collection/bc:entry[1]/bc:title"/>
   <xsl:text> - </xsl:text>
   <xsl:value-of select="bc:collection/@title"/>
  </title>
  </head>
  <body>
   <xsl:apply-templates select="bc:collection"/>
  </body>
 </html>
</xsl:template>

<!-- type 3 is video collections -->
<!-- warn user that this template is meant for videos only. -->
<xsl:template match="bc:collection[@type!=4]">
 <h1>The <em>Album</em> template is meant for music collections only.</h1>
</xsl:template>

<xsl:template match="bc:collection[@type=4]">
 <xsl:apply-templates select="bc:entry[1]"/>
</xsl:template>

<xsl:template match="bc:entry">
 <xsl:variable name="entry" select="."/>
 <xsl:variable name="titleCat" select="key('fieldsByName','title')/@category"/>
 <!-- there might not be a track list -->
 <xsl:variable name="trackCat">
  <xsl:choose>
   <xsl:when test="bc:tracks">
    <xsl:value-of select="key('fieldsByName','track')/@category"/>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="''"/>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:variable>

 <!-- the top table has images in the left cell and main fields in the right.
      2 images can be on the left -->
 <table cellpadding="0" cellspacing="0" width="100%" class="category">
  <tr>
   <td valign="top" rowspan="2">
    <!-- now, show all the images in the entry, type 10 -->
    <xsl:variable name="images" select="../bc:fields/bc:field[@type=10]"/>
    <xsl:for-each select="$images">

     <!-- images will never be multiple, so no need to check for that -->
     <!-- find the value of the image field in the entry -->
     <xsl:variable name="image" select="$entry/*[local-name(.) = current()/@name]"/>
     <!-- check if the value is not empty -->
     <xsl:if test="$image">
      <a>
       <xsl:attribute name="href">
        <xsl:value-of select="concat($imgdir, $image)"/>
       </xsl:attribute>
       <img>
        <xsl:attribute name="src">
         <xsl:value-of select="concat($imgdir, $image)"/>
        </xsl:attribute>
        <!-- limit to maximum widht of 200 of height of 300 -->
        <xsl:call-template name="image-size">
         <xsl:with-param name="limit-width" select="150"/>
         <xsl:with-param name="limit-height" select="200"/>
         <xsl:with-param name="image" select="key('imagesById', $image)"/>
        </xsl:call-template>
       </img>
      </a>
     </xsl:if>
     <xsl:if test="position() mod 2 = 0">
      <br/>
     </xsl:if>
    </xsl:for-each>
   </td>

   <td valign="top" width="100%">
    <!-- now a nested table with the general fields -->
    <div id="banner">

     <h1>
      <xsl:choose>
       <xsl:when test="count(.//bc:artist) = 1">
        <xsl:value-of select=".//bc:artist[1]"/>
       </xsl:when>
       <xsl:otherwise>
        <xsl:text>Various</xsl:text>
       </xsl:otherwise>
      </xsl:choose>
      <xsl:text> - </xsl:text>
      <xsl:value-of select="bc:title"/>

      <!-- Bookcase 0.8 had multiple years in the default video collection -->
      <xsl:if test=".//bc:year">
       <span class="year">
        <xsl:text>(</xsl:text>
        <xsl:value-of select=".//bc:year[1]"/>
        <xsl:text>)</xsl:text>
       </span>
      </xsl:if>

     </h1>
    </div>
   </td>
  </tr>
  <tr>
   <td>

    <table cellspacing="1" cellpadding="0" width="100%">
     <xsl:for-each select="key('fieldsByCat', $titleCat)">
      <xsl:if test="@name != 'title'">
       <tr>
        <th>
         <xsl:value-of select="@title"/>
        </th>
        <td width="75%" class="fieldValue">
         <xsl:call-template name="simple-field-value">
          <xsl:with-param name="entry" select="$entry"/>
          <xsl:with-param name="field" select="@name"/>
         </xsl:call-template>
        </td>
       </tr>
      </xsl:if>
     </xsl:for-each>
    </table>

   </td>
  </tr>
 </table>

 <xsl:if test="bc:tracks">
  <table cellspacing="1" cellpadding="0" width="50%" align="left" class="category">
   <tr class="category">
    <td colspan="2">
     <xsl:value-of select="$trackCat"/>
    </td>
   </tr>
   <xsl:for-each select="bc:tracks/bc:track">
    <tr>
     <th align="right">
      <xsl:value-of select="format-number(position(), '00')"/>
     </th>
     <xsl:choose>
      <xsl:when test="bc:column">
       <td class="fieldValue">
        <xsl:value-of select="bc:column[1]"/>
       </td>
       <td class="fieldValue">
        <em>
         <xsl:value-of select="bc:column[2]"/>
        </em>
       </td>
      </xsl:when>
      <xsl:otherwise>
       <td width="100%" class="fieldValue">
        <xsl:value-of select="."/>
       </td>
      </xsl:otherwise>
     </xsl:choose>
    </tr>
   </xsl:for-each>
  </table>
 </xsl:if>
 
 <!-- now for every thing else -->
 <!-- write categories other than general and images -->
 <xsl:for-each select="$categories[. != $titleCat and
                                   ($trackCat = '' or . != $trackCat) and
                                   key('fieldsByCat',.)[1]/@type != 10]">
  <table cellspacing="1" cellpadding="0" width="50%" align="left" class="category">
   <tr class="category">
    <td colspan="2">
     <xsl:value-of select="."/>
    </td>
   </tr>
   <xsl:for-each select="key('fieldsByCat', .)[@name != 'title' and
                                               @name != 'artist' and
                                               @name != 'year']">
    <xsl:if test="$entry//*[local-name(.)=current()/@name]">
     <tr>
      <xsl:choose>
       <!-- paragraphs -->
       <xsl:when test="@type=2">
        <td class="fieldValue">
         <p>
          <xsl:value-of select="$entry/*[local-name(.)=current()/@name]"/>
         </p>
        </td>
       </xsl:when>
       <!-- single-column table -->
       <xsl:when test="@type=8">
        <td class="fieldValue">
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
            <td width="50%" class="fieldValue">
             <xsl:value-of select="bc:column[1]"/>
            </td>
            <td width="50%" class="fieldValue">
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
        <td width="50%" class="fieldValue">
         <xsl:call-template name="simple-field-value">
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
  <xsl:if test="position() mod 1 = 0">
   <br clear="left"/>
  </xsl:if>
 </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
