<?xml version="1.0"?>
<!-- WARNING: Tellico uses tc as the internal namespace declaration, and it must be identical here!! -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                exclude-result-prefixes="tc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for exporting to ONIX

   Copyright (C) 2005-2006 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at
   http://www.periapsis.org/tellico/

   Most of the schema for this spreadsheet was copied from the
   Alexandria application,(C) Laurent Sansonetti, released under
   GNU GPL, as is Tellico.

   ===================================================================
-->

<!-- import common templates -->
<!-- location depends on being installed correctly -->
<xsl:import href="tellico-common.xsl"/>

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-system="http://www.editeur.org/onix/2.1/reference/onix-international.dtd"/>

<!-- sent date -->
<xsl:param name="sentDate"/>
<xsl:param name="version"/>
 
<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <!-- This stylesheet is designed for Tellico document syntax version 8 -->
 <xsl:call-template name="syntax-version">
  <xsl:with-param name="this-version" select="'9'"/>
  <xsl:with-param name="data-version" select="@syntaxVersion"/>
 </xsl:call-template>

 <ONIXMessage>
  <xsl:apply-templates select="tc:collection"/>
 </ONIXMessage>
</xsl:template>

<xsl:template match="tc:collection[not(@type=2) and not(@type=5)]">
 <xsl:message terminate="yes">
  <xsl:text>ONIX export only works for book collections and bibliographies.</xsl:text>
 </xsl:message>
</xsl:template>

<xsl:template match="tc:collection[@type=2 or @type=5]">
 <Header>
  <SentDate>
   <xsl:value-of select="$sentDate"/>
  </SentDate>
  <MessageNote>
   <xsl:value-of select="@title"/>
  </MessageNote>
 </Header>
 <xsl:apply-templates select="tc:entry"/>
</xsl:template>

<xsl:template match="tc:entry">
 <Product>
  <RecordSourceName>
   <xsl:text>Tellico</xsl:text>
   <xsl:if test="string-length($version) &gt; 0">
    <xsl:value-of select="concat(' ', $version)"/>
   </xsl:if>
  </RecordSourceName>
  <RecordReference>
   <xsl:value-of select="@id"/>
  </RecordReference>
  <NotificationType>03</NotificationType>
  <ProductForm>BA</ProductForm> <!-- book -->
  <ISBN>
   <xsl:value-of select="translate(tc:isbn, '-', '')"/>
  </ISBN>
  <DistinctiveTitle>
   <xsl:value-of select=".//tc:title[1]"/>
  </DistinctiveTitle>
  <xsl:for-each select=".//tc:author">
   <Contributor>
    <ContributorRole>A01</ContributorRole>
    <PersonName>
     <xsl:value-of select="."/>
    </PersonName>
   </Contributor>
  </xsl:for-each>
  <PublisherName>
   <xsl:value-of select=".//tc:publisher[1]"/>
  </PublisherName>
  <xsl:if test="tc:comments">
   <OtherText>
    <TextTypeCode>12</TextTypeCode>
    <TextFormat>00</TextFormat> <!-- ascii -->
    <Text>
     <xsl:value-of select="tc:comments"/>
    </Text>
   </OtherText>
  </xsl:if>
  <!-- png files are not supported by ONIX -->
  <xsl:variable name="lowercase" select="translate(tc:cover, 'JPEGIF', 'jpegif')"/>
  <xsl:if test="substring($lowercase, string-length($lowercase)-2)='jpg' or
				substring($lowercase, string-length($lowercase)-3)='jpeg' or
				substring($lowercase, string-length($lowercase)-2)='gif'">
   <MediaFile>
    <MediaFileTypeCode>04</MediaFileTypeCode>
    <MediaFileFormatCode>
     <xsl:choose>
      <xsl:when test="substring($lowercase, string-length($lowercase)-2)='gif'">
       <xsl:text>02</xsl:text>
      </xsl:when>
      <xsl:when test="substring($lowercase, string-length($lowercase)-2)='jpg' or
				      substring($lowercase, string-length($lowercase)-3)='jpeg'">
       <xsl:text>03</xsl:text>
      </xsl:when>
     </xsl:choose>
    </MediaFileFormatCode> 
    <MediaFileLinkTypeCode>06</MediaFileLinkTypeCode>
    <MediaFileLink> 
     <xsl:text>images/</xsl:text>
     <xsl:value-of select="tc:cover"/>
    </MediaFileLink>
   </MediaFile>
  </xsl:if>

  <ProductWebSite>
   <ProductWebsiteDescription>Amazon</ProductWebsiteDescription>
   <ProductWebsiteLink>
    <xsl:text>http://www.amazon.com/exec/obidos/ASIN/</xsl:text>
    <xsl:value-of select="translate(tc:isbn, '-', '')"/>
   </ProductWebsiteLink>
  </ProductWebSite>
 </Product>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
