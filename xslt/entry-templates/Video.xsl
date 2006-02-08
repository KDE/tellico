<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - Entry template for videos

   Copyright (C) 2003-2006 Robby Stephenson - robby@periapsis.org

   Known Issues:
   o If there is more an one image, there's a lot of white space under
     the top tables.

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/
   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="../tellico-common.xsl"/>

<xsl:output method="html"/>

<xsl:param name="datadir"/> <!-- dir where Tellico data are located -->
<xsl:param name="imgdir"/> <!-- dir where field images are located -->
<xsl:param name="font"/> <!-- default KDE font family -->
<xsl:param name="fgcolor"/> <!-- default KDE foreground color -->
<xsl:param name="bgcolor"/> <!-- default KDE background color -->
<xsl:param name="color1"/> <!-- default KDE highlighted text color -->
<xsl:param name="color2"/> <!-- default KDE highlighted background color -->

<xsl:param name="collection-file"/> <!-- might have a link to parent collection -->

<xsl:key name="fieldsByName" match="tc:field" use="@name"/>
<xsl:key name="fieldsByCat" match="tc:field" use="@category"/>
<xsl:key name="imagesById" match="tc:image" use="@id"/>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<!-- all the categories -->
<xsl:variable name="categories" select="/tc:tellico/tc:collection/tc:fields/tc:field[generate-id(.)=generate-id(key('fieldsByCat',@category)[1])]/@category"/>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<!-- The default layout is pretty boring, but catches every field value in
     the entry. The title is in the top H1 element. -->
<xsl:template match="tc:tellico">
 <!-- This stylesheet is designed for Tellico document syntax version 9 -->
 <xsl:call-template name="syntax-version">
  <xsl:with-param name="this-version" select="'9'"/>
  <xsl:with-param name="data-version" select="@syntaxVersion"/>
 </xsl:call-template>

 <html>
  <head>
  <style type="text/css">
   body {
        margin: 4px;
        padding: 0px;
        font-family: "<xsl:value-of select="$font"/>";
        color: <xsl:value-of select="$fgcolor"/>;
        background-repeat: repeat;
        font-size: 1em;
   }
   #banner {
        padding-bottom: 5px;
        margin-bottom: 8px;
        border-bottom: 2px ridge <xsl:value-of select="$color2"/>;
   }
   img#logo {
        padding-left: 4px;
        padding-top: 2px; /* match h1 */
   }
   h1 {
        color: <xsl:value-of select="$color2"/>;
        font-size: 1.8em;
        text-align: left;
        padding: 2px;
        margin: 0px;
        font-weight: bold;
   }
   span.year {
        font-size: 0.8em;
   }
   span.country {
        font-size: 0.8em;
        font-style: italic;
   }
   h2 {
        font-size: 1.2em;
        margin: 0px;
        padding: 0px;
        padding-left: 2px;
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
        color: <xsl:value-of select="$color1"/>;
        background-color: <xsl:value-of select="$color2"/>;
        text-align: center;
   }
  /* there seems to be a khtml bug, in 3.4.x at least, repeat-x doesn't
     work on the tr element, so have to put it on the td element */
   tr.category td {
        background-image: url(<xsl:value-of select="concat($imgdir, 'gradient_header.png')"/>);
        background-repeat: repeat-x; 
   }
   th {
        font-weight: bold;
        text-align: left;
        color: <xsl:value-of select="$color2"/>;
        padding-left: 3px;
        padding-right: 3px;
   }
   p {
        margin-top: 0px;
        text-align: justify;
        font-size: 90%;
   }
   ul {
        margin-top: 4px;
        margin-bottom: 4px;
        padding: 0px 0px 0px 20px;
   }
   img {
        border: 0px solid;
   }
  </style>
  <title>
   <xsl:value-of select="tc:collection/tc:entry[1]//tc:title[1]"/>
   <xsl:text> - </xsl:text>
   <xsl:value-of select="tc:collection/@title"/>
  </title>
  </head>
  <body>
   <xsl:apply-templates select="tc:collection[1]"/>
   <xsl:if test="$collection-file">
    <hr style="clear:left"/>
    <h4 style="text-align:center">
     <a href="{$collection-file}">&lt;&lt; <xsl:value-of select="tc:collection/@title"/></a>
    </h4>
   </xsl:if>
  </body>
 </html>
</xsl:template>

<!-- type 3 is video collections -->
<!-- warn user that this template is meant for videos only. -->
<xsl:template match="tc:collection[@type!=3]">
 <h1><i18n>This template is meant for video collections only.</i18n></h1>
</xsl:template>

<xsl:template match="tc:collection[@type=3]">
 <xsl:apply-templates select="tc:entry[1]"/>
