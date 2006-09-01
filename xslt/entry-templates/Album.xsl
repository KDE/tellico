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

<xsl:output method="html"
            indent="yes"
            doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
            doctype-system="http://www.w3.org/TR/html4/loose.dtd"
            encoding="utf-8"/>

<xsl:param name="datadir"/> <!-- dir where Tellico data are located -->
<xsl:param name="imgdir"/> <!-- dir where field images are located -->
<xsl:param name="font"/> <!-- font family -->
<xsl:param name="fontsize"/> <!-- font size -->
<xsl:param name="fgcolor"/> <!-- foreground color -->
<xsl:param name="bgcolor"/> <!-- background color -->
<xsl:param name="color1"/> <!-- highlighted text color -->
<xsl:param name="color2"/> <!-- highlighted background color -->

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
        margin: .2em 1%;
        padding: 0;
        font-family: "<xsl:value-of select="$font"/>", Arial, Helvetica, sans-serif;
        font-size: <xsl:value-of select="$fontsize"/>pt;
        background-color: <xsl:value-of select="$color1"/>;
        color: <xsl:value-of select="$color2"/>;
        line-height: 1.6;
   }
   #banner {
        padding-bottom: .2em;
        margin-bottom: .2em;
   }
   img#logo {
        padding-top: .3em; /* match h1 */
   }
   h1 {
        color: <xsl:value-of select="$color1"/>;
        background-color: <xsl:value-of select="$color2"/>;
        background-image: url(<xsl:value-of select="concat($imgdir, 'gradient_header.png')"/>);
        background-repeat: repeat-x; 
        font-size: 1.8em;
        text-align: left;
        padding: .1em .5em;
        margin: 0;
        font-weight: bold;
   }
   span.title {
        font-style: italic
   }
   img {
        padding-right: 6px;
        padding-bottom: 6px;
        border: 0px;
   }
   table.category {
       margin-bottom: 6px;
       float: left;
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
        color: black;
        background: #ccc;
   }
   td.fieldValue {
        color: black;
        background: #ddd;
   }
   th, td.fieldValue {
        padding: .1em .3em;
   }
   p {
       margin-top: 0px;
       text-align: justify;
   }
   img {
       border: 0px solid;
   }
  <!-- since text is our own color, links have to be, too -->
   a {
       color: #006;
   }
   p.navigation {
        font-weight: bold;
        text-align: center;
        clear: both;
   }
  </style>
  <title>
   <xsl:value-of select="tc:collection/tc:entry[1]//tc:title[1]"/>
   <xsl:text>&#xa0;&#8211; </xsl:text>
   <xsl:value-of select="tc:collection/@title"/>
  </title>
  </head>
  <body>
   <xsl:apply-templates select="tc:collection[1]"/>
   <xsl:if test="$collection-file">
    <p class="navigation">
     <a href="{$collection-file}">&lt;&lt; <xsl:value-of select="tc:collection/@title"/></a>
    </p>
   </xsl:if>
  </body>
 </html>
</xsl:template>

<!-- type 3 is video collections -->
<!-- warn user that this template is meant for videos only. -->
<xsl:template match="tc:collection[@type!=4]">
 <h1><i18n>This template is meant for music collections only.</i18n></h1>
</xsl:template>

<xsl:template match="tc:collection[@type=4]">
 <xsl:apply-templates select="tc:entry[1]"/>
</xsl:template>

