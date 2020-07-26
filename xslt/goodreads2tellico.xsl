<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from goodreads.com

   Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Goodreads.com Import" type="2">
   <fields>
    <field name="_default"/>
    <field flags="0" title="Goodreads Link" category="General" format="4" type="7" name="goodreads" i18n="true"/>
   </fields>
   <xsl:apply-templates select="GoodreadsResponse/reviews/review"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="review">
 <entry>
  <xsl:apply-templates select="book"/>
  <rating>
   <xsl:value-of select="rating"/>
  </rating>
  <xsl:if test="shelves/shelf[@name='read']">
   <read>true</read>
  </xsl:if>
 </entry>
</xsl:template>

<xsl:template match="book">
 <title>
   <!-- prefer title_without_series when it exists. XPath | operator returns in document order
        so have to account for that to prefer -->
   <xsl:value-of select="(title_without_series |
                          title[not(following-sibling::title_without_series)])[1]"/>
  </title>

  <isbn>
   <!-- prefer isbn to isbn13 -->
   <xsl:value-of select="(isbn |
                          isbn13[not(following-sibling::isbn)])[1]"/>
  </isbn>

  <publisher>
   <xsl:value-of select="publisher"/>
  </publisher>

  <pub_year>
   <xsl:value-of select="publication_year"/>
  </pub_year>

  <authors>
   <xsl:for-each select="authors/author">
    <author>
     <xsl:value-of select="name"/>
    </author>
   </xsl:for-each>
  </authors>

  <binding i18n="true">
   <xsl:choose>
    <xsl:when test="format='Hardcover'">
     <xsl:text>Hardback</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="format"/>
    </xsl:otherwise>
   </xsl:choose>
  </binding>

  <cover>
   <xsl:value-of select="image_url"/>
  </cover>

  <comments>
   <xsl:value-of select="description"/>
  </comments>

  <pages>
   <xsl:value-of select="num_pages"/>
  </pages>

  <goodreads>
   <xsl:value-of select="link"/>
  </goodreads>

</xsl:template>

</xsl:stylesheet>