</xsl:template>

<xsl:template match="tc:entry">
 <xsl:variable name="entry" select="."/>
 <xsl:variable name="titleCat" select="key('fieldsByName','title')/@category"/>
 <!-- there might not be a cast -->
 <xsl:variable name="castCat">
  <xsl:choose>
   <xsl:when test="tc:casts">
    <xsl:value-of select="key('fieldsByName','cast')/@category"/>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="''"/>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:variable>

 <div id="banner">
  <!-- do format logo -->
  <xsl:choose>
   <xsl:when test="$datadir and tc:medium = 'DVD'">
    <img width="74" height="30" align="right" id="logo">
     <xsl:attribute name="src">
      <xsl:value-of select="concat($datadir,'dvd-logo.png')"/>
     </xsl:attribute>
    </img>
   </xsl:when>
   <xsl:when test="$datadir and tc:medium = 'VHS'">
    <img width="56" height="30" align="right" id="logo">
     <xsl:attribute name="src">
      <xsl:value-of select="concat($datadir,'vhs-logo.png')"/>
     </xsl:attribute>
    </img>
   </xsl:when>
  </xsl:choose>

  <!-- title block -->
  <h1>
   <xsl:value-of select=".//tc:title[1]"/>
   <xsl:if test=".//tc:year|tc:nationality">
    <xsl:text> (</xsl:text>
    <xsl:if test=".//tc:year">
     <span class="year">
      <xsl:value-of select=".//tc:year"/>
     </span>
    </xsl:if>
    <xsl:if test="tc:nationality">
     <xsl:if test=".//tc:year">
      <xsl:text> </xsl:text>
     </xsl:if>
     <span class="country">
      <xsl:value-of select="tc:nationality"/>
     </span>
    </xsl:if>
    <xsl:text>)</xsl:text>
   </xsl:if>
  </h1>

  <h2>
   <xsl:if test="tc:widescreen">
    <xsl:value-of select="concat(key('fieldsByName', 'widescreen')/@title, ' ')"/>
   </xsl:if>
   <xsl:if test="tc:directors-cut">
    <xsl:value-of select="key('fieldsByName', 'directors-cut')/@title"/>
   </xsl:if>
  </h2>
 </div>

 <!-- the images, general group and the cast are each in a table cell -->
 <table cellspacing="1" cellpadding="0" class="category" width="100%">
  <tbody>
   <tr>
    <td valign="top">

     <!-- now, show all the images in the entry, field type 10 -->
     <xsl:variable name="images" select="../tc:fields/tc:field[@type=10]"/>
     <xsl:if test="count($images) &gt; 0">
      <table cellpadding="0" cellspacing="0">
       <tbody>
        <!-- now, show all the images in the entry, type 10 -->
        <xsl:for-each select="$images">
         <tr>
          <td>

           <!-- images will never be multiple, so no need to check for that -->
           <!-- find the value of the image field in the entry -->
           <xsl:variable name="image" select="$entry/*[local-name(.) = current()/@name]"/>
           <!-- check if the value is not empty -->
           <xsl:if test="$image">
            <a>
             <xsl:attribute name="href">
              <xsl:choose>
               <!-- Amazon license requires the image to be linked to the amazon website -->
               <xsl:when test="$entry/tc:amazon">
                <xsl:value-of select="$entry/tc:amazon"/>
               </xsl:when>
               <xsl:otherwise>
                <xsl:value-of select="concat($imgdir, $image)"/>
               </xsl:otherwise>
              </xsl:choose>
             </xsl:attribute>
             <img style="border: 0px">
              <xsl:attribute name="src">
               <xsl:value-of select="concat($imgdir, $image)"/>
              </xsl:attribute>
              <!-- limit to maximum widht of 200 of height of 300 -->
              <xsl:call-template name="image-size">
               <xsl:with-param name="limit-width" select="200"/>
               <xsl:with-param name="limit-height" select="300"/>
               <xsl:with-param name="image" select="key('imagesById', $image)"/>
              </xsl:call-template>
             </img>
            </a>
           </xsl:if>
          </td>
         </tr>
        </xsl:for-each>
       </tbody>
      </table>
     </xsl:if>
    </td>

    <td valign="top" width="50%">
     <!-- show the general group, or more accurately, the title's group -->
     <table cellspacing="1" cellpadding="0" width="100%">
      <tbody>
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
          <td>
           <xsl:call-template name="simple-field-value">
            <xsl:with-param name="entry" select="$entry"/>
            <xsl:with-param name="field" select="@name"/>
           </xsl:call-template>
          </td>
         </tr>
        </xsl:if>
       </xsl:for-each>
      </tbody>
     </table>

    </td>
    <td valign="top" width="50%">
     <!-- now for the cast -->
     <xsl:if test="tc:casts">
      <xsl:variable name="castField" select="key('fieldsByName', 'cast')"/>
      <table cellspacing="1" cellpadding="0" width="100%">
       <thead>
        <tr class="category">
         <td colspan="5"> <!-- never more than 5 columns -->
          <xsl:value-of select="$castField/@title"/>
         </td>
        </tr>
        <xsl:if test="$castField/tc:prop[@name = 'column1']">
         <xsl:variable name="castCols" select="$castField/tc:prop[@name = 'columns']"/>
         <tr>
          <xsl:call-template name="columnTitle">
           <xsl:with-param name="index" select="1"/>
           <xsl:with-param name="max" select="$castCols"/>
           <xsl:with-param name="elem" select="'th'"/>
           <xsl:with-param name="field" select="$castField"/>
          </xsl:call-template>
         </tr>
        </xsl:if>
       </thead>
       <tbody>
        <xsl:for-each select="$entry/tc:casts/tc:cast">
         <tr>
          <xsl:for-each select="tc:column">
           <td width="{floor(100 div count(../tc:column))}%">
            <xsl:if test="position() = 1">
             <xsl:value-of select="."/>
            </xsl:if>
            <xsl:if test="position() &gt; 1">
             <em><xsl:value-of select="."/></em>
            </xsl:if>
            <xsl:text>&#160;</xsl:text>
           </td>
          </xsl:for-each>
         </tr>
        </xsl:for-each>
       </tbody>
      </table>
     </xsl:if>
    </td>
   </tr>
  </tbody>
 </table>

 <br clear="all"/>

 <!-- now for every thing else -->
 <!-- write categories other than general and images -->
 <!-- $castcat might be empty, be careful -->
 <xsl:for-each select="$categories[. != $titleCat and
                                   ($castCat = '' or . != $castCat) and
                                   key('fieldsByCat',.)[1]/@type != 10]">
  <table cellspacing="1" cellpadding="0" width="50%" align="left" class="category">
   <tbody>
    <tr class="category">
     <td colspan="2">
      <xsl:value-of select="."/>
     </td>
    </tr>
    <xsl:for-each select="key('fieldsByCat', .)[@name != 'directors-cut' and
                          @name != 'widescreen']">
     <tr>
      <xsl:choose>
       <!-- paragraphs -->
       <xsl:when test="@type=2">
        <td>
         <p>
          <xsl:value-of select="$entry/*[local-name(.)=current()/@name]" disable-output-escaping="yes"/>
         </p>
        </td>
       </xsl:when>

       <!-- tables are field type 8 -->
       <!-- ok to put category name inside div instead of table here -->
       <xsl:when test="@type = 8">
        <td>
         <!-- look at number of columns -->
         <xsl:choose>
          <xsl:when test="tc:prop[@name = 'columns'] &gt; 1">
           <table>
            <xsl:if test="tc:prop[@name = 'column1']">
             <thead>
              <tr>
               <xsl:call-template name="columnTitle">
                <xsl:with-param name="index" select="1"/>
                <xsl:with-param name="max" select="tc:prop[@name = 'columns']"/>
                <xsl:with-param name="elem" select="'th'"/>
                <xsl:with-param name="field" select="."/>
               </xsl:call-template>
              </tr>
             </thead>
            </xsl:if>
            <tbody>
             <xsl:for-each select="$entry//*[local-name(.) = current()/@name]">
              <tr>
               <xsl:for-each select="tc:column">
                <td width="{100 div count(../tc:column)}%">
                 <xsl:value-of select="."/>
                 <xsl:text>&#160;</xsl:text>
                </td>
               </xsl:for-each>
              </tr>
             </xsl:for-each>
            </tbody>
           </table>
          </xsl:when>
          <xsl:otherwise>
           <ul>
            <xsl:for-each select="$entry//*[local-name(.) = current()/@name]">
             <li>
              <xsl:value-of select="."/>
             </li>
            </xsl:for-each>
           </ul>
          </xsl:otherwise>
         </xsl:choose>
        </td>
       </xsl:when>

       <!-- everything else -->
       <xsl:otherwise>
        <th>
         <xsl:value-of select="@title"/>
        </th>
        <td width="50%">
         <xsl:call-template name="simple-field-value">
          <xsl:with-param name="entry" select="$entry"/>
          <xsl:with-param name="field" select="@name"/>
         </xsl:call-template>
        </td>
       </xsl:otherwise>
      </xsl:choose>
     </tr>
    </xsl:for-each>
   </tbody>
  </table>
  <xsl:if test="position() mod 2 = 0">
   <br clear="left"/>
  </xsl:if>
 </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