<xsl:template match="tc:entry">
 <xsl:variable name="entry" select="."/>
 <xsl:variable name="titleCat" select="key('fieldsByName','title')/@category"/>
 <!-- there might not be a track list -->
 <xsl:variable name="trackCat">
  <xsl:choose>
   <xsl:when test="tc:tracks">
    <xsl:value-of select="key('fieldsByName','track')/@category"/>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="''"/>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:variable>

 <!-- the top table has images in the left cell and main fields in the right.
      2 images can be on the left -->
 <table width="100%" class="category">
  <tr>
   <td valign="top" rowspan="2">
    <!-- now, show all the images in the entry, type 10 -->
    <xsl:variable name="images" select="../tc:fields/tc:field[@type=10]"/>
    <xsl:for-each select="$images">

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
       <img alt="">
        <xsl:attribute name="src">
         <xsl:value-of select="concat($imgdir, $image)"/>
        </xsl:attribute>
        <!-- limit to maximum width of 150 and height of 200 -->
        <xsl:call-template name="image-size">
         <xsl:with-param name="limit-width" select="150"/>
         <xsl:with-param name="limit-height" select="200"/>
         <xsl:with-param name="image" select="key('imagesById', $image)"/>
        </xsl:call-template>
       </img>
      </a>
     </xsl:if>
    </xsl:for-each>
   </td>

   <!-- want all the width we can get -->
   <td valign="top" width="100%">
    <!-- now a nested table with the general fields -->
    <div id="banner">

     <h1>
      <xsl:choose>
       <xsl:when test="count(.//tc:artist) = 1">
        <xsl:value-of select=".//tc:artist[1]"/>
       </xsl:when>
       <xsl:otherwise>
        <i18n>(Various)</i18n>
       </xsl:otherwise>
      </xsl:choose>
      <xsl:text>&#xa0;&#8211; </xsl:text>
      <span class="title">
       <xsl:value-of select=".//tc:title[1]"/>
      </span>

      <!-- Tellico 0.8 had multiple years in the default video collection -->
      <xsl:if test=".//tc:year">
       <span class="year">
        <xsl:text> (</xsl:text>
        <xsl:value-of select=".//tc:year[1]"/>
        <xsl:text>)</xsl:text>
       </span>
      </xsl:if>

     </h1>
    </div>
   </td>
  </tr>
  <tr>
   <td valign="top">

    <table width="100%">
     <xsl:for-each select="key('fieldsByCat', $titleCat)">
      <xsl:if test="@name != 'title'">
       <tr>
        <th>
         <xsl:value-of select="@title"/>
        </th>
        <td class="fieldValue" width="100%">
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

 <xsl:if test="tc:tracks">
  <table width="50%" class="category">

   <xsl:variable name="cols" select="count(tc:tracks/tc:track[1]/tc:column)"/>

    <tr class="category">
     <td colspan="{$cols}">
      <xsl:value-of select="$trackCat"/>
     </td>
    </tr>
    <xsl:if test="key('fieldsByName','track')/tc:prop[@name = 'column1']">
     <tr>
      <th colspan="2" style="text-align:center">
       <xsl:value-of select="key('fieldsByName','track')/tc:prop[@name = 'column1']"/>
       <xsl:if test="not(tc:tracks/tc:track[1]/tc:column[2] = tc:artists/tc:artist[1])">
        <xsl:text> / </xsl:text>
        <em><xsl:value-of select="key('fieldsByName','track')/tc:prop[@name = 'column2']"/></em>
       </xsl:if>
      </th>
      <th style="text-align:center">
       <xsl:value-of select="key('fieldsByName','track')/tc:prop[@name = 'column3']"/>
      </th>
      <xsl:call-template name="columnTitle">
       <xsl:with-param name="index" select="4"/>
       <xsl:with-param name="max" select="$cols"/>
       <xsl:with-param name="elem" select="'th'"/>
       <xsl:with-param name="field" select="key('fieldsByName','track')"/>
      </xsl:call-template>

     </tr>
    </xsl:if>

    <xsl:for-each select="tc:tracks/tc:track">
     <tr>
      <th style="text-align:center">
       <xsl:value-of select="format-number(position(), '00')"/>
      </th>
      <xsl:choose>
       <!-- a three column table could have an empty third column -->
       <xsl:when test="$cols &gt; 1">
        <!-- if it has three columns, assume first is title,
             second is artist and third is track length. -->
        <td class="fieldValue">
         <xsl:if test="string-length(tc:column[1])">
          <xsl:value-of select="tc:column[1]"/>
         </xsl:if>
         <xsl:if test="string-length(tc:column[2]) and not(tc:column[2] = current()/../../tc:artists/tc:artist[1])">
          <xsl:text> / </xsl:text>
          <em><xsl:value-of select="tc:column[2]"/></em>
         </xsl:if>
        </td>
        <td class="fieldValue" style="text-align: right; padding-right: 10px">
         <em><xsl:value-of select="tc:column[3]"/></em>
        </td>
       </xsl:when>
       <xsl:otherwise>
        <td class="fieldValue">
         <xsl:value-of select="."/>
        </td>
       </xsl:otherwise>
      </xsl:choose>
     </tr>
    </xsl:for-each>
   
   <!-- if it has multiple columns,
        and the final one has a ':', add the time together -->
   <!-- it should still work if the first row itself doesn't contain a ':' -->
   <xsl:if test="$cols &gt; 1 and
                 count(tc:tracks/tc:track[contains(tc:column[number($cols)], ':')])">
    <tr>
     <th colspan="2" style="text-align: right; padding-right: 10px;">
      <strong><i18n>Total:</i18n> </strong>
     </th>
     <td class="fieldValue" style="text-align: right; padding-right: 10px">
      <em>
       <xsl:call-template name="sumTime">
        <xsl:with-param name="nodes" select="tc:tracks//tc:column[number($cols)]"/>
       </xsl:call-template>
      </em>
     </td>
    </tr>
   </xsl:if>
  </table>
 </xsl:if>

 <!-- now for every thing else -->
 <!-- write categories other than general and images -->
 <xsl:for-each select="$categories[. != $titleCat and
                                   ($trackCat = '' or . != $trackCat) and
                                   key('fieldsByCat',.)[1]/@type != 10]">

  <xsl:variable name="category" select="."/>
  <xsl:variable name="fields" select="key('fieldsByCat', $category)"/>
  <xsl:variable name="first-type" select="$fields[1]/@type"/>

  <xsl:variable name="n" select="count($entry//*[key('fieldsByName',local-name(.))/@category=$category])"/>

  <!-- only output if there are field values in this category -->
  <xsl:if test="$n &gt; 0">
  <table width="50%" class="category">
   <tr class="category">
    <td colspan="2">
     <xsl:attribute name="colspan">
      <xsl:if test="$first-type = 2">1</xsl:if>
      <xsl:if test="not($first-type = 2)">2</xsl:if>
     </xsl:attribute>
     <xsl:value-of select="."/>
    </td>
   </tr>
   <xsl:for-each select="$fields">
    <xsl:if test="$entry//*[local-name(.)=current()/@name]">
     <tr>
      <xsl:choose>
       <!-- paragraphs -->
       <xsl:when test="@type = 2">
        <td class="fieldValue" width="100%">
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
                <td>
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
        <td class="fieldValue" width="100%">
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
 </xsl:if>
 </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
