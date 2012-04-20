<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:ac="http://www.allocine.net/v6/ns/"
                xmlns:str="http://exslt.org/strings"
                extension-element-prefixes="str ac"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing Allocine search data.

   Copyright (C) 2012 Robby Stephenson -robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Allocine Results" type="3"> <!-- 3 is movies -->
   <fields>
    <field name="_default"/>
    <!-- the importer will actually download the image and ignore this field -->
    <field flags="0" title="Allocine Code" category="General" format="4" type="1" name="allocine-code"/>
    <field flags="0" title="Original Title" category="General" format="1" type="1" name="origtitle" i18n="yes"/>
    <field flags="0" title="Allocine Link" category="General" format="4" type="7" name="allocine" i18n="yes"/>
   </fields>
   <xsl:apply-templates select=".//ac:movie"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="ac:movie">
 <entry>
  <title>
   <xsl:value-of select="ac:title"/>
  </title>

  <origtitle>
   <xsl:value-of select="ac:originalTitle"/>
  </origtitle>

  <year>
   <xsl:value-of select="ac:productionYear"/>
  </year>

  <xsl:if test="not(ac:casting)">
   <casts>
    <xsl:for-each select="str:tokenize(ac:castingShort/ac:actors, ',')">
     <cast>
      <column>
       <xsl:value-of select="normalize-space(.)"/>
      </column>
     </cast>
    </xsl:for-each>
   </casts>

   <directors>
    <xsl:for-each select="str:tokenize(ac:castingShort/ac:directors, ',')">
     <director>
      <xsl:value-of select="normalize-space(.)"/>
     </director>
    </xsl:for-each>
   </directors>
  </xsl:if>

  <xsl:if test="ac:casting">
   <casts>
    <xsl:for-each select="ac:casting/ac:castMember[ac:activity/@code = '8001']">
     <xsl:if test="position() &lt; 11">
      <cast>
       <column>
        <xsl:value-of select="ac:role"/>
       </column>
       <column>
        <xsl:value-of select="ac:person/ac:name"/>
       </column>
      </cast>
     </xsl:if>
    </xsl:for-each>
   </casts>

   <directors>
    <xsl:for-each select="ac:casting/ac:castMember[ac:activity/@code = '8002']">
     <director>
      <xsl:value-of select="ac:person/ac:name"/>
     </director>
    </xsl:for-each>
   </directors>

   <producers>
    <xsl:for-each select="ac:casting/ac:castMember[ac:activity/@code = '8029']">
     <producer>
      <xsl:value-of select="ac:person/ac:name"/>
     </producer>
    </xsl:for-each>
   </producers>

   <composers>
    <xsl:for-each select="ac:casting/ac:castMember[ac:activity/@code = '8003']">
     <composer>
      <xsl:value-of select="ac:person/ac:name"/>
     </composer>
    </xsl:for-each>
   </composers>
  </xsl:if>

  <genres i18n="true">
   <xsl:for-each select="ac:genreList/ac:genre">
    <genre>
     <xsl:value-of select="."/>
    </genre>
   </xsl:for-each>
  </genres>

  <allocine-code>
   <xsl:value-of select="@code"/>
  </allocine-code>

  <allocine>
   <xsl:value-of select="ac:linkList/ac:link[@rel='aco:web']/@href"/>
  </allocine>

  <studios>
   <xsl:for-each select="ac:release/ac:distributor">
    <studio>
     <xsl:value-of select="@name"/>
    </studio>
   </xsl:for-each>
  </studios>

  <nationalitys>
   <xsl:for-each select="ac:nationalityList/ac:nationality">
    <nationality>
     <xsl:value-of select="."/>
    </nationality>
   </xsl:for-each>
  </nationalitys>

  <languages>
   <xsl:for-each select="ac:languageList/ac:language">
    <language>
     <xsl:value-of select="."/>
    </language>
   </xsl:for-each>
  </languages>

  <plot>
   <xsl:value-of select="ac:synopsis"/>
  </plot>

  <cover>
   <xsl:value-of select="ac:poster/@href"/>
  </cover>

  <running-time>
   <xsl:value-of select="ac:runtime div 60"/>
  </running-time>

  <xsl:if test="ac:color[@code = '12001']">
   <color i18n="yes">
    <xsl:text>Color</xsl:text>
   </color>
  </xsl:if>

 </entry>
</xsl:template>

</xsl:stylesheet>
