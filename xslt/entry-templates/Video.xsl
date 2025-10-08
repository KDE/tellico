<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - Entry template for videos

   Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>

   Known Issues:
   o If there is more an one image, there's a lot of white space under
     the top tables.

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org
   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="../tellico-common.xsl"/>

<xsl:output method="html"
            indent="yes"
            doctype-system="about:legacy-compat"
            encoding="utf-8"/>

<xsl:param name="datadir"/> <!-- dir where Tellico data are located -->
<xsl:param name="imgdir"/> <!-- dir where field images are located -->
<xsl:param name="font"/> <!-- font family -->
<xsl:param name="fontsize"/> <!-- font size -->
<xsl:param name="fgcolor"/> <!-- foreground color -->
<xsl:param name="bgcolor"/> <!-- background color -->
<xsl:param name="color1"/> <!-- highlighted text color -->
<xsl:param name="color2"/> <!-- highlighted background color -->
<xsl:param name="linkcolor"/> <!-- link color -->

<xsl:param name="collection-file"/> <!-- might have a link to parent collection -->

<xsl:key name="fieldsByName" match="tc:field" use="@name"/>
<xsl:key name="fieldsByCat" match="tc:field" use="@category"/>
<xsl:key name="imagesById" match="tc:image" use="@id"/>
<xsl:key name="loansByEntry" match="tc:loan" use="@entryRef"/>

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
 <!-- This stylesheet is designed for Tellico document syntax version 12 -->
 <xsl:call-template name="syntax-version">
  <xsl:with-param name="this-version" select="'12'"/>
  <xsl:with-param name="data-version" select="@syntaxVersion"/>
 </xsl:call-template>

 <html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <style type="text/css">
   body {
        margin: .2em 1%;
        padding: 0;
        font-family: "<xsl:value-of select="$font"/>", Arial, Helvetica, sans-serif;
        font-size: <xsl:value-of select="$fontsize"/>pt;
        color: <xsl:value-of select="$fgcolor"/>;
        background: <xsl:value-of select="$bgcolor"/>;
        line-height: 1.6;
   }
   div#banner {
        padding-bottom: .1em;
        margin-bottom: .4em;
        border-bottom: .2em ridge <xsl:value-of select="$color2"/>;
   }
   img#logo {
        padding-left: .5em;
        padding-top: .4em;
        float: right;
   }
   h1 {
        color: <xsl:value-of select="$color2"/>;
        font-size: 1.8em;
        text-align: left;
        padding: .2em;
        margin: 0;
        font-weight: bold;
   }
   span.year {
        font-size: 0.8em;
   }
   span.country {
        font-style: italic;
   }
   h2 {
        font-size: 1.2em;
        margin: 0;
        padding: 0;
        padding-left: .2em;
   }
   img {
        padding-right: 1em;
        padding-bottom: 1em;
   }
   table.category {
        margin-right: 1%;
        margin-bottom: 1em;
        float: left;
        border-collapse: collapse;
        border-spacing: 0px;
        width: 49%;
   }
   tr.category {
        font-weight: bold;
        font-size: 1.2em;
        color: <xsl:value-of select="$color1"/>;
        background: <xsl:value-of select="$color2"/>;
        text-align: center;
   }
  /* there seems to be a khtml bug, in 3.4.x at least, repeat-x doesn't
     work on the tr element, so have to put it on the th element */
   tr.category th {
        background-image: url(<xsl:value-of select="concat($imgdir, 'gradient_header.png')"/>);
        background-repeat: repeat-x;
   }
   th {
        font-weight: bold;
        text-align: left;
        color: <xsl:value-of select="$color2"/>;
        background <xsl:value-of select="$color1"/>;
        padding: 0px 0.2em 0px 0.2em;
        white-space: nowrap;
   }
   thead th {
        color: <xsl:value-of select="$color1"/>;
        background <xsl:value-of select="$color2"/>;
        text-align: center;
   }
   td {
        padding: 0px 0.2em 0px 0.2em;
   }
   td.role {
        font-style: italic;
   }
   p {
        margin-top: 0;
        text-align: justify;
        font-size: .9em;
   }
   ul {
        margin-top: .3em;
        margin-bottom: .3em;
        margin-left: 0;
        padding: 0 0 0 1.5em;
   }
   img {
        border: 0px;
   }
   p.navigation {
        font-weight: bold;
        text-align: center;
        clear: both;
   }
   /* KHTML bug related to directional text.
      See Debian Bug #904259 */
   .year {
        direction: ltr;
        unicode-bidi: embed;
   }
   a {
     color: <xsl:value-of select="$linkcolor"/>;
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
    <img width="74" height="32" id="logo" alt="(DVD)">
     <xsl:attribute name="src">
      <xsl:value-of select="concat($datadir,'dvd-logo.png')"/>
     </xsl:attribute>
    </img>
   </xsl:when>
   <xsl:when test="$datadir and tc:medium = 'VHS'">
    <img width="56" height="30" id="logo" alt="(VHS)">
     <xsl:attribute name="src">
      <xsl:value-of select="concat($datadir,'vhs-logo.png')"/>
     </xsl:attribute>
    </img>
   </xsl:when>
   <xsl:when test="$datadir and tc:medium = 'Blu-ray'">
    <img width="56" height="30" id="logo" alt="(Blu-ray)">
     <xsl:attribute name="src">
      <xsl:value-of select="concat($datadir,'bluray-logo.png')"/>
     </xsl:attribute>
    </img>
   </xsl:when>
   <xsl:when test="$datadir and tc:medium = 'HD DVD'">
    <img width="119" height="30" id="logo" alt="(HD DVD)">
     <xsl:attribute name="src">
      <xsl:value-of select="concat($datadir,'hddvd-logo.png')"/>
     </xsl:attribute>
    </img>
   </xsl:when>
  </xsl:choose>

  <!-- title block -->
  <h1>
   <xsl:value-of select=".//tc:title[1]"/>
   <xsl:if test="tc:widescreen"><xsl:text/>
    <xsl:text>&#xa0;&#8211; </xsl:text><xsl:value-of select="key('fieldsByName', 'widescreen')/@title"/><xsl:text/>
   </xsl:if>
   <xsl:if test="tc:directors-cut"><xsl:text/>
    <xsl:text>&#xa0;&#8211; </xsl:text><xsl:value-of select="key('fieldsByName', 'directors-cut')/@title"/><xsl:text/>
   </xsl:if>
   <xsl:if test=".//tc:year|.//tc:nationality">
     <span class="year">
      <xsl:text> (</xsl:text>
      <xsl:if test=".//tc:year">
       <xsl:value-of select=".//tc:year[1]"/>
      </xsl:if>
      <xsl:if test=".//tc:nationality">
       <xsl:if test=".//tc:year">
        <xsl:text>&#xa0;&#8211; </xsl:text>
       </xsl:if>
       <span class="country">
        <xsl:value-of select=".//tc:nationality[1]"/>
       </span>
      </xsl:if>
     <xsl:text>) </xsl:text>
    </span>
   </xsl:if>
  </h1>

 </div>

 <!-- the images and the rest of the categories are each in a table in a table cell -->
 <table class="category" style="width:100%">
  <tbody>
   <tr>
    <td style="vertical-align:top">

     <!-- now, show all the images in the entry, field type 10 -->
     <xsl:variable name="images" select="../tc:fields/tc:field[@type=10]"/>
     <xsl:if test="count($images) &gt; 0">
      <table>
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
                <xsl:call-template name="image-link">
                 <xsl:with-param name="image" select="key('imagesById', $image)"/>
                 <xsl:with-param name="dir" select="$imgdir"/>
                </xsl:call-template>
               </xsl:otherwise>
              </xsl:choose>
             </xsl:attribute>
             <img alt="">
              <xsl:attribute name="src">
               <xsl:call-template name="image-link">
                <xsl:with-param name="image" select="key('imagesById', $image)"/>
                <xsl:with-param name="dir" select="$imgdir"/>
               </xsl:call-template>
              </xsl:attribute>
              <!-- limit to maximum width of 200 of height of 300 -->
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

    <td style="vertical-align:top">

     <!-- show the general group, or more accurately, the title's group -->
     <table class="category">
      <thead>
       <tr class="category">
        <th colspan="2">
         <xsl:value-of select="$titleCat"/>
        </th>
       </tr>
       </thead>
       <tbody>
       <!-- the year and nationality have already been shown, but the film
            might have multiple values, so go ahead and show them again -->
       <xsl:for-each select="key('fieldsByCat', $titleCat)">
        <xsl:if test="@name != 'title'">
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

     <!-- now for the cast -->
     <xsl:if test="tc:casts">
      <xsl:variable name="castField" select="key('fieldsByName', 'cast')"/>
      <table class="category">
       <thead>
        <tr class="category">
         <th colspan="2">
          <xsl:value-of select="$castField/@title"/>
         </th>
        </tr>
       </thead>
       <tbody>
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
        <xsl:for-each select="$entry/tc:casts/tc:cast">
         <tr>
          <xsl:for-each select="tc:column">
           <td>
            <xsl:if test="position() = 1">
             <xsl:attribute name="class">person</xsl:attribute><xsl:value-of select="."/>
            </xsl:if>
            <xsl:if test="position() &gt; 1">
             <xsl:attribute name="class">role</xsl:attribute><xsl:value-of select="."/>
            </xsl:if>
            <xsl:text>&#160;</xsl:text>
           </td>
          </xsl:for-each>
         </tr>
        </xsl:for-each>
       </tbody>
      </table>
     </xsl:if>

 <!-- now for every thing else -->
 <!-- write categories other than general and images -->
 <!-- $castcat might be empty, be careful -->
 <xsl:for-each select="$categories[. != $titleCat and
                                   ($castCat = '' or . != $castCat) and
                                   key('fieldsByCat',.)[1]/@type != 10]">
  <xsl:if test="key('fieldsByCat', .)[@name != 'directors-cut' and
                                      @name != 'widescreen' and
                                      @name != 'id' and
                                      @name != 'cdate' and
                                      @name != 'mdate']">
  <table class="category">
   <thead>
    <tr class="category">
     <th colspan="2">
      <xsl:value-of select="."/>
     </th>
    </tr>
    </thead>
    <tbody>
    <xsl:for-each select="key('fieldsByCat', .)[@name != 'directors-cut' and
                                                @name != 'widescreen' and
                                                @name != 'id' and
                                                @name != 'cdate' and
                                                @name != 'mdate']">
     <tr>
      <xsl:choose>
       <!-- paragraphs -->
       <xsl:when test="@type=2">
        <td colspan="2">
         <xsl:variable name="output" select="$entry/*[local-name(.)=current()/@name]"/>
         <xsl:if test="string-length($output)">
          <p>
           <xsl:value-of select="$output" disable-output-escaping="yes"/>
          </p>
         </xsl:if>
        </td>
       </xsl:when>

       <!-- tables are field type 8 -->
       <!-- ok to put category name inside div instead of table here -->
       <xsl:when test="@type = 8">
        <td colspan="2">
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
                <td style="width:{100 div count(../tc:column)}%">
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
        <td>
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
  </xsl:if>
 </xsl:for-each>
</td>
</tr>
</tbody>
</table>

<xsl:for-each select="key('loansByEntry', tc:id)">
 <table class="category">
  <thead>
   <tr class="category">
    <th colspan="2"><i18n>Loan</i18n></th>
   </tr>
  </thead>
  <tbody>
   <tr>
    <th><i18n>Borrower</i18n></th>
    <td>
     <xsl:value-of select="../@name"/>
    </td>
   </tr>
   <tr>
    <th><i18n>Loan Date</i18n></th>
    <td>
     <xsl:value-of select="@loanDate"/>
    </td>
   </tr>
   <tr>
    <th><i18n>Due Date</i18n></th>
    <td>
     <xsl:value-of select="@dueDate"/>
    </td>
   </tr>
   <tr>
    <th><i18n>Note</i18n></th>
    <td>
     <xsl:value-of select="."/>
    </td>
   </tr>
  </tbody>
 </table>
</xsl:for-each>

</xsl:template>

</xsl:stylesheet>
